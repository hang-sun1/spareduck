#pragma once

#include <cstddef>
#include <cstdint>

enum Piece: std::size_t {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
};

int piece_to_value(Piece p);