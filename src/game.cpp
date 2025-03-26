#include "../include/game.h"
// ... rest unchanged
#include <iostream>
#include <iomanip>
using namespace std;

void ChainAndLiberties::print() const {
    cout << (piece == Piece::BLACK ? "Black" : "White") << " Chain: ";
    for (const auto& p : chain) {
        cout << "(" << p.x << ", " << p.y << ") ";
    }
    cout << ", Liberties: " << liberties.size() << "\n";
}

Board::Board() : grid(DIMENSION, vector<Piece>(DIMENSION, Piece::NONE)) {}

void Board::printBoard() const {
    cout << "\n   ";
    for (int col = 1; col <= DIMENSION; ++col) {
        cout << setw(2) << col << " ";
    }
    cout << "\n";
    for (int row = 0; row < DIMENSION; ++row) {
        cout << setw(2) << (row + 1) << " ";
        for (int col = 0; col < DIMENSION; ++col) {
            char symbol = grid[row][col] == Piece::BLACK ? 'B' : (grid[row][col] == Piece::WHITE ? 'W' : '0');
            cout << setw(2) << symbol << " ";
        }
        cout << "\n";
    }
}

bool Board::checkMove(int x, int y) const {
    return x >= 0 && x < DIMENSION && y >= 0 && y < DIMENSION && grid[x][y] == Piece::NONE;
}

bool Board::makeMove(int x, int y, Piece p) {
    if (checkMove(x, y)) {
        grid[x][y] = p;
        return true;
    }
    return false;
}

bool Board::makeMove(int move, Piece p) {
    int x = move / DIMENSION;
    int y = move % DIMENSION;
    return makeMove(x, y, p);
}

void Board::findChainAndLiberties(int x, int y, Piece piece, vector<vector<bool>>& visited, 
                                  vector<Point>& chain, unordered_set<Point>& liberties) const {
    vector<pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    queue<Point> q;
    q.push(Point(x, y));
    visited[x][y] = true;
    while (!q.empty()) {
        Point current = q.front();
        q.pop();
        chain.push_back(current);
        for (const auto& dir : directions) {
            int newX = current.x + dir.first;
            int newY = current.y + dir.second;
            if (newX >= 0 && newX < DIMENSION && newY >= 0 && newY < DIMENSION) {
                if (grid[newX][newY] == piece && !visited[newX][newY]) {
                    visited[newX][newY] = true;
                    q.push(Point(newX, newY));
                } else if (grid[newX][newY] == Piece::NONE) {
                    liberties.insert(Point(newX, newY));
                }
            }
        }
    }
}

vector<ChainAndLiberties> Board::listChainsAndLiberties() const {
    vector<ChainAndLiberties> result;
    vector<vector<bool>> visited(DIMENSION, vector<bool>(DIMENSION, false));
    for (int i = 0; i < DIMENSION; ++i) {
        for (int j = 0; j < DIMENSION; ++j) {
            if (!visited[i][j] && grid[i][j] != Piece::NONE) {
                vector<Point> chain;
                unordered_set<Point> liberties;
                findChainAndLiberties(i, j, grid[i][j], visited, chain, liberties);
                result.emplace_back(chain, liberties, grid[i][j]);
            }
        }
    }
    return result;
}

Piece Board::getWinner() const {
    for (const auto& chain : listChainsAndLiberties()) {
        if (chain.getLiberties().empty()) {
            return chain.getPiece() == Piece::BLACK ? Piece::WHITE : Piece::BLACK;
        }
    }
    return Piece::NONE;
}

Piece Board::getPiece(int x, int y) const {
    return grid[x][y];
}
