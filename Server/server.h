#ifndef SERVER_H
#define SERVER_H
#include "games.h"
#include"network.h"
#include<thread> 
#include<mutex>
#include<condition_variable>
using namespace std;

class GameServer{
private:
	Network network;
	int serverSocket;
	int clientSockets[2];
	Game game;
	mutex gameMutex;
	condition_variable mainCv;
	string currentMove;
	bool moveReady;
}
public:
	GameServer(int port);
	void run();
};
#endif
