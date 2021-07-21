#pragma once

#include "board.h"

/*
const int defaultValues[6][10][10] = {
    {
        {...}
    },
}
*/

class Evaluate {
   public:
    int evaluate(Board board);

   private:
    int piece_values(std::array<uint64_t, 2> piece_boards, int value);
};