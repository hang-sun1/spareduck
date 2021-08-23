#include "search.hpp"

#include <iostream>

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    Features: qsearch, futility, delta, tt
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

// Search constructor
Search::Search(Board &start_board, Evaluate &eval) : board(start_board), evaluate(eval) {
    Table t_table;
    std::vector<Move> principal_variation;
    principal_variation.reserve(12);
}

Move Search::get_engine_move() {
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    std::cout << "get_engine_move " << (board.get_side_to_move() ? "black" : "white") << std::endl;

    // Move last pv move to the front (if valid)
    if (principal_variation.size() > 1) {
        Move pv_move = principal_variation[2];
        for (int i = 0; i < move_count; i++) {
            if (moves[i] == pv_move) {
                moves[i] = moves[0];
                moves[0] = pv_move;
                break;
            }
        }
    }

    int best_eval = -100000;
    Move best_move = Move(moves[0]);

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp_pv;

        board.make_move(moves[i]);
        int next_eval = -search(-100000, 100000, 4, temp_pv);
        board.unmake_move(moves[i]);
        std::cout << "next_eval " << moves[i] << " => " << next_eval << std::endl;
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

    return best_move;
}

int Search::search(int alpha, int beta, int depth, std::vector<Move> &pv) {
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    // Mate or stalemate
    if (move_count == 0) {
        return evaluate.evaluate_cheap();
    }

    if (depth <= 0) {
        pv.clear();
        int curr_eval = quiesce(alpha, beta, pv, 0);

        // PV should only have size in the case where there is a non quiet position
        if (pv.size()) {
            NodeType type;
            if (curr_eval <= alpha) {
                type = NodeType::UPPER;
            } else if (curr_eval >= beta) {
                type = NodeType::LOWER;
            } else {
                type = NodeType::EXACT;
            }

            t_table.put(board, pv.front(), curr_eval, type, 0);  // Null move?
        }

        return curr_eval;
    }

    // futility pruning
    bool is_futile = false;
    if (depth == 1) {
        int curr_eval = evaluate.evaluate_cheap();
        const int MINOR_VAL = 310;
        if (curr_eval + MINOR_VAL < alpha) {
            is_futile = true;
        }
    } else if (depth == 2) {
        int curr_eval = evaluate.evaluate_cheap();
        const int ROOK_VAL = 510;
        if (curr_eval + ROOK_VAL < alpha) {
            is_futile = true;
        }
    }
    is_futile = !board.in_check() && is_futile;

    int j = 0;  // move swap counter

    // Check transposition table for current position.
    std::optional<TableEntry> t_position;  //= t_table.get(board);  //6k1/5p2/6p1/2P5/7p/8/7P/6K1 b - - 0 1
    if (t_position.has_value()) {
        if (t_position->get_depth() >= depth) {
            switch (t_position->get_type()) {
                case NodeType::UPPER:
                    if (t_position->get_eval() >= beta) {
                        return t_position->get_eval();
                    }  // else update beta?
                    break;
                case NodeType::LOWER:
                    if (t_position->get_eval() <= alpha) {
                        return alpha;
                    } else {
                        alpha = t_position->get_eval();
                    }
                    break;
                default:
                    return t_position->get_eval();
                    break;
            }

            if (alpha >= beta) {
                return alpha;
            }
        }

        // move ordering: transposition table first
        for (int i = 0; i < move_count; i++) {
            if (moves[i] == t_position->get_move()) {
                moves[i] = moves[0];
                moves[0] = t_position->get_move();
                j++;
                break;
            }
        }
    }

    // move ordering: captures first
    for (int i = j; i < move_count; i++) {
        if (moves[i].is_capture()) {
            Move temp_move = moves[j];
            moves[j] = moves[i];
            moves[i] = temp_move;
            j++;
        }
    }

    std::vector<Move> temp_pv;  // local pv
    int best_eval = INT_MIN;
    NodeType move_type = NodeType::LOWER;

    for (int i = 0; i < move_count; i++) {
        if (is_futile && !moves[i].is_capture() && !moves[i].is_promotion() && i > 0) {  // && !moves[i].is_check()
            continue;
        }

        board.make_move(moves[i]);

        // PV SEARCH
        // Currently this improves search a lot in complex positions but slows down in the opening position
        // Should be better with improved move ordering
        int next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
        /*
        if (move_type == NodeType::EXACT && depth > 1) {
            next_eval = -search(-alpha - 1, -alpha, depth - 1, temp_pv);

            if (next_eval > alpha && next_eval < beta) {
                // Re conduct full search
                next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
            }
        } else {
            next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
        }*/

        board.unmake_move(moves[i]);

        // update best eval
        if (next_eval > best_eval) {
            best_eval = next_eval;

            // Add new move to front of p_var and copy temp onto p_var
            pv.clear();
            pv.push_back(moves[i]);
            for (int j = 0; j < temp_pv.size(); j++) {
                pv.push_back(temp_pv[j]);
            }

            // tighten window
            if (best_eval > alpha) {
                alpha = best_eval;
                move_type = NodeType::EXACT;
            }

            // return position if better than current max
            if (best_eval >= beta) {
                move_type = NodeType::UPPER;
                break;
            }
        }
    }

    // Add new best move to hash table
    t_table.put(board, pv.front(), best_eval, move_type, static_cast<uint8_t>(depth));

    return best_eval;
}

int Search::quiesce(int alpha, int beta, std::vector<Move> &pv, short q_depth) {
    if (q_depth > 5) {
        return alpha;
    }

    // TODO: handle loud positions
    if (!board.in_check()) {
        int stand_pat = evaluate.evaluate_cheap();
        if (stand_pat >= beta) {
            return stand_pat;
        }

        // Delta pruning - TODO: handle promotion
        const int DELTA = 900;  // queen / max material swing value
        if (stand_pat + DELTA < alpha) {
            return alpha;
        }

        // Null move assumption
        if (stand_pat > alpha) {
            alpha = stand_pat;
        }
    }

    //generate all moves
    std::vector<Move> moves = board.get_moves();
    //std::vector<Move> captures = sort_captures(moves);
    int move_count = moves.size();
    int best_eval = INT_MIN;
    std::vector<Move> temp_pv;

    for (int i = 0; i < move_count; i++) {
        // skip if move isn't capture
        if (moves[i].is_capture() || board.in_check()) {
            board.make_move(moves[i]);
            int next_eval = -quiesce(-beta, -alpha, temp_pv, q_depth + 1);
            board.unmake_move(moves[i]);

            if (next_eval > best_eval) {
                best_eval = next_eval;

                pv.clear();  // k1rr4/8/8/8/3R4/3R4/8/7K b - - 0 1
                pv.push_back(moves[i]);
                for (int j = 0; j < temp_pv.size(); j++) {
                    pv.push_back(temp_pv[j]);
                }

                if (next_eval > alpha) {
                    alpha = next_eval;
                }

                if (next_eval >= beta) {
                    return beta;
                }
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
