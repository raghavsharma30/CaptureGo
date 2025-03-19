#include "game.h"
#include <iostream>
#include <string>
#include <utility>
using namespace std;
Game::Game(int goal) : currentTurn(Player::BLACK), blackCaptures(0), whiteCaptures(0), gameover(false), winner(Player::NONE), captureGoal(goal) {
    board = new Player*[BOARD_SIZE];
    for (int i = 0; i < BOARD_SIZE; i++) {
        board[i] = new Player[BOARD_SIZE];
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = Player::NONE;
        }
    }
}
Game::~Game() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        delete[] board[i];
    }
    delete[] board;
}

int Game::countLiberties(int x, int y, set<pair<int, int>>& visited, Player player) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || visited.count({x, y}))
        return 0;
    if (board[x][y] == Player::NONE)
        return 1;
    if (board[x][y] != player)
        return 0;
    visited.insert({x, y});
    int liberties = 0;
    liberties += countLiberties(x + 1, y, visited, player);
    liberties += countLiberties(x - 1, y, visited, player);
    liberties += countLiberties(x, y + 1, visited, player);
    liberties += countLiberties(x, y - 1, visited, player);
    return liberties;
}
bool Game::isValidMove(int x, int y, Player player) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || board[x][y] != Player::NONE)
        return false;
    board[x][y] = player;
    set<pair<int, int>> visited;
    int liberties = countLiberties(x, y, visited, player);
    board[x][y] = Player::NONE;
    return liberties > 0;
}
void Game::captureStones(int x, int y, Player opponent) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || board[x][y] != opponent)
        return;
    set<pair<int, int>> visited;
    if (countLiberties(x, y, visited, opponent) == 0) {
        for (const auto& pos : visited) {
            board[pos.first][pos.second] = Player::NONE;
            if (opponent == Player::BLACK)
                whiteCaptures++;
            else if (opponent == Player::WHITE)
                blackCaptures++;
        }
    }
}
void Game::switchTurn() {
    currentTurn = (currentTurn == Player::BLACK ? Player::WHITE : Player::BLACK);
}
bool Game::makeMove(int x, int y) {
    if (gameover || !isValidMove(x, y, currentTurn))
        return false;
    board[x][y] = currentTurn;
    Player opponent = (currentTurn == Player::BLACK ? Player::WHITE : Player::BLACK);
    captureStones(x + 1, y, opponent);
    captureStones(x - 1, y, opponent);
    captureStones(x, y + 1, opponent);
    captureStones(x, y - 1, opponent);
    if (currentTurn == Player::BLACK && blackCaptures >= 1) {
        gameover = true;
        winner = Player::BLACK;
    }
    else if (currentTurn == Player::WHITE && whiteCaptures >= 1) {
        gameover = true;
        winner = Player::WHITE;
    }
    if (!gameover) {
        switchTurn();
    }
    return true;
}
void Game::displayBoard() const {
    cout << "  ";
    for (int j = 0; j < BOARD_SIZE; j++) cout << j << " ";
    cout << "\n +";
    for (int j = 0; j < BOARD_SIZE; j++) cout << "--";
    cout << "+\n";
    for (int i = 0; i < BOARD_SIZE; i++) { 
        cout << i << "|";
        for (int j = 0; j < BOARD_SIZE; j++) {
            char c = (board[i][j] == Player::NONE ? '0' : (board[i][j] == Player::BLACK ? 'B' : 'W'));
            cout << c << " ";
        }
        cout << "|\n";
    }
    cout << " +";
    for (int j = 0; j < BOARD_SIZE; j++) cout << "--";
    cout << "+\n";
    cout << "Current Turn: " << (currentTurn == Player::BLACK ? "BLACK" : "WHITE") << "\n";
    cout << "Black captures: " << blackCaptures << ", White captures: " << whiteCaptures << "\n"; 
    if (gameover) {
        cout << "Game Over! Winner: " << (winner == Player::BLACK ? "BLACK" : "WHITE") << "\n";
    }
}
string Game::serialize() const {
    string result;
    result += (currentTurn == Player::BLACK ? "BLACK" : "WHITE") + string("\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) { 
            if (board[i][j] == Player::NONE) result += "0";
            else if (board[i][j] == Player::BLACK) result += "1";
            else result += "2";
        }
        result += "\n";
    }
    result += to_string(blackCaptures) + " " + to_string(whiteCaptures) + "\n"; 
    result += to_string(gameover ? 1 : 0) + " " + to_string(winner == Player::NONE ? 0 : (winner == Player::BLACK ? 1 : 2)) + "\n";
    return result;
}
void Game::deserialize(const string& state) {
    size_t pos = 0;
    auto getNextLine = [&state, &pos]() -> string {
        size_t end = state.find('\n', pos);
        if (end == string::npos) return "";
        string line = state.substr(pos, end - pos);
        pos = end + 1;
        return line;
    };
    string line = getNextLine();
    currentTurn = (line == "BLACK" ? Player::BLACK : Player::WHITE);
    for (int i = 0; i < BOARD_SIZE; i++) {
        line = getNextLine();
        for (int j = 0; j < BOARD_SIZE; j++) {
            char cell = line[j];
            board[i][j] = (cell == '0' ? Player::NONE : (cell == '1' ? Player::BLACK : Player::WHITE));
        }
    }
    line = getNextLine();
    size_t spacePos = line.find(' ');
    blackCaptures = stoi(line.substr(0, spacePos));
    whiteCaptures = stoi(line.substr(spacePos + 1));
    line = getNextLine();
    spacePos = line.find(' ');
    gameover = (line.substr(0, spacePos) == "1");
    int win = stoi(line.substr(spacePos + 1));
    winner = (win == 0 ? Player::NONE : (win == 1 ? Player::BLACK : Player::WHITE));
}

pair<int, int>* Game::getEmptyPositions(int& count) const {
    count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == Player::NONE) count++;
        }
    }
    pair<int, int>* emptyPositions = new pair<int, int>[count];
    int idx = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == Player::NONE) {
                emptyPositions[idx] = {i, j}; 
                idx++;
            }
        }
    }
    return emptyPositions;
}
