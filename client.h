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
    string username;
    Player playerRole;
    Game game;
    bool isAI;

public:
    GameClient(int socket, const string& username, bool isAI, const string& role);
    void run();
};

#endif
