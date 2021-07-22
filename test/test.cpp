#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#define private public

#include "../src/board.h"
#include <cstdint>


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
        auto moves = b.get_moves();
        auto first_move = moves[0];
        auto knight_board = b.knights[0];
        b.make_move(first_move);
        moves = b.get_moves();
        REQUIRE(b.get_moves().size() == 20);
        b.make_move(b.get_moves()[0]);
        REQUIRE(b.get_moves().size() == 20);
    }
}