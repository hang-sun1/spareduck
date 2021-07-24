#include "table_entry.h"

TableEntry::TableEntry(uint32_t hash, Move move, NodeType type, uint8_t depth) {
    this->hash = hash;
    this->move = move;
    this->type = type;
    this->depth = depth;
}

uint32_t TableEntry::get_upper_hash() const {
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
