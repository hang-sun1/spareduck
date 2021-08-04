#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "history.hpp"
#include "move.hpp"
#include "side.hpp"

using std::uint64_t;

class Board {
   private:
    uint64_t hash;
    std::stack<History> history;
    std::array<std::array<uint64_t, 64>, 12> hash_helper;
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> knight_defends;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> bishop_defends;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> rook_defends;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> queen_defends;
    std::array<uint64_t, 2> kings;
    std::array<uint64_t, 2> king_defends;
    std::array<uint64_t, 2> pawns;
    std::array<uint64_t, 2> pawn_defends;
    std::array<uint64_t, 2> all_per_side;
    // std::array<uint64_t, 2> attack_maps;
    std::array<uint64_t, 2> defense_maps;
    std::array<std::array<uint64_t, 64>, 8> rank_attack_lookup;
    std::array<uint64_t, 64> diagonal_mask_lookup;
    std::array<uint64_t, 64> antidiagonal_mask_lookup;
    std::array<uint64_t, 64> king_lookup;
    std::array<uint64_t, 64> knight_lookup;
    std::array<std::array<uint64_t, 64>, 64> in_between;
    uint8_t en_passant_target;
    std::array<bool, 2> short_castle_rights;
    std::array<bool, 2> long_castle_rights;
    uint64_t generate_rook_moves(uint8_t square, uint64_t board_occ) const;
    uint64_t generate_bishop_moves(uint8_t square, uint64_t board_occ) const;
    uint64_t generate_queen_moves(uint8_t square, uint64_t board_occ) const;
    uint64_t generate_knight_moves(uint8_t square, uint64_t board_occ) const;
    uint64_t generate_king_moves(uint8_t square, uint64_t board_occ) const;
    uint64_t generate_pawn_moves(uint8_t square, uint64_t board_occ, Side side) const;
    uint64_t generate_pawn_attacks(uint8_t square, uint64_t board_occ, Side side) const;
    std::vector<std::pair<uint64_t, uint8_t>> defense_maps_for_piece(uint64_t piece_board, uint64_t board_occ, char move_type, Side side) const;
    Side side_to_move;
    std::array<std::vector<uint8_t>, 2> pinned_pieces;
    std::vector<Move> moves;
    std::vector<Move> generate_moves();
    void update_board_state(Move move);
    std::vector<Move> made_moves;
    std::vector<std::array<uint64_t, 2>*> moved_piece_boards;
    std::vector<std::array<uint64_t, 2>*> taken_piece_boards;
    uint64_t rook_xray_attacks(uint64_t occ, uint64_t blockers, uint8_t square) const;
    uint64_t bishop_xray_attacks(uint64_t occ, uint64_t blockers, uint8_t square) const;

    bool king_still_under_attack(uint8_t move_dest, uint64_t king_board, uint64_t piece_board, char move_type, Side other_side) const;
    void parse_fen(std::string fen);

   public:
    uint64_t nodes_evaluated = 0;
    // using enum Side;
    // initizlizes a board in the starting position
    Board();
    Board(std::string fen);
    // The below methods generate lookup tables that allow efficient determination of available moves
    // for the various pieces, as well as helper functions that assist in this
    static std::array<uint64_t, 64> generate_knight_lookup();
    static std::array<uint64_t, 64> generate_king_lookup();
    static std::array<std::array<uint64_t, 64>, 8> generate_rank_attacks();
    static std::array<uint64_t, 64> generate_diagonal_mask_map();
    static std::array<uint64_t, 64> generate_antidiagonal_mask_map();
    static std::array<std::array<uint64_t, 64>, 12> initialize_hash();
    static std::array<std::array<uint64_t, 64>, 64> generate_in_between();

    // move and side functions
    void make_move(Move move);
    void unmake_move(Move move);
    int get_side_to_move();
    std::vector<Move> get_moves();
    std::vector<uint16_t> get_moves_as_u16();
    std::vector<std::string> get_moves_algebraic();
    // getters for the current position
    std::array<uint64_t, 2> get_knights() const;
    std::array<uint64_t, 2> get_bishops() const;
    std::array<uint64_t, 2> get_rooks() const;
    std::array<uint64_t, 2> get_queens() const;
    std::array<uint64_t, 2> get_kings() const;
    std::array<uint64_t, 2> get_pawns() const;
    std::array<std::vector<uint8_t>, 2> get_pins() const;
    // returns piece positions
    std::vector<uint8_t> get_piece_pos(char piece_type) const;

    uint64_t intial_hash() const;
    uint64_t get_hash() const;
    bool in_check() const;
    bool is_checkmate() const;
    bool is_stalemate() const;
};
