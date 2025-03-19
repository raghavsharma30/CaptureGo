#include "client.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
GameClient::GameClient(int socket, const string& username, bool isAI, const string& role) 
    : clientSocket(socket), username(username), isAI(isAI) {
    srand(time(nullptr));
    if (role == "BLACK") {
        playerRole = Player::BLACK;
        cout << "Assigned role: BLACK\n";
    } else if (role == "WHITE") {
        playerRole = Player::WHITE;
        cout << "Assigned role: WHITE\n";
    } else {
        cerr << "Invalid role: " << role << "\n";
        network.closeSocket(clientSocket);
        exit(1);
    }
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
