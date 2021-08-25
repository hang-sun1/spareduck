#include "search.hpp"

#include <assert.h>

#include <cstddef>
#include <iostream>

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
}

Move Search::get_engine_move() {
    auto unsorted_moves = board.get_moves();
    std::vector<Move> moves = sort_moves(unsorted_moves);
    for (auto &m : moves) {
        std::cout << m << std::endl;
    }
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
    Move best_move = moves[0];

    for (int i = 0; i < move_count; i++) {
        std::vector<Move> temp_pv;
        auto side = board.get_side_to_move() ? Side::BLACK : Side::WHITE;
        auto pieces_involved = board.make_move(moves[i]);
        uint8_t white_king_square = __builtin_ffsll(board.get_kings()[0]) - 1;
        uint8_t black_king_square = __builtin_ffsll(board.get_kings()[1]) - 1;
        assert(pieces_involved[0].has_value());

        if (white_king_square >= 64 || black_king_square >= 64) {
            board.unmake_move(moves[i]);
            continue;
        }
        assert(white_king_square < 64);
        assert(black_king_square < 64);
        int next_eval;

        if (pieces_involved[0].value() != KING) {
            nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
            next_eval = -search(-100000, 100000, 4, temp_pv);
            board.unmake_move(moves[i]);
            nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
        } else {
            nnue.reset_nnue(pieces_involved[1], this->board);
            next_eval = -search(-100000, 100000, 4, temp_pv);
            board.unmake_move(moves[i]);
            nnue.reset_nnue(pieces_involved[1], this->board);
        }

        // board.make_move(moves[i]);
        // int next_eval = -search(-100000, 100000, 1, temp_pv);
        // board.unmake_move(moves[i]);
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
    auto unsorted_moves = board.get_moves();
    std::vector<Move> moves = sort_moves(unsorted_moves);
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
    is_futile = !board.in_check() && is_futile;

    int j = 0;  // move swap counter

    // Check transposition table for current position.
    std::optional<TableEntry> t_position;  //= t_table.get(board);  //6k1/5p2/6p1/2P5/7p/8/7P/6K1 b - - 0 1
    if (t_position.has_value()) {
        if (t_position->get_depth() >= depth) {
            switch (t_position->get_type()) {
                case NodeType::UPPER:
                    if (t_position->get_eval() > beta) {
                        //return t_position->get_eval();
                    } else {
                        //beta = t_position->get_eval();
                    }
                    break;
                case NodeType::LOWER:
                    if (t_position->get_eval() < alpha) {
                        //return alpha;
                    } else {
                        alpha = t_position->get_eval();
                    }
                    break;
                default:
                    return t_position->get_eval();
                    break;
            }

            if (alpha >= beta) {
                return t_position->get_eval();
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

    std::vector<Move> temp_pv;  // local pv
    int best_eval = INT_MIN;
    NodeType move_type = NodeType::LOWER;

    for (int i = 0; i < move_count; i++) {
        if (is_futile && !moves[i].is_capture() && !moves[i].is_promotion() && i > 0) {  // && !moves[i].is_check()
            continue;
        }
        auto side = board.get_side_to_move() ? Side::BLACK : Side::WHITE;
        auto pieces_involved = board.make_move(moves[i]);
        uint8_t white_king_square = __builtin_ffsll(board.get_kings()[0]) - 1;
        uint8_t black_king_square = __builtin_ffsll(board.get_kings()[1]) - 1;
        if (white_king_square >= 64 || black_king_square >= 64) {
            board.unmake_move(moves[i]);
            continue;
        }
        assert(white_king_square < 64);
        assert(black_king_square < 64);

        int next_eval;

        if (pieces_involved[0].value() != KING) {
            nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
            // PV SEARCH
            // Currently this improves search a lot in complex positions but slows down in the opening position
            if (move_type == NodeType::EXACT && depth > 1) {
                next_eval = -search(-alpha - 1, -alpha, depth - 1, temp_pv);

                if (next_eval > alpha && next_eval < beta) {
                    // Re-conduct full search
                    next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
                }
            } else {
                next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
            }
            board.unmake_move(moves[i]);
            nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
        } else {
            nnue.reset_nnue(pieces_involved[1], this->board);
            // Copy pasting code, gross
            if (move_type == NodeType::EXACT && depth > 1) {
                next_eval = -search(-alpha - 1, -alpha, depth - 1, temp_pv);
                if (next_eval > alpha && next_eval < beta) {
                    next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
                }
            } else {
                next_eval = -search(-beta, -alpha, depth - 1, temp_pv);
            }
            board.unmake_move(moves[i]);
            nnue.reset_nnue(pieces_involved[1], this->board);
        }

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
    auto unsorted_moves = board.get_moves();
    std::vector<Move> moves = sort_moves(unsorted_moves);
    std::vector<Move> temp_pv;
    int best_eval = INT_MIN;

    for (size_t i = 0; i < moves.size(); i++) {
        // skip if move isn't capture
        if (moves[i].is_capture() || board.in_check()) {
            auto side = board.get_side_to_move() ? Side::BLACK : Side::WHITE;
            auto pieces_involved = board.make_move(moves[i]);
            uint8_t white_king_square = __builtin_ffsll(board.get_kings()[0]) - 1;
            uint8_t black_king_square = __builtin_ffsll(board.get_kings()[1]) - 1;
            // std::cout << (unsigned int) white_king_square << (unsigned int) black_king_square << std::endl;
            // std::cout << "MOVECOUNT " << move_count << std::endl;
            // FIXME ugly hack
            if (white_king_square >= 64 || black_king_square >= 64) {
                board.unmake_move(moves[i]);
                continue;
            }

            int next_eval;

            if (pieces_involved[0].value() != KING) {
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, false);
                next_eval = -quiesce(-beta, -alpha, temp_pv, q_depth + 1);
                board.unmake_move(moves[i]);
                nnue.update_non_king_move(moves[i], pieces_involved[0].value(), pieces_involved[1], std::nullopt, white_king_square, black_king_square, side, true);
            } else {
                nnue.reset_nnue(pieces_involved[1], this->board);
                next_eval = -quiesce(-beta, -alpha, temp_pv, q_depth + 1);
                board.unmake_move(moves[i]);
                nnue.reset_nnue(pieces_involved[1], this->board);
            }
            // board.make_move(moves[i]);
            // int next_eval = -quiesce(-beta, -alpha, temp_pv);
            // board.unmake_move(moves[i]);

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
std::vector<Move> Search::sort_moves(std::vector<Move> &moves) {
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

    std::vector<Move> others;

    for (int i = 0; i < moves.size(); i++) {
        if (moves[i].is_capture()) {
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
            others.push_back(moves[i]);
        }
    }

    std::vector<Move> sorted_moves;
    sorted_moves.reserve(high.size() + mid.size() + low.size() + lower.size() + lowest.size() + others.size());
    sorted_moves.insert(sorted_moves.end(), high.begin(), high.end());
    sorted_moves.insert(sorted_moves.end(), mid.begin(), mid.end());
    sorted_moves.insert(sorted_moves.end(), low.begin(), low.end());
    sorted_moves.insert(sorted_moves.end(), lower.begin(), lower.end());
    sorted_moves.insert(sorted_moves.end(), lowest.begin(), lowest.end());
    sorted_moves.insert(sorted_moves.end(), others.begin(), others.end());

    return sorted_moves;
}

std::vector<Move> Search::get_principal_variation() {
    return principal_variation;
    //return t_table.get_variation(*board);
}
