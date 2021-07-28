#pragma once

#include <array>
#include <cstdint>

#include "board.h"
#include "move.h"

class Evaluate {
   public:
    Evaluate(Board& board);
    int evaluate_cheap();
    int static_evaluate_cheap(Board board);
    int evaluate();
    int static_evaluate(Board board);
    int move_evaluate(Board board, Move move) const;

   private:
    Board& board;
    void initialize_pst();
    int piece_values(std::array<uint64_t, 2> piece_boards, int value);
};