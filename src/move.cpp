#include "move.h"

#include <cstdint>

Move::Move(uint16_t from, uint16_t to, MoveType type) {
    move_repr = (from << 10) | (to << 4) | static_cast<uint16_t>(type);
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