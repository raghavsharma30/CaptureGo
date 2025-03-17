#ifndef SERVER_H
#define SERVER_H
#include "game.h"
#include "network.h"
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
using namespace std;
class GameServer {
private:
    Network network;
    int serverSocket;
    queue<int> waitingClients;
    vector<thread> gameThreads;
    mutex queueMutex;

public:
    GameServer(int port);
    void run();
    void startGame(int client1, int client2);
};
#endif
