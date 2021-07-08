#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "../src/board.h"
#include <cstdint>


TEST_CASE("proper moves are generated", "[board]") {
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
    // SECTION("generates valid rank moves") {
    //     auto lookup_table = Board::generate_rank_attack_lookup();
    //     REQUIRE(lookup_table[0b01001100][3] == 0b01110100);
    // }
    // SECTION("generates valid file moves") {
    //     auto lookup_table = Board::generate_file_attack_lookup();
    //     REQUIRE(lookup_table[0b11011001][32] ==  0x0001010001000000ULL);
    // }
    // SECTION("generates valid diagonal moves") {
    //     auto lookup_table = Board::generate_diagonal_attack_lookup();
    //     REQUIRE(lookup_table[1][0] ==  0x8040201008040200ULL);
    //     REQUIRE(lookup_table[0x40][6] == 0x8000);
    //     REQUIRE(lookup_table[1][48] == 0x0200000000000000);
    //     REQUIRE(lookup_table[0b1111111][53] == 0x4000100000000000);
    // }
    // SECTION("generates valid antidiagonal moves") {
    //     auto lookup_table = Board::generate_antidiagonal_attack_lookup();
    //     REQUIRE(lookup_table[128][7] == 0x0102040810204000);
    //     REQUIRE(lookup_table[0b11111110][22] == 0x20008000);
    //     REQUIRE(lookup_table[0b00011111][18] == 0x02000800);
    //     REQUIRE(lookup_table[0b00010010][26] == 0x0200081000);
    // }

    // SECTION("gives correct number of queen moves") {
    //     auto rank_table = Board::generate_rank_attack_lookup();
    //     auto file_table = Board::generate_file_attack_lookup();
    //     auto diag_table = Board::generate_diagonal_attack_lookup();
    //     auto adiag_table = Board::generate_antidiagonal_attack_lookup();
    //     // assume queen on d4
    //     auto rank_attack = rank_table[0b00001000][27];
    //     auto file_attack = file_table[0b00001000][27];
    //     auto diag_attack = diag_table[0b00001000][27];
    //     auto adiag_attack = adiag_table[0b00001000][27];
    //     auto moves = std::popcount(rank_attack | file_attack | diag_attack | adiag_attack);
    //     REQUIRE(moves == 27);
    // }
}