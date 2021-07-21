#pragma once

#include "board.h"
#include "move.h"

/*
const int defaultValues[6][10][10] = {
    {
        {...}
    },
}
*/

class Evaluate {
   public:
    Evaluate(Board board);
    int move_evaluate(Move move);
    int static_evaluate();
    int static_evaluate_cheap();

   private:
    Board board;
    void initialize_pst();
    int piece_values(std::array<uint64_t, 2> piece_boards, int value);
};