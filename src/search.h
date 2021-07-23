#ifndef SEARCH_H
#define SEARCH_H

#include <climits>
#include <vector>

#include "board.h"
#include "evaluate.h"
#include "move.h"

class Search {
   public:
    Search(Board& board);
    Move get_engine_move();

   private:
    int search(int alpha, int beta, int depth);
    int quiesce(int alpha, int beta);
    Board board; // Board& board;
    Evaluate evaluate;
    std::vector<Move> p_var;
};

#endif