#ifndef SERVER_H
#define SERVER_H
#include "game.h"
#include "network.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
using namespace std;

class Player {
private:
    Piece piece;
    string username;
    int socket;
    mutex mtx;
    condition_variable cv;
public:
    Player(int socket, const string& username) : piece(Piece::NONE), username(username), socket(socket) {}
    Player(int socket, const string& username, Piece piece) : piece(piece), username(username), socket(socket) {}
    string getUsername() const { return username; }
    Piece getPiece() const { return piece; }
    Piece getOpponentPiece() const { return piece == Piece::BLACK ? Piece::WHITE : Piece::BLACK; }
    void setPiece(Piece piece) { this->piece = piece; }
    int getSocket() const { return socket; }
    mutex& getMutex() { return mtx; }
    condition_variable& getCondition() { return cv; }
};

class Game {
private:
    int currentPlayer;
    Board board;
    Player* players[2];
    int sockets[2];
public:
    Game(Player* p1, Player* p2);
    void run();
};

class Server {
private:
    vector<Player*> connectedPlayers;
    queue<Player*> gameQueue;
    mutex queueMutex;
    condition_variable queueCond;
    void processQueue();
    Game* waitForPlayers();
    void startGame(Game* game);
    void handleClient(int sock);
public:
    static bool isPortInUse(int port);
    void run();
};
#endif
