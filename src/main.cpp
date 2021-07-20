#include <iostream>
#include <string>
#include <vector>

#include "board.h"
#include "move.h"

#ifndef TESTING
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
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
void make_move(int from, int to) {
    game_board.make_move(Move(from, to, MoveType::QUIET));
    return;
}
}

// Returns all of the possible moves in the position.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
std::vector<uint16_t> get_moves() {
    std::cout << "moves:" << std::endl;
    std::vector<uint16_t> moves = game_board.get_moves_as_u16();
    std::cout << moves.size() << std::endl;
    for (int i = 0; i < moves.size(); ++i)
        std::cout << moves.at(i) << ' ';
    std::cout << "end" << std::endl;
    return moves;
}
}

// Returns true if the king is in check.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
bool in_check() {
    // TODO: write in check method in Board
    return game_board.in_check();
}
}

// Main function initializes a new board.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int main() {
    std::cout << "hello" << std::endl;
}
}


#ifndef TESTING
EMSCRIPTEN_BINDINGS(module) {
    function("get_moves", &get_moves);
    register_vector<uint16_t>("vector<uint16_t>");
}
#endif