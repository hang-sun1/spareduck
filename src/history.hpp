#pragma once

#include <vector>

#include "move.hpp"
#include "side.hpp"

struct History {
    Side side_to_move;
    std::array<uint64_t, 2> all_per_side;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> pawns;
    std::array<uint64_t, 2> kings;
    std::array<uint64_t, 2> attack_maps;
    std::array<bool, 2> short_castle_rights;
    std::array<bool, 2> long_castle_rights;
    std::vector<Move> moves;
    uint8_t en_passant_target;
    uint64_t hash;
};