#include "search.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "table_entry.hpp"

#include <assert.h>

#include <climits>
#include <cstddef>
#include <iostream>
#include <cstring>

/*
    A simple implementation of fail-soft negamax alpha-beta search.
    Features: qsearch, futility, delta, tt
    More info:
    - http://en.wikipedia.org/wiki/Alpha-beta_pruning
    - http://chessprogramming.wikispaces.com/Alpha-Beta
*/

// Search constructor
Search::Search(Board &start_board, Evaluate &eval, NNUE &net) : board(start_board), evaluate(eval), nnue(net) {
    Table t_table;
    std::vector<Move> principal_variation;
    principal_variation.reserve(12);
    std::memset(history, 0, sizeof(int)*64*64*2);
    std::memset(butterfly, 0, sizeof(int)*64*64*2);
}

Move Search::get_engine_move() {
    std::cout << "get_engine_move " << (board.get_side_to_move() ? "black" : "white") << std::endl;
    auto start = std::chrono::steady_clock::now();

    Move final_move;

    auto unsorted_moves = board.get_moves();
    std::vector<Move> moves = sort_moves(unsorted_moves, 0);

    // print moves
    for (auto &m : moves) {
        std::cout << m << std::endl;
    }
    int move_count = moves.size();

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
    principal_variation.clear();

    size_t node_count = 0;

    for (size_t depth = 1;; depth++) {
        std::cout << "depth: " << depth << std::endl;
        for (Move move : principal_variation) {
            std::cout << move << " ";
        }
        std::cout << std::endl;
        // Move last pv move to the front (if valid)
        if (principal_variation.size() > 1) {
            Move pv_move = principal_variation[0];
            for (int i = 0; i < move_count; i++) {
                if (moves[i] == pv_move) {
                    moves[i] = moves[0];
                    moves[0] = pv_move;
                    break;
                }
            }
        }

        int best_eval = -100000;
        Move best_move = moves[0];
        int best_move_index = 0;

        bool valid_moves = false;
        int moves_searched = 0;
        for (int i = 0; i < move_count; i++) {
            std::vector<Move> temp_pv;
            auto side = board.get_side_to_move() ? Side::BLACK : Side::WHITE;
            auto pieces_involved = board.make_move(moves[i]);
            if (!board.is_pos_valid(moves[i])) {
                board.unmake_move(moves[i]);
                continue;
            }
            ++node_count; 
            moves_searched += 1;

            if (moves_searched >= 6 && depth >= 3 && !moves[i].is_capture() &&
                !moves[i].is_promotion() && !board.in_check((Side) board.get_side_to_move())) {

            }

            valid_moves = true;
            uint8_t white_king_square = __builtin_ffsll(board.get_kings()[0]) - 1;
            uint8_t black_king_square = __builtin_ffsll(board.get_kings()[1]) - 1;
            assert(pieces_involved[0].has_value());

            assert(white_king_square < 64);
            assert(black_king_square < 64);
            int next_eval;

            if (pieces_involved[0].value() != KING) {
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
                next_eval = -pvs(-100000, 100000, NodeType::LOWER, depth, 1, temp_pv, &node_count, true);
                board.unmake_move(moves[i]);
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
            } else {
                nnue.reset_nnue(pieces_involved[1], this->board);
                next_eval = -pvs(-100000, 100000, NodeType::LOWER, depth, 1, temp_pv, &node_count, true);
                board.unmake_move(moves[i]);
                nnue.reset_nnue(pieces_involved[1], this->board);
            }

            std::cout << "next_eval " << moves[i] << " => " << next_eval << std::endl;
            // update bestEval
            if (next_eval > best_eval) {
                best_eval = next_eval;
                best_move_index = i;
                best_move = moves[i];

                // copy temporary variation onto principal variation with new best move
                principal_variation.clear();
                principal_variation.push_back(best_move);
                for (int j = 0; j < temp_pv.size(); j++) {
                    principal_variation.push_back(temp_pv[j]);
                }
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > SEARCH_TIME) {
                goto finish_search;
            }

        }
        t_table.put(board, best_move, best_eval, NodeType::EXACT, static_cast<uint8_t>(depth));
        final_move = best_move;
        // if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > SEARCH_TIME) {
        //     final_move = best_move;
        //     goto finish_search;
        // }

        std::cout << "time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << std::endl
                  << std::endl;
        std::cout << "Nodes searched: " << node_count << std::endl;
    
    }


finish_search:
    t_table.clear();
    std::memset(history, 0, sizeof(int)*64*64*2);
    std::memset(butterfly, 0, sizeof(int)*64*64*2);

    return final_move;
}


int Search::pvs(int alpha, int beta, NodeType move_type, size_t depth, size_t ply, std::vector<Move> &pv, size_t* node_count, bool nullable) {
    if (depth <= 0) {
        // int curr_eval = quiesce(alpha, beta, pv, 0, node_count);
        int curr_eval = evaluate.evaluate();
        //
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

    int swap_count = 0;  // move swap counter

    // Check transposition table for current position.

    std::optional<TableEntry> t_position = t_table.get(board);  //6k1/5p2/6p1/2P5/7p/8/7P/6K1 b - - 0 1
    if (enable_tt) {
        if (t_position.has_value()) {
            if (t_position->get_depth() >= depth) {
                switch (t_position->get_type()) {
                    case NodeType::UPPER:
                        if (t_position->get_eval() >= beta) {
                            return t_position->get_eval();
                        } else {
                            // beta = t_position->get_eval();
                            alpha = std::max(alpha, t_position->get_eval());
                        }
                        break;
                    case NodeType::LOWER:
                        if (t_position->get_eval() <= alpha) {
                            return t_position->get_eval();
                            // return alpha;
                        } else {
                            // alpha = t_position->get_eval();
                            beta = std::min(beta, t_position->get_eval());
                        }
                        break;
                    default:
                        return t_position->get_eval();
                        break;
                }

                if (alpha >= beta) {
                    // return t_position->get_eval();
                    return alpha;
                }
            }
        }
    }

    auto unsorted_moves = board.get_moves();
    int move_count = unsorted_moves.size();

    // // Mate or stalemate
    // if (move_count == 0) {
    //     return evaluate.evaluate();
    // }

    // sort moves
    std::vector<Move> moves = sort_moves(unsorted_moves, ply);
    std::vector<Move> temp_moves;
    // move ordering: transposition table first
    if (enable_tt) {
        if (t_position.has_value() && (t_position->get_type() == NodeType::EXACT || t_position->get_type() == NodeType::UPPER) &&
            t_position->get_depth() >= depth) {
            for (int i = 0; i < moves.size(); i++) {
                if (moves[i].origin_square() == t_position->get_move().origin_square() && moves[i].destination_square() == t_position->get_move().destination_square()
                    && moves[i].type() == t_position->get_move().type()) {
                    temp_moves.push_back(moves[i]);
                    for (int j = 0; j < moves.size(); ++j) {
                        if (j != i) {
                            temp_moves.push_back(moves[j]);
                        }
                    }
                    moves = temp_moves;
                    swap_count++;
                    break;
                }
            }
        }
    }

    // futility pruning
    bool is_futile = false;
    if (depth == 1) {
        int curr_eval = evaluate.evaluate();
        const int MINOR_VAL = 310;
        if (curr_eval + MINOR_VAL < alpha) {
            is_futile = true;
        }
    } else if (depth == 2) {
        int curr_eval = evaluate.evaluate();
        const int ROOK_VAL = 510;
        if (curr_eval + ROOK_VAL < alpha) {
            is_futile = true;
        }
    }
    is_futile = !board.in_check((Side) board.get_side_to_move()) && is_futile;

    std::vector<Move> temp_pv;  // local pv
    int best_eval = INT_MIN;
    move_type = NodeType::LOWER;
    bool any_valid_moves = false;
    int best_eval_idx = 0;



    int child = 0;
    int moves_searched = 0;

    for (int i = 0; i < move_count; i++) {
        auto side = board.get_side_to_move() ? Side::BLACK : Side::WHITE;
        auto pieces_involved = board.make_move(moves[i]);
        auto is_in_check_before_move = board.in_check((Side) board.get_side_to_move());
        if (!board.is_pos_valid(moves[i])) {
            board.unmake_move(moves[i]);
            continue;
        }
        *node_count += 1;
        any_valid_moves = true;
        
        if (is_futile && !moves[i].is_capture() && !moves[i].is_promotion() && i > 0) {  // && !moves[i].is_check()
            board.unmake_move(moves[i]);
            continue;
        }
        uint8_t white_king_square = __builtin_ffsll(board.get_kings()[0]) - 1;
        uint8_t black_king_square = __builtin_ffsll(board.get_kings()[1]) - 1;
        assert(white_king_square < 64);
        assert(black_king_square < 64);

        int next_eval;
        
        auto temp_alpha = alpha;
        auto temp_beta = beta;
        
        
        if (moves_searched < 1 && !board.in_check((Side) board.get_side_to_move()) && depth >= 3 && nullable) {
            int null_eval; 
            board.unmake_move(moves[i]);
            size_t null_depth = depth-3;
            if (pieces_involved[0].value() != KING) {
                board.make_null_move();
                null_eval = -pvs(-beta, -beta+1, move_type, null_depth, ply+1, temp_pv, node_count, false);
                board.unmake_move(Move(0, 0, MoveType::QUIET));
            } else {
                board.make_null_move();
                null_eval = -pvs(-beta, -beta+1, move_type, null_depth, ply+1, temp_pv, node_count, false);
                board.unmake_move(Move(0, 0, MoveType::QUIET));
            } 

            if (null_eval >= beta) {
                
                return null_eval;
                // goto FINISH;
            } else {
                board.make_move(moves[i]);
            }
        }
        moves_searched += 1;
        // update the butterfly board
        butterfly[board.get_side_to_move()][moves[i].origin_square()][moves[i].destination_square()] += 1;
        // late move reduction  
        if (moves_searched >= 4 && depth >= 3 && !moves[i].is_capture() &&
            !moves[i].is_promotion() && !board.in_check((Side) board.get_side_to_move())) {
            auto reduced_depth = depth-1;
            // if (moves_searched >= 20 && depth >= 4) {
            //     reduced_depth = depth - depth / 3;
            // }
            if (pieces_involved[0].value() != KING) {
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
                next_eval = -pvs(-alpha-1, -alpha, move_type, reduced_depth - 1, ply+1, temp_pv, node_count, true);
                board.unmake_move(moves[i]);
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
            } else {
                nnue.reset_nnue(pieces_involved[1], this->board);
                next_eval = -pvs(-alpha-1, -alpha, move_type, reduced_depth - 1, ply+1, temp_pv, node_count, true);
                board.unmake_move(moves[i]);
                nnue.reset_nnue(pieces_involved[1], this->board);
            } 

            if (next_eval < alpha) {
                goto FINISH;
            } else {
                board.make_move(moves[i]);
            }
        }

        // temp_alpha = alpha;
        // temp_beta = beta;
        if (child > 0) {
            if (alpha > 0) {
                beta = alpha +1;
            } else {
                beta = alpha - 1;
            }
        }

        if (pieces_involved[0].value() != KING) {
            nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
            // PV SEARCH
            // Currently this improves search a lot in complex positions but slows down in the opening position
            // next_eval = pvs(alpha, beta, move_type, depth, temp_pv);
            next_eval = -pvs(-beta, -alpha, move_type, depth - 1, ply+1, temp_pv, node_count, true);
            board.unmake_move(moves[i]);
            nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
        } else {
            nnue.reset_nnue(pieces_involved[1], this->board);

            next_eval = -pvs(-beta, -alpha, move_type, depth - 1, ply+1, temp_pv, node_count, true);
            // next_eval = pvs(alpha, beta, move_type, depth, temp_pv);

            board.unmake_move(moves[i]);
            nnue.reset_nnue(pieces_involved[1], this->board);
        } 
        alpha = temp_alpha;
        beta = temp_beta;

        if (child > 0 && next_eval > alpha && next_eval < beta) {
            board.make_move(moves[i]);
            if (pieces_involved[0].value() != KING) {
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
                // PV SEARCH
                // Currently this improves search a lot in complex positions but slows down in the opening position
                // next_eval = pvs(alpha, beta, move_type, depth, temp_pv);
                next_eval = -pvs(-beta, -next_eval, move_type, depth - 1, ply+1, temp_pv, node_count, true);
                board.unmake_move(moves[i]);
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
            } else {
                nnue.reset_nnue(pieces_involved[1], this->board);

                next_eval = -pvs(-beta, -next_eval, move_type, depth - 1, ply+1, temp_pv, node_count, true);
                // next_eval = pvs(alpha, beta, move_type, depth, temp_pv);

                board.unmake_move(moves[i]);
                nnue.reset_nnue(pieces_involved[1], this->board);
            }

        }
        FINISH:
        // update best eval
        if (next_eval > best_eval) {
            best_eval = next_eval;
            best_eval_idx = i;
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
                if (moves[i].type() == MoveType::QUIET || moves[i].type() == MoveType::DOUBLE_PAWN_PUSH) {
                    if (killers[ply][0].origin_square() != moves[i].origin_square()
                        || killers[ply][0].destination_square() != moves[i].destination_square()
                        || killers[ply][0].type() != moves[i].type()) {
                            killers[ply][1] = killers[ply][0];
                            killers[ply][0] = moves[i];
                    }

                    history[board.get_side_to_move()][moves[i].origin_square()][moves[i].destination_square()] += 1; //depth * depth; 
                }
                move_type = NodeType::UPPER;
                break;
            }
        }
        child += 1;
    }
    if (!any_valid_moves) {
        // this position is checkmate
        if (board.in_check(static_cast<Side>(board.get_side_to_move()))) {
            return -99999;
            // return INT_MIN+1;
        }
        // this position is stalemate
        return 0; 
    }

    if (enable_tt) {
        // Add new best move to hash table
        t_table.put(board, moves[best_eval_idx], best_eval, move_type, static_cast<uint8_t>(depth));
    }

    return best_eval;
}

int Search::quiesce(int alpha, int beta, std::vector<Move> &pv, short q_depth, size_t* node_count) {
    *node_count += 1;
    if (q_depth > 5) {
        return evaluate.evaluate();
    }

    // TODO: handle loud positions
    // if (!board.in_check((Side) board.get_side_to_move())) {
        int stand_pat = evaluate.evaluate();
        if (stand_pat >= beta) {
            // return stand_pat;
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
    // }

    //generate all moves
    auto unsorted_moves = board.get_moves();

    // sort moves
    // std::vector<Move> moves = sort_quiescence_moves(unsorted_moves);
    std::vector<Move> moves = sort_moves(unsorted_moves, 99);
    std::vector<Move> temp_pv;
    int best_eval = INT_MIN;
    bool any_valid_moves = false;

    for (size_t i = 0; i < moves.size(); i++) {
        // skip if move isn't capture
        if (moves[i].is_capture() || board.in_check((Side) board.get_side_to_move())) {  // || moves[i].is_check()
            auto side = board.get_side_to_move() ? Side::BLACK : Side::WHITE;
            auto pieces_involved = board.make_move(moves[i]);
            if (!board.is_pos_valid(moves[i])) {
                board.unmake_move(moves[i]);
                continue;
            }
            any_valid_moves = true;
            uint8_t white_king_square = __builtin_ffsll(board.get_kings()[0]) - 1;
            uint8_t black_king_square = __builtin_ffsll(board.get_kings()[1]) - 1;

            int next_eval;

            if (pieces_involved[0].value() != KING) {
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
                next_eval = -quiesce(-beta, -alpha, temp_pv, q_depth + 1, node_count);
                board.unmake_move(moves[i]);
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
            } else {
                nnue.reset_nnue(pieces_involved[1], this->board);
                next_eval = -quiesce(-beta, -alpha, temp_pv, q_depth + 1, node_count);
                board.unmake_move(moves[i]);
                nnue.reset_nnue(pieces_involved[1], this->board);
            }

            if (next_eval > best_eval) {
                best_eval = next_eval;

                pv.clear();  // k1rr4/8/8/8/3R4/3R4/8/7K b - - 0 1
                pv.push_back(moves[i]);
                for (int j = 0; j < temp_pv.size(); j++) {
                    pv.push_back(temp_pv[j]);
                }
                
                if (next_eval >= beta) {
                    return next_eval;
                }

                if (next_eval > alpha) {
                    alpha = next_eval;
                }
            }
        } else  {
            board.make_move(moves[i]);
            if (!board.is_pos_valid(moves[i])) {
                board.unmake_move(moves[i]);
                continue;
            }
            board.unmake_move(moves[i]);
            any_valid_moves = true;
        }
    }
    if (!any_valid_moves) {
        // this position is checkmate
        if (board.in_check(static_cast<Side>(board.get_side_to_move()))) {
            return -99999;
        }
        // this position is stalemate
        return 0; 
    }

    return std::max(best_eval, stand_pat);
}

// bucket sorting captures using Most Valuable Victim - Least Valuable Aggressor
std::vector<Move> Search::sort_moves(std::vector<Move> &moves, size_t ply) {
    // 400-800
    std::vector<Move> high;
    // 200-300
    std::vector<Move> mid;
    // 0 - 100
    std::vector<Move> low;

    std::vector<Move> killer_moves;
    // -100 - -300
    std::vector<Move> lower;
    // < -300
    std::vector<Move> lowest;

    std::vector<Move> others;

    for (int i = 0; i < moves.size(); i++) {
        if (moves[i].is_capture() && moves[i].type() != MoveType::EN_PASSANT) {
            auto side = static_cast<Side>(board.get_side_to_move());
            auto other_side = static_cast<Side>(1 - board.get_side_to_move());
            auto captured = board.piece_on_square(moves[i].destination_square(), other_side);
            // assert(moves[i].get_captured().has_value());
            auto captured_value = piece_to_value(captured);
            auto moved_value = piece_to_value(board.piece_on_square(moves[i].origin_square(), side));
            auto diff = captured_value - moved_value;
            if (diff >= 400) {
                high.push_back(moves[i]);
            } else if (diff >= 200) {
                mid.push_back(moves[i]);
            } else if (diff >= 0) {
                low.push_back(moves[i]);
            } else if (diff >= -300) {
                lower.push_back(moves[i]);
            } else {
                lowest.push_back(moves[i]);
            }
        } else {
            if (killers[ply][0].origin_square() == moves[i].origin_square()
                && killers[ply][0].destination_square() == moves[i].destination_square()
                && killers[ply][0].type() == moves[i].type()) {
                    killer_moves.push_back(moves[i]);
            } else if (killers[ply][1].origin_square() == moves[i].origin_square()
                && killers[ply][1].destination_square() == moves[i].destination_square()
                && killers[ply][1].type() == moves[i].type()) {
                    killer_moves.push_back(moves[i]);
            } else {
                others.push_back(moves[i]);
            }
        }
    }

    auto capture_sort = [this](Move a, Move b) {
        auto side = static_cast<Side>(board.get_side_to_move());
        auto other_side = static_cast<Side>(1 - board.get_side_to_move());
        auto a_captured = board.piece_on_square(a.destination_square(), other_side);
        // assert(moves[i].get_captured().has_value());
        auto a_captured_value = piece_to_value(a_captured);
        auto a_moved_value = piece_to_value(board.piece_on_square(a.origin_square(), side));
        auto a_diff = a_captured_value - a_moved_value;
        
        auto b_captured = board.piece_on_square(a.destination_square(), other_side);
        // assert(moves[i].get_captured().has_value());
        auto b_captured_value = piece_to_value(b_captured);
        auto b_moved_value = piece_to_value(board.piece_on_square(a.origin_square(), side));
        auto b_diff = b_captured_value - b_moved_value;
        
        return a_diff > b_diff;
    };

    std::sort(high.begin(), high.end(), capture_sort);
    std::sort(mid.begin(), mid.end(), capture_sort);
    std::sort(low.begin(), low.end(), capture_sort);
    std::sort(lower.begin(), lower.end(), capture_sort);
    std::sort(lowest.begin(), lowest.end(), capture_sort);

    std::sort(others.begin(), others.end(), [this](Move a, Move b) {
        auto side = board.get_side_to_move();
        auto a_hist = history[side][a.origin_square()][a.destination_square()];
        auto a_butterfly = butterfly[side][a.origin_square()][a.destination_square()];
        auto b_hist = history[side][b.origin_square()][b.destination_square()];
        auto b_butterfly = butterfly[side][b.origin_square()][b.destination_square()];
        if (a_butterfly == 0 || b_butterfly == 0) {
            return a_hist > b_hist;
        }
        return (a_hist / a_butterfly) > (b_hist / b_butterfly);
        // return a_hist > b_hist;
    });

    std::vector<Move> sorted_moves;
    sorted_moves.reserve(high.size() + mid.size() + low.size() + lower.size() + lowest.size() + others.size());
    sorted_moves.insert(sorted_moves.end(), high.begin(), high.end());
    sorted_moves.insert(sorted_moves.end(), mid.begin(), mid.end());
    sorted_moves.insert(sorted_moves.end(), low.begin(), low.end());
    sorted_moves.insert(sorted_moves.end(), killer_moves.begin(), killer_moves.end());
    sorted_moves.insert(sorted_moves.end(), others.begin(), others.end());
    sorted_moves.insert(sorted_moves.end(), lower.begin(), lower.end());
    sorted_moves.insert(sorted_moves.end(), lowest.begin(), lowest.end());

    return sorted_moves;
}

std::vector<Move> Search::sort_quiescence_moves(std::vector<Move>& moves) {
    std::vector<Move> sorted_moves;
    std::vector<Move> non_captures;
    for (auto move : moves) {
        if (move.is_capture() && board.static_exchange_evaluation(move) >= 0) {
            sorted_moves.push_back(move);
        } else {
            non_captures.push_back(move);
        }
    }

    std::sort(sorted_moves.begin(), sorted_moves.end(), [this](Move a, Move b) {
        return board.static_exchange_evaluation(a) > board.static_exchange_evaluation(b);
    });

    sorted_moves.insert(sorted_moves.end(), non_captures.begin(), non_captures.end());
    return sorted_moves;
}

std::vector<Move> Search::get_principal_variation() {
    return principal_variation;
    //return t_table.get_variation(*board);
}
