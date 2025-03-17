#include "client.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

GameClient::GameClient(const string& serverIp, int port, const string& username, bool isAI) 
    : username(username), isAI(isAI) {
    srand(time(nullptr));
    clientSocket = network.createClientSocket(serverIp, port);
    if (clientSocket < 0) {
        cerr << "Failed to connect to server\n";
        exit(1);
    }
    network.sendData(clientSocket, username); 
    string message = network.receiveData(clientSocket);
    if (message == "Waiting in queue...") {
        cout << "Waiting in queue...\n";
        message = network.receiveData(clientSocket);
    }
    playerRole = (message == "BLACK" ? Player::BLACK : Player::WHITE);
    cout << "Assigned role: " << (playerRole == Player::BLACK ? "BLACK" : "WHITE") << "\n";
}

void GameClient::run() {
    while (!game.isGameOver()) {
        string state = network.receiveData(clientSocket);
        if (state.empty()) {
            cerr << "Server disconnected\n";
            break;
        }
        game.deserialize(state);
        game.displayBoard();
        if (game.isGameOver()) {
            break;
        }
        if (game.getCurrentTurn() == playerRole) {
            string move;
            if (isAI) {
                int count;
                pair<int, int>* emptyPositions = game.getEmptyPositions(count);
                if (count > 0) {
                    int idx = rand() % count;
                    move = to_string(emptyPositions[idx].first) + "," + to_string(emptyPositions[idx].second);
                    cout << "AI move: " << move << "\n";
                } else {
                    cerr << "No valid moves available for AI\n";
                    break;
                }
                delete[] emptyPositions;
            } else {
                cout << "Your turn! Enter move (x,y): ";
                getline(cin, move);
            }
            network.sendData(clientSocket, move);
        }
    }
    network.closeSocket(clientSocket);
}

