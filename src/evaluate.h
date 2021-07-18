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
   public:
    int evaluate(Board board);

   private:
    int piece_counts(uint64_t piece_board);
};

#endif