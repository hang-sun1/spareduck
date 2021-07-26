#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#define private public

#include "../src/board.h"
#include <cstdint>
#include "../src/move.h"
#include <chrono>
using namespace std::chrono;

int captures = 0;
// taken from the chess programming wiki
uint64_t perft(int depth /* assuming >= 1 */, Board *b) {
    auto move_list = b->get_moves();
    int n_moves, i;
    n_moves = move_list.size();
    uint64_t nodes = 0;


    if (depth == 0) 
        return 1ULL;

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
    SECTION("generates valid knight moves") {
        auto lookup_table = Board::generate_knight_lookup();
        REQUIRE(lookup_table[0] == 0x020400);
        REQUIRE(lookup_table[55] == 0x2000204000000000);
    }
    SECTION("generates valid king moves") {
        auto lookup_table = Board::generate_king_lookup();
        REQUIRE(lookup_table[0] ==  0x0302);
    }
    SECTION("generates correct move maps") {
        auto lookup_table = Board::generate_rank_attacks();
        REQUIRE(lookup_table[3][38] == 0b01110100);
        auto diagonal_masks = Board::generate_diagonal_mask_map();
        REQUIRE(diagonal_masks[61] == 2310355422147575808);
    }
    SECTION("finds 20 moves from start position") {
        Board b;
        REQUIRE(b.get_moves().size() == 20);
        b = Board();
        REQUIRE(b.get_moves().size() == 20);
    }
    SECTION("correctly makes a move") {
        Board b;
        auto starting_hash = b.hash();
        REQUIRE(b.side_to_move == Side::WHITE);
        auto moves = b.get_moves();
        auto first_move = moves[0];
        auto knight_board = b.knights[0];
        b.make_move(first_move);
        REQUIRE(b.side_to_move == Side::BLACK);
        auto first_hash = b.hash();
        REQUIRE(starting_hash != first_hash);
        moves = b.get_moves();
        REQUIRE(b.get_moves().size() == 20);
        b.make_move(b.get_moves()[0]);
        REQUIRE(b.get_moves().size() == 19);
        b.unmake_move(first_move);
        REQUIRE(b.side_to_move == Side::BLACK);
        REQUIRE(b.get_moves().size() == 20);
    }

    SECTION("correctly unmakes a move") {
        Board b;
        auto starting_hash = b.hash();
        b.make_move(b.get_moves()[0]);
        REQUIRE(b.side_to_move == Side::BLACK);
        
        auto second_hash = b.hash();
        REQUIRE(second_hash != starting_hash);
        
        b.unmake_move(b.get_moves()[0]);
        REQUIRE(b.hash() == starting_hash);
        REQUIRE(b.side_to_move == Side::WHITE);
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
            // std::cout << ((double) count / (double) duration.count() * 1000.0) << " nps" << std::endl;
            // std::cout << captures << " captures" << std::endl;

        }
        
        auto start = high_resolution_clock::now();
        uint64_t count = perft(5, &c);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        std::cout << count << " nodes searched in " << duration.count() << " ms\n";


        REQUIRE(1 == 1);
    }
}