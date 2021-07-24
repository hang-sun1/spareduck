#include "search.h"

#include <iostream>

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

// Search constructor
Search::Search(Board& start_board) {
    board = start_board;
    Evaluate evaluate;
    std::vector<Move> principal_variation;
}

Move Search::get_engine_move() {
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    std::cout << "get_engine_move " << board.get_side_to_move() << std::endl;

    int best_eval = -1000;
    Move best_move = Move(moves[0]);

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp;

        board.make_move(moves[i]);
        int next_eval = -search(-1000, 1000, 3, temp);
        board.unmake_move(moves[i]);

        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            best_move = moves[i];

            temp.push_back(best_move);
            principal_variation = temp;
        }
    }

    std::cout << "PRINCIPAL-VARIATION" << std::endl;
    for (int i = 0; i < principal_variation.size(); i++)
        std::cout << i << "th move " << principal_variation.at(i).origin_square_algebraic() << std::endl;

    return best_move;
}

int Search::search(int alpha, int beta, int depth, std::vector<Move> p_var) {
    if (depth == 0) {
        //return evaluate.static_evaluate_cheap(board);
        return quiesce(alpha, beta, p_var);
    }

    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    // move ordering: hash table and captures first

    int best_eval = INT_MIN;

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp;

        board.make_move(moves[i]);
        int next_eval = -search(-beta, -alpha, depth - 1, p_var);
        board.unmake_move(moves[i]);

        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            // tighten window
            if (best_eval > alpha) {
                alpha = best_eval;
            }

            temp.push_back(moves[i]);
            p_var = temp;
        }
        // return position if better than current max
        if (best_eval >= beta) {
            break;
        }
    }
    return best_eval;
}

int Search::quiesce(int alpha, int beta, std::vector<Move> p_var) {
    p_var.clear();

    if (board.in_check()) {
        return evaluate.static_evaluate_cheap(board);
    }

    int stand_pat = evaluate.static_evaluate_cheap(board);
    if (stand_pat > beta) {
        return stand_pat;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    //generate all moves
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp;

        // skip if move isn't capture
        if (!moves[i].is_capture()) {
            continue;
        }

        board.make_move(moves[i]);
        int next_eval = -quiesce(-beta, -alpha, temp);
        board.unmake_move(moves[i]);

        if (next_eval >= beta) {
            return next_eval;
        }
        if (next_eval > alpha) {
            alpha = next_eval;
        }

        temp.insert(temp.begin(), moves[i]);
        p_var = temp;
    }

    return alpha;
}

std::vector<Move> Search::get_pv() {
    return principal_variation;
}
