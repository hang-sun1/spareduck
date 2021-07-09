#include <cstdint>
#include <bit>
#include <vector>
#include "board.h"
#include <iostream>
#include <algorithm>
#include <array>

using std::uint64_t;

Board::Board() {
    this->all_per_side[0] = 0x0100;
    this->all_per_side[0] = 0;
    this->rank_attack_lookup = Board::generate_rank_attacks();
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
std::array<std::array<uint64_t, 64>, 2> Board::generate_pawn_move_lookup() {
    for (size_t i = 0; i < 2; ++i) {
        for (size_t s = 0; s < 64; ++s) {
            // generate moves for white
            if (i == 0) {

            }
        }
    }
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
                #ifndef _MSVC_VER
                    uint8_t first_blocker_above = static_cast<uint8_t>(__builtin_ffsll(bits_upper_to_square));
                #endif
                #ifdef _MSVC_VER
                    unsigned long first_blocker_above;
                    _BitScanForward(&first_blocker_above, bits_upper_to_square);
                    uint8_t first_blocker_above = static_cast<uint8_t>(first_blocker_above); 
                #endif
                // check to make sure there are actually squares above  
                // set the appropriate number of bits to add to the attack map
                uint8_t bits = 0xff >> (8 - first_blocker_above);
                // shift the bits into the attack map
                attack |= bits << (square + 1); 
            }
            if (square != 0) {
                // extract bits below the square being tested
                uint8_t bits_lower_to_square = occ & (0xff >> (8-square));
                // count how many squares can be attacked below the square
                #ifndef _MSVC_VER
                    uint8_t attackable_squares_lower_to_square = static_cast<uint8_t>(__builtin_clzll(bits_lower_to_square) - 56 - (8-square)+1);
                #endif
                #ifdef _MSVC_VER
                    unsigned long attackable_squares_lower_to_square;
                    _BitScanReverse(&attackable_squares_lower_to_square, bits_lower_to_square);
                    uint8_t attackable_squares_lower_to_square = static_cast<uint8_t>(attackable_squares_lower_to_square);
                #endif
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

uint64_t Board::generate_rook_moves(uint8_t square) {
    // first determine which rank the piece is on
    auto rank = square >> 3;
    // get the complete occupancy of the board
    auto board_occ = this->all_per_side[0] | this->all_per_side[1];
    // shift the board right so the rank our square is on is now
    // the first rank
    auto board_occ_shifted = board_occ >> (8*rank);
    uint8_t board_occ_byte = static_cast<uint8_t>(board_occ_shifted);
    uint64_t rank_moves = this->rank_attack_lookup[square][board_occ_byte] << (8 * rank);

    auto file = square & 7;
    uint64_t a_file_mask = 0x0101010101010101;
    uint64_t h_file_mask = 0x8080808080808080;
    uint64_t c2h7_diag_mask = 0x0080402010080400;
    uint64_t a1h8_diag_mask = 0x8040201008040201;

    board_occ = (board_occ << file) & a_file_mask;
    board_occ = (c2h7_diag_mask * board_occ) >> 58;
    board_occ = a1h8_diag_mask * this->rank_attack_lookup[(square^56) >> 3][static_cast<uint8_t>(board_occ)];
    uint64_t file_moves = (h_file_mask & board_occ) >> (file ^ 7);
    return file_moves | rank_moves;
}


