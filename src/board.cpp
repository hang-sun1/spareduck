#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <stack>
#include <vector>
#include <assert.h>

#include "history.hpp"
#include "move.hpp"
#include "board.hpp"
#include "piece.hpp"

#include "../magic-bits/include/magic_bits.hpp"

using std::uint64_t;

Board::Board(magic_bits::Attacks* att) {
    this->attacks = att;
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
    // this->attack_maps[1] = defense_maps[1] & (~all_per_side[1]);
    // this->attack_maps[0] = defense_maps[0] & (~all_per_side[0]);
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    std::vector<Move> empty;
    this->moves = empty;
    this->made_moves = empty;
    this->moved_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->taken_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->hash_helper = Board::initialize_hash();
    this->hash = initial_hash();
    this->moves = this->generate_moves();
    // there is no en passant target yet, so just set it to some square off the board
    this->en_passant_target = 65;
    piece_map_vec.reserve(50);
}

Board::Board(std::string fen) {
    this->in_between = Board::generate_in_between();
    this->history = std::stack<History>();
    piece_map_vec.reserve(50);
    parse_fen(fen);
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    this->all_per_side[0] = pawns[0] | rooks[0] | knights[0] | bishops[0] | queens[0] | kings[0];
    this->all_per_side[1] = pawns[1] | rooks[1] | knights[1] | bishops[1] | queens[1] | kings[1];
    pawn_defends = { 0, 0 };
    rook_defends = { 0, 0};
    knight_defends = { 0, 0 };
    bishop_defends = { 0, 0 };
    king_defends = { 0, 0 };
    std::array<std::array<uint64_t, 2>*, 6> arr = { &pawns, &rooks, &knights, &bishops, &queens, &kings };
    std::array<std::array<uint64_t, 2>*, 6> def_maps = { &pawn_defends, &rook_defends, &knight_defends, 
        &bishop_defends, &queen_defends, &king_defends };
    std::array<char, 6> gen_funcs = { 'P', 'r', 'n', 'b', 'q', 'k' };

    auto current_move = static_cast<size_t>(side_to_move);
    auto other_move = 1 - current_move;
    // side_to_move = Side::BLACK;
    for (int i = 0; i < 6; ++i) {
        piece_map_vec.clear();
        this->defense_maps_for_piece((*arr[i])[other_move], all_per_side[current_move] | all_per_side[other_move],
                gen_funcs[i], static_cast<Side>(other_move), piece_map_vec);
        (*def_maps[i])[other_move] = 0;
        for (auto &map: piece_map_vec) {
            (*def_maps[i])[other_move] |= map.first;
        }
    }
    // side_to_move = Side::WHITE;
    for (int i = 0; i < 6; ++i) {
        piece_map_vec.clear();
        this->defense_maps_for_piece((*arr[i])[current_move], all_per_side[current_move] | all_per_side[other_move],
                gen_funcs[i], side_to_move, piece_map_vec);
        (*def_maps[i])[current_move] = 0;
        for (auto &map: piece_map_vec) {
            (*def_maps[i])[current_move] |= map.first;
        }
    }

    this->defense_maps[1] = pawn_defends[1] | rook_defends[1] | knight_defends[1] | bishop_defends[1] | queen_defends[1] | king_defends[1];
    this->defense_maps[0] = pawn_defends[0] | rook_defends[0] | knight_defends[0] | bishop_defends[0] | queen_defends[0] | king_defends[0];
    // this->attack_maps[1] = this->defense_maps[1] & (~all_per_side[1]);
    // this->attack_maps[0] = this->defense_maps[0] & (~all_per_side[0]);
    this->king_lookup = Board::generate_king_lookup();
    this->knight_lookup = Board::generate_knight_lookup();
    this->moved_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->taken_piece_boards = std::vector<std::array<uint64_t, 2> *>();
    this->hash_helper = Board::initialize_hash();
    this->hash = Board::initial_hash();
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
uint64_t Board::generate_pawn_moves(uint8_t square, uint64_t board_occ, Side side) const {
    uint64_t one_piece_board = 1ULL << square;
    uint64_t moves = 0;
    uint64_t all_pieces = board_occ;
    uint64_t second_rank = 0xff00;
    uint64_t seventh_rank = 0xff000000000000;

    if (side == Side::WHITE) {
        uint64_t one_square = one_piece_board << 8;
        if (!(one_square & all_pieces)) {
            moves |= one_square;
            uint64_t two_square = one_piece_board << 16;
            if (!(two_square & all_pieces) && (one_piece_board & second_rank)) {
                moves |= two_square;
            }
        }
    } else if (side == Side::BLACK) {
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

uint64_t Board::generate_pawn_attacks(uint8_t square, uint64_t board_occ, Side side) const {
    uint64_t a_file = 0x0101010101010101;
    uint64_t h_file = 0x8080808080808080;
    uint64_t one_piece_board = 1ULL << square;
    uint64_t moves = 0;
    uint8_t file = square & 7;
    // uint64_t all_pieces = board_occ;
    if (side == Side::WHITE) {
        uint64_t potential_moves = (one_piece_board << 7) | (one_piece_board << 9);
        if (file == 0) {
            potential_moves &= ~h_file;
        } else if (file == 7) {
            potential_moves &= ~a_file;
        }
        moves |= potential_moves;
    } else if (side == Side::BLACK) {
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

// return a bitboard containing legal moves for a rook on
// the provided square
uint64_t Board::generate_rook_moves(uint8_t square, uint64_t board_occ) const {
    if (square >= 64) {
        return 0;
    }
    assert(square < 64);
    uint64_t rook_attacks = attacks->Rook(board_occ, square);
    return rook_attacks &= ~(1ULL << square);
}

uint64_t Board::generate_bishop_moves(uint8_t square, uint64_t board_occ) const {
    if (square >= 64) {
        return 0;
    }
    assert(square < 64);
    uint64_t bishop_attacks = attacks->Bishop(board_occ, square);
    return bishop_attacks &= ~(1ULL << square);
}

uint64_t Board::generate_queen_moves(uint8_t square, uint64_t board_occ) const {
    if (square >= 64) {
        return 0;
    }
    assert(square < 64);
    uint64_t queen_attacks = (unsigned long long) attacks->Queen((uint64_t) board_occ, (int) square);
    // std::cout << queen_attacks << ", " << (int)square << std::endl;
    return queen_attacks;// &= ~(1ULL << square);
}

uint64_t Board::generate_knight_moves(uint8_t square, uint64_t board_occ) const {
    if (square >= 64) {
        return 0;
    }
    assert(square < 64);
    auto defended_squares = knight_lookup[square];
    return defended_squares;
}

// returns king moves, but does not exclude moves that wander into check
uint64_t Board::generate_king_moves(uint8_t square, uint64_t board_occ) const {
    assert(square < 64);
    auto defended_squares = king_lookup[square];
    return defended_squares;
}

std::array<std::array<uint64_t, 64>, 13> Board::initialize_hash() {
    std::random_device rd;
    std::mt19937_64 e2(rd());

    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));

    std::array<std::array<uint64_t, 64>, 13> table;
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 13; ++j) {
            uint64_t random_num = dist(e2);
            table[j][i] = random_num;
        }
    }

    return table;
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

std::array<uint64_t, 64> Board::generate_antidiagonal_mask_map(){
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
    vec_of_moves.reserve(50);

    auto board_occ = all_per_side[0] | all_per_side[1];

    std::array<char, 6> gen_funcs = {'b', 'n', 'r', 'q', 'k'};
    std::array<uint64_t, 6> arr = {bishops[side], knights[side], rooks[side], queens[side],
                                   kings[side], pawns[side]};

    piece_map_vec.clear();
    defense_maps_for_piece(pawns[side], board_occ, 'P', side_to_move, piece_map_vec);
    for (auto &m: piece_map_vec) {
        m.first &= all_per_side[other_side];
        m.first &= ~all_per_side[side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while (dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
            MoveType type = MoveType::QUIET;
            if ((1ULL << dest) & all_per_side[other_side]) {
                type = MoveType::CAPTURE;
            }
            if (((1ULL << dest) & 0xff00000000000000) || ((1ULL << dest) & 0xff)) {
                Move promote_to_bishop = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP);
                Move promote_to_knight = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT);
                Move promote_to_queen = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN);
                Move promote_to_rook = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_ROOK);
                if (type != MoveType::CAPTURE) {
                    Move promote_to_bishop = Move(from, dest, MoveType::PROMOTE_TO_BISHOP);
                    Move promote_to_knight = Move(from, dest, MoveType::PROMOTE_TO_KNIGHT);
                    Move promote_to_queen = Move(from, dest, MoveType::PROMOTE_TO_QUEEN);
                    Move promote_to_rook = Move(from, dest, MoveType::PROMOTE_TO_ROOK);
                }
                vec_of_moves.push_back(promote_to_queen);
                vec_of_moves.push_back(promote_to_rook);
                vec_of_moves.push_back(promote_to_bishop);
                vec_of_moves.push_back(promote_to_knight);
            } else {
                vec_of_moves.push_back(Move(from, dest, type));
            }
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }
    
    piece_map_vec.clear();
    defense_maps_for_piece(pawns[side], board_occ, 'p', side_to_move, piece_map_vec);
    for (auto &m: piece_map_vec) {
        m.first &= ~all_per_side[side];
        auto map = m.first;
        auto from = m.second;
        int dest_plus_one = __builtin_ffsll(map);
        while (dest_plus_one) {
            auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
            MoveType type = MoveType::QUIET;
            if ((1ULL << dest) & all_per_side[other_side]) {
                type = MoveType::CAPTURE;
            }
            if (((1ULL << dest) & 0xff00000000000000) || ((1ULL << dest) & 0xff)) {
                Move promote_to_bishop = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP);
                Move promote_to_knight = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT);
                Move promote_to_queen = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN);
                Move promote_to_rook = Move(from, dest, MoveType::CAPTURE_AND_PROMOTE_TO_ROOK);
                if (type != MoveType::CAPTURE) {
                    Move promote_to_bishop = Move(from, dest, MoveType::PROMOTE_TO_BISHOP);
                    Move promote_to_knight = Move(from, dest, MoveType::PROMOTE_TO_KNIGHT);
                    Move promote_to_queen = Move(from, dest, MoveType::PROMOTE_TO_QUEEN);
                    Move promote_to_rook = Move(from, dest, MoveType::PROMOTE_TO_ROOK);
                }
                vec_of_moves.push_back(promote_to_queen);
                vec_of_moves.push_back(promote_to_rook);
                vec_of_moves.push_back(promote_to_bishop);
                vec_of_moves.push_back(promote_to_knight);
            } else {
                vec_of_moves.push_back(Move(from, dest, type));
            }
            map &= ~(1ULL << dest);
            dest_plus_one = __builtin_ffsll(map);
        }
    }

    for (int i = 0; i < 5; ++i) {
        piece_map_vec.clear();
        defense_maps_for_piece(arr[i], board_occ, gen_funcs[i], side_to_move, piece_map_vec);
        for (auto &m : piece_map_vec) {
            m.first &= ~all_per_side[side];
            auto map = m.first;
            auto from = m.second;
            int dest_plus_one = __builtin_ffsll(map);
            while (dest_plus_one) {
                auto dest = static_cast<uint8_t>(dest_plus_one) - 1;
                MoveType type = MoveType::QUIET;
                if ((1ULL << dest) & all_per_side[other_side]) {
                    type = MoveType::CAPTURE;
                }
                vec_of_moves.push_back(Move(from, dest, type));
                map &= ~(1ULL << dest);
                dest_plus_one = __builtin_ffsll(map);
            }
        }
    }


    // if (short_castle_rights[side]) {
    //     uint64_t relevant_squares = side ? 0x7000000000000000ULL : 0x70ULL;
    //     uint64_t to_be_vacated = side ? 0x6000000000000000ULL : 0x60ULL;
    //     auto total_occ = all_per_side[side] | all_per_side[other_side];
    //     if (!(defense_maps[other_side] & relevant_squares) && (total_occ & to_be_vacated) == 0) {
    //         if (!side) {
    //             vec_of_moves.push_back(Move(4, 6, MoveType::SHORT_CASTLES));
    //         } else {
    //             vec_of_moves.push_back(Move(60, 62, MoveType::SHORT_CASTLES));
    //         }
    //     }
    // }
    // if (long_castle_rights[side]) {
    //     uint64_t relevant_squares = side ? 0x1c00000000000000ULL : 0x1cULL;
    //     uint64_t to_be_vacated = side ? 0xe00000000000000ULL : 0xeULL;
    //     auto total_occ = all_per_side[side] | all_per_side[other_side];
    //     if (!(defense_maps[other_side] & relevant_squares) && (total_occ & to_be_vacated) == 0) {
    //         // vec_of_moves.push_back(Move(0, 0, MoveType::LONG_CASTLES));
    //         if (!side) {
    //             vec_of_moves.push_back(Move(4, 2, MoveType::LONG_CASTLES));
    //         } else {
    //             vec_of_moves.push_back(Move(60, 58, MoveType::LONG_CASTLES));
    //         }
    //     }
    // }

    // TODO add castles and pawn stuff (including en passant)

    return vec_of_moves;
}

std::array<std::optional<Piece>, 2> Board::make_move(Move move) {
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
    history.hash = hash;
    this->history.push(history);
    size_t current_move = static_cast<size_t>(side_to_move);
    size_t other_move = 1 - current_move;

    uint64_t move_bitboard = 0;
    if (move.type() == MoveType::DOUBLE_PAWN_PUSH) {
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
    std::array<Piece, 6> piece_idents = { BISHOP, KNIGHT, ROOK, QUEEN, KING, PAWN };

    if (move.type() == MoveType::SHORT_CASTLES) {
        if (side_to_move == Side::WHITE) {
            kings[current_move] = 0x40;
            rooks[current_move] &= ~0x80;
            rooks[current_move] |= 0x20;
            hash ^= hash_helper[2*KING][4];
            hash ^= hash_helper[2*KING][6];
            hash ^= hash_helper[2*ROOK][7];
            hash ^= hash_helper[2*ROOK][5];
            move_bitboard = 0xf0;
        } else {
            kings[current_move] = 0x4000000000000000;
            rooks[current_move] &= ~0x8000000000000000;
            rooks[current_move] |= 0x2000000000000000;
            hash ^= hash_helper[2*KING+1][60];
            hash ^= hash_helper[2*KING+1][62];
            hash ^= hash_helper[2*ROOK+1][63];
            hash ^= hash_helper[2*ROOK+1][61];
            move_bitboard = 0xf000000000000000;
        }
        moved = 4;
        short_castle_rights[current_move] = false;
    } else if (move.type() == MoveType::LONG_CASTLES) {
        if (side_to_move == Side::WHITE) {
            kings[current_move] = 0x4;
            rooks[current_move] &= ~0x1;
            rooks[current_move] |= 0x8;
            hash ^= hash_helper[2*KING][4];
            hash ^= hash_helper[2*KING][2];
            hash ^= hash_helper[2*ROOK][0];
            hash ^= hash_helper[2*ROOK][3];
            move_bitboard = 0x1f;
        } else {
            kings[current_move] = 0x400000000000000;
            rooks[current_move] &= ~0x100000000000000;
            rooks[current_move] |= 0x800000000000000;
            hash ^= hash_helper[2*KING+1][60];
            hash ^= hash_helper[2*KING+1][58];
            hash ^= hash_helper[2*ROOK+1][56];
            hash ^= hash_helper[2*ROOK+1][59];
            move_bitboard = 0x1f00000000000000;
        }
        moved = 4;
        long_castle_rights[current_move] = false;
    } else {
        auto from = move.origin_square();
        auto to = move.destination_square();
        move_bitboard = (1ULL << from) | (1ULL << to);

        if (short_castle_rights[current_move]) {
            if (side_to_move == Side::WHITE) {
                if (from == 4 || from == 7) {
                    short_castle_rights[current_move] = false;
                    hash ^= hash_helper[13][1];
                }
            } else {
                if (from == 63 || from == 60) {
                    short_castle_rights[current_move] = false;
                    hash ^= hash_helper[13][2];
                }
            }
        }

        if (long_castle_rights[current_move]) {
            if (side_to_move == Side::WHITE) {
                if (from == 4 || from == 0) {
                    long_castle_rights[current_move] = false;
                    hash ^= hash_helper[13][3];
                }
            } else {
                if (from == 60 || from == 56) {
                    long_castle_rights[current_move] = false;
                    hash ^= hash_helper[13][4];
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
                hash ^= hash_helper[2*piece_idents[i]+current_move][from];
                hash ^= hash_helper[2*piece_idents[i]+current_move][to];
                moved = i;
                // moved.push_back(i);
            }
            if (((*piece_arr_ptr)[other_move] & (1ULL << to))) {
                (*piece_arr_ptr)[other_move] &= ~(1ULL << to);
                hash ^= hash_helper[2*piece_idents[i]+other_move][to];
                captured = i;
                // captured.push_back(i);
                // capture = true;
            }
        }
        
        if (move.is_promotion()) {
            if (move.type() == MoveType::PROMOTE_TO_QUEEN || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN) {
                pawns[current_move] &= ~(1ULL << move.origin_square());
                queens[current_move] |= (1ULL << move.destination_square());
                hash ^= hash_helper[2*QUEEN+current_move][to];
                hash ^= hash_helper[2*PAWN+current_move][to];
            } else if (move.type() == MoveType::PROMOTE_TO_ROOK || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_ROOK) {
                pawns[current_move] &= ~(1ULL << move.origin_square());
                rooks[current_move] |= (1ULL << move.destination_square());
                hash ^= hash_helper[2*ROOK+current_move][to];
                hash ^= hash_helper[2*PAWN+current_move][to];
            } else if (move.type() == MoveType::PROMOTE_TO_KNIGHT || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT) {
                pawns[current_move] &= ~(1ULL << move.origin_square());
                knights[current_move] |= (1ULL << move.destination_square());
                hash ^= hash_helper[2*KNIGHT+current_move][to];
                hash ^= hash_helper[2*PAWN+current_move][to];
            } else if (move.type() == MoveType::PROMOTE_TO_BISHOP || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP) {
                pawns[current_move] &= ~(1ULL << move.origin_square());
                bishops[current_move] |= (1ULL << move.destination_square());
                hash ^= hash_helper[2*BISHOP+current_move][to];
                hash ^= hash_helper[2*PAWN+current_move][to];
            }
        }

        
    }

    // attack_maps[current_move] = 0;
    all_per_side[current_move] = 0;
    all_per_side[other_move] = 0;
    all_per_side[current_move] = rooks[current_move] | bishops[current_move] | knights[current_move] |
                                 queens[current_move] | kings[current_move] | pawns[current_move];
    all_per_side[other_move] = rooks[other_move] | bishops[other_move] | knights[other_move] |
                               queens[other_move] | kings[other_move] | pawns[other_move];

    
    this->side_to_move = static_cast<Side>(other_move);
    //  TODO: MOVE THIS UNTIL MOVE LEGALITY IS VERIFIED TO SAVE TIME ON MOVEGEN
    this->moves = generate_moves();

    auto cap = captured >= 0 ? std::make_optional(piece_idents[captured]) : std::nullopt;

    // if (captured >= 0) {
    //     assert(piece_idents[captured] != KING);
    // }
    assert(moved >= 0); 
    std::array<std::optional<Piece>, 2> pieces_involved = { std::make_optional(piece_idents[moved]), cap };
    return pieces_involved;
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
    // this->attack_maps = pop.attack_maps;
    this->long_castle_rights = pop.long_castle_rights;
    this->short_castle_rights = pop.short_castle_rights;
    this->moves = pop.moves;
    this->en_passant_target = pop.en_passant_target;
    this->hash = pop.hash;
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
                                    char move_type, Side other_side) const {
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
                defense_map = generate_pawn_attacks(square_from, dest_board, other_side);
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

bool Board::is_pos_valid() {
    size_t current_move = static_cast<size_t>(side_to_move);
    size_t other_move = 1 - current_move;
    // check whether or not the king is under attack

    uint64_t board_occ = all_per_side[0] | all_per_side[1];
    uint8_t king_pos = __builtin_ffsll(kings[other_move]) - 1;
    
    if (knight_lookup[king_pos] & knights[current_move]) {
        return false;
    } else if (king_lookup[king_pos] & kings[current_move]) {
        return false;
    }

    uint64_t queen = attacks->Queen(board_occ, king_pos);
    if (queen & queens[current_move]) {
        return false;
    }
    uint64_t bishop = attacks->Bishop(board_occ, king_pos);
    if (bishop & bishops[current_move]) {
        return false;
    }
    uint64_t rook = attacks->Rook(board_occ, king_pos);
    if (rook & rooks[current_move]) {
        return false;
    }

    uint64_t pawn = generate_pawn_attacks(king_pos, 0xffffffffffffffff, static_cast<Side>(other_move));
    if (pawn & pawns[current_move]) {
        return false;
    }
    return true;
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
    i++;
    side_to_move = fen[i] == 'w' ? Side::WHITE : Side::BLACK;
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
void Board::defense_maps_for_piece(uint64_t piece_board, uint64_t board_occ, char move_type, Side side,
    std::vector<std::pair<uint64_t, uint8_t>> &maps_vec) const {
    uint64_t p_board = piece_board;
    if (piece_board == 0) {
        return;
        // std::cout << move_type << std::endl;
    }
    assert(piece_board != 0);

    // std::vector<std::pair<uint64_t, uint8_t>> maps;
    int set_bit = __builtin_ffsll(p_board);
    while (set_bit) {
        uint8_t square_from = static_cast<uint8_t>(set_bit - 1);
        assert(square_from < 64);
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
                defense_map = generate_pawn_moves(square_from, board_occ, side);
                break;
            case 'P':
                defense_map = generate_pawn_attacks(square_from, board_occ, side);
                break;
        }
        maps_vec.push_back(std::make_pair(defense_map, square_from));

        p_board &= ~(1ULL << square_from);
        set_bit = __builtin_ffsll(p_board);
    }
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

std::vector<uint8_t> Board::get_piece_pos(Piece piece_type, Side s) const {
    std::vector<uint8_t> pos;
    size_t side = static_cast<size_t>(s); 
    auto piece_board = pawns[side];
    switch(piece_type) {
        case PAWN:
            piece_board = pawns[side];
            break;
        case ROOK:
            piece_board = rooks[side];
            break;
        case BISHOP:
            piece_board = bishops[side];
            break;
        case QUEEN:
            piece_board = queens[side];
            break;
        case KING:
            piece_board = kings[side];
            break;
        case KNIGHT:
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



uint64_t Board::initial_hash() const {
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
        int j = -1;
        uint64_t piece_board = 1ULL << i;
        if ((wp & piece_board)) {
            j = PAWN;
        } else if ((bp & piece_board)) {
            j = PAWN;
        } else if ((wq & piece_board)) {
            j = QUEEN;
        } else if ((bq & piece_board)) {
            j = QUEEN;
        } else if ((wr & piece_board)) {
            j = ROOK;
        } else if ((br & piece_board)) {
            j = ROOK;
        } else if ((wb & piece_board)) {
            j = BISHOP;
        } else if ((bb & piece_board)) {
            j = BISHOP;
        } else if ((wk & piece_board)) {
            j = KING;
        } else if ((bk & piece_board)) {
            j = KING;
        } else if ((wn & piece_board)) {
            j = KNIGHT;
        } else if ((bn & piece_board)) {
            j = KNIGHT;
        }

        if (j > 0) {
            j = 2*j + static_cast<size_t>(side_to_move);
            hash = hash ^ hash_helper[j][i];
        }
    }

    // of the 13th row, the first column is for the side to move, the second and third
    // column represent kingside castling rights of white and black respectively
    // the fourth and fifth represent queenside castling rights for white and black respectively
    // (they don't need to be set rn, but rather just applied when the states they represent change)

    return hash;
}

uint64_t Board::get_hash() const {
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
    return !in_check() && moves.size() == 0;
}

Piece Board::piece_on_square(uint8_t square, Side s) {
    auto side_index = static_cast<size_t>(s);
    uint64_t piece_board = 1ULL << square;
    Piece pieces[6] = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
    uint64_t boards[6] = { pawns[side_index], knights[side_index], bishops[side_index],  rooks[side_index], queens[side_index], kings[side_index] } ;
    for (int i = 0; i < 6; ++i) {
        if ((piece_board & boards[i])) {
            return pieces[i];
        }
    } 
    // it should never reach here
    assert(true);
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
        std::cout << moves.size() << std::endl;
        this->make_move(moves[i]);
        if (!is_pos_valid()) {
            this->unmake_move(moves[i]);
            continue;
        }
        unmake_move(moves[i]);
        l_moves.at(j++) = moves[i].origin_square_algebraic();
        l_moves.at(j++) = moves[i].destination_square_algebraic();
    }
    return l_moves;
}
