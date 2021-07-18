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
    std::array<uint64_t, 2> attack_maps;
    std::array<uint64_t, 2> defense_maps;
    std::array<std::array<uint64_t, 64>, 8> rank_attack_lookup;
    std::array<uint64_t, 64> diagonal_mask_lookup;
    std::array<uint64_t, 64> antidiagonal_mask_lookup;
    std::array<uint64_t, 64> king_lookup;
    std::array<uint64_t, 64> knight_lookup;
    uint64_t generate_rook_moves(uint8_t square);
    uint64_t generate_bishop_moves(uint8_t square);
    uint64_t generate_queen_moves(uint8_t square);
    uint64_t generate_knight_moves(uint8_t square);
    uint64_t generate_king_moves(uint8_t square);
    std::vector<Move> generate_moves();
    std::vector<Move> Board::moves_for_piece(uint64_t piece_board, uint64_t (Board::*gen_func)(uint8_t));
    Side side_to_move;
    std::vector<Move> moves;

   public:
    // using enum Side;
    // initizlizes a board in the starting position
    Board();
    // The below methods generate lookup tables that allow efficient determination of available moves
    // for the various pieces, as well as helper functions that assist in this
    static std::array<uint64_t, 64> generate_knight_lookup();
    static std::array<uint64_t, 64> generate_king_lookup();
    static std::array<std::array<uint64_t, 64>, 2> generate_pawn_move_lookup(uint8_t square, uint64_t occ);
    static std::array<std::array<uint64_t, 64>, 2> generate_pawn_attack_lookup();
    static std::array<std::array<uint64_t, 64>, 8> generate_rank_attacks();
    static std::array<uint64_t, 64> generate_diagonal_mask_map();
    static std::array<uint64_t, 64> generate_antidiagonal_mask_map();
    void make_move(Move move);
    void unmake_move(Move move);
    std::array<uint64_t, 2> get_knights();
    std::array<uint64_t, 2> get_bishops();
    std::array<uint64_t, 2> get_rooks();
    std::array<uint64_t, 2> get_queens();
    std::array<uint64_t, 2> get_kings();
    std::array<uint64_t, 2> get_pawns();
};