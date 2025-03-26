#include "../include/server.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
using namespace std;

Game::Game(Player* p1, Player* p2) {
    players[0] = p1;
    players[1] = p2;
    sockets[0] = p1->getSocket();
    sockets[1] = p2->getSocket();
    p1->setPiece(Piece::BLACK);
    p2->setPiece(Piece::WHITE);
    string msg = "NEWGAME~" + p1->getUsername() + "~" + p2->getUsername() + "\n";
    Network::sendData(sockets[0], msg);
    Network::sendData(sockets[1], msg);
    currentPlayer = 0;
}

void Game::run() {
    unique_lock<mutex> lock1(players[0]->getMutex(), defer_lock);
    unique_lock<mutex> lock2(players[1]->getMutex(), defer_lock);
    lock(lock1, lock2);
    while (true) {
        Piece winner = board.getWinner();
        if (winner != Piece::NONE) {
            string msg = "GAMEOVER~VICTORY~" + (winner == Piece::BLACK ? players[0]->getUsername() : players[1]->getUsername()) + "\n";
            Network::sendData(sockets[0], msg);
            Network::sendData(sockets[1], msg);
            break;
        }
        string input = Network::receiveData(sockets[currentPlayer]);
        if (input.empty()) {
            string msg = "GAMEOVER~DISCONNECT~" + players[1 - currentPlayer]->getUsername() + "\n";
            Network::sendData(sockets[1 - currentPlayer], msg);
            break;
        }
        auto tokens = split(input, '~');
        if (tokens[0] == "MOVE") {
            int move = stoi(tokens[1]);
            if (!board.makeMove(move, players[currentPlayer]->getPiece())) {
                Network::sendData(sockets[currentPlayer], "ERROR~Invalid Move\n");
                continue;
            }
            Network::sendData(sockets[0], input);
            Network::sendData(sockets[1], input);
            currentPlayer = 1 - currentPlayer;
        }
    }
    players[0]->getCondition().notify_one();
    players[1]->getCondition().notify_one();
}

void Server::processQueue() {
    while (true) {
        Game* game = waitForPlayers();
        if (game) startGame(game);
    }
}

Game* Server::waitForPlayers() {
    unique_lock<mutex> lock(queueMutex);
    queueCond.wait(lock, [this] { return gameQueue.size() >= 2; });
    Player* p1 = gameQueue.front(); gameQueue.pop();
    Player* p2 = gameQueue.front(); gameQueue.pop();
    cout << "Game started between " << p1->getUsername() << " and " << p2->getUsername() << "\n";
    return new Game(p1, p2);
}

void Server::startGame(Game* game) {
    thread([game] {
        game->run();
        delete game;
    }).detach();
}

void Server::handleClient(int sock) {
    Player* player = nullptr;
    string buffer;
    while (true) {
        string message = Network::receiveData(sock);
        if (message.empty()) {
            if (player) {
                cout << "Client " << player->getUsername() << " disconnected\n";
                // Note: Cleanup of connectedPlayers not fully implemented here
            }
            break;
        }
        buffer += message;
        size_t pos;
        while ((pos = buffer.find('\n')) != string::npos) {
            string msg = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            cout << "Received from client: " << msg << "\n";
            auto tokens = split(msg, '~');
            if (tokens[0] == "HELLO") {
                cout << "Received HELLO from: " << tokens[1] << "\n";
                Network::sendData(sock, "HELLO~CaptureGo Server\n");
            } else if (tokens[0] == "LOGIN") {
                string username = tokens[1];
                cout << "Login attempt with username: " << username << "\n";
                bool usernameTaken = false;
                for (Player* p : connectedPlayers) {
                    if (p->getUsername() == username) {
                        usernameTaken = true;
                        break;
                    }
                }
                if (usernameTaken) {
                    cout << "Username taken: Yes\n";
                    Network::sendData(sock, "ALREADYLOGGEDIN\n");
                } else {
                    cout << "Username taken: No\n";
                    player = new Player(sock, username);
                    connectedPlayers.push_back(player);
                    Network::sendData(sock, "LOGIN\n");
                }
            } else if (tokens[0] == "QUEUE") {
                if (!player) {
                    Network::sendData(sock, "ERROR~Login required\n");
                    continue;
                }
                {
                    lock_guard<mutex> lock(queueMutex);
                    gameQueue.push(player);
                    queueCond.notify_one();
                }
                unique_lock<mutex> playerLock(player->getMutex());
                player->getCondition().wait(playerLock);
            }
        }
    }
    // Cleanup (e.g., remove player, close sock) not fully implemented here
}

void Server::run() {
    int port = 8081;
    string input;
    cout << "Enter port number or press enter for default port number (8081): ";
    getline(cin, input);
    port = input.empty() ? 8081 : stoi(input);

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        cout << "Failed to create server socket: " << strerror(errno) << "\n";
        return;
    }
    int opt = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cout << "Failed to set SO_REUSEADDR: " << strerror(errno) << "\n";
        close(serverSock);
        return;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "Bind failed: " << strerror(errno) << "\n";
        close(serverSock);
        return;
    }
    if (listen(serverSock, 10) < 0) {
        cout << "Listen failed: " << strerror(errno) << "\n";
        close(serverSock);
        return;
    }
    cout << "Server listening on port " << port << "\n";
    thread queueThread(&Server::processQueue, this);
    queueThread.detach();
    while (true) {
        cout << "Waiting for client connection...\n";
        int clientSock = accept(serverSock, nullptr, nullptr);
        if (clientSock < 0) {
            cout << "Accept failed: " << strerror(errno) << "\n";
            continue;
        }
        cout << "Client connected\n";
        thread(&Server::handleClient, this, clientSock).detach();
    }
}
