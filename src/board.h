#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "move.h"

using std::uint64_t;

enum class Side : size_t {
    WHITE = 0,
    BLACK = 1,
};

class Board {
   private:
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> kings;
    std::array<uint64_t, 2> pawns;
    std::array<uint64_t, 2> all_per_side;
    // a map of the squares which each side attack (subset of defense)
    std::array<uint64_t, 2> attack_maps;
    // a map of the squares which each side defends
    std::array<uint64_t, 2> defense_maps;
    // a map of attacks if there were no opposing pieces on the board
    // essentially the set of attacked squaresif piece movement weren't
    // blocked by opposing pieces
    std::array<uint64_t, 2> unimpeded_maps;
    std::array<std::array<uint64_t, 64>, 8> rank_attack_lookup;
    std::array<uint64_t, 64> diagonal_mask_lookup;
    std::array<uint64_t, 64> antidiagonal_mask_lookup;
    std::array<uint64_t, 64> king_lookup;
    std::array<uint64_t, 64> knight_lookup;
    uint8_t en_passant_target;
    uint64_t generate_rook_moves(uint8_t square);
    uint64_t generate_bishop_moves(uint8_t square);
    uint64_t generate_queen_moves(uint8_t square);
    uint64_t generate_knight_moves(uint8_t square);
    uint64_t generate_king_moves(uint8_t square);
    uint64_t generate_pawn_moves(uint8_t square);
    uint64_t generate_pawn_attacks(uint8_t square);
    std::vector<Move> moves_for_piece(uint64_t piece_board, uint64_t (Board::*gen_func)(uint8_t));
    Side side_to_move;
    std::vector<Move> moves;
    std::vector<Move> generate_moves();

   public:
    // using enum Side;
    // initizlizes a board in the starting position
    Board();
    // The below methods generate lookup tables that allow efficient determination of available moves
    // for the various pieces, as well as helper functions that assist in this
    static std::array<uint64_t, 64> generate_knight_lookup();
    static std::array<uint64_t, 64> generate_king_lookup();
    static std::array<std::array<uint64_t, 64>, 2> generate_pawn_attack_lookup();
    static std::array<std::array<uint64_t, 64>, 8> generate_rank_attacks();
    static std::array<uint64_t, 64> generate_diagonal_mask_map();
    static std::array<uint64_t, 64> generate_antidiagonal_mask_map();
    // move and side functions
    void make_move(Move move);
    void unmake_move(Move move);
    int get_side_to_move();
    std::vector<Move> get_moves();
    std::vector<uint16_t> get_moves_as_u16();
    std::vector<std::string> get_moves_algebraic();
    // getters for the current position
    std::array<uint64_t, 2> get_knights();
    std::array<uint64_t, 2> get_bishops();
    std::array<uint64_t, 2> get_rooks();
    std::array<uint64_t, 2> get_queens();
    std::array<uint64_t, 2> get_kings();
    std::array<uint64_t, 2> get_pawns();

    unsigned long long perft(unsigned int depth) {
        if (depth == 0) {
            return 1;
        }
    }
};