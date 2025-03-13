#ifndef GAME_H
#define GAME_H
#include <string>
#include <set>
using namespace std;

const int BOARD_SIZE = 7;
enum class Player{ NONE, BLACK, WHITE };
class Game{
private : 
	Player** board;
	Player currentTurn;
	int blackCaptures;
	int whiteCaptures;
	bool gameover;
	Player winner;
	const int captureGoal;
	int countLiberties(int x, int y, set<pair<int, int>>& visited, Player player);
	bool isValidMove(int x, int y, Player player);
	void captureStones(int x, int y, Player opponent);
	void switchTurn();

public: 
	Game(int goal = 1);
	~Game();
	bool makeMove(int x. int y);
	void displayBoard() const;
	bool isGameOver() const{ return gameOver;}
	Player getWinner() const{ return winner;}
	Player getCurrentTurn(){ return currentTurun; }
	string serialize(const string& state);
	pair<int, int>* getEmptyPositions(int& count) const;
};
#endif

