#include <iostream>

#include "board.h"

#ifndef TESTING
#include <emscripten.h>
#endif

Board game_board;

extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int add_two(int a, int b) { return a + b; }
}

// Returns side to move.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int get_side_to_move() { return game_board.get_side_to_move(); }
}

// Plays a move on the board.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
void make_move() {
    // TODO: convert move from algebraic or implement make_move_algebraic in game_board
    //game_board.make_move();
    return;
}
}

// Returns all of the possible moves in the position.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
std::vector<std::string> generate_moves() {
    // TODO: convert moves to map of square->squares in algebraic notation
    //return game_board.get_moves();
    std::vector<std::string> test;
    test.push_back("oyoy");
    return test;
}
}

// Returns true if the king is in check.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
bool in_check() {
    // TODO: write in check method in Board
    //return game_board.in_check();
    return false;
}
}

// Main function initializes a new board.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int main() {
    std::cout << "hello" << std::endl;
    game_board = Board();
}
}
