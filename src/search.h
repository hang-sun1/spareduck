#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "evaluate.h"

class Search {
    //public:
    //abstract the search?

   private:
    int search(int alpha, int beta, int depth);
    int quiesce(int alpha, int beta);
};

#endif