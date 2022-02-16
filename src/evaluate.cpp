#include "evaluate.hpp"

#include <iostream>
#include <bit>

/* 
    Move evaluation engine.
    Powered by gokul.
    https://www.chessprogramming.org/Kaissa#Evaluation
    https://www.chessprogramming.org/Evaluation
    https://www.chessprogramming.org/Piece-Square_Tables
*/

// Piece-square table: used to give additional value to pieces based on their position.
// Should be in the range [-100, 100]
short pst_white[6][8][8] = {
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {20, 20, 25, 35, 35, 25, 20, 20},
     {5, 10, 15, 30, 30, 15, 10, 5},
     {0, 5, 15, 30, 30, 15, 5, 0},
     {5, 0, -5, 5, 5, -5, 0, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}},  // pawn

    {{-10, -5, 0, 5, 5, 0, -5, -10},
     {-5, 0, 5, 10, 10, 5, 0, -5},
     {10, 15, 25, 35, 35, 25, 15, -10},
     {5, 5, 10, 30, 30, 10, 5, 5},
     {0, 0, 0, 30, 30, 0, 0, 0},
     {5, 10, 20, 20, 20, 20, 10, 5},
     {-35, -20, -5, 5, 5, -5, -20, -35},
     {-50, -35, -15, -10, -10, -15, -35, -50}},  // knight

    {{-5, 0, 5, 10, 10, 5, 0, -5},
     {0, 5, 10, 20, 20, 10, 5, 0},
     {10, 15, 25, 40, 40, 25, 15, 10},
     {5, 10, 20, 30, 30, 20, 10, 5},
     {5, 10, 15, 25, 25, 15, 10, 5},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {-10, -5, -20, -5, -5, -20, -5, -10}},  // bishop

    {{5, 10, 10, 10, 10, 10, 10, 5},
     {10, 10, 15, 20, 20, 15, 10, 10},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {0, 10, 15, 25, 25, 15, 10, 5},
     {0, 5, 10, 20, 20, 10, 5, 0},
     {0, 0, 10, 15, 15, 10, 0, 0},
     {-5, 0, 0, 10, 10, 0, 0, -5},
     {-10, 0, 0, 15, 15, 10, 0, -10}},  // rook

    {{5, 10, 15, 15, 15, 15, 10, 5},
     {5, 10, 15, 25, 25, 30, 15, 10},
     {5, 20, 25, 30, 30, 35, 20, 5},
     {0, 5, 15, 25, 25, 15, 5, 0},
     {0, 5, 10, 20, 20, 10, 5, 0},
     {-5, 5, 0, 5, 5, 5, 5, -5},
     {-10, -5, 0, 0, 0, 0, -5, -10},
     {-15, -10, -5, -10, -10, -5, -10, -15}},  // queen

    {{5, 10, 15, 30, 30, 15, 10, 5},
     {5, 10, 15, 20, 20, 15, 10, 5},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 10, 20, 30, 30, 20, 10, 5},
     {0, 10, 15, 30, 30, 15, 10, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {10, 10, 0, 0, 0, 0, 10, 10},
     {10, 15, 30, 0, 0, 10, 40, 10}}  // king
};
/*
short pst_black[6][8][8] = {
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 5, 10, 27, 27, 10, 5, 5},
     {0, 0, 0, 25, 25, 0, 0, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}},  // pawn
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 5, 10, 27, 27, 10, 5, 5},
     {0, 0, 0, 25, 25, 0, 0, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}},  // knight
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 5, 10, 27, 27, 10, 5, 5},
     {0, 0, 0, 25, 25, 0, 0, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}},  // bishop
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 5, 10, 27, 27, 10, 5, 5},
     {0, 0, 0, 25, 25, 0, 0, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}},  // rook
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 5, 10, 27, 27, 10, 5, 5},
     {0, 0, 0, 25, 25, 0, 0, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}},  // queen
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {50, 50, 50, 50, 50, 50, 50, 50},
     {10, 10, 20, 30, 30, 20, 10, 10},
     {5, 5, 10, 27, 27, 10, 5, 5},
     {0, 0, 0, 25, 25, 0, 0, 0},
     {5, -5, -10, 0, 0, -10, -5, 5},
     {5, 10, 10, -25, -25, 10, 10, 5},
     {0, 0, 0, 0, 0, 0, 0, 0}}  // king
     
};*/

// Values of each piece pawn, knight, bishop, rook, queen, king.
// Should remain near 100 * piece value with max of ~32,000
static const short piece[6] = {100, 300, 300, 500, 900, 30000};

// Evaluation constructor
Evaluate::Evaluate(Board& start_board, NNUE& nnue) : board(start_board), nnue(nnue) {
    // this->board = start_board;
}

// Adds piece values to pst
void Evaluate::initialize_pst() {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                pst_white[i][j][k] += piece[i];
            }
        }
    }
    return;
}

// A cheap evaluation that just uses piece counts
int Evaluate::evaluate_cheap() const {
    int value = 0;

    // value = board.get_moves().size() * (board.get_side_to_move() ? -1 : 1);
    value += piece_values(board.get_pawns(), piece[0]);
    value += piece_values(board.get_knights(), piece[1]);
    value += piece_values(board.get_bishops(), piece[2]);
    value += piece_values(board.get_rooks(), piece[3]);
    value += piece_values(board.get_queens(), piece[4]);
    value += piece_values(board.get_kings(), piece[5]);

    return value * (board.get_side_to_move() ? -1 : 1);
}

// More expensive evaluation that calculates the score of the position based on the pst
int Evaluate::evaluate() const {
    // return evaluate_cheap();
    int value = 0;

    auto pawns = board.get_pawns();
    auto knights = board.get_knights();
    auto bishops = board.get_bishops();
    auto rooks = board.get_rooks();
    auto queens = board.get_queens();
    auto kings = board.get_kings();

    size_t piece_count = 0;
    piece_count += std::popcount(pawns[0]) + std::popcount(pawns[1]);
    piece_count += std::popcount(knights[0]) + std::popcount(knights[1]);
    piece_count += std::popcount(bishops[0]) + std::popcount(bishops[1]);
    piece_count += std::popcount(rooks[0]) + std::popcount(rooks[1]);
    piece_count += std::popcount(queens[0]) + std::popcount(queens[1]);
    piece_count += std::popcount(kings[0]) + std::popcount(kings[1]);

    value = nnue.evaluate(piece_count, static_cast<Side>(board.get_side_to_move())); //* (board.get_side_to_move() ? -1 : 1);

    return value; //  * (board.get_side_to_move() ? -1 : 1);
}

// Updates the evaluation based on the current move
int Evaluate::move_evaluate(Board board, Move move) const {
    // std::array<uint16_t, 2> origin = move.origin_square_cartesian();
    // std::array<uint16_t, 2> dest = move.destination_square_cartesian();
    int value = -1;
    //int value = pst[][dest[0]][dest[1]] - pst[][origin[0]][origin[1]];

    //if (capture)
    //  value += pst[][dest[0]][dest[1]];

    // TODO: castling and promotion and other heuristics
    return value;
}

int Evaluate::piece_values(std::array<uint64_t, 2> piece_boards, int value) const {
    return value * (__builtin_popcountll(piece_boards[0]) - __builtin_popcountll(piece_boards[1]));
}