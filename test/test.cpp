#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include <chrono>
#include <cstdint>

#define private public

#include "../src/board.hpp"
#include "../src/move.hpp"
#include "../src/nnue.hpp"

using namespace std::chrono;

// taken from the chess programming wiki
uint64_t perft(int depth /* assuming >= 1 */, Board *b) {
    auto move_list = b->get_moves();
    int n_moves, i;
    n_moves = move_list.size();
    uint64_t nodes = 0;


    if (depth == 1) 
        return (uint64_t) n_moves;
    // if (depth == 0) {
    //     return 1ULL;
    // }

    for (i = 0; i < n_moves; i++) {
        auto m = move_list[i];
        if (static_cast<uint16_t>(m.type()) == static_cast<uint16_t>(MoveType::CAPTURE)) {
            captures += 1;
        }
        b->make_move(move_list[i]);
        nodes += perft(depth - 1, b);
        b->unmake_move(move_list[i]);
    }
    return nodes;
}


TEST_CASE("proper moves are generated", "[board]") {
    SECTION("finds 20 moves from start position") {
        Board b;
        REQUIRE(b.get_moves().size() == 20);
        b = Board();
        REQUIRE(b.get_moves().size() == 20);
    }
    SECTION("correctly makes a move") {
        Board b;
        auto starting_hash = b.get_hash();
        auto moves = b.get_moves();
        auto first_move = moves[0];
        b.make_move(first_move);
        auto first_hash = b.get_hash();
        REQUIRE(starting_hash != first_hash);
        moves = b.get_moves();
        REQUIRE(b.get_moves().size() == 20);
        b.make_move(b.get_moves()[0]);
        REQUIRE(b.get_moves().size() == 19);
        b.unmake_move(first_move);
        REQUIRE(b.get_moves().size() == 20);
    }

    SECTION("correctly unmakes a move") {
        Board b;
        auto starting_hash = b.get_hash();
        b.make_move(b.get_moves()[0]);
        
        auto second_hash = b.get_hash();
        REQUIRE(second_hash != starting_hash);
        
        b.unmake_move(b.get_moves()[0]);
        REQUIRE(b.get_hash() == starting_hash);
    }

    SECTION("generates correct number of moves to certain depth") {
        Board b;
        Board c = b;
        auto the_move = b.get_moves()[4];
        b.make_move(the_move);
        the_move = b.get_moves()[0];
        b.make_move(the_move);
        the_move = b.get_moves()[9];
        for (auto &m: b.get_moves()) {
            Board a = b;
            std::cout << m.origin_square_algebraic() << m.destination_square_algebraic() << std::endl;
            a.moves = std::vector<Move> {m};
            auto start = high_resolution_clock::now();
            uint64_t count = perft(2, &a);
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            std::cout << count << " nodes searched in " << duration.count() << " ms\n";
            // std::cout << captures << " captures" << std::endl;
        }
        
        auto start = high_resolution_clock::now();
        uint64_t count = perft(4, &c);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        std::cout << count << " nodes searched in " << duration.count() << " ms\n";
            std::cout << ((double) count / (double) duration.count() * 1000.0) << " nps" << std::endl;


        REQUIRE(count == 197281);
    }

    SECTION("fen parser works correctly") {
        Board a;
        Board b("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        REQUIRE(a.all_per_side == b.all_per_side);
        REQUIRE(a.get_moves().size() == b.get_moves().size());
    }
}
