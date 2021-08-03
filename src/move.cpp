#include "move.hpp"

Move::Move(uint16_t from, uint16_t to, MoveType type) {
    move_repr = (from << 10) | (to << 4) | static_cast<uint16_t>(type);
    this->t = type;
}

// New move from algebraic notation.
Move::Move(std::string from, std::string to) {
    uint16_t from_square = (from[0] - 'a') | 7 | (from[1] - '1') << 3;
    uint16_t to_square = (to[0] - 'a') | 7 | (from[1] - '1') << 3;
    MoveType type = MoveType::QUIET;

    move_repr = (from_square << 10) | (to_square << 4) | static_cast<uint16_t>(type);
}

Move::Move() {}

std::ostream& operator<<(std::ostream& strm, const Move& move) {
    return strm << " (" << move.origin_square_algebraic() << ", " << move.destination_square_algebraic() << ") ";
}

bool operator==(const Move& lhs, const Move& rhs) {
    return lhs.get_move_repr() == rhs.get_move_repr();
}

uint16_t Move::get_move_repr() const {
    return this->move_repr;
}

uint16_t Move::origin_square() const {
    return move_repr >> 10;
}

uint16_t Move::destination_square() const {
    return (move_repr >> 4) & 63;
}

MoveType Move::type() const {
    return t;
    // return static_cast<MoveType>(move_repr & 0xf);
}

bool Move::is_capture() const {
    return static_cast<uint16_t>(t) == 2 || static_cast<uint16_t>(t) > 8;
}

bool Move::is_promotion() const {
    auto as_integer = static_cast<uint16_t>(t);
    return as_integer >= 5 && as_integer <= 12;
}

std::string Move::origin_square_algebraic() const{
    uint16_t origin = origin_square();
    char algebraic_move[3] = {(char)((origin & 7) + 'a'),
                              (char)((origin >> 3) + '1'),
                              '\0'};
    return std::string(algebraic_move);
}

std::string Move::destination_square_algebraic() const {
    uint16_t destination = destination_square();
    char algebraic_move[3] = {(char)((destination & 7) + 'a'),
                              (char)((destination >> 3) + '1'),
                              '\0'};
    return std::string(algebraic_move);
}

std::array<uint16_t, 2> Move::origin_square_cartesian() const {
    uint16_t origin = origin_square();
    return {static_cast<uint16_t>(origin & 7), static_cast<uint16_t>(origin >> 3)};
}

std::array<uint16_t, 2> Move::destination_square_cartesian()const {
    uint16_t destination = destination_square();
    return {static_cast<uint16_t>(destination & 7), static_cast<uint16_t>(destination >> 3)};
}
