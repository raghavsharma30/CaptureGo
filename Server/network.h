#ifndef NETWORK_H
#define NETWORK_H
#include <string>
using namespace std;
class Network {
public:
    Network();
    ~Network();
    int createServerSocket(int port);
    int acceptClient(int serverSocket);
    int createClientSocket(const std::string& serverIp, int port);
    bool sendData(int socket, const std::string& data);
    std::string receiveData(int socket);
    void closeSocket(int socket);
private:
    void initialize();
    void cleanup();
};
#endif
