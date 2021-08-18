#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <optional>

#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "search.hpp"
#include "game.hpp"

#ifndef TESTING
#include <emscripten.h>
#include <emscripten/bind.h>
#include <wasm_simd128.h>
#include <emscripten/fetch.h>
using namespace emscripten;
#endif
#include <immintrin.h>

Board game_board;
NNUE nnue; //(static_cast<Side>(game_board.get_side_to_move()));
Evaluate board_evaluate(game_board, nnue);
Search search_engine(game_board, board_evaluate, nnue);

//dummy comment


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
void make_move(int from, int to, bool promotion, int promote_to) {
    auto moves = game_board.get_moves();
    auto mov = Move(from, to, MoveType::QUIET);
    auto promote_type = MoveType::PROMOTE_TO_QUEEN;
    auto capture_promote_type = MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN;

    if (promotion) {
        switch (promote_to) {
            case 113:
                break;
            case 114:
                promote_type = MoveType::PROMOTE_TO_ROOK;
                capture_promote_type = MoveType::CAPTURE_AND_PROMOTE_TO_ROOK;
                break;
            case 98:
                promote_type = MoveType::PROMOTE_TO_BISHOP;
                capture_promote_type = MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP;
                break;
            case 110:
                promote_type = MoveType::PROMOTE_TO_KNIGHT;
                capture_promote_type = MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT;
                break;
        }
    }

    for (auto &m : moves) {
        if (m.origin_square() == from && m.destination_square() == to) {
            if (promotion) {
                std::cout << static_cast<uint16_t>(m.type()) << std::endl;
                if (m.type() == promote_type || m.type() == capture_promote_type) {
                    std::cout << "sometihing happened!" << std::endl;
                    mov = m;
                    break;
                }
            } else {
                mov = m;
                break;
            }
        }
    }
    auto side = game_board.get_side_to_move() ? Side::BLACK : Side::WHITE;
    auto pieces_involved = game_board.make_move(mov);
    uint8_t white_king_square = __builtin_ffsll(game_board.get_kings()[0]) - 1;
    uint8_t black_king_square = __builtin_ffsll(game_board.get_kings()[1]) - 1;
    
    int next_eval;

    if (pieces_involved[0].value() != KING) {
        nnue.update_non_king_move(mov, pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
    } else {
        nnue.reset_nnue(mov, pieces_involved[1], white_king_square, black_king_square, side, game_board);
    }
    // game_board.make_move(mov);
}
}

// Returns all of the possible moves in the position.
std::vector<std::string> get_moves() {
    std::vector<std::string> moves = game_board.get_moves_algebraic();
    std::cout << "moves:" << moves.size() << std::endl;
    for (int i = 0; i < moves.size(); ++i)
        std::cout << moves.at(i) << ' ';
    std::cout << "end" << std::endl;
    return moves;
}

// Returns engine's best move
std::string get_engine_move() {
    //Search test = Search(game_board);
    Move move = search_engine.get_engine_move();
    game_board.make_move(move);
    return move.origin_square_algebraic() + move.destination_square_algebraic();
}

// Returns engine's evaluation for the position
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
double get_engine_evaluation() {
    return board_evaluate.evaluate() * (game_board.get_side_to_move() ? -1 : 1);
}
}

// Returns engine's best / principal variation for the current game_board position.
std::vector<Move> get_principal_variation() {
    return search_engine.get_principal_variation();
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

// Starts a game from a position / puzzle
extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
void start_from_position(std::string fen) {
    game_board = Board(fen);
    return;
}
}

// Runs a test on the DB of FEN positions, returns the failed positions engine PV.
std::vector<std::string> test_position(std::string fen, std::string move) {
    start_from_position(fen);
    Move start_move = Move(move.substr(0, 2), move.substr(2, 4));  // BUG: this move representation doesnt work
    std::cout << "test_position " << move << " " << start_move << std::endl;
    game_board.make_move(start_move);

    search_engine.get_engine_move();
    std::vector<Move> pv = search_engine.get_principal_variation();
    std::vector<std::string> pv_algebraic;

    for (int i = 0; i < pv.size(); i++) {
        pv_algebraic.push_back(pv[i].origin_square_algebraic() + pv[i].destination_square_algebraic());
    }

    return pv_algebraic;
}

#ifndef TESTING 
void on_succeed(emscripten_fetch_t* fetch) {
    std::cout << "network loading succeeded with " << fetch->numBytes << " bytes downloaded" << std::endl;
    NNUE new_nnue(Side::WHITE, fetch);
    std::cout << "START OF NNUE TESTING" << std::endl;
    // Board a("r3k2r/p6p/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // Board a("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/4K2R w Kkq - 0 1");
    // Board a("r1bqkbnr/pppppppp/2n5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2");
    //Board a("rnbqkbnr/1ppppppp/p7/8/2PPP3/2N2N2/PP2BPPP/R1BQ1RK1 w kq - 0 8");
    // Board a("2bqkbnr/pppppppp/8/4P3/3P4/2NB1N2/PPP2PPP/R1BQK2R w KQkq - 1 7");
    Board a;
    new_nnue.ready = true;
    new_nnue.reset_nnue(Move(), std::nullopt, 4, 60, Side::WHITE, a);
    auto eval = new_nnue.evaluate(32, Side::BLACK);
    std::cout << "Eval: " << eval << std::endl;
    std::cout << "END OF NNUE TESTING" << std::endl;
    nnue = std::move(new_nnue);
    nnue.ready = true;
    emscripten_fetch_close(fetch);
}

void on_fail(emscripten_fetch_t* fetch) {
    std::cout << "failed to load network" << std::endl;
    emscripten_fetch_close(fetch);
}
#endif

// Main function.
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int main() {
    auto side = game_board.get_side_to_move() ? Side::BLACK : Side::WHITE;
    uint8_t white_king_square = __builtin_ffsll(game_board.get_kings()[0]) - 1;
    uint8_t black_king_square = __builtin_ffsll(game_board.get_kings()[1]) - 1;
    nnue.reset_nnue(Move(), std::nullopt , white_king_square, black_king_square, side, game_board);

    std::cout << "attempting to load network" << std::endl;
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
    attr.onsuccess = on_succeed;
    attr.onerror = on_fail;
    emscripten_fetch(&attr, "https://storage.googleapis.com/spareduck/network.bin");

}

#ifndef TESTING
EMSCRIPTEN_BINDINGS(module) {
    // Function bindings
    function("get_moves", &get_moves);
    register_vector<std::string>("vector<std::string>");
    function("get_engine_move", &get_engine_move);
    function("get_principal_variation", &get_principal_variation);
    register_vector<Move>("vector<Move>");
    function("test_position", &test_position);
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
    
    // class_<Game>("Game")
    //     .constructor<>()
    //     .function("make_move", &Game::make_move)
    //     .function("get_moves", &Game::get_moves)
    //     .function("get_engine_moves()", &Game::get_engine_moves)
    //     .function("get_engine_evaluation", &Game::get_engine_evaluation)
    //     .function("get_principal_variation", &Game::get_principal_variation)
    //     .function("in_check", &Game::in_check)
    //     .function("start_from_position", &Game::start_from_position)
    //     .function("test_position", &Game::test_position)
    //     ;

}
#endif