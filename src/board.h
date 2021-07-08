#include <cstdint>
#include <array>

using std::uint64_t;

class Board {
private:
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> kings;
    uint64_t all;

    uint64_t get_rank_moveboard(uint8_t square, uint64_t occ);

public:
    // The below methods generate lookup tables that allow efficient determination of available moves
    // for the various pieces, as well as helper functions that assist in this
    static std::array<uint64_t, 64> generate_knight_lookup();
    static std::array<uint64_t, 64> generate_king_lookup();
    static std::array<uint64_t, 64> generate_pawn_lookup();
    static std::array<std::array<uint64_t, 64>, 8> generate_rank_attacks();
};