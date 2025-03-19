#ifndef GAME_H
#define GAME_H
#include <string>
#include <set>
#include <utility>
using namespace std;
enum class Player { NONE, BLACK, WHITE };
const int BOARD_SIZE = 7;

class Game {
private:
    Player** board;
    Player currentTurn;
    int blackCaptures;
    int whiteCaptures;
    bool gameover;
    Player winner;
    int captureGoal;
    int countLiberties(int x, int y, set<pair<int, int>>& visited, Player player);

public:
    Game(int goal = 5);
    ~Game();
    bool isValidMove(int x, int y, Player player);
    void captureStones(int x, int y, Player opponent);
    void switchTurn();
    bool makeMove(int x, int y);
    void displayBoard() const;
    string serialize() const;
    void deserialize(const string& state);
    pair<int, int>* getEmptyPositions(int& count) const;
    bool isGameOver() const { return gameover; }
    Player getCurrentTurn() const { return currentTurn; }
};

#endif
