#pragma once

#include <array>
#include <cstdint>

#include "board.h"
#include "move.h"

class Evaluate {
   public:
    Evaluate();
    Evaluate(Board board);
    int move_evaluate(Board board, Move move) const;
    int static_evaluate();
    int static_evaluate_cheap();
    int static_evaluate_cheap(Board board);

   private:
    Board board;
    void initialize_pst();
    int piece_values(std::array<uint64_t, 2> piece_boards, int value);
};