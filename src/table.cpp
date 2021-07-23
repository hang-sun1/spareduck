#include "table.h"

#define TABLE_LENGTH (2 << 24)

Table::Table() {
    table.reserve(TABLE_LENGTH);
}

void Table::put(const Board position, const TableEntry entry) {
    uint32_t hash_index = position.hash() & (TABLE_LENGTH - 1);
    table[hash_index] = entry;
}

TableEntry Table::get(const Board position) {
    const uint64_t hash = position.hash();
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    if (table[hash_index].get_hash() == hash) {
        return table[hash_index];
    }
    return table[0];  // bad fix
}

TableEntry Table::get(uint64_t hash) {
    uint32_t hash_index = hash & (TABLE_LENGTH - 1);
    if (table[hash_index].get_hash() == hash) {
        return table[hash_index];
    }
    return table[0];  // bad fix
}
