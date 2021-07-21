#include "move.h"

Move::Move(uint16_t from, uint16_t to, MoveType type) {
    move_repr = (from << 10) | (to << 4) | static_cast<uint16_t>(type);
}

// New move from algebraic notation.
Move::Move(std::string from, std::string to) {
    uint16_t from_square = (from[0] - 'a') | 7 | (from[1] - '1') << 3;
    uint16_t to_square = (to[0] - 'a') | 7 | (from[1] - '1') << 3;
    MoveType type = MoveType::QUIET;

    move_repr = (from_square << 10) | (to_square << 4) | static_cast<uint16_t>(type);
}

uint16_t Move::origin_square() {
    return move_repr >> 10;
}

uint16_t Move::destination_square() {
    return (move_repr >> 4) & 63;
}

MoveType Move::type() {
    return static_cast<MoveType>(move_repr & 0xff);
}

std::string Move::origin_square_algebraic() {
    uint16_t origin = origin_square();
    char algebraic_move[3] = {(char)((origin & 7) + 'a'),
                              (char)((origin >> 3) + '1'),
                              '\0'};
    return std::string(algebraic_move);
}

std::string Move::destination_square_algebraic() {
    uint16_t destination = destination_square();
    char algebraic_move[3] = {(char)((destination & 7) + 'a'),
                              (char)((destination >> 3) + '1'),
                              '\0'};
    return std::string(algebraic_move);
}

std::array<uint16_t, 2> Move::origin_square_cartesian() {
    uint16_t origin = origin_square();
    return {static_cast<uint16_t>(origin & 7), static_cast<uint16_t>(origin >> 3)};
}

std::array<uint16_t, 2> Move::destination_square_cartesian() {
    uint16_t destination = destination_square();
    return {static_cast<uint16_t>(destination & 7), static_cast<uint16_t>(destination >> 3)};
}
