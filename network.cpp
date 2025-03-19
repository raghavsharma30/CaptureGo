#include "network.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;
int Network::createServerSocket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) return -1;
    if (listen(sock, 5) < 0) return -1;
    return sock;
}

int Network::createClientSocket(const string& serverIp, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) return -1;
    return sock;
}

int Network::acceptClient(int serverSocket) {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    return accept(serverSocket, (sockaddr*)&clientAddr, &addrLen);
}

void Network::sendData(int socket, const string& data) {
    send(socket, data.c_str(), data.length(), 0);
}

string Network::receiveData(int socket) {
    char buffer[1024] = {0};
    int bytesRead = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) return "";
    return string(buffer, bytesRead);
}

void Network::closeSocket(int socket) {
    close(socket);
}
