#include <climits>
#include "search.h"

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

int Search::search(int alpha, int beta, int depth) {
    if (depth == 0) {
        return quiesce(alpha, beta);
    }

    // generate moves
    int move_count = 10;

    int best_eval = INT_MIN;

    for (int i = 0; i < move_count; i++) {
        //make move
        int next_eval = -search(-alpha, -beta, depth - 1);
        //unmakemove

        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
        }
        // return position if better than current max
        if (best_eval >= beta) {
            break;
        }
        // tighten window
        if (best_eval > alpha) {
            alpha = best_eval;
        }
    }

    return best_eval;  // are we looking for move or evaluation
}

int Search::quiesce(int alpha, int beta) {
    //check if king in check
    //if true run search w depth 2??

    int stand_pat;  // = evaluate();
    if (stand_pat > beta) {
        return stand_pat;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    //generate captures
    int move_count = 10;

    for (int i = 0; i < move_count; i++) {
        //makemove
        int next_eval = -quiesce(-beta, -alpha);
        //unmakemove

        if (next_eval >= beta) {
            return next_eval;
        }
        if (next_eval > alpha) {
            alpha = next_eval;
        }
    }

    return alpha;
}