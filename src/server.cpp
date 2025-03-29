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
    cout << "Sending NEWGAME to both players\n";
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
            cout << "Sending GAMEOVER: Victory for " << (winner == Piece::BLACK ? players[0]->getUsername() : players[1]->getUsername()) << "\n";
            Network::sendData(sockets[0], msg);
            Network::sendData(sockets[1], msg);
            break;
        }
        string input = Network::receiveData(sockets[currentPlayer]);
        if (input.empty()) {
            string msg = "GAMEOVER~DISCONNECT~" + players[currentPlayer]->getUsername() + "\n";
            cout << "Sending GAMEOVER: Disconnection of " << players[currentPlayer]->getUsername() << "\n";
            Network::sendData(sockets[1 - currentPlayer], msg);
            break;
        }
        auto tokens = split(input, '~');
        if (tokens[0] == "MOVE") {
            int move = stoi(tokens[1]);
            int x = move / 7;
            int y = move % 7;
            if (board.getPiece(x, y) != Piece::NONE || !board.checkMove(x, y)) {
                Network::sendData(sockets[currentPlayer], "ERROR~Invalid Move\n");
                continue;
            }
            Piece piece = players[currentPlayer]->getPiece();
            board.makeMove(x, y, piece);
            cout << "Move accepted: " << move << " by " << players[currentPlayer]->getUsername() << "\n";
            string moveMsg = "MOVE~" + to_string(move) + "~" + (piece == Piece::BLACK ? "BLACK" : "WHITE") + "\n";
            Network::sendData(sockets[0], moveMsg);
            Network::sendData(sockets[1], moveMsg);
            currentPlayer = 1 - currentPlayer;
        }
    }
    players[0]->getCondition().notify_one();
    players[1]->getCondition().notify_one();
}

void Server::processQueue() {
    while (true) {
        unique_lock<mutex> lock(queueMutex);
        queueCond.wait(lock, [this] { return gameQueue.size() >= 2; });
        Player* p1 = gameQueue.front(); gameQueue.pop();
        Player* p2 = gameQueue.front(); gameQueue.pop();
        lock.unlock();
        cout << "Game started between " << p1->getUsername() << " and " << p2->getUsername() << "\n";
        Game* game = new Game(p1, p2);
        startGame(game);
    }
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
                lock_guard<mutex> lock(queueMutex);
                connectedPlayers.erase(
                    remove_if(connectedPlayers.begin(), connectedPlayers.end(),
                              [player](Player* p) { return p->getUsername() == player->getUsername(); }),
                    connectedPlayers.end());
                queue<Player*> tempQueue;
                while (!gameQueue.empty()) {
                    Player* p = gameQueue.front();
                    gameQueue.pop();
                    if (p != player) tempQueue.push(p);
                }
                gameQueue = tempQueue;
                delete player;
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
                Network::sendData(sock, "HELLO~CaptureGo Server\n");
            } else if (tokens[0] == "LOGIN") {
                string username = tokens[1];
                lock_guard<mutex> lock(queueMutex);
                bool usernameTaken = false;
                for (Player* p : connectedPlayers) {
                    if (p->getUsername() == username) {
                        usernameTaken = true;
                        break;
                    }
                }
                if (usernameTaken) {
                    Network::sendData(sock, "ALREADYLOGGEDIN\n");
                } else {
                    player = new Player(sock, username);
                    connectedPlayers.push_back(player);
                    Network::sendData(sock, "LOGIN\n");
                }
            } else if (msg == "LISTUSERS") {
                lock_guard<mutex> lock(queueMutex);
                string userList = "USERLIST~";
                for (Player* p : connectedPlayers) userList += p->getUsername() + ",";
                if (!connectedPlayers.empty()) userList.pop_back();
                userList += "\n";
                Network::sendData(sock, userList);
            } else if (msg == "LISTQUEUE") {
                lock_guard<mutex> lock(queueMutex);
                string queueList = "QUEUELIST~";
                if (gameQueue.empty()) {
                    queueList += "No players in queue.\n";
                } else {
                    queue<Player*> temp = gameQueue;
                    while (!temp.empty()) {
                        queueList += temp.front()->getUsername() + ",";
                        temp.pop();
                    }
                    queueList.pop_back();
                    queueList += "\n";
                }
                Network::sendData(sock, queueList);
            } else if (msg == "QUEUE") {
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
    Network::closeSocket(sock);
}

void Server::run() {
    int port = 8081;
    string input;
    cout << "Enter port number or press enter for default (8081): ";
    getline(cin, input);
    port = input.empty() ? 8081 : stoi(input);

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        cout << "Failed to create server socket: " << strerror(errno) << "\n";
        return;
    }
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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
        int clientSock = accept(serverSock, nullptr, nullptr);
        if (clientSock < 0) {
            cout << "Accept failed: " << strerror(errno) << "\n";
            continue;
        }
        cout << "Client connected\n";
        thread(&Server::handleClient, this, clientSock).detach();
    }
}

bool Server::isPortInUse(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bool inUse = bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0;
    close(sock);
    return inUse;
}
