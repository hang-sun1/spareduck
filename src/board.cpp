#include <cstdint>
#include <bit>
#include <vector>
#include "board.h"
#include <iostream>
#include <algorithm>
#include <array>

using std::uint64_t;

static const uint64_t A_FILE_MASK = 0x0101010101010101;
static const uint64_t MAIN_DIAGONAL = 0x8040201008040201;

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

/* 
    Creates a 2d array that determines a piece's possible moves along a rank.
    Indexed by the square (0-7) along the rank as well as the rank's occupancy.
    The occupancy is a 6 bit number that determines which squares are occupied
    along the rank. For example 0b111111 states that the middle six squres are occupied in
    the rank. Only six bits are used as the 2 squares at the ends of the ranks
    can be ignored as they cannot block movement to further squares.
*/
std::array<std::array<uint64_t, 64>, 8> Board::generate_rank_attacks() {
    std::cout << "hello world" << std::endl;
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

