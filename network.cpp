#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
using namespace std;
int Network::createClientSocket(const string& hostname, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cout << "Failed to create client socket: " << strerror(errno) << "\n";
        return -1;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(hostname.c_str());
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "Connect failed: " << strerror(errno) << "\n";
        close(sock);
        return -1;
    }
    return sock;
}
string Network::receiveData(int socket) {
    char buffer[1024] = {0};
    int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    return bytes > 0 ? string(buffer, bytes) : "";
}
void Network::sendData(int socket, const string& data) {
    send(socket, data.c_str(), data.size(), 0);
}
void Network::closeSocket(int socket) {
    close(socket);
}
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    for (char c : s) {
        if (c == delimiter && !token.empty()) {
            tokens.push_back(token);
            token.clear();
        } else if (c != delimiter) {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}
