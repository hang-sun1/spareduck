#include "board.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>
#include <memory>
#include <stack>
#include <random>
#include <cmath>

#include "move.h"

using std::uint64_t;

Board::Board() {
    this->history = std::make_shared<std::stack<Board>>();
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
    this->pawn_defends = { 0xff0000, 0xff0000000000 };
    this->rook_defends = {0x8142, 0x4281000000000000 };
    this->knight_defends = { 0xa51800, 0x18a50000000000};
    this->bishop_defends = { 0x5a00, 0x5a000000000000};
    this->queen_defends = { 0x1c14, 0x141c000000000000 };
    this->king_defends = { 0x3828, 0x2838000000000000 };
    this->defense_maps[1] = pawn_defends[1], rook_defends[1], knight_defends[1], bishop_defends[1], queen_defends[1], king_defends[1];
    this->defense_maps[0] = pawn_defends[0], rook_defends[0], knight_defends[0], bishop_defends[0], queen_defends[0], king_defends[0];
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
    this->moved_piece_boards = std::vector<std::array<uint64_t, 2>*>();
    this->taken_piece_boards = std::vector<std::array<uint64_t, 2>*>();
    this->hash_helper = Board::initialize_hash();
    this->moves = this->generate_moves();
    // there is no en passant target yet, so just set it to some square off the board
    this->en_passant_target = 65;
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
uint64_t Board::generate_pawn_moves(uint8_t square, uint64_t board_occ) const {
    size_t to_move = static_cast<size_t>(side_to_move);
    size_t not_to_move = 1 - to_move;
    uint64_t one_piece_board = 1ULL << square;
    uint64_t moves = 0;
    uint64_t all_pieces = all_per_side[not_to_move] | all_per_side[to_move];
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
    size_t to_move = static_cast<size_t>(side_to_move);
    size_t not_to_move = 1 - to_move;
    uint64_t one_piece_board = 1ULL << square;
    uint64_t moves = 0;
    uint8_t file = square & 7;
    uint64_t all_pieces = all_per_side[not_to_move] | all_per_side[to_move];
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
    board_occ = this->all_per_side[0] | this->all_per_side[1];
    board_occ = (board_occ >> file) & a_file_mask;
    board_occ = ((c2h7_diag_mask * board_occ) >> 58) & 63;
    board_occ = a1h8_diag_mask * this->rank_attack_lookup[(square ^ 56) >> 3][static_cast<uint8_t>(board_occ)];
    uint64_t file_moves = (h_file_mask & board_occ) >> (file ^ 7);

    auto to_move = static_cast<size_t>(side_to_move);
    auto defended_squares = file_moves | rank_moves;
    // auto attacked_squares = defended_squares & (~all_per_side[to_move]);
    // defense_maps[to_move] |= defended_squares;
    // attack_maps[to_move] |= attacked_squares;
    return defended_squares;
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
    auto to_move = static_cast<size_t>(side_to_move);
    auto defended_squares = diagonal_defend | antidiagonal_defend;
    return defended_squares;
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

    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2,61)), std::llround(std::pow(2,62)));

    std::array<std::array<uint64_t, 64>, 12> table;
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 12; ++j) {
            uint64_t random_num = dist(e2);
            table[j][i] = random_num;
        }
    } 
    return table;
}

std::vector<Move> Board::generate_moves() const {
    // figure out which side is to move
    size_t side = static_cast<size_t>(side_to_move);
    size_t other_side = 1 - side;
    std::vector<Move> vec_of_moves;
    std::vector<std::pair<uint64_t, uint8_t>> most_maps;
    auto board_occ = all_per_side[0] | all_per_side[1];
    auto queen_maps = defense_maps_for_piece(queens[side], board_occ, &Board::generate_queen_moves);
    auto bishop_maps = defense_maps_for_piece(bishops[side], board_occ, &Board::generate_bishop_moves);
    auto rook_maps = defense_maps_for_piece(rooks[side], board_occ, &Board::generate_rook_moves);
    auto king_maps = defense_maps_for_piece(kings[side], board_occ, &Board::generate_king_moves);
    auto knight_maps = defense_maps_for_piece(knights[side], board_occ, &Board::generate_knight_moves);
    auto pawn_move_maps = defense_maps_for_piece(pawns[side], board_occ, &Board::generate_pawn_moves);
    auto pawn_capture_maps = defense_maps_for_piece(pawns[side], board_occ, &Board::generate_pawn_attacks);

    for (auto &m: king_maps) {
        m.first &= ~attack_maps[other_side];
    }
    // std::cout << (knight_maps[0]) << std::endl;

    for (auto &m: pawn_capture_maps) {
        m.first &= ~all_per_side[side];
        m.first &= all_per_side[other_side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while(dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) -1;
            if (((1ULL << dest) & 0xff00000000000000) || ((1ULL << dest) & 0xff)) {
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

    for (auto &m: pawn_move_maps) {
        m.first &= ~all_per_side[side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while(dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) -1;
            if (((1ULL << dest) & 0xff00000000000000) && ((1ULL << dest) & 0xff)) {
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
    most_maps.insert(most_maps.end(), king_maps.begin(), king_maps.end());
    most_maps.insert(most_maps.end(), knight_maps.begin(), knight_maps.end());
    // all_maps.insert(all_maps.end(), pawn_move_maps.begin(), pawn_move_maps.end());
    // all_maps.insert(all_maps.end(), pawn_capture_maps.begin(), pawn_capture_maps.end());

    for (auto &m: most_maps) {
        m.first &= ~all_per_side[side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while(dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) -1;
            MoveType type = MoveType::QUIET;
            if (((1ULL << dest) & all_per_side[other_side])) {
                type = MoveType::CAPTURE;
            }
            vec_of_moves.push_back(Move(from, dest, type));
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }


    // check if the king is in check
    if (kings[side] & attack_maps[other_side]) {

    }

    // TODO add castles and pawn stuff (including en passant)

    return vec_of_moves;
}


void Board::make_move(Move move) {
    this->history->push(*this);
    size_t current_move = static_cast<size_t>(side_to_move);
    size_t other_move = 1 - current_move;

    auto from = move.origin_square();
    auto to = move.destination_square();
    
    std::array<std::array<uint64_t, 2>*, 6> arr = { &bishops, &knights, &rooks, &queens, &kings, &pawns };
    std::vector<int> moved;
    std::vector<int> captured; 
    bool capture = false;
    for (int i = 0; i < 6; ++i) {
        auto piece_arr_ptr = arr[i];
        if (((*piece_arr_ptr)[current_move] & (1ULL << from))) {
            (*piece_arr_ptr)[current_move] &= ~(1ULL << from);
            (*piece_arr_ptr)[current_move] |= 1ULL << to;
            moved.push_back(i);
        }
        if (((*piece_arr_ptr)[other_move] & (1ULL << to))) {
            (*piece_arr_ptr)[other_move] &= ~(1ULL << to);
            captured.push_back(i);
            capture = true;
        }
    } 
    

    attack_maps[current_move] = 0;
    defense_maps[current_move] = 0;
    all_per_side[current_move] = 0;
    all_per_side[current_move] = rooks[current_move] | bishops[current_move] | knights[current_move] |
                                 queens[current_move] | kings[current_move] | pawns[current_move];
    all_per_side[other_move] = rooks[other_move] | bishops[other_move] | knights[other_move] |
                                 queens[other_move] | kings[other_move] | pawns[other_move];

    
    std::array<std::array<uint64_t, 2>*, 6> defense_maps = { &bishop_defends, &knight_defends, 
        &rook_defends, &queen_defends, &king_defends, &pawn_defends };
    std::array<uint64_t (Board::*)(uint8_t, uint64_t) const, 6> gen_funcs = { &Board::generate_bishop_moves,
        &Board::generate_knight_moves, &Board::generate_rook_moves, &Board::generate_queen_moves, &Board::generate_king_moves,
        &Board::generate_pawn_moves };

    for (auto &i: captured) {
        auto maps = this->defense_maps_for_piece((*arr[i])[other_move], all_per_side[current_move] | all_per_side[other_move],
            gen_funcs[i]);

        for (auto &map: maps) {
            (*defense_maps[i])[other_move] |= map.first;
        }
    }

    for (auto &i: moved) {
        auto maps = this->defense_maps_for_piece((*arr[i])[current_move], all_per_side[current_move] | all_per_side[other_move],
            gen_funcs[i]);
        for (auto &map: maps) {
            (*defense_maps[i])[current_move] |= map.first;
        }
    }
    // now flip whose turn it is
    this->side_to_move = static_cast<Side>(other_move);
    // generate the moves for the next side (which also updates attack and defenes maps for
    // the new side to move)
    this->moves = generate_moves();

}

void Board::unmake_move(Move move) {
    auto pop = this->history->top();
    this->history->pop();
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
    this->all_per_side = pop.all_per_side;
    this->attack_maps = pop.attack_maps;
    this->moves = pop.moves;
    this->en_passant_target = pop.en_passant_target;
}

// uint64_t Board::xray_attacks(uint64_t occ, uint64_t blockers, uint8_t square, uint64_t (Board::*gen_func)(uint8_t)) {
//     auto attacks = (this->*gen_func)(square);
//     blockers &= attacks;
//     return attacks ^ (this->*gen_func)()    
// }

// This function generates all of the moves for a specific type of piece
// TODO: make sure to check that the piece being moved isn't pinned, also finish implementing this
std::vector<std::pair<uint64_t, uint8_t>> Board::defense_maps_for_piece(uint64_t piece_board, uint64_t board_occ, 
    uint64_t (Board::*gen_func)(uint8_t, uint64_t) const) const {
    
    uint64_t p_board = piece_board;

    std::vector<std::pair<uint64_t, uint8_t>> maps;
    int set_bit = __builtin_ffsll(p_board);
    while (set_bit) {
        uint8_t square_from = static_cast<uint8_t>(set_bit - 1);
        uint64_t defense_map = (this->*gen_func)(square_from, board_occ);
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

bool Board::in_check() {
    size_t to_move = static_cast<size_t>(side_to_move);
    size_t other = 1 - to_move;
    bool in_check = (kings[to_move] & attack_maps[other]) != 0;
    return in_check;
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
