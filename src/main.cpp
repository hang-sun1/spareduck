#include <iostream>
#include <string>
#include <vector>

#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "search.hpp"
#include "game.hpp"

#ifndef TESTING
#include <emscripten.h>
#include <emscripten/bind.h>
#include <wasm_simd128.h>
using namespace emscripten;
#endif
#include <immintrin.h>

Board game_board;
Evaluate board_evaluate(game_board);
Search search_engine(game_board);

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
    game_board.make_move(mov);
    return;
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
    return board_evaluate.evaluate_cheap() * (game_board.get_side_to_move() ? -1 : 1);
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

// Main function.
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int main() {
    alignas(16) int8_t a[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    alignas(16) std::array<int8_t, 16> b = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    alignas(16) std::array<int16_t, 8> output;
    __m128i in_a = _mm_load_si128((__m128i *)&a);
    __m128i in_b = _mm_load_si128((__m128i *)&b);
    __m128i prod = _mm_maddubs_epi16(in_a, in_b);

    _mm_store_si128((__m128i *)&output, prod);
    /*__m128i hi64  = _mm_shuffle_epi32(prod, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i sum64 = _mm_add_epi32(hi64, prod);
    __m128i hi32 = _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i sum32 = _mm_add_epi32(sum64, hi32);
    __m128i hi16 = _mm_shuffle_epi8(sum32, _mm_set1_epi8( _MM_SHUFFLE(1, 0, 3, 2)));
    __m128i sum16 = _mm_add_epi8(sum32, hi16);
    int x = _mm_cvtsi128_si32(sum16); */
    __m128i x = _mm_hadd_epi16(prod, prod);
    x = _mm_hadd_epi16(x, x);
    x = _mm_hadd_epi16(x, x);
    int res = _mm_extract_epi16(x, 0);

    //v128_t ina = wasm_v128_load(&a);
    //v128_t inb = wasm_v128_load(&b);
    //v128_t prod = wasm_i16x8_mul(ina, inb);
    //wasm_v128_store(&output, prod);
    for (auto &n : output) {
        std::cout << (int)n << " ";
    }
    std::cout << std::endl;
    std::cout << res << std::endl;
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
    
    class_<Game>("Game")
        .constructor()
        .function("make_move", &Game::make_move)
        .function("get_moves", &Game::get_moves)
        .function("get_engine_moves()", &Game::get_engine_moves())
        .function("get_engine_evaluation", &Game::get_engine_evaluation)
        .function("get_principal_variation", &Game::get_principal_variation)
        .function("in_check", &Game::in_check)
        .function("start_from_position", &Game::start_from_position)
        .function("test_position", &Game::test_position)
        ;

}
#endif