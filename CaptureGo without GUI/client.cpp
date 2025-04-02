#include "client.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <chrono>
using namespace std;

int HumanPlayer::getMove(const Board& b) {
    while (true) {
        cout << "Enter move as x,y (e.g., 4,4): ";
        string input;
        getline(cin, input);
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        size_t comma = input.find(',');
        if (comma == string::npos) {
            cout << "Invalid format! Use x,y, e.g., 4,4.\n";
            continue;
        }
        try {
            string rowStr = input.substr(0, comma);
            string colStr = input.substr(comma + 1);
            rowStr.erase(0, rowStr.find_first_not_of(" \t"));
            rowStr.erase(rowStr.find_last_not_of(" \t") + 1);
            colStr.erase(0, colStr.find_first_not_of(" \t"));
            colStr.erase(colStr.find_last_not_of(" \t") + 1);
            int row = stoi(rowStr);
            int col = stoi(colStr);
            row--; col--; 
            if (b.checkMove(row, col)) {
                return row * 7 + col;
            }
            cout << "Invalid move! Position must be empty and within 1-7.\n";
        } catch (const invalid_argument&) {
            cout << "Invalid input! Coordinates must be numbers, e.g., 4,4.\n";
        } catch (const out_of_range&) {
            cout << "Coordinates out of range! Use numbers between 1 and 7, e.g., 4,4.\n";
        }
    }
}

AIPlayer::AIPlayer(int socket, const string& username) : ClientPlayer(socket, username), rng(chrono::system_clock::now().time_since_epoch().count()) {}
AIPlayer::AIPlayer(int socket, const string& username, Piece piece) : ClientPlayer(socket, username, piece), rng(chrono::system_clock::now().time_since_epoch().count()) {}

int AIPlayer::getMove(const Board& b) {
    auto chains = b.listChainsAndLiberties();
    const ChainAndLiberties* target = nullptr;
    size_t minLiberties = numeric_limits<size_t>::max();
    for (const auto& chain : chains) {
        if (chain.getPiece() != piece && chain.getLiberties().size() < minLiberties) {
            minLiberties = chain.getLiberties().size();
            target = &chain;
        }
    }
    if (!target || target->getLiberties().empty()) {
        uniform_int_distribution<int> dist(0, 48);
        while (true) {
            int move = dist(rng);
            if (b.checkMove(move / 7, move % 7)) {
                return move;
            }
        }
    } else {
        vector<Point> libs(target->getLiberties().begin(), target->getLiberties().end());
        uniform_int_distribution<size_t> dist(0, libs.size() - 1);
        Point move = libs[dist(rng)];
        return move.x * 7 + move.y;
    }
}

ClientGame::ClientGame(ClientPlayer* player) : player(player), sock(player->getSocket()) {
    myTurn = player->getPiece() == Piece::BLACK;
}

void ClientGame::run() {
    bool gameOver = false;
    while (!gameOver) {
        board.printBoard();
        if (myTurn && board.getWinner() == Piece::NONE) {
            cout << "Your turn as " << (player->getPiece() == Piece::BLACK ? "Black" : "White") << "\n";
            int move = player->getMove(board);
            Network::sendData(sock, "MOVE~" + to_string(move));
        } else if (board.getWinner() == Piece::NONE) {
            cout << "Opponent's turn as " << (player->getOpponentPiece() == Piece::BLACK ? "Black" : "White") << "\n";
        } else {
            Piece winner = board.getWinner();
            cout << (winner == player->getPiece() ? "You won!\n" : "You lost!\n");
            gameOver = true;
            break;
        }
        string message = Network::receiveData(sock);
        if (message.empty()) {
            cout << "Connection lost!\n";
            gameOver = true;
            break;
        }
        auto tokens = split(message, '~');
        if (tokens[0] == "MOVE" && myTurn) {
            int move = stoi(tokens[1]);
            cout << "You played: (" << (move / 7 + 1) << ", " << (move % 7 + 1) << ")\n";
            board.makeMove(move, player->getPiece());
            myTurn = false;
        } else if (tokens[0] == "ERROR" && myTurn) {
            cout << "Invalid move!\n";
        } else if (tokens[0] == "MOVE" && !myTurn) {
            int move = stoi(tokens[1]);
            cout << "Opponent played: (" << (move / 7 + 1) << ", " << (move % 7 + 1) << ")\n";
            board.makeMove(move, player->getOpponentPiece());
            myTurn = true;
        } else if (tokens[0] == "GAMEOVER") {
            if (tokens[1] == "VICTORY") {
                cout << (tokens[2] == player->getUsername() ? "You won!\n" : "You lost!\n");
            } else if (tokens[1] == "DISCONNECT") {
                cout << "Opponent disconnected!\n";
            }
            gameOver = true;
            // Check for rematch
            string rematch = Network::receiveData(sock);
            if (rematch == "REMATCH?") {
                cout << "Play again with the same opponent? (yes/no): ";
                string answer;
                getline(cin, answer);
                transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
                Network::sendData(sock, answer == "yes" ? "YES" : "NO");
                if (answer == "yes") {
                    // Start a new game immediately
                    ClientGame newGame(player);
                    newGame.run();
                    return;
                }
            }
            break;
        }
    }
}

void Client::run() {
    string hostname = "127.0.0.1";
    int port = 8081;
    string input;
    cout << "Enter server IP address or press enter for local host or default address : ";
    getline(cin, input);
    if (!input.empty()) hostname = input;
    cout << "Enter port number or press enter for default port number (8081) : ";
    getline(cin, input);
    if (!input.empty()) port = stoi(input);
    int sock = Network::createClientSocket(hostname, port);
    if (sock == -1) {
        cerr << "Failed to connect to server.\n";
        return;
    }
    Network::sendData(sock, "HELLO~CaptureGo Client");
    Network::receiveData(sock);
    string username;
    while (true) {
        cout << "Enter username: ";
        getline(cin, username);
        Network::sendData(sock, "LOGIN~" + username);
        string response = Network::receiveData(sock);
        if (response == "LOGIN") break;
        cout << "Username taken!\n";
    }
    while (true) {
        cout << "Menu:\n"
             << "1. List users on the server\n"
             << "2. Enter as a player (human)\n"
             << "3. Enter as AI\n"
             << "4. Exit\n"
             << "5. Players waiting for opponent\n";
        int option = 0;
        while (true) {
            cout << "Option (1-5): ";
            getline(cin, input);
            if (input.empty()) {
                cout << "Please enter a number between 1 and 5.\n";
                continue;
            }
            try {
                option = stoi(input);
                if (option < 1 || option > 5) {
                    cout << "Invalid option! Please enter a number between 1 and 5.\n";
                    continue;
                }
                break;
            } catch (const invalid_argument&) {
                cout << "Invalid input! Please enter a number between 1 and 5.\n";
            } catch (const out_of_range&) {
                cout << "Number out of range! Please enter a number between 1 and 5.\n";
            }
        }
        if (option == 2 || option == 3) {
            Network::sendData(sock, "QUEUE");
            cout << "Waiting in queue...\n";
            string message = Network::receiveData(sock);
            auto tokens = split(message, '~');
            if (tokens[0] == "NEWGAME") {
                Piece piece = tokens[1] == username ? Piece::BLACK : Piece::WHITE;
                ClientPlayer* player;
                if (option == 2) {
                    player = new HumanPlayer(sock, username, piece);
                } else {
                    player = new AIPlayer(sock, username, piece);
                }
                cout << "You are playing as " << (piece == Piece::BLACK ? "Black" : "White") << "\n";
                cout << "You are playing against " << (tokens[1] == username ? tokens[2] : tokens[1]) << "\n";
                ClientGame game(player);
                game.run();
                delete player;
            }
        } else if (option == 1) {
            Network::sendData(sock, "LIST");
            auto tokens = split(Network::receiveData(sock), '~');
            cout << "Connected users:\n";
            for (size_t i = 1; i < tokens.size(); ++i) {
                cout << i << ". " << tokens[i] << "\n";
            }
        } else if (option == 4) {
            cout << "Exiting...\n";
            break;
        } else if (option == 5) {
            Network::sendData(sock, "QUEUE_LIST");
            auto tokens = split(Network::receiveData(sock), '~');
            cout << "Players waiting for opponent:\n";
            if (tokens.size() == 1 && tokens[0] == "QUEUE_LIST") {
                cout << "No players in queue.\n";
            } else {
                for (size_t i = 1; i < tokens.size(); ++i) {
                    cout << i << ". " << tokens[i] << "\n";
                }
            }
        }
    }
    Network::closeSocket(sock);
}
