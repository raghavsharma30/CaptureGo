#ifndef CLIENT_H
#define CLIENT_H
#include "game.h"
#include "network.h"
#include <random>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
using namespace std;

class ClientPlayer {
protected:
    int socket;           
    string username;       
    Piece piece;          
public:
    ClientPlayer(int socket, const string& username) : socket(socket), username(username), piece(Piece::NONE) {}
    ClientPlayer(int socket, const string& username, Piece piece) : socket(socket), username(username), piece(piece) {}
    virtual ~ClientPlayer() = default;
    string getUsername() const { return username; }
    Piece getPiece() const { return piece; }
    Piece getOpponentPiece() const { return piece == Piece::BLACK ? Piece::WHITE : Piece::BLACK; }
    int getSocket() const { return socket; }
    virtual int getMove(const Board& b) = 0;
};

class HumanPlayer : public ClientPlayer {
public:
    HumanPlayer(int socket, const string& username) : ClientPlayer(socket, username) {}
    HumanPlayer(int socket, const string& username, Piece piece) : ClientPlayer(socket, username, piece) {}
    int getMove(const Board& b) override;
};

class AIPlayer : public ClientPlayer {
private:
    mt19937 rng;
public:
    AIPlayer(int socket, const string& username);
    AIPlayer(int socket, const string& username, Piece piece);
    int getMove(const Board& b) override;
};

class Client {
public:
    static void run();
};
#endif
