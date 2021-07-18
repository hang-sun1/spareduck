#ifndef EVAL_H
#define EVAL_H

#include "board.h"

/*
const int defaultValues[6][10][10] = {
    {
        {...}
    },
}
*/

class Evaluate {
   private:
    int piece_values(std::array<uint64_t, 2> piece_boards, int value);

   public:
    int evaluate(Board board);
};

#endif