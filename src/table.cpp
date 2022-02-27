#include "table.hpp"

#include <iostream>

#define TABLE_BITS 25
#define TABLE_LENGTH (1 << TABLE_BITS)
#define INDEX_MASK (TABLE_LENGTH - 1)

/*
    Position / transposition table;
    https://www.chessprogramming.org/Transposition_Table
*/

Table::Table() {
    table.reserve(TABLE_LENGTH);
    for (int i = 0; i < TABLE_LENGTH; i++) {
        table[i] = TableEntry(0, {}, 0, {}, 0);
    }
    this->g_seed = 1223; // Arbitrary small-ish prime seed
}

void Table::put(const Board& position, const TableEntry entry) {
    uint32_t hash_index = position.get_hash() & INDEX_MASK;
    table[hash_index] = entry;
    return;
}

void Table::put(const Board& position, Move move, int32_t eval, NodeType type, uint8_t depth) {
    uint64_t hash = position.get_hash();
    uint32_t hash_index = hash & INDEX_MASK;
    uint32_t hash_upper = hash >> TABLE_BITS;
    //table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
    //return; // Short circuit random replace
    int rand = fastrand();
    if (table[hash_index].get_upper_hash() > 0) {
        // if (rand > 8000) {
        //     table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
        // } else if (depth > table[hash_index].get_depth()) {
        //     table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
        // }
        if (depth > table[hash_index].get_depth()) {
            table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
        }
    } else {
        table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
    }
    return;
}

std::optional<TableEntry> Table::get(const Board& position) {
    const uint64_t hash = position.get_hash();
    uint32_t hash_index = hash & INDEX_MASK;
    uint32_t hash_upper = hash >> TABLE_BITS;
    if (table[hash_index].get_upper_hash() == hash_upper) {
        return table[hash_index];
    }
    return {};
}

std::optional<TableEntry> Table::get(uint64_t hash) {
    uint32_t hash_index = hash & INDEX_MASK;
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
        std::cout << node->get_move() << " ";
        variation.push_back(node->get_move());
        position.make_move(node->get_move());
        node = this->get(position);
    }
    std::cout << std::endl;

    while (count--) {
        position.unmake_move(Move());
    }

    return variation;
}

// does this work??
void Table::clear() {
    for (int i = 0; i < TABLE_LENGTH; i++) {
        // table[i] = TableEntry();
        table[i] = TableEntry(0, {}, 0, {}, 0);
    }
}

int Table::fastrand() {
    this->g_seed = (214013 * this->g_seed + 2531011);
    return (this->g_seed >> 16) & 0x7FFF;
}