#pragma once

#include <climits>
#include <optional>
#include <vector>
#include <chrono>

#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "table.hpp"
#include "table_entry.hpp"

#define SEARCH_TIME 10000

class Search {
   public:
    Search(Board& board, Evaluate& eval, NNUE& net);
    Move get_engine_move();
    std::vector<Move> get_principal_variation();

   private:
    Board& board;
    Evaluate& evaluate;
    NNUE& nnue;
    Table t_table;
    std::vector<Move> principal_variation;
    int search(int alpha, int beta, int depth, std::vector<Move>& p_var);
    int quiesce(int alpha, int beta, std::vector<Move>& p_var, short q_depth);
    int pvs(int alpha, int beta, NodeType move_type, size_t depth, std::vector<Move> &temp_pv);
    std::vector<Move> sort_moves(std::vector<Move>& moves);
};