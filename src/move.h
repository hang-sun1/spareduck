#pragma once

#include <cstdint>
#include <array>
#include <string>

enum class MoveType : uint16_t {
    QUIET = 0,
    DOUBLE_PAWN_PUSH = 1,
    CAPTURE = 2,
    SHORT_CASTLES = 3,
    LONG_CASTLES = 4,
    PROMOTE_TO_KNIGHT = 5,
    PROMOTE_TO_QUEEN = 6,
    PROMOTE_TO_ROOK = 7,
    PROMOTE_TO_BISHOP = 8,
    CAPTURE_AND_PROMOTE_TO_KNIGHT = 9,
    CAPTURE_AND_PROMOTE_TO_QUEEN = 10,
    CAPTURE_AND_PROMOTE_TO_ROOK = 11,
    CAPTURE_AND_PROMOTE_TO_BISHOP = 12,
};

class Move {
   public:
    Move();
    // using enum MoveType;
    Move(uint16_t from, uint16_t to, MoveType type);

    // Move from algebraic
    Move(std::string from, std::string to);

    uint16_t origin_square();

    uint16_t destination_square();

    MoveType type();

    std::string origin_square_algebraic();

    std::string destination_square_algebraic();

    std::array<uint16_t, 2> origin_square_cartesian();

    std::array<uint16_t, 2> destination_square_cartesian();

    bool is_capture();

   private:
    uint16_t move_repr;
};
