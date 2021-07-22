#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
#include "evaluate.h"

class Search {
   public:
    Search(Board board);
    Move get_engine_move();

   private:
    int search(int alpha, int beta, int depth);
    int quiesce(int alpha, int beta);
    Board board;
    Evaluate evaluate;
};

#endif