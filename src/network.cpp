#include "../include/network.h"
#include <sys/socket.h>    // For socket(), connect(), etc.
#include <netinet/in.h>    // For sockaddr_in, htons(), etc.
#include <arpa/inet.h>     // For inet_addr()
#include <unistd.h>        // For close()
#include <fcntl.h>         // For fcntl()
#include <poll.h>          // For poll()
#include <errno.h>         // For errno
#include <string.h>        // For strerror()
#include <iostream>        // For cout
#include <string>          // For std::string
using namespace std;

int Network::createClientSocket(const string& hostname, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cout << "Failed to create client socket: " << strerror(errno) << "\n";
        return -1;
    }
    
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(hostname.c_str());

    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
    if (result < 0) {
        if (errno == EINPROGRESS) {
            struct pollfd pfd;
            pfd.fd = sock;
            pfd.events = POLLOUT;
            int poll_result = poll(&pfd, 1, 5000);
            if (poll_result > 0) {
                int valopt;
                socklen_t lon = sizeof(int);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)&valopt, &lon);
                if (valopt) {
                    cout << "Connect failed: " << strerror(valopt) << "\n";
                    close(sock);
                    return -1;
                }
            } else {
                cout << "Connection timed out or failed\n";
                close(sock);
                return -1;
            }
        } else {
            cout << "Connect failed: " << strerror(errno) << "\n";
            close(sock);
            return -1;
        }
    }

    // Keep socket non-blocking (as per previous fix for GUI hang)
    return sock;
}

string Network::receiveData(int socket) {
    char buffer[1024] = {0};
    int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        return string(buffer, bytes);
    }
    return "";
}

void Network::sendData(int socket, const string& data) {
    send(socket, data.c_str(), data.size(), 0);
    fsync(socket);  // Ensure data is sent immediately
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
