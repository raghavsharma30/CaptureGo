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
string connectedUsers;
string waitingPlayers;

Piece getOpponentPiece(Piece piece) {
    return piece == Piece::BLACK ? Piece::WHITE : Piece::BLACK;
}

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
    auto chains = b.listChainsAndLiberties();
    const ChainAndLiberties* target = nullptr;
    size_t minLiberties = numeric_limits<size_t>::max();
    for (const auto& chain : chains) {
        if (chain.getPiece() != piece && chain.getLiberties().size() < minLiberties) {
            minLiberties = chain.getLiberties().size();
            target = &chain;
        }
    }
    if (!target || target->getLiberties().empty()) {
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
    } else {
        vector<Point> libs(target->getLiberties().begin(), target->getLiberties().end());
        uniform_int_distribution<size_t> dist(0, libs.size() - 1);
        Point move = libs[dist(rng)];
        return move.x * GRID_SIZE + move.y;
    }
}

int HumanPlayer::getMove(const Board& b) {
    return -1; // Handled via GUI
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
                    int move = row * GRID_SIZE + col;
                    if (board.checkMove(row, col)) {
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
                Piece piece = (tokens[2] == "BLACK") ? Piece::BLACK : Piece::WHITE;
                board.makeMove(row, col, piece);
                myTurn = (piece != myPiece); // My turn if opponent just moved
            }
            else if (tokens[0] == "ERROR" && state == GameState::IN_GAME) {
                if (tokens[1] == "Invalid Move") {
                    ImGui::OpenPopup("Invalid Move");
                    myTurn = true; // Retry on invalid move
                }
            }
            else if (tokens[0] == "GAMEOVER") {
                state = GameState::GAME_OVER;
                if (tokens[1] == "VICTORY") {
                    cout << (tokens[2] == username ? "You won!\n" : "You lost!\n");
                } else if (tokens[1] == "DISCONNECT") {
                    cout << "Opponent disconnected!\n";
                }
            }
            else if (tokens[0] == "USERLIST") {
                connectedUsers = tokens[1];
            }
            else if (tokens[0] == "QUEUELIST") {
                waitingPlayers = tokens[1];
            }
        }

        // AI move logic: Continuous execution when it's my turn
        if (state == GameState::IN_GAME && myTurn && isAI && aiPlayer != nullptr) {
            int move = aiPlayer->getMove(board);
            if (move != -1) {
                Network::sendData(sock, "MOVE~" + to_string(move) + "\n");
                myTurn = false;
            }
            SDL_Delay(100); // Small delay to prevent flooding server
        }

        SDL_SetRenderDrawColor(renderer, 245, 222, 179, 255);
        SDL_RenderClear(renderer);

        if (state == GameState::IN_GAME || state == GameState::GAME_OVER) {
            drawBoard(board);
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (state == GameState::DISCONNECTED) {
            ImGui::SetNextWindowSize(ImVec2(300, 150));
            ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoResize);
            if (ImGui::Button("Connect to Server", ImVec2(260, 40))) state = GameState::CONNECTING;
            if (ImGui::Button("Exit", ImVec2(260, 40))) quit = true;
            ImGui::End();
        }
        else if (state == GameState::CONNECTING) {
            ImGui::SetNextWindowSize(ImVec2(300, 200));
            ImGui::Begin("Connect", nullptr, ImGuiWindowFlags_NoResize);
            static char ipBuf[64] = "127.0.0.1";
            static char portBuf[64] = "8081";
            static char userBuf[64] = "Player";
            ImGui::InputText("IP", ipBuf, 64);
            ImGui::InputText("Port", portBuf, 64);
            ImGui::InputText("Username", userBuf, 64);
            if (ImGui::Button("Connect", ImVec2(260, 40))) {
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
                    if (helloResponse == "HELLO~CaptureGo Server\n") {
                        Network::sendData(sock, "LOGIN~" + username + "\n");
                        string response;
                        for (int i = 0; i < 5; ++i) {
                            response = Network::receiveData(sock);
                            if (!response.empty()) break;
                            SDL_Delay(100);
                        }
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
            if (ImGui::BeginPopupModal("Connection Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Failed to connect to server.");
                if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Login Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Username taken or invalid.");
                if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::End();
        }
        else if (state == GameState::CONNECTED_MENU) {
            ImGui::SetNextWindowSize(ImVec2(400, 300));
            ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoResize);
            ImGui::Text("Menu:");
            if (ImGui::Button("1. List users on the server", ImVec2(360, 40))) {
                Network::sendData(sock, "LISTUSERS\n");
            }
            if (ImGui::Button("2. Enter as a player (human)", ImVec2(360, 40))) {
                isAI = false;
                Network::sendData(sock, "QUEUE\n");
                state = GameState::IN_QUEUE;
            }
            if (ImGui::Button("3. Enter as AI", ImVec2(360, 40))) {
                isAI = true;
                Network::sendData(sock, "QUEUE\n");
                state = GameState::IN_QUEUE;
            }
            if (ImGui::Button("4. Exit", ImVec2(360, 40))) {
                quit = true;
            }
            if (ImGui::Button("5. Players waiting for opponent", ImVec2(360, 40))) {
                Network::sendData(sock, "LISTQUEUE\n");
            }
            ImGui::Text("Connected users: %s", connectedUsers.c_str());
            ImGui::Text("Players waiting: %s", waitingPlayers.c_str());
            ImGui::End();
        }
        else if (state == GameState::IN_QUEUE) {
            ImGui::SetNextWindowSize(ImVec2(300, 100));
            ImGui::Begin("Queue", nullptr, ImGuiWindowFlags_NoResize);
            ImGui::Text("Waiting for an opponent...");
            ImGui::End();
        }
        else if (state == GameState::IN_GAME) {
            ImGui::SetNextWindowSize(ImVec2(300, 100));
            ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoResize);
            if (myTurn) {
                ImGui::Text("Your turn as %s vs %s", myPiece == Piece::BLACK ? "Black" : "White", opponentName.c_str());
                if (!isAI) {
                    ImGui::Text("Click the board to place your stone.");
                }
            } else {
                ImGui::Text("Opponent's turn (%s)", opponentName.c_str());
            }
            if (ImGui::BeginPopupModal("Invalid Move", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Invalid move! Try again.");
                if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::End();
        }
        else if (state == GameState::GAME_OVER) {
            ImGui::SetNextWindowSize(ImVec2(300, 150));
            ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoResize);
            Piece winner = board.getWinner();
            if (winner == myPiece) ImGui::Text("You won!");
            else if (winner != Piece::NONE) ImGui::Text("You lost!");
            else ImGui::Text("Game ended (disconnected?)");
            if (ImGui::Button("Back to Menu", ImVec2(260, 40))) {
                state = GameState::CONNECTED_MENU;
                if (aiPlayer) {
                    delete aiPlayer;
                    aiPlayer = nullptr;
                }
            }
            ImGui::End();
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
