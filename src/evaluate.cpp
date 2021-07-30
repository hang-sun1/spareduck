#include "evaluate.h"

#include <iostream>

/* 
    Move evaluation engine.
    Powered by gokul.
    https://www.chessprogramming.org/Kaissa#Evaluation
    https://www.chessprogramming.org/Evaluation
    https://www.chessprogramming.org/Piece-Square_Tables
*/

// Piece-square table: used to give additional value to pieces based on their position.
// Should be in the range [-100, 100]
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

// Values of each piece pawn, knight, bishop, rook, queen, king.
// Should remain near 100 * piece value with max of ~32,000
static const short piece[6] = {100, 300, 300, 500, 900, 30000};

// Evaluation constructor
Evaluate::Evaluate(Board& start_board) : board(start_board) {
    this->board = start_board;
}

// Adds piece values to pst
void Evaluate::initialize_pst() {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                pst[i][j][k] += piece[i];
            }
        }
    }
    return;
}

// A cheap evaluation that just uses piece counts
int Evaluate::evaluate_cheap() {
    int value = 0;

    value += piece_values(board.get_pawns(), piece[0]);
    value += piece_values(board.get_knights(), piece[1]);
    value += piece_values(board.get_bishops(), piece[2]);
    value += piece_values(board.get_rooks(), piece[3]);
    value += piece_values(board.get_queens(), piece[4]);
    value += piece_values(board.get_kings(), piece[5]);

    if (board.is_checkmate()) {
        value = 2e9 * (board.get_side_to_move() ? -1 : 1);
    } else if (board.is_stalemate()) {
        value = 0;
    }
    // whats the best way to generate both side's moves?

    return value * (board.get_side_to_move() ? -1 : 1);
}
int Evaluate::static_evaluate_cheap(Board board) {
    int value = 0;

    value += piece_values(board.get_pawns(), piece[0]);
    value += piece_values(board.get_knights(), piece[1]);
    value += piece_values(board.get_bishops(), piece[2]);
    value += piece_values(board.get_rooks(), piece[3]);
    value += piece_values(board.get_queens(), piece[4]);
    value += piece_values(board.get_kings(), piece[5]);

    // whats the best way to generate both side's moves?

    return value * (board.get_side_to_move() ? -1 : 1);
}

// More expensive evaluation that calculates the score of the position based on the pst
int Evaluate::evaluate() {
    int value = 0;

    // TODO: get all cartesian positions of pieces on the board

    return value * (board.get_side_to_move() ? -1 : 1);
}
int Evaluate::static_evaluate(Board board) {
    int value = 0;

    // TODO: get all cartesian positions of pieces on the board

    return value * (board.get_side_to_move() ? -1 : 1);
}

// Updates the evaluation based on the current move
int Evaluate::move_evaluate(Board board, Move move) const {
    std::array<uint16_t, 2> origin = move.origin_square_cartesian();
    std::array<uint16_t, 2> dest = move.destination_square_cartesian();
    int value = -1;
    //int value = pst[][dest[0]][dest[1]] - pst[][origin[0]][origin[1]];

    //if (capture)
    //  value += pst[][dest[0]][dest[1]];

    // TODO: castling and promotion and other heuristics
    return value;
}

int Evaluate::piece_values(std::array<uint64_t, 2> piece_boards, int value) {
    return value * (__builtin_popcountll(piece_boards[0]) - __builtin_popcountll(piece_boards[1]));
}