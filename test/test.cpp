#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#define private public

#include "../src/board.h"
#include <cstdint>


TEST_CASE("proper moves are generated", "[board]") {
    Board b;
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
    }
    SECTION("finds 20 moves from start position") {
        REQUIRE(b.get_moves().size() == 20);
    }
    SECTION("correctly makes a move") {
        auto moves = b.get_moves();
        auto first_move = moves[0];
        // for (auto &m: moves) {
        //     std::cout << m.origin_square() << "-" << m.destination_square() << std::endl;
        // }
        b.make_move(first_move);
        REQUIRE(b.get_moves().size() == 20);
        b.make_move(b.get_moves()[0]);
        std::cout << b.all_per_side[0] << std::endl;
        REQUIRE(b.get_moves().size() == 20);
    }
}