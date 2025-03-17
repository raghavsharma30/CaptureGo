#include "client.h"
#include <iostream>
#include <cstdlib>
using namespace std;
int main() {
    string username;
    cout << "Enter username: ";
    getline(cin, username);
    Network network;
    int clientSocket = network.createClientSocket("127.0.0.1", 8080); 
    if (clientSocket < 0) {
        cerr << "Failed to connect to server\n";
        return 1;
    }
    while (true) {
        cout << "Menu:\n1. List users on the server\n2. Enter as a player (human)\n3. Enter as AI\n4. Exit\nChoice: ";
        string choice;
        getline(cin, choice);
        if (choice == "4") {
            cout << "Exiting...\n";
            network.closeSocket(clientSocket);
            return 0;
        }
        if (choice == "1") {
            network.sendData(clientSocket, "LIST_USERS " + username);
            string userList = network.receiveData(clientSocket);
            if (userList.empty()) {
                cout << "No response from server or connection lost\n";
                network.closeSocket(clientSocket);
                clientSocket = network.createClientSocket("127.0.0.1", 8080); 
                if (clientSocket < 0) {
                    cerr << "Failed to reconnect to server\n";
                    return 1;
                }
            } else {
                cout << "Users on the server:\n" << userList << "\n";
            }
            continue;
        }
        if (choice == "2" || choice == "3") {
            network.closeSocket(clientSocket);
            bool isAI = (choice == "3");
            GameClient client("127.0.0.1", 8080, username, isAI);
            client.run();
            cout << "Game has ended, returning to menu...\n";
            clientSocket = network.createClientSocket("127.0.0.1", 8080);
            if (clientSocket < 0) {
                cerr << "Failed to reconnect to server\n";
                return 1;
            }
        } else {
            cout << "Invalid choice, please try again\n";
        }
    }
    return 0;
}
