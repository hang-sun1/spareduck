#include "board.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <stack>
#include <vector>

#include "history.h"
#include "move.h"

using std::uint64_t;

Board::Board() {
    this->in_between = Board::generate_in_between();
    this->history = std::stack<History>();
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
    this->pawn_defends = {0xff0000, 0xff0000000000};
    this->rook_defends = {0x8142, 0x4281000000000000};
    this->knight_defends = {0xa51800, 0x18a50000000000};
    this->bishop_defends = {0x5a00, 0x5a000000000000};
    this->queen_defends = {0x1c14, 0x141c000000000000};
    this->king_defends = {0x3828, 0x2838000000000000};
    this->short_castle_rights = {true, true};
    this->long_castle_rights = {true, true};
    this->defense_maps[1] = pawn_defends[1] | rook_defends[1] | knight_defends[1] | bishop_defends[1] | queen_defends[1] | king_defends[1];
    this->defense_maps[0] = pawn_defends[0] | rook_defends[0] | knight_defends[0] | bishop_defends[0] | queen_defends[0] | king_defends[0];
    this->all_per_side[0] = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    this->all_per_side[1] = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];
    this->attack_maps[1] = defense_maps[1] & (~all_per_side[1]);
    this->attack_maps[0] = defense_maps[0] & (~all_per_side[0]);
    this->rank_attack_lookup = Board::generate_rank_attacks();
    this->diagonal_mask_lookup = Board::generate_diagonal_mask_map();
    this->antidiagonal_mask_lookup = Board::generate_antidiagonal_mask_map();
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    std::vector<Move> empty;
    this->moves = empty;
    this->made_moves = empty;
    this->moved_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->taken_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->hash_helper = Board::initialize_hash();
    this->moves = this->generate_moves();
    // there is no en passant target yet, so just set it to some square off the board
    this->en_passant_target = 65;
}

Board::Board(std::string fen) {
    this->in_between = Board::generate_in_between();
    this->history = std::stack<History>();
    parse_fen(fen);
    this->rank_attack_lookup = Board::generate_rank_attacks();
    this->diagonal_mask_lookup = Board::generate_diagonal_mask_map();
    this->antidiagonal_mask_lookup = Board::generate_antidiagonal_mask_map();
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    this->all_per_side[0] = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    this->all_per_side[1] = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];

    std::array<std::array<uint64_t, 2>*, 6> arr = { &pawns, &rooks, &knights, &bishops, &queens, &kings };
    std::array<std::array<uint64_t, 2>*, 6> def_maps = { &pawn_defends, &rook_defends, &knight_defends, 
        &bishop_defends, &queen_defends, &king_defends };
    std::array<char, 6> gen_funcs = { 'P', 'r', 'n', 'b', 'q', 'k' };

    auto current_move = static_cast<size_t>(side_to_move);
    auto other_move = 1 - current_move;
    for (int i = 0; i < 6; ++i) {
        auto maps = this->defense_maps_for_piece((*arr[i])[other_move], all_per_side[current_move] | all_per_side[other_move],
                gen_funcs[i]);
        (*def_maps[i])[other_move] = 0;
        for (auto &map: maps) {
            (*def_maps[i])[other_move] |= map.first;
        }
    }
    for (int i = 0; i < 6; ++i) {
        auto maps = this->defense_maps_for_piece((*arr[i])[current_move], all_per_side[current_move] | all_per_side[other_move],
                gen_funcs[i]);
        (*def_maps[i])[current_move] = 0;
        for (auto &map: maps) {
            (*def_maps[i])[current_move] |= map.first;
        }
    }

    this->defense_maps[1] = pawn_defends[1] | rook_defends[1] | knight_defends[1] | bishop_defends[1] | queen_defends[1] | king_defends[1];
    this->defense_maps[0] = pawn_defends[0] | rook_defends[0] | knight_defends[0] | bishop_defends[0] | queen_defends[0] | king_defends[0];
    this->attack_maps[1] = this->defense_maps[1] & (~all_per_side[1]);
    this->attack_maps[0] = this->defense_maps[0] & (~all_per_side[0]);
    this->rank_attack_lookup = Board::generate_rank_attacks();
    this->diagonal_mask_lookup = Board::generate_diagonal_mask_map();
    this->antidiagonal_mask_lookup = Board::generate_antidiagonal_mask_map();
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    std::vector<Move> empty;
    this->moves = empty;
    this->made_moves = empty;
    this->moved_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->taken_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->hash_helper = Board::initialize_hash();
    this->moves = this->generate_moves();
}

// returns a array containing the valid moves for a king given
// the king's square. Indexed by the king's square
std::array<uint64_t, 64> Board::generate_king_lookup() {
    uint64_t not_a_file = 0xfefefefefefefefe;
    uint64_t not_h_file = 0x7f7f7f7f7f7f7f7f;

    std::array<uint64_t, 64> lookup_table;

    for (size_t i = 0; i < 64; ++i) {
        uint64_t b = 1ULL << i;
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

    for (uint64_t i = 0; i < 64; ++i) {
        uint64_t b = 1ULL << i;
        uint64_t north_northeast = (b << 17) & not_a_file;
        uint64_t north_easteast = (b << 10) & not_a_or_b_file;
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
uint64_t Board::generate_pawn_moves(uint8_t square, uint64_t board_occ) const {
    uint64_t one_piece_board = 1ULL << square;
    uint64_t moves = 0;
    uint64_t all_pieces = board_occ;
    uint64_t second_rank = 0xff00;
    uint64_t seventh_rank = 0xff000000000000;

    if (side_to_move == Side::WHITE) {
        uint64_t one_square = one_piece_board << 8;
        if (!(one_square & all_pieces)) {
            moves |= one_square;
            uint64_t two_square = one_piece_board << 16;
            if (!(two_square & all_pieces) && (one_piece_board & second_rank)) {
                moves |= two_square;
            }
        }
    } else if (side_to_move == Side::BLACK) {
        uint64_t one_square = one_piece_board >> 8;
        if (!(one_square & all_pieces)) {
            moves |= one_square;
            uint64_t two_square = one_piece_board >> 16;
            if (!(two_square & all_pieces) && (one_piece_board & seventh_rank)) {
                moves |= two_square;
            }
        }
    }
    return moves;
}

uint64_t Board::generate_pawn_attacks(uint8_t square, uint64_t board_occ) const {
    uint64_t a_file = 0x0101010101010101;
    uint64_t h_file = 0x8080808080808080;
    uint64_t one_piece_board = 1ULL << square;
    uint64_t moves = 0;
    uint8_t file = square & 7;
    // uint64_t all_pieces = board_occ;
    if (side_to_move == Side::WHITE) {
        uint64_t potential_moves = (one_piece_board << 7) | (one_piece_board << 9);
        if (file == 0) {
            potential_moves &= ~h_file;
        } else if (file == 7) {
            potential_moves &= ~a_file;
        }
        moves |= potential_moves;
    } else if (side_to_move == Side::BLACK) {
        uint64_t potential_moves = (one_piece_board >> 7) | (one_piece_board >> 9);
        if (file == 0) {
            potential_moves &= ~h_file;
        } else if (file == 7) {
            potential_moves &= ~a_file;
        }
        moves |= potential_moves;
    }
    return moves;
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
            lookup_table[square][o] = attack & (~(1ULL << square));
        }
    }
    return lookup_table;
}

// return a bitboard containing legal moves for a rook on
// the provided square
uint64_t Board::generate_rook_moves(uint8_t square, uint64_t board_occ) const {
    // first determine which rank the piece is on
    uint8_t rank = square >> 3;
    // get the complete occupancy of the board
    // uint64_t board_occ = this->all_per_side[0] | this->all_per_side[1];
    // shift the board right so the rank our square is on is now
    // the first rank
    uint64_t board_occ_shifted = board_occ >> (8 * rank + 1);
    uint8_t board_occ_byte = static_cast<uint8_t>(board_occ_shifted) & 63;
    uint64_t rank_moves = this->rank_attack_lookup[square & 7][board_occ_byte] << (8 * rank);

    uint8_t file = square & 7;
    uint64_t a_file_mask = 0x0101010101010101;
    uint64_t h_file_mask = 0x8080808080808080;
    uint64_t c2h7_diag_mask = 0x0080402010080400;
    uint64_t a1h8_diag_mask = 0x8040201008040201;
    board_occ = (board_occ >> file) & a_file_mask;
    board_occ = ((c2h7_diag_mask * board_occ) >> 58) & 63;
    board_occ = a1h8_diag_mask * this->rank_attack_lookup[(square ^ 56) >> 3][static_cast<uint8_t>(board_occ)];
    uint64_t file_moves = (h_file_mask & board_occ) >> (file ^ 7);

    // auto to_move = static_cast<size_t>(side_to_move);
    auto defended_squares = file_moves | rank_moves;
    // auto attacked_squares = defended_squares & (~all_per_side[to_move]);
    // defense_maps[to_move] |= defended_squares;
    // attack_maps[to_move] |= attacked_squares;
    return defended_squares & ~(1ULL << square);
}

uint64_t Board::generate_bishop_moves(uint8_t square, uint64_t board_occ) const {
    auto file = square & 7;
    uint64_t a_file_mask = 0x0101010101010101;
    uint64_t b_file_mask = 0x0202020202020202;

    // auto board_occ = this->all_per_side[0] | this->all_per_side[1];
    auto diagonal_defend = this->diagonal_mask_lookup[square] & board_occ;
    diagonal_defend = (b_file_mask * diagonal_defend) >> 58;
    diagonal_defend = a_file_mask * this->rank_attack_lookup[file][diagonal_defend];
    diagonal_defend = diagonal_defend & this->diagonal_mask_lookup[square];

    auto antidiagonal_defend = this->antidiagonal_mask_lookup[square] & board_occ;
    antidiagonal_defend = this->antidiagonal_mask_lookup[square] & board_occ;
    antidiagonal_defend = (b_file_mask * antidiagonal_defend) >> 58;
    antidiagonal_defend = a_file_mask * this->rank_attack_lookup[file][antidiagonal_defend];
    antidiagonal_defend = antidiagonal_defend & this->antidiagonal_mask_lookup[square];
    //diagonal_defend = 0;
    // auto to_move = static_cast<size_t>(side_to_move);
    auto defended_squares = diagonal_defend | antidiagonal_defend;
    return defended_squares & ~(1ULL << square);
}

uint64_t Board::generate_queen_moves(uint8_t square, uint64_t board_occ) const {
    return Board::generate_bishop_moves(square, board_occ) | Board::generate_rook_moves(square, board_occ);
}

uint64_t Board::generate_knight_moves(uint8_t square, uint64_t board_occ) const {
    auto defended_squares = knight_lookup[square];
    return defended_squares;
}

// returns king moves, but does not exclude moves that wander into check
uint64_t Board::generate_king_moves(uint8_t square, uint64_t board_occ) const {
    auto defended_squares = king_lookup[square];
    return defended_squares;
}

std::array<uint64_t, 64> Board::generate_diagonal_mask_map() {
    uint64_t a1h8_mask = 0x8040201008040201ULL;
    uint64_t b1h7_mask = 0x80402010080402ULL;
    uint64_t c1h6_mask = 0x804020100804ULL;
    uint64_t d1h5_mask = 0x8040201008ULL;
    uint64_t e1h4_mask = 0x80402010ULL;
    uint64_t f1h3_mask = 0x804020ULL;
    uint64_t g1h2_mask = 0x8040ULL;
    uint64_t h1_mask = 0x80ULL;
    uint64_t a2g8_mask = 0x4020100804020100ULL;
    uint64_t a3f8_mask = 0x2010080402010000ULL;
    uint64_t a4e8_mask = 0x1008040201000000ULL;
    uint64_t a5d8_mask = 0x804020100000000ULL;
    uint64_t a6c8_mask = 0x402010000000000ULL;
    uint64_t a7b8_mask = 0x201000000000000ULL;
    uint64_t a8_mask = 0x100000000000000ULL;

    std::array<uint64_t, 64> lookup_table;
    std::vector<int> a1h8_diag = {0, 9, 18, 27, 36, 45, 54, 63};
    std::vector<int> b1h7_diag = {1, 10, 19, 28, 37, 46, 55};
    std::vector<int> c1h6_diag = {2, 11, 20, 29, 38, 47};
    std::vector<int> d1h5_diag = {3, 12, 21, 30, 39};
    std::vector<int> e1h4_diag = {4, 13, 22, 31};
    std::vector<int> f1h3_diag = {5, 14, 23};
    std::vector<int> g1h2_diag = {6, 15};
    std::vector<int> h1_diag = {7};
    std::vector<int> a2g8_diag = {8, 17, 26, 35, 44, 53, 62};
    std::vector<int> a3f8_diag = {16, 25, 34, 43, 52, 61};
    std::vector<int> a4e8_diag = {24, 33, 42, 51, 60};
    std::vector<int> a5d8_diag = {32, 41, 50, 59};
    std::vector<int> a6c8_diag = {40, 49, 58};
    std::vector<int> a7b8_diag = {48, 57};
    std::vector<int> a8_diag = {56};

    for (size_t s = 0; s < 64; ++s) {
        int square = static_cast<int>(s);

        if (std::count(a1h8_diag.begin(), a1h8_diag.end(), square)) {
            lookup_table[square] = a1h8_mask;
        } else if (std::count(b1h7_diag.begin(), b1h7_diag.end(), square)) {
            lookup_table[square] = b1h7_mask;
        } else if (std::count(c1h6_diag.begin(), c1h6_diag.end(), square)) {
            lookup_table[square] = c1h6_mask;
        } else if (std::count(d1h5_diag.begin(), d1h5_diag.end(), square)) {
            lookup_table[square] = d1h5_mask;
        } else if (std::count(e1h4_diag.begin(), e1h4_diag.end(), square)) {
            lookup_table[square] = e1h4_mask;
        } else if (std::count(f1h3_diag.begin(), f1h3_diag.end(), square)) {
            lookup_table[square] = f1h3_mask;
        } else if (std::count(g1h2_diag.begin(), g1h2_diag.end(), square)) {
            lookup_table[square] = g1h2_mask;
        } else if (std::count(h1_diag.begin(), h1_diag.end(), square)) {
            lookup_table[square] = h1_mask;
        } else if (std::count(a2g8_diag.begin(), a2g8_diag.end(), square)) {
            lookup_table[square] = a2g8_mask;
        } else if (std::count(a3f8_diag.begin(), a3f8_diag.end(), square)) {
            lookup_table[square] = a3f8_mask;
        } else if (std::count(a4e8_diag.begin(), a4e8_diag.end(), square)) {
            lookup_table[square] = a4e8_mask;
        } else if (std::count(a5d8_diag.begin(), a5d8_diag.end(), square)) {
            lookup_table[square] = a5d8_mask;
        } else if (std::count(a6c8_diag.begin(), a6c8_diag.end(), square)) {
            lookup_table[square] = a6c8_mask;
        } else if (std::count(a7b8_diag.begin(), a7b8_diag.end(), square)) {
            lookup_table[square] = a7b8_mask;
        } else if (std::count(a8_diag.begin(), a8_diag.end(), square)) {
            lookup_table[square] = a8_mask;
        }
    }
    return lookup_table;
}

std::array<uint64_t, 64> Board::generate_antidiagonal_mask_map() {
    uint64_t h1a8_mask = 0x102040810204080;
    std::vector<int> h1a8_diag = {7, 14, 21, 28, 35, 42, 49, 56};
    uint64_t g1a7_mask = 0x1020408102040;
    std::vector<int> g1a7_diag = {6, 13, 20, 27, 34, 41, 48};
    uint64_t f1a6_mask = 0x10204081020;
    std::vector<int> f1a6_diag = {5, 12, 19, 26, 33, 40};
    uint64_t e1a5_mask = 0x102040810;
    std::vector<int> e1a5_diag = {4, 11, 18, 25, 32};
    uint64_t d1a4_mask = 0x1020408;
    std::vector<int> d1a4_diag = {3, 10, 17, 24};
    uint64_t c1a3_mask = 0x10204;
    std::vector<int> c1a3_diag = {2, 9, 16};
    uint64_t b1a2_mask = 0x0102;
    std::vector<int> b1a2_diag = {1, 8};
    uint64_t a1_mask = 0x01;
    std::vector<int> a1_diag = {0};
    uint64_t h2b8_mask = 0x204081020408000;
    std::vector<int> h2b8_diag = {15, 22, 29, 36, 43, 50, 57};
    uint64_t h3c8_mask = 0x408102040800000;
    std::vector<int> h3c8_diag = {23, 30, 37, 44, 51, 58};
    uint64_t h4d8_mask = 0x810204080000000;
    std::vector<int> h4d8_diag = {31, 38, 45, 52, 59};
    uint64_t h5e8_mask = 0x1020408000000000;
    std::vector<int> h5e8_diag = {39, 46, 53, 60};
    uint64_t h6f8_mask = 0x2040800000000000;
    std::vector<int> h6f8_diag = {47, 54, 61};
    uint64_t h7g8_mask = 0x4080000000000000;
    std::vector<int> h7g8_diag = {55, 62};
    uint64_t h8_mask = 0x8000000000000000;
    std::vector<int> h8_diag = {63};

    std::array<uint64_t, 64> lookup_table;

    for (size_t square = 0; square < 64; ++square) {
        if (std::count(h1a8_diag.begin(), h1a8_diag.end(), square)) {
            lookup_table[square] = h1a8_mask;
        } else if (std::count(g1a7_diag.begin(), g1a7_diag.end(), square)) {
            lookup_table[square] = g1a7_mask;
        } else if (std::count(f1a6_diag.begin(), f1a6_diag.end(), square)) {
            lookup_table[square] = f1a6_mask;
        } else if (std::count(e1a5_diag.begin(), e1a5_diag.end(), square)) {
            lookup_table[square] = e1a5_mask;
        } else if (std::count(d1a4_diag.begin(), d1a4_diag.end(), square)) {
            lookup_table[square] = d1a4_mask;
        } else if (std::count(c1a3_diag.begin(), c1a3_diag.end(), square)) {
            lookup_table[square] = c1a3_mask;
        } else if (std::count(b1a2_diag.begin(), b1a2_diag.end(), square)) {
            lookup_table[square] = b1a2_mask;
        } else if (std::count(a1_diag.begin(), a1_diag.end(), square)) {
            lookup_table[square] = a1_mask;
        } else if (std::count(h2b8_diag.begin(), h2b8_diag.end(), square)) {
            lookup_table[square] = h2b8_mask;
        } else if (std::count(h3c8_diag.begin(), h3c8_diag.end(), square)) {
            lookup_table[square] = h3c8_mask;
        } else if (std::count(h4d8_diag.begin(), h4d8_diag.end(), square)) {
            lookup_table[square] = h4d8_mask;
        } else if (std::count(h5e8_diag.begin(), h5e8_diag.end(), square)) {
            lookup_table[square] = h5e8_mask;
        } else if (std::count(h6f8_diag.begin(), h6f8_diag.end(), square)) {
            lookup_table[square] = h6f8_mask;
        } else if (std::count(h7g8_diag.begin(), h7g8_diag.end(), square)) {
            lookup_table[square] = h7g8_mask;
        } else if (std::count(h8_diag.begin(), h8_diag.end(), square)) {
            lookup_table[square] = h8_mask;
        }
    }
    return lookup_table;
}

std::array<std::array<uint64_t, 64>, 12> Board::initialize_hash() {
    std::random_device rd;
    std::mt19937_64 e2(rd());

    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));

    std::array<std::array<uint64_t, 64>, 12> table;
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 12; ++j) {
            uint64_t random_num = dist(e2);
            table[j][i] = random_num;
        }
    }
    return table;
}

std::array<std::array<uint64_t, 64>, 64> Board::generate_in_between() {
    std::array<std::array<uint64_t, 64>, 64> lookup_table;
    auto diagonal_lookup = generate_diagonal_mask_map();
    auto antidiagonal_lookup = generate_antidiagonal_mask_map();
    for (uint8_t i = 0; i < 64; ++i) {
        for (uint8_t j = 0; j < 64; ++j) {
            if (i == j) {
                continue;
            }
            uint8_t i_file = i & 7;
            uint8_t j_file = j & 7;
            uint8_t i_rank = i >> 3;
            uint8_t j_rank = j >> 3;
            auto i_diag = diagonal_lookup[i];
            auto j_diag = diagonal_lookup[j];
            auto i_adiag = antidiagonal_lookup[i];
            auto j_adiag = antidiagonal_lookup[j];
            if (i_file == j_file) {
                auto lower = std::min(i_rank, j_rank);
                auto higher = std::max(i_rank, j_rank);
                // auto file = i_file;
                // auto lower_sq = std::min(i, j);
                // int k = 1;
                lookup_table[i][j] = 0;
                for (size_t k = lower + 1; k < higher; ++k) {
                    int shift_amount = 8 * k;
                    lookup_table[i][j] |= (1ULL << shift_amount) << i_file;
                }
            } else if (i_rank == j_rank) {
                lookup_table[i][j] = 0;
                auto lower_sq = std::min(i, j);
                auto higher_sq = std::min(i, j);
                for (size_t k = lower_sq + 1; k < higher_sq; ++k) {
                    lookup_table[i][j] |= (1ULL << k);
                }
            } else if (i_diag == j_diag) {
                auto lower = std::min(i_rank, j_rank);
                auto higher = std::max(i_rank, j_rank);
                lookup_table[i][j] = i_diag;
                for (size_t k = 0; k < 64; ++k) {
                    uint8_t f = k >> 3;
                    if (f <= lower || f >= higher) {
                        lookup_table[i][j] &= ~(1ULL << k);
                    }
                }
            } else if (i_adiag == j_adiag) {
                auto lower = std::min(i_rank, j_rank);
                auto higher = std::max(i_rank, j_rank);
                lookup_table[i][j] = i_adiag;
                for (size_t k = 0; k < 64; ++k) {
                    uint8_t f = k >> 3;
                    if (f <= lower || f >= higher) {
                        lookup_table[i][j] &= ~(1ULL << k);
                    }
                }

            } else {
                lookup_table[i][j] = 0;
            }
        }
    }
    return lookup_table;
}

std::vector<Move> Board::generate_moves() {
    // figure out which side is to move
    size_t side = static_cast<size_t>(side_to_move);
    size_t other_side = 1 - side;
    std::vector<Move> vec_of_moves;

    uint64_t pinned = 0;
    int king_loc = __builtin_ffsll(kings[side]) - 1;
    pinned_pieces[side].clear();
    std::array<uint64_t, 64> available_moves;

    uint64_t pinner = rook_xray_attacks(all_per_side[side] | all_per_side[other_side], all_per_side[side], king_loc) & (rooks[other_side] | queens[other_side]);

    while (pinner) {
        int next_square = __builtin_ffsll(pinner) - 1;
        auto pins = in_between[king_loc][next_square] & all_per_side[side];
        if (pins) {
            uint8_t pinned_square = (uint8_t)__builtin_ffsll(pins) - 1;
            pinned_pieces[side].push_back(pinned_square);
            uint64_t avail_moves = in_between[king_loc][next_square] | (1ULL << next_square);
            available_moves[pinned_square] = avail_moves;
        }
        pinned |= pins;

        pinner &= ~(1ULL << next_square);
    }

    pinner = bishop_xray_attacks(all_per_side[side] | all_per_side[other_side], all_per_side[side], king_loc) & (bishops[other_side] | queens[other_side]);

    while (pinner) {
        int next_square = __builtin_ffsll(pinner) - 1;
        auto pins = in_between[king_loc][next_square] & all_per_side[side];
        if (pins) {
            uint8_t pinned_square = (uint8_t)__builtin_ffsll(pins) - 1;
            pinned_pieces[side].push_back(pinned_square);
            uint64_t avail_moves = in_between[king_loc][next_square] | (1ULL << next_square);
            available_moves[pinned_square] = avail_moves;
        }
        pinned |= pins;

        pinner &= ~(1ULL << next_square);
    }
    auto board_occ = all_per_side[0] | all_per_side[1];
    auto queen_maps = defense_maps_for_piece(queens[side], board_occ, 'q');
    auto bishop_maps = defense_maps_for_piece(bishops[side], board_occ, 'b');
    auto rook_maps = defense_maps_for_piece(rooks[side], board_occ, 'r');
    auto king_maps = defense_maps_for_piece(kings[side], board_occ, 'k');
    auto knight_maps = defense_maps_for_piece(knights[side], board_occ, 'n');
    auto pawn_move_maps = defense_maps_for_piece(pawns[side], board_occ, 'p');
    auto pawn_capture_maps = defense_maps_for_piece(pawns[side], board_occ, 'P');

    std::array<char, 6> gen_funcs = {'b', 'n', 'r', 'q', 'k', 'p'};
    std::array<uint64_t, 6> arr = {bishops[other_side], knights[other_side], rooks[other_side], queens[other_side],
                                   kings[other_side], pawns[other_side]};

    int piece_giving_check = -1;

    // uint64_t blocking_squares = 0xffffffffffffffffUL;
    if (in_check()) {
        int check_counter = 0;
        if ((queen_defends[other_side] & kings[side])) {
            check_counter += 1;
            piece_giving_check = 3;
        }
        if ((rook_defends[other_side] & kings[side])) {
            check_counter += 1;
            piece_giving_check = 2;
        }
        if ((bishop_defends[other_side] & kings[side])) {
            check_counter += 1;
            piece_giving_check = 0;
        }
        if ((knight_defends[other_side] & kings[side])) {
            check_counter += 1;
            piece_giving_check = 1;
        }
        if ((pawn_defends[other_side] & kings[side])) {
            check_counter += 1;
            piece_giving_check = 5;
        }
        if (check_counter == 2) {
            queen_maps.clear();
            rook_maps.clear();
            bishop_maps.clear();
            knight_maps.clear();
            pawn_move_maps.clear();
            pawn_capture_maps.clear();
        }
    }
    std::vector<std::pair<uint64_t, uint8_t>> most_maps;
    // auto board_occ = all_per_side[0] | all_per_side[1];
    // auto queen_maps = defense_maps_for_piece(queens[side], board_occ, &Board::generate_queen_moves);
    // auto bishop_maps = defense_maps_for_piece(bishops[side], board_occ, &Board::generate_bishop_moves);
    // auto rook_maps = defense_maps_for_piece(rooks[side], board_occ, &Board::generate_rook_moves);
    // auto king_maps = defense_maps_for_piece(kings[side], board_occ, &Board::generate_king_moves);
    // auto knight_maps = defense_maps_for_piece(knights[side], board_occ, &Board::generate_knight_moves);
    // auto pawn_move_maps = defense_maps_for_piece(pawns[side], board_occ, &Board::generate_pawn_moves);
    // auto pawn_capture_maps = defense_maps_for_piece(pawns[side], board_occ, &Board::generate_pawn_attacks);

    // for (auto &m: king_maps) {
    //     m.first &= ~attack_maps[other_side];
    // }

    for (auto &m : pawn_capture_maps) {
        m.first &= ~all_per_side[side];
        m.first &= all_per_side[other_side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while (dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
            if (piece_giving_check >= 0) {
                if (king_still_under_attack(dest, kings[side], arr[piece_giving_check], gen_funcs[piece_giving_check])) {
                    map &= ~(1ULL << dest);
                    dest_plus_one = __builtin_ffsll(map);
                    continue;
                }
            }
            if (((1ULL << from) & pinned) && !((1ULL << dest) & available_moves[from])) {
                map &= ~(1ULL << dest);
                dest_plus_one = __builtin_ffsll(map);
                continue;
            }
            if (dest == en_passant_target) {
                Move en_passant = Move(from, dest, MoveType::EN_PASSANT);
                vec_of_moves.push_back(en_passant);
            }
            // has the pawn reached the back rank?
            else if (((1ULL << dest) & 0xff00000000000000) || ((1ULL << dest) & 0xff)) {
                Move promote_to_bishop = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP);
                Move promote_to_knight = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT);
                Move promote_to_queen = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN);
                Move promote_to_rook = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_ROOK);
                vec_of_moves.push_back(promote_to_queen);
                vec_of_moves.push_back(promote_to_rook);
                vec_of_moves.push_back(promote_to_bishop);
                vec_of_moves.push_back(promote_to_knight);
            } else {
                vec_of_moves.push_back(Move(from, dest, MoveType::CAPTURE));
            }
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }

    for (auto &m : pawn_move_maps) {
        m.first &= ~all_per_side[side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while (dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
            if (piece_giving_check >= 0) {
                if (king_still_under_attack(dest, kings[side], arr[piece_giving_check], gen_funcs[piece_giving_check])) {
                    map &= ~(1ULL << dest);
                    dest_plus_one = __builtin_ffsll(map);
                    continue;
                }
            }
            if (((1ULL << from) & pinned) && !((1ULL << dest) & available_moves[from])) {
                map &= ~(1ULL << dest);
                dest_plus_one = __builtin_ffsll(map);
                continue;
            }
            if (((1ULL << dest) & 0xff00000000000000) || ((1ULL << dest) & 0xff)) {
                Move promote_to_bishop = Move(from, dest, MoveType::PROMOTE_TO_BISHOP);
                Move promote_to_knight = Move(from, dest, MoveType::PROMOTE_TO_KNIGHT);
                Move promote_to_queen = Move(from, dest, MoveType::PROMOTE_TO_QUEEN);
                Move promote_to_rook = Move(from, dest, MoveType::PROMOTE_TO_ROOK);
                vec_of_moves.push_back(promote_to_queen);
                vec_of_moves.push_back(promote_to_rook);
                vec_of_moves.push_back(promote_to_bishop);
                vec_of_moves.push_back(promote_to_knight);
            } else {
                vec_of_moves.push_back(Move(from, dest, MoveType::QUIET));
            }
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }

    most_maps.insert(most_maps.end(), queen_maps.begin(), queen_maps.end());
    most_maps.insert(most_maps.end(), bishop_maps.begin(), bishop_maps.end());
    most_maps.insert(most_maps.end(), rook_maps.begin(), rook_maps.end());
    // most_maps.insert(most_maps.end(), king_maps.begin(), king_maps.end());
    most_maps.insert(most_maps.end(), knight_maps.begin(), knight_maps.end());
    // all_maps.insert(all_maps.end(), pawn_move_maps.begin(), pawn_move_maps.end());
    // all_maps.insert(all_maps.end(), pawn_capture_maps.begin(), pawn_capture_maps.end());

    for (auto &m : most_maps) {
        m.first &= ~all_per_side[side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while (dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
            // if (piece_giving_check >= 0 && (kings[side] & (1 << from))) {
            //     if (king_still_under_attack(dest, (1ULL << dest), arr[piece_giving_check], gen_funcs[piece_giving_check])) {
            //         map &= ~(1ULL << dest);
            //         dest_plus_one = __builtin_ffsll(map);
            //         continue;
            //     }
            if (piece_giving_check >= 0) {
                if (king_still_under_attack(dest, kings[side], arr[piece_giving_check], gen_funcs[piece_giving_check])) {
                    map &= ~(1ULL << dest);
                    dest_plus_one = __builtin_ffsll(map);
                    continue;
                }
            }
            if (((1ULL << from) & pinned) && !((1ULL << dest) & available_moves[from])) {
                map &= ~(1ULL << dest);
                dest_plus_one = __builtin_ffsll(map);
                continue;
            }
            MoveType type = MoveType::QUIET;
            if (((1ULL << dest) & all_per_side[other_side])) {
                type = MoveType::CAPTURE;
            }
            vec_of_moves.push_back(Move(from, dest, type));
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }

    for (auto &m : king_maps) {
        m.first &= ~all_per_side[side];
        m.first &= ~defense_maps[other_side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while (dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
            if (piece_giving_check >= 0) {
                if (king_still_under_attack(dest, (1ULL << dest), arr[piece_giving_check], gen_funcs[piece_giving_check])) {
                    map &= ~(1ULL << dest);
                    dest_plus_one = __builtin_ffsll(map);
                    continue;
                }
            }
            MoveType type = MoveType::QUIET;
            if (((1ULL << dest) & all_per_side[other_side])) {
                type = MoveType::CAPTURE;
            }
            vec_of_moves.push_back(Move(from, dest, type));
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }

    if (short_castle_rights[side]) {
        uint64_t relevant_squares = side ? 0x7000000000000000ULL : 0x70ULL;
        uint64_t to_be_vacated = side ? 0x6000000000000000ULL : 0x60ULL;
        auto total_occ = all_per_side[side] | all_per_side[other_side];
        if (!(defense_maps[other_side] & relevant_squares) && (total_occ & to_be_vacated) == 0) {
            if (!side) {
                vec_of_moves.push_back(Move(4, 6, MoveType::SHORT_CASTLES));
            } else {
                vec_of_moves.push_back(Move(60, 62, MoveType::SHORT_CASTLES));
            }
        }
    }
    if (long_castle_rights[side]) {
        uint64_t relevant_squares = side ? 0x1c00000000000000ULL : 0x1cULL;
        uint64_t to_be_vacated = side ? 0xe00000000000000ULL : 0xeULL;
        auto total_occ = all_per_side[side] | all_per_side[other_side];
        if (!(defense_maps[other_side] & relevant_squares) && (total_occ & to_be_vacated) == 0) {
            // vec_of_moves.push_back(Move(0, 0, MoveType::LONG_CASTLES));
            if (!side) {
                vec_of_moves.push_back(Move(4, 2, MoveType::LONG_CASTLES));
            } else {
                vec_of_moves.push_back(Move(60, 58, MoveType::LONG_CASTLES));
            }
        }
    }

    // TODO add castles and pawn stuff (including en passant)

    return vec_of_moves;
}

void Board::make_move(Move move) {
    History history;
    history.all_per_side = all_per_side;
    history.queens = queens;
    history.rooks = rooks;
    history.knights = knights;
    history.bishops = bishops;
    history.pawns = pawns;
    history.kings = kings;
    history.queen_defends = queen_defends;
    history.rook_defends = rook_defends;
    history.knight_defends = knight_defends;
    history.bishop_defends = bishop_defends;
    history.pawn_defends = pawn_defends;
    history.king_defends = king_defends;
    history.defense_maps = defense_maps;
    history.short_castle_rights = short_castle_rights;
    history.long_castle_rights = long_castle_rights;
    history.moves = moves;
    history.en_passant_target = en_passant_target;
    history.side_to_move = side_to_move;
    this->history.push(history);
    size_t current_move = static_cast<size_t>(side_to_move);
    size_t other_move = 1 - current_move;

    uint64_t move_bitboard = 0;
    if (move.type() == MoveType::DOUBLE_PAWN_PUSH) {
        std::cout << "HELLO";
        // the current side to move is black
        if (current_move) {
            en_passant_target = move.origin_square() - 8;
        } else {
            en_passant_target = move.origin_square() + 8;
        }
    } else if (move.type() == MoveType::EN_PASSANT) {
        pawns[other_move] &= ~(1 << en_passant_target);
        en_passant_target = 65;
    } else {
        en_passant_target = 65;
    }
    
    int moved = -1;
    int captured = -1;
    std::array<std::array<uint64_t, 2> *, 6> arr = {&bishops, &knights, &rooks, &queens, &kings, &pawns};

    if (move.type() == MoveType::SHORT_CASTLES) {
        if (side_to_move == Side::WHITE) {
            kings[current_move] = 0x40;
            rooks[current_move] &= ~0x80;
            rooks[current_move] |= 0x20;
            move_bitboard = 0xf0;
        } else {
            kings[current_move] = 0x4000000000000000;
            rooks[current_move] &= ~0x8000000000000000;
            rooks[current_move] |= 0x2000000000000000;
            move_bitboard = 0xf000000000000000;
        }
        short_castle_rights[current_move] = false;
    } else if (move.type() == MoveType::LONG_CASTLES) {
        if (side_to_move == Side::WHITE) {
            kings[current_move] = 0x4;
            rooks[current_move] &= ~0x1;
            rooks[current_move] |= 0x8;
            move_bitboard = 0x1f;
        } else {
            kings[current_move] = 0x400000000000000;
            rooks[current_move] &= ~0x100000000000000;
            rooks[current_move] |= 0x800000000000000;
            move_bitboard = 0x1f00000000000000;
        }
        long_castle_rights[current_move] = false;
    } else {
        auto from = move.origin_square();
        auto to = move.destination_square();
        move_bitboard = (1ULL << from) | (1ULL << to);

        if (short_castle_rights[current_move]) {
            if (side_to_move == Side::WHITE) {
                if (from == 4 || from == 7) {
                    short_castle_rights[current_move] = false;
                }
            } else {
                if (from == 63 || from == 60) {
                    short_castle_rights[current_move] = false;
                }
            }
        }

        if (long_castle_rights[current_move]) {
            if (side_to_move == Side::WHITE) {
                if (from == 4 || from == 0) {
                    long_castle_rights[current_move] = false;
                }
            } else {
                if (from == 60 || from == 56) {
                    long_castle_rights[current_move] = false;
                }
            }
        }

        // std::vector<int> moved;
        // std::vector<int> captured;
        // bool capture = false;
        for (int i = 0; i < 6; ++i) {
            auto piece_arr_ptr = arr[i];
            if (((*piece_arr_ptr)[current_move] & (1ULL << from))) {
                (*piece_arr_ptr)[current_move] &= ~(1ULL << from);
                (*piece_arr_ptr)[current_move] |= 1ULL << to;
                moved = i;
                // moved.push_back(i);
            }
            if (((*piece_arr_ptr)[other_move] & (1ULL << to))) {
                (*piece_arr_ptr)[other_move] &= ~(1ULL << to);
                captured = i;
                // captured.push_back(i);
                // capture = true;
            }
        }
    }

    attack_maps[current_move] = 0;
    defense_maps[current_move] = 0;
    all_per_side[current_move] = 0;
    all_per_side[current_move] = rooks[current_move] | bishops[current_move] | knights[current_move] |
                                 queens[current_move] | kings[current_move] | pawns[current_move];
    all_per_side[other_move] = rooks[other_move] | bishops[other_move] | knights[other_move] |
                               queens[other_move] | kings[other_move] | pawns[other_move];

    
    std::array<std::array<uint64_t, 2>*, 6> def_maps = { &bishop_defends, &knight_defends, 
        &rook_defends, &queen_defends, &king_defends, &pawn_defends };
    std::array<char, 6> gen_funcs = { 'b', 'n', 'r', 'q', 'k', 'P' };

    for (size_t i = 0; i < 6; ++i) {
        if (i == captured || ((*def_maps[i])[other_move] & move_bitboard)) {
            auto maps = this->defense_maps_for_piece((*arr[i])[other_move], all_per_side[current_move] | all_per_side[other_move],
                gen_funcs[i]);
            (*def_maps[i])[other_move] = 0;
            for (auto &map: maps) {
                (*def_maps[i])[other_move] |= map.first;
            }
        }
    }

    for (size_t i = 0; i < 6; ++i) {
        if (i == moved || ((*def_maps[i])[current_move] & move_bitboard)) {
            auto maps = this->defense_maps_for_piece((*arr[i])[current_move], all_per_side[current_move] | all_per_side[other_move],
                gen_funcs[i]);
            (*def_maps[i])[current_move] = 0;
            for (auto &map: maps) {
                (*def_maps[i])[current_move] |= map.first;
            }
        }
    }

    // now flip whose turn it is
    this->side_to_move = static_cast<Side>(other_move);
    this->defense_maps[current_move] = pawn_defends[current_move] | rook_defends[current_move] |
                                       knight_defends[current_move] | bishop_defends[current_move] | queen_defends[current_move] | king_defends[current_move];
    this->defense_maps[other_move] = pawn_defends[other_move] | rook_defends[other_move] |
                                     knight_defends[other_move] | bishop_defends[other_move] | queen_defends[other_move] | king_defends[other_move];
    this->attack_maps[current_move] = this->defense_maps[current_move] & all_per_side[current_move];
    this->attack_maps[other_move] = this->defense_maps[other_move] & all_per_side[other_move];
    // generate the moves for the next side (which also updates attack and defenes maps for
    // the new side to move)
    this->moves = generate_moves();
}

void Board::unmake_move(Move move) {
    auto pop = this->history.top();
    this->history.pop();
    this->all_per_side = pop.all_per_side;
    this->pawns = pop.pawns;
    this->rooks = pop.rooks;
    this->knights = pop.knights;
    this->bishops = pop.bishops;
    this->queens = pop.queens;
    this->kings = pop.kings;
    this->side_to_move = pop.side_to_move;
    this->pawn_defends = pop.pawn_defends;
    this->rook_defends = pop.rook_defends;
    this->knight_defends = pop.knight_defends;
    this->bishop_defends = pop.bishop_defends;
    this->queen_defends = pop.queen_defends;
    this->king_defends = pop.king_defends;
    this->defense_maps = pop.defense_maps;
    this->attack_maps = pop.attack_maps;
    this->long_castle_rights = pop.long_castle_rights;
    this->short_castle_rights = pop.short_castle_rights;
    this->moves = pop.moves;
    this->en_passant_target = pop.en_passant_target;
}

uint64_t Board::rook_xray_attacks(uint64_t occ, uint64_t blockers, uint8_t square) const {
    auto attacks = generate_rook_moves(square, occ);
    blockers &= attacks;
    return attacks ^ generate_rook_moves(square, occ ^ blockers);
}

uint64_t Board::bishop_xray_attacks(uint64_t occ, uint64_t blockers, uint8_t square) const {
    auto attacks = generate_bishop_moves(square, occ);
    blockers &= attacks;
    return attacks ^ generate_bishop_moves(square, occ ^ blockers);
}

bool Board::king_still_under_attack(uint8_t move_dest, uint64_t king_board, uint64_t piece_board,
                                    char move_type) const {
    uint64_t dest_board = 1ULL << move_dest;
    uint64_t p_board = piece_board;
    p_board &= ~(1ULL << move_dest);
    uint64_t master_map = 0;
    int set_bit = __builtin_ffsll(p_board);
    while (set_bit) {
        uint8_t square_from = static_cast<uint8_t>(set_bit - 1);
        uint64_t defense_map = 0;
        switch (move_type) {
            case 'q':
                defense_map = generate_queen_moves(square_from, dest_board);
                break;
            case 'r':
                defense_map = generate_rook_moves(square_from, dest_board);
                break;
            case 'b':
                defense_map = generate_bishop_moves(square_from, dest_board);
                break;
            case 'n':
                defense_map = generate_knight_moves(square_from, dest_board);
                break;
            case 'k':
                defense_map = generate_king_moves(square_from, dest_board);
                break;
            case 'p':
                defense_map = generate_pawn_attacks(square_from, dest_board);
                break;
            // case 'P':
            //     defense_map = generate_pawn_attacks(square_from, dest_board);
            //     break;
        }
        master_map |= defense_map;
        p_board &= ~(1ULL << square_from);
        set_bit = __builtin_ffsll(p_board);
    }
    return master_map & king_board;
}

void Board::parse_fen(std::string fen) {
    // parse the piece positions
    int s = 0;
    int i = 0;
    rooks = {0, 0};
    knights = {0, 0};
    bishops = {0, 0};
    queens = {0, 0};
    kings = {0, 0};
    pawns = {0, 0};
    rook_defends = {0, 0};
    knight_defends = {0, 0};
    bishop_defends = {0, 0}, queen_defends = {0, 0};
    king_defends = {0, 0};
    pawn_defends = {0, 0};
    short_castle_rights = {false, false};
    long_castle_rights = {false, false};

    while (s < 64) {
        auto rank = 7 - (s >> 3);
        auto file = s & 7;
        auto square = 8 * rank + file;
        auto next_character = fen[i];
        if (next_character == '/') {
            ++i;
            continue;
        }
        if (isdigit(next_character)) {
            s += static_cast<int>(next_character - '0');
            ++i;
            continue;
        }
        auto side = isupper(next_character) ? 0 : 1;
        next_character = tolower(next_character);
        // std::cout << next_character << std::endl;
        switch (next_character) {
            case 'r':
                rooks[side] |= 1ULL << square;
                break;
            case 'n':
                knights[side] |= 1ULL << square;
                break;
            case 'b':
                bishops[side] |= 1ULL << square;
                break;
            case 'q':
                queens[side] |= 1ULL << square;
                break;
            case 'k':
                kings[side] |= 1ULL << square;
                break;
            case 'p':
                pawns[side] |= 1ULL << square;
                break;
        }
        ++s;
        ++i;
    }
    // check side to move
    i += 2;
    side_to_move = fen[i] == 'w' ? Side::WHITE : Side::WHITE;
    // check castling rights
    i += 2;

    while (fen[i] != ' ') {
        if (fen[i] == '-') {
            i += 1;
            continue;
        }
        switch (fen[i]) {
            case 'k':
                short_castle_rights[1] = true;
                break;
            case 'K':
                short_castle_rights[0] = true;
                break;
            case 'q':
                long_castle_rights[1] = true;
                break;
            case 'Q':
                long_castle_rights[0] = true;
                break;
        }
        ++i;
    }
    // now check en passant target square if present;
    ++i;

    if (fen[i] != '-') {
        int file = (int)(fen[i] - 'a');
        int rank = (int)(fen[i + 1] - '1');
        en_passant_target = (uint8_t)(8 * rank + file);
    }
}

// This function generates all of the moves for a specific type of piece
// TODO: make sure to check that the piece being moved isn't pinned, also finish implementing this
std::vector<std::pair<uint64_t, uint8_t>> Board::defense_maps_for_piece(uint64_t piece_board, uint64_t board_occ, char move_type) const {
    uint64_t p_board = piece_board;

    std::vector<std::pair<uint64_t, uint8_t>> maps;
    int set_bit = __builtin_ffsll(p_board);
    while (set_bit) {
        uint8_t square_from = static_cast<uint8_t>(set_bit - 1);
        uint64_t defense_map = 0;
        switch (move_type) {
            case 'q':
                defense_map = generate_queen_moves(square_from, board_occ);
                break;
            case 'r':
                defense_map = generate_rook_moves(square_from, board_occ);
                break;
            case 'b':
                defense_map = generate_bishop_moves(square_from, board_occ);
                break;
            case 'n':
                defense_map = generate_knight_moves(square_from, board_occ);
                break;
            case 'k':
                defense_map = generate_king_moves(square_from, board_occ);
                break;
            case 'p':
                defense_map = generate_pawn_moves(square_from, board_occ);
                break;
            case 'P':
                defense_map = generate_pawn_attacks(square_from, board_occ);
                break;
        }
        maps.push_back(std::make_pair(defense_map, square_from));

        p_board &= ~(1ULL << square_from);
        set_bit = __builtin_ffsll(p_board);
    }
    return maps;
}

void Board::update_board_state(Move move) {
}

// Getter for side to move
int Board::get_side_to_move() {
    return static_cast<size_t>(side_to_move);
}

// Getters for boards
std::array<uint64_t, 2> Board::get_knights() const {
    return knights;
}

std::array<uint64_t, 2> Board::get_bishops() const {
    return bishops;
}

std::array<uint64_t, 2> Board::get_rooks() const {
    return rooks;
}

std::array<uint64_t, 2> Board::get_queens() const {
    return queens;
}

std::array<uint64_t, 2> Board::get_kings() const {
    return kings;
}

std::array<uint64_t, 2> Board::get_pawns() const {
    return pawns;
}

std::array<std::vector<uint8_t>, 2> Board::get_pins() const {
    return pinned_pieces; 
}

std::vector<uint8_t> Board::get_piece_pos(char piece_type) const {
    std::vector<uint8_t> pos;
    size_t side = static_cast<size_t>(side_to_move); 
    auto piece_board = pawns[side];
    switch(piece_type) {
        case 'p':
            piece_board = pawns[side];
            break;
        case 'r':
            piece_board = rooks[side];
            break;
        case 'b':
            piece_board = bishops[side];
            break;
        case 'q':
            piece_board = queens[side];
            break;
        case 'k':
            piece_board = kings[side];
            break;
        case 'n':
            piece_board = knights[side];
            break;
    }

    int set_bit = __builtin_ffsll(piece_board);
    while (set_bit) {
        pos.push_back(set_bit-1);
        piece_board &= ~(1ULL << (set_bit -1));
        set_bit = __builtin_ffsll(piece_board);
    }
    return pos;
}



uint64_t Board::hash() const {
    uint64_t hash = 0;
    for (int i = 0; i < 64; ++i) {
        auto wp = pawns[0];
        auto bp = pawns[1];
        auto wq = queens[0];
        auto bq = queens[1];
        auto wr = rooks[0];
        auto br = rooks[1];
        auto wb = bishops[0];
        auto bb = bishops[1];
        auto wk = kings[0];
        auto bk = kings[1];
        auto wn = knights[0];
        auto bn = knights[1];
        int j = 0;
        uint64_t piece_board = 1ULL << i;
        if ((wp & piece_board)) {
            j = 1;
        } else if ((bp & piece_board)) {
            j = 2;
        } else if ((wq & piece_board)) {
            j = 3;
        } else if ((bq & piece_board)) {
            j = 4;
        } else if ((wr & piece_board)) {
            j = 5;
        } else if ((br & piece_board)) {
            j = 6;
        } else if ((wb & piece_board)) {
            j = 7;
        } else if ((bb & piece_board)) {
            j = 8;
        } else if ((wk & piece_board)) {
            j = 9;
        } else if ((bk & piece_board)) {
            j = 10;
        } else if ((wn & piece_board)) {
            j = 11;
        } else if ((bn & piece_board)) {
            j = 12;
        }
        if (j) {
            hash = hash ^ hash_helper[j][i];
        }
    }
    return hash;
}

// TODO create a field in the class for this
// to reduce the number of calculations
bool Board::in_check() const {
    size_t to_move = static_cast<size_t>(side_to_move);
    size_t other = 1 - to_move;
    bool in_check = (kings[to_move] & defense_maps[other]) != 0;
    return in_check;
}

bool Board::is_checkmate() const {
    return in_check() && moves.size() == 0;
}

bool Board::is_stalemate() const {
    return in_check() && moves.size() != 0;
}

std::vector<Move> Board::get_moves() {
    return moves;
}

std::vector<uint16_t> Board::get_moves_as_u16() {
    std::vector<uint16_t> m;
    for (auto &move : moves) {
        uint16_t as_u16 = (move.origin_square() << 6) | move.destination_square();
        m.push_back(as_u16);
    }
    return m;
}

// returns moves as an array of strings / squares where
// l_moves[i] is the origin and l_moves[i+1] is the destination
std::vector<std::string> Board::get_moves_algebraic() {
    std::vector<std::string> l_moves(moves.size() * 2);
    int j = 0;
    for (int i = 0; i < moves.size(); i++) {
        l_moves.at(j++) = moves[i].origin_square_algebraic();
        l_moves.at(j++) = moves[i].destination_square_algebraic();
    }
    return l_moves;
}
