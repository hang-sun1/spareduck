#include "search.h"

#include <climits>

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

// Search constructor
Search::Search(Board start_board) {
    board = start_board;
    Evaluate evaluate;
}

Move Search::get_engine_move() {
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    int best_eval = -1000;
    Move best_move = Move(moves[0]);

    for (int i = 0; i < move_count; i++) {
        board.make_move(moves[i]);
        int next_eval = -search(-1000, 1000, 3);
        board.unmake_move(moves[i]);

        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            best_move = moves[i];
        }
    }

    return best_move;
}

int Search::search(int alpha, int beta, int depth) {
    if (depth == 0) {
        return evaluate.static_evaluate_cheap(board);
        //return quiesce(alpha, beta);
    }

    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    int best_eval = INT_MIN;

    for (int i = 0; i < move_count; i++) {
        board.make_move(moves[i]);
        int next_eval = -search(-alpha, -beta, depth - 1);
        board.unmake_move(moves[i]);

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

    return best_eval;
}

int Search::quiesce(int alpha, int beta) {
    // if (board.in_check()){...}
    //if true run search w depth 2??

    int stand_pat = 0;  // = evaluate();
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