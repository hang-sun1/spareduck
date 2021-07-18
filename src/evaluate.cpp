#include "evaluate.h"

/* 
    Move evaluation engine.
    Powered by gokul.
*/

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