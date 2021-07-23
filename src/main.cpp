#include <iostream>
#include <string>
#include <vector>

#include "board.h"
#include "evaluate.h"
#include "move.h"
#include "search.h"

#ifndef TESTING
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
#endif

Board game_board;
Evaluate board_eval = Evaluate(game_board);
Search search_engine = Search(game_board);

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
std::vector<std::string> get_moves() {
    std::vector<std::string> moves = game_board.get_moves_algebraic();
    std::cout << "moves:" << moves.size() << std::endl;
    for (int i = 0; i < moves.size(); ++i)
        std::cout << moves.at(i) << ' ';
    std::cout << "end" << std::endl;
    return moves;
}
}

// Returns engine's best move
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
std::string get_engine_move() {
    Search test = Search(game_board);
    Move move = test.get_engine_move();
    game_board.make_move(move);
    return move.origin_square_algebraic() + move.destination_square_algebraic();
}
}

// Returns engine's evaluation for the position
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
double get_engine_evaluation() {
    return board_eval.static_evaluate_cheap(game_board);
}
}

// Returns true if the king is in check.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
bool in_check() {
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
    register_vector<std::string>("vector<std::string>");
    function("get_engine_move", &get_engine_move);
}
#endif