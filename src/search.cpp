#include "search.h"

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

int Search::search(int alpha, int beta, int depth)
{
    if (depth == 0)
    {
        //return evaluate();
        //https://www.chessprogramming.org/Quiescence_Search
    }

    // generate moves
    int movecount = 10;

    int bestEval = INT_MIN;

    for (int i = 0; i < movecount; i++)
    {
        //make move
        int nextEval = -search(-alpha, -beta, depth - 1);
        //unmakemove

        // update bestEval
        if (nextEval > bestEval)
        {
            bestEval = nextEval;
        }
        // return position if better than current max
        if (bestEval >= beta)
        {
            break;
        }
        // tighten window
        if (bestEval > alpha)
        {
            alpha = bestEval;
        }
    }

    return bestEval; // are we looking for move or evaluation
}