#ifndef GAME_H
#define GAME_H
#include <vector>
#include <unordered_set>
#include <queue>
#include <string>
using namespace std;
enum class Piece {
    NONE,
    WHITE,
    BLACK
};
struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
    bool operator==(const Point& other) const {
    return x == other.x && y == other.y; 
    }
};
namespace std {
    template<> struct hash<Point> {
        size_t operator()(const Point& p) const { 
        return hash<int>()(p.x) ^ hash<int>()(p.y); 
        }
    };
}
class ChainAndLiberties {
private:
    vector<Point> chain;
    unordered_set<Point> liberties;
    Piece piece;

public:
    ChainAndLiberties(const vector<Point>& chain, const unordered_set<Point>& liberties, Piece piece) : chain(chain), liberties(liberties), piece(piece) {}
    const vector<Point>& getChain() const { return chain; }
    const unordered_set<Point>& getLiberties() const { 
    return liberties; 
    }
    Piece getPiece() const { return piece; }

    void print() const;
};
class Board {
private:
    static const int DIMENSION = 7;
    vector<vector<Piece>> grid;

    void findChainAndLiberties(int x, int y, Piece piece, vector<vector<bool>>& visited, vector<Point>& chain, unordered_set<Point>& liberties) const;
public:
    Board();

    void printBoard() const;
    bool checkMove(int x, int y) const;
    bool makeMove(int x, int y, Piece p);
    bool makeMove(int move, Piece p);
    vector<ChainAndLiberties> listChainsAndLiberties() const;
    Piece getWinner() const;
};
#endif
