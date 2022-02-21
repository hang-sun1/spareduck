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

#define SEARCH_TIME 2000
#define KILLERS 2
#define MAX_DEPTH 100

class Search {
   public:
    Search(Board& board, Evaluate& eval, NNUE& net);
    Move get_engine_move();
    std::vector<Move> get_principal_variation();
    bool enable_tt = true;

   private:
    Board& board;
    Evaluate& evaluate;
    NNUE& nnue;
    Table t_table;
    Move killers[MAX_DEPTH][KILLERS];
    std::vector<Move> principal_variation;
    int search(int alpha, int beta, int depth, std::vector<Move>& p_var);
    int quiesce(int alpha, int beta, std::vector<Move>& p_var, short q_depth, size_t* node_count);
    int pvs(int alpha, int beta, NodeType move_type, size_t depth, size_t ply, std::vector<Move> &temp_pv, size_t* node_count, bool nullable);
    std::vector<Move> sort_moves(std::vector<Move>& moves, size_t ply);
};