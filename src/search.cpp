#include "search.h"

#include <iostream>

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

// Search constructor
Search::Search(Board &start_board) : board(start_board), evaluate(Evaluate(start_board)) {
    Table t_table;
    std::vector<Move> principal_variation;
    principal_variation.reserve(10);
}

Move Search::get_engine_move() {
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    std::cout << "get_engine_move " << (board.get_side_to_move() ? "black" : "white") << std::endl;

    int best_eval = -10000;
    Move best_move = Move(moves[0]);

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp_pv;

        board.make_move(moves[i]);
        int next_eval = -search(-1000, 1000, 4, temp_pv);
        board.unmake_move(moves[i]);
        std::cout << "next_eval " << next_eval << std::endl;
        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            best_move = moves[i];

            // copy temporary variation onto principal variation with new best move
            principal_variation.clear();
            principal_variation.push_back(best_move);
            for (int j = 0; j < temp_pv.size(); j++) {
                principal_variation.push_back(temp_pv[j]);
            }
        }
    }

    t_table.clear();

    std::cout << "PRINCIPAL-VARIATION" << std::endl;
    for (int i = 0; i < principal_variation.size(); i++)
        std::cout << i << "th move " << principal_variation.at(i) << std::endl;

    return best_move;
}

int Search::search(int alpha, int beta, int depth, std::vector<Move> &pv) {
    if (depth <= 0) {
        //int curr_eval = evaluate.evaluate_cheap();
        int curr_eval = quiesce(alpha, beta, pv, 0);

        NodeType type;  // Should this always be EXACT??
        if (curr_eval <= alpha) {
            type = NodeType::UPPER;
        } else if (curr_eval >= beta) {
            type = NodeType::LOWER;
        } else {
            type = NodeType::EXACT;
        }
        if (pv.size())
            t_table.put(board, pv.front(), curr_eval, type, static_cast<uint8_t>(depth));
        //pv.clear();  // REMOVE WHEN USING QUIESCE;
        return curr_eval;
    }

    /* futility pruning
    bool is_futile = false;
    if (depth == 1) {
        if (!(pv.front().is_capture() || board.in_check())) {
            int curr_eval = evaluate.evaluate_cheap();
            const int MINOR_VAL = 300;
            if (curr_eval + MINOR_VAL < alpha) {
                is_futile = true;
            }
        }
    }*/

    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();
    std::vector<Move> temp_pv;
    int best_eval = INT_MIN;

    // Check transposition table for current position.
    std::optional<TableEntry> t_position;  // = t_table.get(*board);

    if (t_position.has_value()) {
        if (t_position->get_depth() >= depth) {
            switch (t_position->get_type()) {
                case NodeType::UPPER:
                    if (t_position->get_eval() < beta)
                        beta = t_position->get_eval();
                    break;
                case NodeType::LOWER:
                    if (t_position->get_eval() > alpha)
                        alpha = t_position->get_eval();
                    break;
                default:
                    return t_position->get_eval();
            }

            // return if alpha > beta?
        }

        // move ordering: transposition table first
        for (int i = 0; i < move_count; i++) {
            if (moves[i] == t_position->get_move()) {  // TODO: check if move valid?
                moves[i] = moves[0];
                moves[0] = t_position->get_move();
                break;
            }
        }
    }

    // move ordering:  captures first
    int j = 1;  // move swap counter
    for (int i = 1; i < move_count; i++) {
        if (moves[i].is_capture()) {
            Move temp_move = moves[j];
            moves[j] = moves[i];
            moves[i] = temp_move;
            j++;
        }
    }

    for (int i = 0; i < move_count; i++) {
        board.make_move(moves[i]);
        int next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
        board.unmake_move(moves[i]);

        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            // tighten window
            if (best_eval > alpha) {
                alpha = best_eval;
            }

            // Add new move to front of p_var and copy temp onto p_var
            pv.clear();
            pv.push_back(moves[i]);
            for (int j = 0; j < temp_pv.size(); j++) {
                pv.push_back(temp_pv[j]);
            }
        }
        // return position if better than current max
        if (best_eval >= beta) {
            break;
        }
    }

    // Add new best move to hash table
    NodeType type;
    if (best_eval <= alpha) {
        type = NodeType::UPPER;
    } else if (best_eval >= beta) {
        type = NodeType::LOWER;
    } else {
        type = NodeType::EXACT;
    }
    t_table.put(board, pv.front(), best_eval, type, static_cast<uint8_t>(depth));

    return best_eval;
}

int Search::quiesce(int alpha, int beta, std::vector<Move> &pv, int depth) {
    pv.clear();
    //std::cout << "qsearch depth: " << depth << std::endl;
    //std::cout << "alpha and beta: " << alpha << " " << beta << std::endl;

    // TODO: handle loud positions
    if (board.in_check()) {
        return evaluate.evaluate_cheap();
        //std::vector<Move> temp_pv;
        //return -search(alpha, beta, 1, temp_pv);
    }

    int stand_pat = evaluate.evaluate_cheap();
    if (stand_pat >= beta) {
        return beta;
    }
    // Delta pruning - TODO: handle promotion
    const int DELTA = 900;  // queen / max material swing value
    if (stand_pat + DELTA < alpha) {
        return alpha;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    //generate all moves
    std::vector<Move> moves = board.get_moves();
    //std::vector<Move> captures = sort_captures(moves);
    int move_count = moves.size();
    std::vector<Move> temp_pv;

    for (int i = 0; i < move_count; i++) {
        // skip if move isn't capture
        if (moves[i].is_capture()) {
            //std::cout << "capture: " << moves[i] << std::endl;
            board.make_move(moves[i]);
            int next_eval = -quiesce(-beta, -alpha, temp_pv, depth + 1);
            board.unmake_move(moves[i]);

            if (next_eval >= beta) {
                // Add new move to front of p_var and copy temp onto p_var
                pv.clear();
                pv.push_back(moves[i]);
                for (int j = 0; j < temp_pv.size(); j++) {
                    pv.push_back(temp_pv[j]);
                }
                return beta;
            }
            if (next_eval > alpha) {
                alpha = next_eval;
            }
        }
    }

    return alpha;
}

// bucket sorting captures using Most Valuable Victim - Least Valuable Aggressor
std::vector<Move> Search::sort_captures(std::vector<Move> moves) {
    // 400-800
    std::vector<Move> high;
    // 200-300
    std::vector<Move> mid;
    // 0 - 100
    std::vector<Move> low;
    // -100 - -300
    std::vector<Move> lower;
    // < -300
    std::vector<Move> lowest;

    for (int i = 0; i < moves.size(); i++) {
        if (moves[i].is_capture()) {
            // TODO: switch statement for pieces
            high.push_back(moves[i]);
        }
    }

    std::vector<Move> captures;
    captures.reserve(high.size() + mid.size() + low.size() + lower.size() + lowest.size());
    captures.insert(captures.end(), high.begin(), high.end());
    captures.insert(captures.end(), mid.begin(), mid.end());
    captures.insert(captures.end(), low.begin(), low.end());
    captures.insert(captures.end(), lower.begin(), lower.end());
    captures.insert(captures.end(), lowest.begin(), lowest.end());

    return captures;
}

std::vector<Move> Search::get_principal_variation() {
    return principal_variation;
    //return t_table.get_variation(*board);
}
