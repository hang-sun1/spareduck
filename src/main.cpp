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
    auto moves = game_board.get_moves();
    auto mov = Move(from, to, MoveType::QUIET);
    for (auto &m: moves) {
        if (m.origin_square() == from && m.destination_square() == to) {
            mov = m;
            break;
        }
    }
    game_board.make_move(mov);
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
    //Search test = Search(game_board);
    Move move = search_engine.get_engine_move();
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
    return board_eval.evaluate_cheap() * (game_board.get_side_to_move() ? -1 : 1);
}
}

// Returns engine's best / principal variation for the current position.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
std::vector<Move> get_principal_variation() {
    return search_engine.get_principal_variation();
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

// Runs a test on the DB of FEN positions, returns the failed positions as FEN
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
std::vector<std::string> test_positions() {
    std::vector<std::string> test;
    test.push_back("r2qkb1r/pp2nppp/3p4/2pNN1B1/2BnP3/3P4/PPP2PPP/R2bK2R w KQkq - 1 0");
    return test;
}
}

// Starts a game from a position / puzzle
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
bool start_from_position(std::string fen) {
    game_board = Board(fen);
    return true;
}
}

// Main function initializes a new board.
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int main() {
}
}

#ifndef TESTING
EMSCRIPTEN_BINDINGS(module) {
    // Function bindings
    function("get_moves", &get_moves);
    register_vector<std::string>("vector<std::string>");
    function("get_engine_move", &get_engine_move);
    function("get_principal_variation", &get_principal_variation);
    register_vector<Move>("vector<Move>");
    function("test_positions", &test_positions);
    function("start_from_position", &start_from_position);

    // MoveType and Move bindings
    enum_<MoveType>("MoveType")
        .value("QUIET", MoveType::QUIET)
        .value("DOUBLE_PAWN_PUSH", MoveType::DOUBLE_PAWN_PUSH)
        .value("CAPTURE", MoveType::CAPTURE)
        .value("SHORT_CASTLES", MoveType::SHORT_CASTLES)
        .value("LONG_CASTLES", MoveType::LONG_CASTLES)
        .value("PROMOTE_TO_KNIGHT", MoveType::PROMOTE_TO_KNIGHT)
        .value("PROMOTE_TO_QUEEN", MoveType::PROMOTE_TO_QUEEN)
        .value("PROMOTE_TO_ROOK", MoveType::PROMOTE_TO_ROOK)
        .value("PROMOTE_TO_BISHOP", MoveType::PROMOTE_TO_BISHOP)
        .value("CAPTURE_AND_PROMOTE_TO_KNIGHT", MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT)
        .value("CAPTURE_AND_PROMOTE_TO_QUEEN", MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN)
        .value("CAPTURE_AND_PROMOTE_TO_ROOK", MoveType::CAPTURE_AND_PROMOTE_TO_ROOK)
        .value("CAPTURE_AND_PROMOTE_TO_BISHOP", MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP)
        .value("EN_PASSANT", MoveType::EN_PASSANT);
    class_<Move>("Move")
        .constructor<uint16_t, uint16_t, MoveType>()
        .function("origin_square_algebraic", &Move::origin_square_algebraic)
        .function("destination_square_algebraic", &Move::destination_square_algebraic);
}
#endif