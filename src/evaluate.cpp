#include "evaluate.h"

/* 
    Move evaluation engine.
    Powered by gokul.
    https://www.chessprogramming.org/Kaissa#Evaluation
    https://www.chessprogramming.org/Evaluation
    https://www.chessprogramming.org/Piece-Square_Tables
*/

// Piece-square table: used to give additional value to pieces based on their position
// should be in the range [-100, 100]
short pst[6][8][8] = {
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
};

// Values of each piece pawn, knight, bishop, rook, queen, king
// should remain near 100 * piece value with max of ~32,000
static const short piece[6] = {100, 300, 300, 500, 900, 30000};

// Evaluation constructor
Evaluate::Evaluate(Board start_board) {
    board = start_board;
}

// Adds piece values to pst
void initialize_pst() {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                pst[i][j][k] += piece[i];
            }
        }
    }
    return;
}

// A cheap static evaluation that just uses piece counts
int Evaluate::static_evaluate_cheap() {
    int value = 0;

    value += piece_values(board.get_pawns(), piece[0]);
    value += piece_values(board.get_knights(), piece[1]);
    value += piece_values(board.get_bishops(), piece[2]);
    value += piece_values(board.get_rooks(), piece[3]);
    value += piece_values(board.get_queens(), piece[4]);

    // whats the best way to generate both side's moves?

    return value * (board.get_side_to_move() ? -1 : 1);
}

// More expensive evaluation that calculates the score of the position based on the pst
int Evaluate::static_evaluate() {
    int value = 0;

    value += piece_values(board.get_pawns(), piece[0]);
    value += piece_values(board.get_knights(), piece[1]);
    value += piece_values(board.get_bishops(), piece[2]);
    value += piece_values(board.get_rooks(), piece[3]);
    value += piece_values(board.get_queens(), piece[4]);

    // whats the best way to generate both side's moves?

    return value * (board.get_side_to_move() ? -1 : 1);
}

// Updates the evaluation based on the current move
int Evaluate::move_evaluate(Move move) {
    std::array<uint16_t, 2> origin = move.origin_square_cartesian();
    std::array<uint16_t, 2> destination = move.destination_square_cartesian();
    return -1;
}

int piece_values(std::array<uint64_t, 2> piece_boards, int value) {
    return value * (__builtin_popcount(piece_boards[0]) - __builtin_popcount(piece_boards[1]));
}