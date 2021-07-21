#include "evaluate.h"

/* 
    Move evaluation engine.
    Powered by gokul.
    https://www.chessprogramming.org/Kaissa#Evaluation
    https://www.chessprogramming.org/Evaluation
    https://www.chessprogramming.org/Piece-Square_Tables
*/

static const short pst[6][8][8] = {
    {{0,  0, 0, 0, 0, 0, 0, 0},
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

int Evaluate::evaluate(Board board) {
    int value = 0;

    value += piece_values(board.get_knights(), 300);
    value += piece_values(board.get_bishops(), 300);
    value += piece_values(board.get_rooks(), 500);
    value += piece_values(board.get_pawns(), 100);
    value += piece_values(board.get_queens(), 900);

    // whats the best way to generate both side's moves?

    return value * (board.get_side_to_move() ? -1 : 1);
}

int piece_values(std::array<uint64_t, 2> piece_boards, int value) {
    return value * (__builtin_popcount(piece_boards[0]) - __builtin_popcount(piece_boards[1]));
}