#include "board.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>

#include "move.h"

using std::uint64_t;

Board::Board() {
    this->all_per_side[0] = 0xffff;
    this->all_per_side[1] = 0xffff000000000000;
    this->pawns[0] = 0xff00;
    this->pawns[1] = 0xff000000000000;
    this->rooks[0] = 0x81;
    this->rooks[1] = 0x8100000000000000;
    this->knights[0] = 0x42;
    this->knights[1] = 0x4200000000000000;
    this->bishops[0] = 0x24;
    this->bishops[1] = 0x2400000000000000;
    this->queens[0] = 0x08;
    this->queens[1] = 0x800000000000000;
    this->kings[0] = 0x10;
    this->kings[1] = 0x1000000000000000;
    this->side_to_move = Side::WHITE;
    this->attack_maps[1] = 0xff0000000000;
    this->defense_maps[1] = 0xffffff0000000000;
    this->rank_attack_lookup = Board::generate_rank_attacks();
    this->diagonal_mask_lookup = Board::generate_diagonal_mask_map();
    this->antidiagonal_mask_lookup = Board::generate_antidiagonal_mask_map();
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    this->moves = this->generate_moves();
}

// returns a array containing the valid moves for a king given
// the king's square. Indexed by the king's square
std::array<uint64_t, 64> Board::generate_king_lookup() {
    uint64_t not_a_file = 0xfefefefefefefefe;
    uint64_t not_h_file = 0x7f7f7f7f7f7f7f7f;

    std::array<uint64_t, 64> lookup_table;

    for (size_t i = 0; i < 64; ++i) {
        uint64_t b = 1 << i;
        uint64_t north = b << 8;
        uint64_t north_east = (b << 9) & not_a_file;
        uint64_t east = (b << 1) & not_a_file;
        uint64_t south_east = (b >> 7) & not_a_file;
        uint64_t south = b >> 8;
        uint64_t south_west = (b >> 9) & not_h_file;
        uint64_t west = (b >> 1) & not_h_file;
        uint64_t north_west = (b << 7) & not_h_file;

        lookup_table[i] = north | north_east | east | south_east | south | south_west | west | north_west;
    }
    return lookup_table;
}

// returns a array containing the valid moves for a knight given
// the knight's square. Indexed by the knight's square
std::array<uint64_t, 64> Board::generate_knight_lookup() {
    uint64_t not_a_file = 0xfefefefefefefefe;
    uint64_t not_a_or_b_file = 0xfdfdfdfdfdfdfdfd & not_a_file;
    uint64_t not_h_file = 0x7f7f7f7f7f7f7f7f;
    uint64_t not_g_or_h_file = 0xbfbfbfbfbfbfbfbf & not_h_file;

    std::array<uint64_t, 64> lookup_table;

    for (size_t i = 0; i < 64; ++i) {
        uint64_t b = 1 << i;
        uint64_t north_northeast = (b << 17) & not_a_file;
        uint64_t north_easteast = (b << 10) & not_a_file;
        uint64_t south_easteast = (b >> 6) & not_a_or_b_file;
        uint64_t south_southeast = (b >> 15) & not_a_file;
        uint64_t north_northwest = (b << 15) & not_h_file;
        uint64_t north_westwest = (b << 6) & not_g_or_h_file;
        uint64_t south_west_west = (b >> 10) & not_g_or_h_file;
        uint64_t south_south_west = (b >> 17) & not_h_file;

        lookup_table[i] = north_northeast | north_easteast | south_easteast | south_southeast |
                          north_northwest | north_westwest | south_west_west | south_south_west;
    }
    return lookup_table;
}

// return an array containing the available pawn moves, this includes only
// non capture moves
std::array<std::array<uint64_t, 64>, 2> Board::generate_pawn_move_lookup(uint8_t square, uint64_t occ) {
    std::array<std::array<uint64_t, 64>, 2> lookup_tables;
    for (size_t s = 0; s < 64; ++s) {
        // generate moves for white
    }
    return lookup_tables;
}

/* 
    Creates a 2d array that determines a piece's possible moves along a rank.
    Indexed by the square (0-7) along the rank as well as the rank's occupancy.
    The occupancy is a 6 bit number that determines which squares are occupied
    along the rank. For example 0b111111 states that the middle six squres are occupied in
    the rank. Only six bits are used as the 2 squares at the ends of the ranks
    can be ignored as they cannot block movement to further squares.
*/
std::array<std::array<uint64_t, 64>, 8> Board::generate_rank_attacks() {
    std::array<std::array<uint64_t, 64>, 8> lookup_table;
    for (uint8_t square = 0; square < 8; ++square) {
        for (uint8_t o = 0; o < 64; ++o) {
            uint8_t occ = o;
            occ = occ << 1;
            occ |= 0b10000001;
            uint8_t attack = 0;
            if (square != 7) {
                // extract the bits above the square being tested
                uint8_t bits_upper_to_square = occ >> (square + 1);
                // find the location of the first piece blocking our piece in the msb direction
                // (the location is relative to square)
                uint8_t first_blocker_above = static_cast<uint8_t>(__builtin_ffsll(bits_upper_to_square));

                // set the appropriate number of bits to add to the attack map
                uint8_t bits = 0xff >> (8 - first_blocker_above);
                // shift the bits into the attack map
                attack |= bits << (square + 1);
            }
            if (square != 0) {
                // extract bits below the square being tested
                uint8_t bits_lower_to_square = occ & (0xff >> (8 - square));
                // count how many squares can be attacked below the square
                uint8_t attackable_squares_lower_to_square = static_cast<uint8_t>(__builtin_clzll(bits_lower_to_square) - 56 - (8 - square) + 1);
                // find the index of the first piece (or edge of board) that blocks the piece's movement
                int first_blocker_below_index = square - attackable_squares_lower_to_square;
                // add this half into the attack map
                uint8_t bits = 0xff >> (8 - attackable_squares_lower_to_square);
                attack |= bits << first_blocker_below_index;
            }
            lookup_table[square][o] = attack & (~(1 << square));
        }
    }
    return lookup_table;
}

// return a bitboard containing legal moves for a rook on
// the provided square
uint64_t Board::generate_rook_moves(uint8_t square) {
    // first determine which rank the piece is on
    auto rank = square >> 3;
    // get the complete occupancy of the board
    auto board_occ = this->all_per_side[0] | this->all_per_side[1];
    // shift the board right so the rank our square is on is now
    // the first rank
    auto board_occ_shifted = board_occ >> (8 * rank);
    uint8_t board_occ_byte = static_cast<uint8_t>(board_occ_shifted);
    uint64_t rank_moves = this->rank_attack_lookup[square][board_occ_byte] << (8 * rank);

    auto file = square & 7;
    uint64_t a_file_mask = 0x0101010101010101;
    uint64_t h_file_mask = 0x8080808080808080;
    uint64_t c2h7_diag_mask = 0x0080402010080400;
    uint64_t a1h8_diag_mask = 0x8040201008040201;

    board_occ = (board_occ << file) & a_file_mask;
    board_occ = (c2h7_diag_mask * board_occ) >> 58;
    board_occ = a1h8_diag_mask * this->rank_attack_lookup[(square ^ 56) >> 3][static_cast<uint8_t>(board_occ)];
    uint64_t file_moves = (h_file_mask & board_occ) >> (file ^ 7);

    auto to_move = static_cast<size_t>(side_to_move);
    auto defended_squares = file_moves | rank_moves;
    auto attacked_squares = defended_squares & (~all_per_side[to_move]);
    defense_maps[to_move] |= defended_squares;
    attack_maps[to_move] |= attacked_squares;
    return attacked_squares;
}

uint64_t Board::generate_bishop_moves(uint8_t square) {
    auto file = square & 7;
    uint64_t a_file_mask = 0x0101010101010101;
    uint64_t b_file_mask = 0x0202020202020202;

    auto board_occ = this->all_per_side[0] | this->all_per_side[1];
    auto diagonal_defend = this->diagonal_mask_lookup[square] & board_occ;
    diagonal_defend = (b_file_mask * diagonal_defend) >> 58;
    diagonal_defend = a_file_mask * this->rank_attack_lookup[file][diagonal_defend];
    diagonal_defend = diagonal_defend & this->diagonal_mask_lookup[square];

    auto antidiagonal_defend = this->antidiagonal_mask_lookup[square] & board_occ;
    antidiagonal_defend = this->antidiagonal_mask_lookup[square] & board_occ;
    antidiagonal_defend = (b_file_mask * antidiagonal_defend) >> 58;
    antidiagonal_defend = a_file_mask * this->rank_attack_lookup[file][antidiagonal_defend];
    antidiagonal_defend = antidiagonal_defend & this->antidiagonal_mask_lookup[square];

    auto to_move = static_cast<size_t>(side_to_move);
    auto defended_squares = diagonal_defend | antidiagonal_defend;
    auto attacked_squares = defended_squares & (~all_per_side[to_move]);
    defense_maps[to_move] |= defended_squares;
    attack_maps[to_move] |= attacked_squares;
    return attacked_squares;
}

uint64_t Board::generate_queen_moves(uint8_t square) {
    return Board::generate_bishop_moves(square) | Board::generate_rook_moves(square);
}

uint64_t Board::generate_knight_moves(uint8_t square) {
    size_t current_side = static_cast<size_t>(side_to_move);
    auto defended_squares = knight_lookup[square];
    auto attacked_squares = defended_squares & (~all_per_side[current_side]);
    defense_maps[current_side] |= defended_squares;
    attack_maps[current_side] |= attacked_squares;

    return attacked_squares;
}

uint64_t Board::generate_king_moves(uint8_t square) {
    auto defended_squares = king_lookup[square];
    size_t current_side = static_cast<size_t>(side_to_move);
    size_t other_side = 1 - current_side;
    auto attacked_squares = defended_squares & (~defense_maps[other_side]) & (~all_per_side[current_side]);
    defense_maps[current_side] |= defended_squares;
    attack_maps[current_side] |= attacked_squares;
    return attacked_squares;
}

bool on_same_diagonal(uint8_t square1, uint8_t square2) {
    return ((square2 - square1) & 7) == ((square2 >> 3) - (square1 >> 3));
}

std::array<uint64_t, 64> Board::generate_diagonal_mask_map() {
    uint64_t a1h8_mask = 0x8040201008040201;
    uint64_t b1h7_mask = 0x80402010080402;
    uint64_t c1h6_mask = 0x804020100804;
    uint64_t d1h5_mask = 0x8040201008;
    uint64_t e1h4_mask = 0x80402010;
    uint64_t f1h3_mask = 0x804020;
    uint64_t g1h2_mask = 0x8040;
    uint64_t h1_mask = 0x80;
    uint64_t a2g8_mask = 0x4020100804020100;
    uint64_t a3f8_mask = 0x2010080402010000;
    uint64_t a4e8_mask = 0x1008040201000000;
    uint64_t a5d8_mask = 0x804020100000000;
    uint64_t a6c8_mask = 0x402010000000000;
    uint64_t a7b8_mask = 0x201000000000000;
    uint64_t a8_mask = 0x100000000000000;

    std::array<uint64_t, 64> lookup_table;

    for (size_t square = 0; square < 64; ++square) {
        if (on_same_diagonal(square, 0)) {
            lookup_table[square] = a1h8_mask;
        } else if (on_same_diagonal(square, 1)) {
            lookup_table[square] = b1h7_mask;
        } else if (on_same_diagonal(square, 2)) {
            lookup_table[square] = c1h6_mask;
        } else if (on_same_diagonal(square, 3)) {
            lookup_table[square] = d1h5_mask;
        } else if (on_same_diagonal(square, 4)) {
            lookup_table[square] = e1h4_mask;
        } else if (on_same_diagonal(square, 5)) {
            lookup_table[square] = f1h3_mask;
        } else if (on_same_diagonal(square, 6)) {
            lookup_table[square] = g1h2_mask;
        } else if (on_same_diagonal(square, 7)) {
            lookup_table[square] = h1_mask;
        } else if (on_same_diagonal(square, 8)) {
            lookup_table[square] = a2g8_mask;
        } else if (on_same_diagonal(square, 16)) {
            lookup_table[square] = a3f8_mask;
        } else if (on_same_diagonal(square, 24)) {
            lookup_table[square] = a4e8_mask;
        } else if (on_same_diagonal(square, 32)) {
            lookup_table[square] = a5d8_mask;
        } else if (on_same_diagonal(square, 40)) {
            lookup_table[square] = a6c8_mask;
        } else if (on_same_diagonal(square, 48)) {
            lookup_table[square] = a7b8_mask;
        } else if (on_same_diagonal(square, 54)) {
            lookup_table[square] = a8_mask;
        }
    }
    return lookup_table;
}

std::array<uint64_t, 64> Board::generate_antidiagonal_mask_map() {
    uint64_t h1a8_mask = 0x102040810204080;
    uint64_t g1a7_mask = 0x1020408102040;
    uint64_t f1a6_mask = 0x10204081020;
    uint64_t e1a5_mask = 0x102040810;
    uint64_t d1a4_mask = 0x1020408;
    uint64_t c1a3_mask = 0x1020408;
    uint64_t b1a2_mask = 0x0102;
    uint64_t a1_mask = 0x01;
    uint64_t h2b8_mask = 0x204081020408000;
    uint64_t h3c8_mask = 0x408102040800000;
    uint64_t h4d8_mask = 0x810204080000000;
    uint64_t h5e8_mask = 0x1020408000000000;
    uint64_t h6f8_mask = 0x2040800000000000;
    uint64_t h7g8_mask = 0x4080000000000000;
    uint64_t h8_mask = 0x8000000000000000;

    std::array<uint64_t, 64> lookup_table;

    for (size_t square = 0; square < 64; ++square) {
        if (on_same_diagonal(square, 7)) {
            lookup_table[square] = h1a8_mask;
        } else if (on_same_diagonal(square, 6)) {
            lookup_table[square] = g1a7_mask;
        } else if (on_same_diagonal(square, 5)) {
            lookup_table[square] = f1a6_mask;
        } else if (on_same_diagonal(square, 4)) {
            lookup_table[square] = e1a5_mask;
        } else if (on_same_diagonal(square, 3)) {
            lookup_table[square] = d1a4_mask;
        } else if (on_same_diagonal(square, 2)) {
            lookup_table[square] = c1a3_mask;
        } else if (on_same_diagonal(square, 1)) {
            lookup_table[square] = b1a2_mask;
        } else if (on_same_diagonal(square, 0)) {
            lookup_table[square] = a1_mask;
        } else if (on_same_diagonal(square, 57)) {
            lookup_table[square] = h2b8_mask;
        } else if (on_same_diagonal(square, 58)) {
            lookup_table[square] = h3c8_mask;
        } else if (on_same_diagonal(square, 59)) {
            lookup_table[square] = h4d8_mask;
        } else if (on_same_diagonal(square, 60)) {
            lookup_table[square] = h5e8_mask;
        } else if (on_same_diagonal(square, 61)) {
            lookup_table[square] = h6f8_mask;
        } else if (on_same_diagonal(square, 62)) {
            lookup_table[square] = h7g8_mask;
        } else if (on_same_diagonal(square, 63)) {
            lookup_table[square] = h8_mask;
        }
    }
    return lookup_table;
}

std::vector<Move> Board::generate_moves() {
    // figure out which side is to move
    size_t side = static_cast<size_t>(side_to_move);
    std::vector<Move> moves;
    auto queen_moves = moves_for_piece(queens[side], &generate_queen_moves);
    auto bishop_moves = moves_for_piece(bishops[side], &generate_bishop_moves);
    auto rook_moves = moves_for_piece(rooks[side], &generate_bishop_moves);
    auto king_moves = moves_for_piece(kings[side], &generate_king_moves);
    auto knight_moves = moves_for_piece(knights[side], &generate_knight_moves);

    moves.insert(moves.end(), queen_moves.begin(), queen_moves.end());
    moves.insert(moves.end(), bishop_moves.begin(), bishop_moves.end());
    moves.insert(moves.end(), rook_moves.begin(), rook_moves.end());
    moves.insert(moves.end(), king_moves.begin(), king_moves.end());
    moves.insert(moves.end(), knight_moves.begin(), knight_moves.end());

    // TODO add castles and pawn stuff (including en passant)

    // generate queen moves first
    return moves;
}

void Board::make_move(Move move) {
    size_t current_move = static_cast<size_t>(side_to_move);

    // TODO: Actually move pieces

    // calling generate_moves again regenerates
    // the attack and defense maps of the side that
    // just moved, though doing so is slightly inefficient
    generate_moves();

    // now flip whose turn it is
    side_to_move = static_cast<Side>(1 - current_move);

    // generate the moves for the next side (which also updates attack and defenes maps for
    // the new side to move)
    this->moves = this->generate_moves();
}

void Board::unmake_move(Move move) {
}

// This function generates all of the moves for a specific type of piece
// TODO: make sure to check that the piece being moved isn't pinned, also finish implementing this
std::vector<Move> Board::moves_for_piece(uint64_t piece_board, uint64_t (Board::*gen_func)(uint8_t)) {
    size_t to_move = static_cast<size_t>(side_to_move);
    size_t not_to_move = 1 - to_move;

    std::vector<Move> moves;

    int set_bit = __builtin_ffsll(piece_board);
    while (set_bit) {
        uint8_t square_from = static_cast<uint8_t>(set_bit - 1);
        auto move_board = (this->*gen_func)(square_from);
        int dest_plus_one = __builtin_ffsll(move_board);
        while (dest_plus_one) {
            uint16_t dest = static_cast<uint16_t>(dest_plus_one - 1);
            // TODO figure out what type of move it is (instead of assuming it's quiet);
            MoveType type = MoveType::QUIET;
            moves.push_back(Move(square_from, dest, type));
            move_board &= ~(1 << dest);
            dest_plus_one = __builtin_ffsll(move_board);
        }
        piece_board &= ~(1 << square_from);
        set_bit = __builtin_ffsll(piece_board);
    }
    return moves;
}

// Getter for side to move
int Board::get_side_to_move() {
    return static_cast<size_t>(side_to_move);;
}

// Getters for boards
std::array<uint64_t, 2> Board::get_knights() {
    return knights;
}

std::array<uint64_t, 2> Board::get_bishops() {
    return bishops;
}

std::array<uint64_t, 2> Board::get_rooks() {
    return rooks;
}

std::array<uint64_t, 2> Board::get_queens() {
    return queens;
}

std::array<uint64_t, 2> Board::get_kings() {
    return kings;
}

std::array<uint64_t, 2> Board::get_pawns() {
    return pawns;
}
