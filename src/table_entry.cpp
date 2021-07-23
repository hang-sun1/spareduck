#include "table_entry.h"

TableEntry::TableEntry(uint64_t hash, Move move, NodeType type, uint8_t depth) {
    this->hash = hash;
    this->move = move;
    this->type = type;
    this->depth = depth;
}

uint64_t TableEntry::get_hash() const {
    return hash;
}

Move TableEntry::get_move() const {
    return move;
}

NodeType TableEntry::get_type() const {
    return type;
}

int8_t TableEntry::get_depth() const {
    return depth;
}
