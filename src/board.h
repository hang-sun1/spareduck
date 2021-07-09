#include <cstdint>
#include <array>
#include <memory>

using std::uint64_t;

class Board {
private:
    std::array<uint64_t, 2> knights;
    std::array<uint64_t, 2> bishops;
    std::array<uint64_t, 2> rooks;
    std::array<uint64_t, 2> queens;
    std::array<uint64_t, 2> kings;
    std::array<uint64_t, 2> all_per_side;
    std::array<std::array<uint64_t, 64>, 8> rank_attack_lookup;
    uint64_t generate_rook_moves(uint8_t square);
    uint64_t generate_bishop_moves(uint8_t square);
public:
    // initizlizes a board in the starting position
    Board();
    // The below methods generate lookup tables that allow efficient determination of available moves
    // for the various pieces, as well as helper functions that assist in this
    static std::array<uint64_t, 64> generate_knight_lookup();
    static std::array<uint64_t, 64> generate_king_lookup();
    static std::array<std::array<uint64_t, 64>, 2> generate_pawn_move_lookup();
    static std::array<std::array<uint64_t, 64>, 2> generate_pawn_attack_lookup();
    static std::array<std::array<uint64_t, 64>, 8> generate_rank_attacks();
};