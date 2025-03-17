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

        string message = network.receiveData(clientSocket);
        if (message.empty()) {
            cout << "Client disconnected\n"; 
            network.closeSocket(clientSocket);
            continue;
        }
        size_t spacePos = message.find(' ');
        string command = (spacePos != string::npos) ? message.substr(0, spacePos) : "";
        string username = (spacePos != string::npos) ? message.substr(spacePos + 1) : message;

        if (command == "LIST_USERS") {
            lock_guard<mutex> lock(queueMutex);
            string userList = username;
            queue<int> tempQueue = waitingClients;
            while (!tempQueue.empty()) {
                int queuedSocket = tempQueue.front();
                tempQueue.pop();
                if (clientUsernames.count(queuedSocket)) {
                    userList += "\n" + clientUsernames[queuedSocket];
                }
            }
            network.sendData(clientSocket, userList.empty() ? "No users in queue" : userList);
            continue; 
        }
        lock_guard<mutex> lock(queueMutex);
        clientUsernames[clientSocket] = username;
        if (waitingClients.empty()) {
            waitingClients.push(clientSocket);
            network.sendData(clientSocket, "Waiting in queue...");
            cout << "Player " << username << " waiting in queue\n";
        } else {
            int waitingClient = waitingClients.front();
            waitingClients.pop();
            string player1Name = clientUsernames[waitingClient];
            string player2Name = username;
            if (player1Name.empty() || player2Name.empty()) {
                cerr << "Error: Missing username for game start\n";
            }
            cout << "Game started between " << player1Name << " and " << player2Name << "\n";
            gameThreads.emplace_back(&GameServer::startGame, this, waitingClient, clientSocket);
            gameThreads.back().detach();
        }
    }
}
void GameServer::startGame(int client1, int client2) {
    network.sendData(client1, "BLACK");
    network.sendData(client2, "WHITE");
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
