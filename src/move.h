#pragma once

#include <array>
#include <cstdint>
#include <ostream>
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
    EN_PASSANT = 13,
};

class Move {
   public:
    Move();
    // using enum MoveType;
    Move(uint16_t from, uint16_t to, MoveType type);

    // Move from algebraic
    Move(std::string from, std::string to);

    friend std::ostream& operator<<(std::ostream&, const Move&);

    friend bool operator==(const Move& lhs, const Move& rhs);

    uint16_t get_move_repr() const;

    uint16_t origin_square() const;

    uint16_t destination_square() const;

    MoveType type() const;

    bool is_capture() const;

    std::string origin_square_algebraic() const;

    std::string destination_square_algebraic() const;

    std::array<uint16_t, 2> origin_square_cartesian() const;

    std::array<uint16_t, 2> destination_square_cartesian() const;

   private:
    uint16_t move_repr;
    MoveType t;
};
