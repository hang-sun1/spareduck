#pragma once

#include <vector>

#include "move.h"
#include "board.h"
#include "side.h"

struct History {
    Side side_to_move;
    std::array<uint64_t, 2> all_per_side;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> pawns;
    std::array<uint64_t, 2> kings;
    std::array<uint64_t, 2> queen_defends;
    std::array<uint64_t, 2> rook_defends;
    std::array<uint64_t, 2> knight_defends;
    std::array<uint64_t, 2> bishop_defends;
    std::array<uint64_t, 2> pawn_defends;
    std::array<uint64_t, 2> king_defends;
    std::array<uint64_t, 2> defense_maps;
    std::array<uint64_t, 2> attack_maps;
    std::array<bool, 2> short_castle_rights;
    std::array<bool, 2> long_castle_rights;
    std::vector<Move> moves;
    uint8_t en_passant_target;
};