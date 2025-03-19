#include "server.h"
#include <iostream>
#include <map>
using namespace std;
GameServer::GameServer(int port) : serverSocket(-1) {
    serverSocket = network.createServerSocket(port);
    if (serverSocket < 0) {
        cerr << "Failed to create server socket\n";
        exit(1);
    }
}
void GameServer::run() {
    cout << "Server listening on port...\n";
    map<int, string> clientUsernames;
    while (true) {
        int clientSocket = network.acceptClient(serverSocket);
        if (clientSocket < 0) {
            cerr << "Failed to accept client\n";
            continue;
        }
        cout << "Client connected\n";
        gameThreads.emplace_back(&GameServer::handleClient, this, clientSocket, ref(clientUsernames));
        gameThreads.back().detach();
    }
}
void GameServer::handleClient(int clientSocket, map<int, string>& clientUsernames) {
    string message = network.receiveData(clientSocket);
    if (message.empty()) {
        cout << "Client disconnected\n";
        network.closeSocket(clientSocket);
        return;
    }
    size_t spacePos = message.find(' ');
    string command = (spacePos != string::npos) ? message.substr(0, spacePos) : "";
    string rest = (spacePos != string::npos) ? message.substr(spacePos + 1) : message;
    spacePos = rest.find(' ');
    string username = (spacePos != string::npos) ? rest.substr(0, spacePos) : rest;
    {
        lock_guard<mutex> lock(queueMutex);
        for (const auto& pair : clientUsernames) {
            if (pair.second == username) {
                network.sendData(clientSocket, "Username already taken");
                network.closeSocket(clientSocket);
                return;
            }
        }
        connectedClients.insert(clientSocket);
        clientUsernames[clientSocket] = username;
    }
    if (command == "LIST_USERS") {
        lock_guard<mutex> lock(queueMutex);
        string userList;
        for (int sock : connectedClients) {
            if (clientUsernames.count(sock)) {
                userList += clientUsernames[sock] + "\n";
            }
        }
        network.sendData(clientSocket, userList.empty() ? "No users connected" : userList);
    } else if (command == "JOIN_GAME") {
        lock_guard<mutex> lock(queueMutex);
        if (waitingClients.empty()) {
            waitingClients.push(clientSocket);
            network.sendData(clientSocket, "Waiting");
            cout << "Player " << username << " waiting in queue\n";
        } else {
            int waitingClient = waitingClients.front();
            waitingClients.pop();
            string player1Name = clientUsernames[waitingClient];
            string player2Name = username;
            cout << "Game started between " << player1Name << " and " << player2Name << "\n";
            network.sendData(waitingClient, "BLACK");
            network.sendData(clientSocket, "WHITE");
            gameThreads.emplace_back(&GameServer::startGame, this, waitingClient, clientSocket);
            gameThreads.back().detach();
            return;
        }
    } else {
        network.sendData(clientSocket, "Invalid initial command");
        network.closeSocket(clientSocket);
        return;
    }
    while (true) {
        message = network.receiveData(clientSocket);
        if (message.empty()) {
            cout << "Client disconnected\n";
            lock_guard<mutex> lock(queueMutex);
            network.closeSocket(clientSocket);
            connectedClients.erase(clientSocket);
            clientUsernames.erase(clientSocket);
            break;
        }
        spacePos = message.find(' ');
        command = (spacePos != string::npos) ? message.substr(0, spacePos) : "";
        rest = (spacePos != string::npos) ? message.substr(spacePos + 1) : message;
        spacePos = rest.find(' ');
        username = (spacePos != string::npos) ? rest.substr(0, spacePos) : rest;
        lock_guard<mutex> lock(queueMutex);
        if (command == "LIST_USERS") {
            string userList;
            for (int sock : connectedClients) {
                if (clientUsernames.count(sock)) {
                    userList += clientUsernames[sock] + "\n";
                }
            }
            network.sendData(clientSocket, userList.empty() ? "No users connected" : userList);
            continue;
        }
        if (command == "LIST_WAITING") {
            string waitingList;
            queue<int> tempQueue = waitingClients;
            while (!tempQueue.empty()) {
                int sock = tempQueue.front();
                tempQueue.pop();
                if (clientUsernames.count(sock)) {
                    waitingList += clientUsernames[sock] + "\n";
                }
            }
            network.sendData(clientSocket, waitingList.empty() ? "No players waiting" : waitingList);
            continue;
        }
        if (command == "JOIN_GAME") {
            if (waitingClients.empty()) {
                waitingClients.push(clientSocket);
                network.sendData(clientSocket, "Waiting");
                cout << "Player " << username << " waiting in queue\n";
            } else {
                int waitingClient = waitingClients.front();
                waitingClients.pop();
                string player1Name = clientUsernames[waitingClient];
                string player2Name = username;
                cout << "Game started between " << player1Name << " and " << player2Name << "\n";
                network.sendData(waitingClient, "BLACK");
                network.sendData(clientSocket, "WHITE");
                gameThreads.emplace_back(&GameServer::startGame, this, waitingClient, clientSocket);
                gameThreads.back().detach();
                return;
            }
        }
    }
}

void GameServer::startGame(int client1, int client2) {
    Game game;
    while (!game.isGameOver()) {
        string state = game.serialize();
        network.sendData(client1, state);
        network.sendData(client2, state);
        Player currentTurn = game.getCurrentTurn();
        int currentClient = (currentTurn == Player::BLACK) ? client1 : client2;
        string move = network.receiveData(currentClient);
        if (move.empty()) {
            cout << "Client disconnected during game\n";
            lock_guard<mutex> lock(queueMutex);
            connectedClients.erase(client1);
            connectedClients.erase(client2);
            break;
        }
        int x = move[0] - '0';
        int y = move[2] - '0';
        if (!game.makeMove(x, y)) {
            cout << "Invalid move by client\n";
        }
    }
    string finalState = game.serialize();
    network.sendData(client1, finalState);
    network.sendData(client2, finalState);
    cout << "Client disconnected\n";
    network.closeSocket(client1);
    cout << "Client disconnected\n";
    network.closeSocket(client2);
}
