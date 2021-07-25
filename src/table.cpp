#include "table.h"

#define TABLE_LENGTH (2 << 24)

Table::Table() {
    table.reserve(TABLE_LENGTH);
}

void Table::put(const Board position, const TableEntry entry) {
    uint32_t hash_index = position.hash() & (TABLE_LENGTH - 1);
    table[hash_index] = entry;
}

void Table::put(Board position, Move move, int16_t eval, NodeType type, uint8_t depth) {
    uint64_t hash = position.hash();
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    uint32_t hash_upper = (hash - hash_index) >> 8;
    table[hash_index] = TableEntry(hash_upper, move, eval, type, depth);
}

std::optional<TableEntry> Table::get(const Board position) {
    const uint64_t hash = position.hash();
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    uint32_t hash_upper = (hash - hash_index) >> 8;
    if (table[hash_index].get_upper_hash() == hash_upper) {
        return table[hash_index];
    }
    return {};
}

std::optional<TableEntry> Table::get(uint64_t hash) {
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    uint32_t hash_upper = (hash - hash_index) >> 8;
    if (table[hash_index].get_upper_hash() == hash_upper) {
        return table[hash_index];
    }
    return {};
}
