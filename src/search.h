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
    std::vector<Move> get_pv();

   private:
    Board board;  // Board& board;
    Evaluate evaluate;
    std::vector<Move> principal_variation;
    int search(int alpha, int beta, int depth, std::vector<Move> p_var);
    int quiesce(int alpha, int beta, std::vector<Move> p_var);
};

#endif