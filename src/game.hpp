#pragma once

#include <thread>

#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"

class Game {
  private:
    Board game_board;
    Evaluate board_evaluate;
    Search search_engine;

  public:
    Game();
       
    int get_side_to_move();

    void make_move(int from, int to, bool promotion, int promote_to);

    // void make_move(std::string move);
    std::vector<std::string> get_moves();

    std::string get_engine_moves();

    double get_engine_evaluation();

    std::vector<Move> get_principal_variation();

    bool in_check();

    void start_from_position(std::string fen);

    std::vector<std::string> test_position(std::string fen, std::string move);
};
