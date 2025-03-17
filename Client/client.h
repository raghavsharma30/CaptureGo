#ifndef CLIENT_H
#define CLIENT_H
#include "game.h"
#include "network.h"
#include <string>
using namespace std;
class GameClient {
private:
    Network network;
    int clientSocket;
    Game game;
    Player playerRole;
    string username;
    bool isAI;

public:
    GameClient(const string& serverIp, int port, const string& username, bool isAI);
    void run();
};
#endif
