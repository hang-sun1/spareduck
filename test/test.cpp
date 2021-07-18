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
        std::cout << b.defense_maps[0];
        REQUIRE(b.get_moves().size() == 20);
    }
}