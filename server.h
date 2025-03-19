#ifndef SERVER_H
#define SERVER_H
#include "game.h"
#include "network.h"
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <set>
#include <map>
using namespace std;
class GameServer {
private:
    Network network;
    int serverSocket;
    queue<int> waitingClients;
    vector<thread> gameThreads;
    mutex queueMutex;
    set<int> connectedClients;
    void handleClient(int clientSocket, map<int, string>& clientUsernames);

public:
    GameServer(int port);
    void run();
    void startGame(int client1, int client2);
};

#endif
