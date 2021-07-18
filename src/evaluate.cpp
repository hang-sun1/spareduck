#include "evaluate.h"

/* 
    Move evaluation engine.
    Powered by gokul.
*/

int Evaluate::evaluate(Board board) {
    return -1;
}

int Evaluate::piece_counts(uint64_t piece_board) {
    return __builtin_popcount(piece_board);
}