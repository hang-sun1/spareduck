#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <optional>

#include "history.hpp"
#include "move.hpp"
#include "side.hpp"
#include "piece.hpp"

#include "../magic-bits/include/magic_bits.hpp"

using std::uint64_t;

class Board {
   private:
    magic_bits::Attacks* attacks;
    uint64_t hash;
    std::stack<History> history;
    std::array<std::array<uint64_t, 64>, 13> hash_helper;
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> kings;
    std::array<uint64_t, 2> pawns;
    std::array<uint64_t, 2> all_per_side;
    // std::array<uint64_t, 2> attack_maps;
    std::array<uint64_t, 2> defense_maps;
    std::array<uint64_t, 64> king_lookup;
    std::array<uint64_t, 64> knight_lookup;
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
    // std::vector<std::pair<uint64_t, uint8_t>> defense_maps_for_piece(uint64_t piece_board, uint64_t board_occ, char move_type, Side side) const;
    void defense_maps_for_piece(uint64_t piece_board, uint64_t board_occ, char move_type, Side side,
        std::vector<std::pair<uint64_t, uint8_t>> &maps_vec) const;
    Side side_to_move;
    std::array<std::vector<uint8_t>, 2> pinned_pieces;
    std::vector<Move> moves;
    std::vector<std::pair<uint64_t, uint8_t>> piece_map_vec;
    std::vector<Move> generate_moves();
    void update_board_state(Move move);
    std::vector<Move> made_moves;
    std::vector<std::array<uint64_t, 2>*> moved_piece_boards;
    std::vector<std::array<uint64_t, 2>*> taken_piece_boards;

    void parse_fen(std::string fen);

   public:
    uint64_t nodes_evaluated = 0;
    // using enum Side;
    // initizlizes a board in the starting position
    Board(magic_bits::Attacks* att);
    Board(std::string fen, magic_bits::Attacks* att);
    // The below methods generate lookup tables that allow efficient determination of available moves
    // for the various pieces, as well as helper functions that assist in this
    static std::array<uint64_t, 64> generate_knight_lookup();
    static std::array<uint64_t, 64> generate_king_lookup();
    static std::array<std::array<uint64_t, 64>, 13> initialize_hash();

    // move and side functions
    std::array<std::optional<Piece>, 2> make_move(Move move);
    void unmake_move(Move move);
    bool king_attacked(uint8_t king_square, uint64_t board_occ, Side side);
    bool is_pos_valid(Move move);
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
    std::vector<uint8_t> get_piece_pos(Piece piece_type, Side s) const;

    uint64_t initial_hash() const;
    uint64_t get_hash() const;
    bool in_check(Side side);
    Piece piece_on_square(uint8_t square, Side s);
};
