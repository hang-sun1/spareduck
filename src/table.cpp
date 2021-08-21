#include "table.hpp"

#include <iostream>

#define TABLE_BITS 25
#define TABLE_LENGTH (1 << TABLE_BITS)

/*
    Position / transposition table;
    https://www.chessprogramming.org/Transposition_Table
*/

Table::Table() {
    table.reserve(TABLE_LENGTH);
    for (int i = 0; i < TABLE_LENGTH; i++) {
        table[i] = TableEntry(0, {}, 0, {}, 0);
    }
}

void Table::put(const Board& position, const TableEntry entry) {
    uint32_t hash_index = position.get_hash() & (TABLE_LENGTH - 1);
    table[hash_index] = entry;
}

void Table::put(const Board& position, Move move, int16_t eval, NodeType type, uint8_t depth) {
    uint64_t hash = position.get_hash();
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    uint32_t hash_upper = hash >> TABLE_BITS;
    table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
}

std::optional<TableEntry> Table::get(const Board& position) {
    const uint64_t hash = position.get_hash();
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    uint32_t hash_upper = hash >> TABLE_BITS;
    if (table[hash_index].get_upper_hash() == hash_upper) {
        return table[hash_index];
    }
    return {};
}

std::optional<TableEntry> Table::get(uint64_t hash) {
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    uint32_t hash_upper = hash >> TABLE_BITS;
    if (table[hash_index].get_upper_hash() == hash_upper) {
        return table[hash_index];
    }
    return {};
}

// Gets the principal variation of a position
// Currently out of use due to frequency of infinite looping
std::vector<Move> Table::get_variation(Board& position) {
    std::vector<Move> variation;
    uint8_t count = 0;

    std::optional<TableEntry> node = this->get(position);
    while (node && count < 5) {  // RANDOMLY CHOSEN PV DEPTH CUTOFF
        count++;
        std::cout << node->get_move() << std::endl;
        variation.push_back(node->get_move());
        position.make_move(node->get_move());
        node = this->get(position);
    }

    while (count--) {
        position.unmake_move(Move());
    }

    return variation;
}

// does this work??
void Table::clear() {
    for (int i = 0; i < TABLE_LENGTH; i++) {
        table[i] = TableEntry(0, {}, 0, {}, 0);
    }
}
