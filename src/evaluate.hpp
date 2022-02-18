#pragma once

#include <array>
#include <cstdint>

#include "board.hpp"
#include "move.hpp"
#include "nnue.hpp"

class Evaluate {
   public:
    Evaluate(Board& board, NNUE& nnue, bool is_cheap);
    int evaluate_cheap() const;
    int evaluate() const;
    int start_evaluate() const;
    int move_evaluate(Board board, Move move) const;
    void move_devaluate(Board board, Move move) const;

   private:
    Board& board;
    NNUE& nnue;
    bool is_cheap;
    void initialize_pst();
    int piece_values(std::array<uint64_t, 2> piece_boards, int value) const;
};