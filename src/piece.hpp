#pragma once

#include <cstdint>

enum Piece: std::size_t {
    PAWN = 0,
    ROOK = 1,
    KNIGHT = 2,
    BISHOP = 3,
    QUEEN = 4,
    KING = 5,
};