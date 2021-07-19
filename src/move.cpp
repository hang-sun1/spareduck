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
