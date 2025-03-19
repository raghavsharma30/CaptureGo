#include "client.h"
#include <iostream>
#include <cstdlib>
using namespace std;
int main() {
    Network network;
    int clientSocket;
    string username;
    while (true) {
        cout << "Enter username: ";
        getline(cin, username);
        clientSocket = network.createClientSocket("127.0.0.1", 8080);
        if (clientSocket < 0) {
            cerr << "Failed to connect to server\n";
            return 1;
        }
        network.sendData(clientSocket, "LIST_USERS " + username);
        string response = network.receiveData(clientSocket);
        if (response == "Username already taken") {
            cout << "Username already taken, please choose another\n";
            network.closeSocket(clientSocket);
            continue;
        }
        break;
    }
    while (true) {
        cout << "Menu:\n1. List users on the server\n2. Enter as a player (human)\n3. Enter as AI\n4. Exit\n5. Players waiting for opponent\nChoice: ";
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
            } else {
                cout << "Users on the server:\n" << userList;
            }
            continue;
        }
        if (choice == "5") {
            network.sendData(clientSocket, "LIST_WAITING " + username);
            string waitingList = network.receiveData(clientSocket);
            if (waitingList.empty()) {
                cout << "No response from server or connection lost\n";
            } else {
                cout << "Players waiting for opponent:\n" << waitingList;
            }
            continue;
        }
        if (choice == "2" || choice == "3") {
            network.sendData(clientSocket, "JOIN_GAME " + username + " " + (choice == "2" ? "HUMAN" : "AI"));
            string response = network.receiveData(clientSocket);
            string role;
            if (response == "Waiting") {
                cout << "Waiting in queue...\n";
                role = network.receiveData(clientSocket);
            } else {
                role = response;
            }
            if (role == "BLACK" || role == "WHITE") {
                GameClient client(clientSocket, username, choice == "3", role);
                client.run();
                cout << "Game has ended, returning to menu...\n";
                clientSocket = network.createClientSocket("127.0.0.1", 8080);
                if (clientSocket < 0) {
                    cerr << "Failed to reconnect to server\n";
                    return 1;
                }
                network.sendData(clientSocket, "LIST_USERS " + username);
                network.receiveData(clientSocket);
            } else {
                cout << "Failed to join game: " << role << "\n";
            }
            continue;
        }
        cout << "Invalid choice, please try again\n";
    }
    return 0;
}
