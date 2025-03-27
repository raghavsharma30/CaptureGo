#include "../include/client.h"
#include <iostream>
#include <chrono>
#include <random>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 7;
const int CELL_SIZE = 70;
const int BOARD_OFFSET_X = (WINDOW_WIDTH - GRID_SIZE * CELL_SIZE) / 2;
const int BOARD_OFFSET_Y = (WINDOW_HEIGHT - GRID_SIZE * CELL_SIZE) / 2;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
bool quit = false;

int sock = -1;
string username;
Piece myPiece;
bool myTurn = false;
Board board;
enum class GameState { DISCONNECTED, CONNECTING, CONNECTED_MENU, IN_QUEUE, IN_GAME, GAME_OVER };
GameState state = GameState::DISCONNECTED;
bool isAI = false;
string opponentName;
AIPlayer* aiPlayer = nullptr; 

Piece getOpponentPiece(Piece piece);

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
        return false;
    }
    window = SDL_CreateWindow("Capture Go", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << "\n";
        return false;
    }
    if (TTF_Init() == -1) {
        cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << "\n";
        return false;
    }
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!font) {
        cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << "\n";
        return false;
    }
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    return true;
}

void closeSDL() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void drawBoard(const Board& board) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
    for (int i = 0; i <= GRID_SIZE; i++) {
        SDL_RenderDrawLine(renderer, BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE, 
                           BOARD_OFFSET_X + GRID_SIZE * CELL_SIZE, BOARD_OFFSET_Y + i * CELL_SIZE);
        SDL_RenderDrawLine(renderer, BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y, 
                           BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + GRID_SIZE * CELL_SIZE);
    }
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            Piece piece = board.getPiece(row, col);
            if (piece != Piece::NONE) {
                int x = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
                int y = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;
                SDL_SetRenderDrawColor(renderer, piece == Piece::BLACK ? 0 : 255, 
                                       piece == Piece::BLACK ? 0 : 255, piece == Piece::BLACK ? 0 : 255, 255);
                SDL_Rect circle = {x - CELL_SIZE / 3, y - CELL_SIZE / 3, CELL_SIZE * 2 / 3, CELL_SIZE * 2 / 3};
                SDL_RenderFillRect(renderer, &circle); 
            }
        }
    }
}

AIPlayer::AIPlayer(int socket, const string& username) 
    : ClientPlayer(socket, username), rng(chrono::system_clock::now().time_since_epoch().count()) {}

AIPlayer::AIPlayer(int socket, const string& username, Piece piece) 
    : ClientPlayer(socket, username, piece), rng(chrono::system_clock::now().time_since_epoch().count()) {}

int AIPlayer::getMove(const Board& b) {
    uniform_int_distribution<int> dist(0, GRID_SIZE * GRID_SIZE - 1); 
    for (int i = 0; i < 100; ++i) { 
        int move = dist(rng);
        int row = move / GRID_SIZE;
        int col = move % GRID_SIZE;
        if (b.checkMove(row, col)) {
            return move;
        }
    }
    return -1;
}

int HumanPlayer::getMove(const Board& b) {
    return -1; 
}

void Client::run() {
    if (!initSDL()) {
        cerr << "SDL initialization failed. Exiting.\n";
        return;
    }

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && state == GameState::IN_GAME && myTurn && !isAI) {
                int x = e.button.x;
                int y = e.button.y;
                if (x >= BOARD_OFFSET_X && x < BOARD_OFFSET_X + GRID_SIZE * CELL_SIZE &&
                    y >= BOARD_OFFSET_Y && y < BOARD_OFFSET_Y + GRID_SIZE * CELL_SIZE) {
                    int col = (x - BOARD_OFFSET_X) / CELL_SIZE;
                    int row = (y - BOARD_OFFSET_Y) / CELL_SIZE;
                    if (board.checkMove(row, col)) {
                        int move = row * GRID_SIZE + col;
                        Network::sendData(sock, "MOVE~" + to_string(move) + "\n");
                        myTurn = false; 
                    }
                }
            }
        }

        string message = (sock != -1) ? Network::receiveData(sock) : "";
        if (!message.empty()) {
            cout << "Received: " << message << "\n";
            auto tokens = split(message, '~');
            if (tokens[0] == "NEWGAME") {
                cout << "Starting new game with " << tokens[1] << " and " << tokens[2] << "\n";
                myPiece = (tokens[1] == username) ? Piece::BLACK : Piece::WHITE;
                opponentName = (tokens[1] == username) ? tokens[2] : tokens[1];
                myTurn = (myPiece == Piece::BLACK);
                state = GameState::IN_GAME;
                board = Board();
                if (isAI && aiPlayer == nullptr) {
                    aiPlayer = new AIPlayer(sock, username, myPiece);
                }
            }
            else if (tokens[0] == "MOVE" && state == GameState::IN_GAME) {
                int move = stoi(tokens[1]);
                int row = move / GRID_SIZE;
                int col = move % GRID_SIZE;
                board.makeMove(row, col, getOpponentPiece(myPiece));
                myTurn = true;
            }
            else if (tokens[0] == "ERROR" && state == GameState::IN_GAME) {
                if (tokens[1] == "Invalid Move") {
                    ImGui::OpenPopup("Invalid Move");
                    myTurn = true;
                }
            }
            else if (tokens[0] == "GAMEOVER") {
                cout << "Received GAMEOVER: " << tokens[1] << "\n";
                state = GameState::GAME_OVER;
            }
        }

        SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);  
        SDL_RenderClear(renderer);
s
        if (state == GameState::IN_GAME || state == GameState::GAME_OVER) {
            drawBoard(board);
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        switch (state) {
            case GameState::DISCONNECTED:
                ImGui::Begin("Main Menu");
                if (ImGui::Button("Connect to Server")) state = GameState::CONNECTING;
                if (ImGui::Button("Exit")) quit = true;
                ImGui::End();
                break;

            case GameState::CONNECTING:
                ImGui::Begin("Connect");
                static char ipBuf[64] = "127.0.0.1";
                static char portBuf[64] = "8081";
                static char userBuf[64] = "Player";
                ImGui::InputText("IP", ipBuf, 64);
                ImGui::InputText("Port", portBuf, 64);
                ImGui::InputText("Username", userBuf, 64);
                if (ImGui::Button("Connect")) {
                    string ip = ipBuf;
                    int port = stoi(portBuf);
                    username = userBuf;
                    sock = Network::createClientSocket(ip, port);
                    if (sock != -1) {
                        Network::sendData(sock, "HELLO~CaptureGo Client\n");
                        string helloResponse;
                        for (int i = 0; i < 5; ++i) {
                            helloResponse = Network::receiveData(sock);
                            if (!helloResponse.empty()) break;
                            SDL_Delay(100);
                        }
                        cout << "Received after HELLO: " << helloResponse << "\n";
                        if (helloResponse == "HELLO~CaptureGo Server\n") {
                            Network::sendData(sock, "LOGIN~" + username + "\n");
                            string response;
                            for (int i = 0; i < 5; ++i) {
                                response = Network::receiveData(sock);
                                if (!response.empty()) break;
                                SDL_Delay(100);
                            }
                            cout << "Received after LOGIN: " << response << "\n";
                            if (response == "LOGIN\n") {
                                state = GameState::CONNECTED_MENU;
                            } else {
                                ImGui::OpenPopup("Login Failed");
                            }
                        } else {
                            ImGui::OpenPopup("Connection Failed");
                        }
                    } else {
                        ImGui::OpenPopup("Connection Failed");
                    }
                }
                if (ImGui::BeginPopupModal("Connection Failed")) {
                    ImGui::Text("Failed to connect to server.");
                    if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("Login Failed")) {
                    ImGui::Text("Username taken or invalid.");
                    if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }
                ImGui::End();
                break;

            case GameState::CONNECTED_MENU:
                ImGui::Begin("Menu");
                if (ImGui::Button("Play as Human")) {
                    isAI = false;
                    Network::sendData(sock, "QUEUE\n");
                    state = GameState::IN_QUEUE;
                }
                if (ImGui::Button("Play as AI")) {
                    isAI = true;
                    Network::sendData(sock, "QUEUE\n");
                    state = GameState::IN_QUEUE;
                }
                if (ImGui::Button("Disconnect")) {
                    Network::closeSocket(sock);
                    sock = -1;
                    state = GameState::DISCONNECTED;
                }
                ImGui::End();
                break;

            case GameState::IN_QUEUE:
                ImGui::Begin("Queue");
                ImGui::Text("Waiting for an opponent...");
                ImGui::End();
                break;

            case GameState::IN_GAME:
                ImGui::Begin("Status");
                if (myTurn) {
                    ImGui::Text("Your turn as %s vs %s", myPiece == Piece::BLACK ? "Black" : "White", opponentName.c_str());
                    if (isAI && aiPlayer != nullptr) {
                        int move = aiPlayer->getMove(board);
                        if (board.checkMove(move / GRID_SIZE, move % GRID_SIZE)) {
                            Network::sendData(sock, "MOVE~" + to_string(move) + "\n");
                            myTurn = false; 
                        }
                    } else if (!isAI) {
                        ImGui::Text("Click the board to place your stone.");
                    }
                } else {
                    ImGui::Text("Opponent's turn (%s)", opponentName.c_str());
                }
                ImGui::End();

                if (ImGui::BeginPopupModal("Invalid Move", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Invalid move! Try again.");
                    if (ImGui::Button("OK")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                break;

            case GameState::GAME_OVER:
                ImGui::Begin("Game Over");
                Piece winner = board.getWinner();
                if (winner == myPiece) {
                    ImGui::Text("You won!");
                } else if (winner != Piece::NONE) {
                    ImGui::Text("You lost!");
                } else {
                    ImGui::Text("Game ended (disconnected?)");
                }
                if (ImGui::Button("Back to Menu")) {
                    state = GameState::CONNECTED_MENU;
                    if (aiPlayer) {
                        delete aiPlayer;
                        aiPlayer = nullptr; 
                    }
                }
                ImGui::End();
                break;
        }


        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    if (aiPlayer) {
        delete aiPlayer;
        aiPlayer = nullptr;
    }
    if (sock != -1) Network::closeSocket(sock);
    closeSDL();
}

Piece getOpponentPiece(Piece piece) {
    return piece == Piece::BLACK ? Piece::WHITE : Piece::BLACK;
}
