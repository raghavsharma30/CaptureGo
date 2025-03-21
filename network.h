#ifndef NETWORK_H
#define NETWORK_H
#include <string>
#include <vector>
using namespace std;
class Network {
public:
    static int createClientSocket(const string& hostname, int port);
    static string receiveData(int socket);
    static void sendData(int socket, const string& data);
    static void closeSocket(int socket);
};
vector<string> split(const string& s, char delimiter);
#endif
