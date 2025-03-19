#ifndef NETWORK_H
#define NETWORK_H
#include <string>
using namespace std;
class Network {
public:
    int createServerSocket(int port);
    int createClientSocket(const string& serverIp, int port);
    int acceptClient(int serverSocket);
    void sendData(int socket, const string& data);
    string receiveData(int socket);
    void closeSocket(int socket);
};

#endif
