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
}

Move Search::get_engine_move() {
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    std::cout << "get_engine_move " << board.get_side_to_move() << std::endl;
    std::cout << "current eval " << evaluate.evaluate_cheap() << std::endl;

    int best_eval = -10000;
    Move best_move = Move(moves[0]);

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp;

        board.make_move(moves[i]);
        int next_eval = -search(-1000, 1000, 4, temp);  // +?
        board.unmake_move(moves[i]);
        
        std::cout << "next_eval " << next_eval << " i " << i <<std::endl;

        // update bestEval
        if (next_eval > best_eval) {
            std::cout << "updating best_move " << next_eval << " i " << i <<std::endl;
            best_eval = next_eval;
            best_move = moves[i];

            // copy temporary variation onto principal variation with new best move
            principal_variation.clear();
            principal_variation.reserve(temp.size() + 1);
            principal_variation[0] = best_move;
            for (int j = 0; j < temp.size(); j++) {
                principal_variation[j + 1] = temp[j];
            }
        }
    }

    t_table.clear();

    std::cout << "PRINCIPAL-VARIATION" << std::endl;
    for (int i = 0; i < principal_variation.size(); i++)
        std::cout << i << "th move " << principal_variation.at(i).origin_square_algebraic() << " " << principal_variation.at(i).destination_square_algebraic() << std::endl;

    return best_move;
}

int Search::search(int alpha, int beta, int depth, std::vector<Move> &p_var) {
    if (depth == 0) {
        int curr_eval = evaluate.evaluate_cheap();
        //int curr_eval = quiesce(alpha, beta, p_var);

        NodeType type;  // Should this always be EXACT??
        if (curr_eval <= alpha) {
            type = NodeType::UPPER;
        } else if (curr_eval >= beta) {
            type = NodeType::LOWER;
        } else {
            type = NodeType::EXACT;
        }
        if (p_var.size())
            t_table.put(board, p_var.front(), curr_eval, type, static_cast<uint8_t>(depth));
        // p_var.clear();
        return curr_eval;
    }

    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();
    std::vector<Move> temp;
    int best_eval = INT_MIN;

    // Check transposition table for current position.
    std::optional<TableEntry> t_position;// = t_table.get(*board);

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
            Move temp = moves[j];
            moves[j] = moves[i];
            moves[i] = temp;
            j++;
        }
    }

    for (int i = 0; i < move_count; i++) {
        board.make_move(moves[i]);
        int next_eval = -search(-beta, -alpha, depth - 1, temp);
        board.unmake_move(moves[i]);

        // update bestEval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            // tighten window
            if (best_eval > alpha) {
                alpha = best_eval;
            }

            // Add new move to front of p_var and copy temp onto p_var
            if (p_var.size() == 0) {
                p_var.push_back(moves[i]);
            } else {
                p_var[0] = moves[i];
            }
            for (int j = 0; j < temp.size(); j++) {
                if (p_var.size() - 2 < j) {
                    p_var.push_back(temp[j]);
                } else {
                    p_var[j + 1] = temp[j];
                }
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
    t_table.put(board, p_var.front(), best_eval, type, static_cast<uint8_t>(depth));

    return best_eval;
}

int Search::quiesce(int alpha, int beta, std::vector<Move> &p_var) {
    p_var.clear();

    // TODO: handle loud positions
    if (board.in_check()) {
        return evaluate.evaluate_cheap();
    }

    int stand_pat = evaluate.evaluate_cheap();
    if (stand_pat >= beta) {
        return stand_pat;
    }
    // Delta pruning - TODO: handle promotion, in check
    const int DELTA = 900;  // queen / max material swing value
    if (stand_pat + DELTA < alpha) {
        return alpha;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    //generate all moves
    std::vector<Move> moves = board.get_moves();
    int move_count = moves.size();

    // TODO: Most Valuable Victim - Least Valuable Aggressor
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

        temp.push_back(moves[i]);
        p_var = temp;
    }

    return alpha;
}

std::vector<Move> Search::get_principal_variation() {
    std::cout << "pv size= " << principal_variation.size() << std::endl;
    return principal_variation;
    //return t_table.get_variation(*board);
}
