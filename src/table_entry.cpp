#include "table_entry.h"

TableEntry::TableEntry(uint64_t hash_, Move move_, NodeType type_, uint8_t depth_) {
    hash = hash_;
    move = move_;
    type = type_;
    depth = depth_;
};

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
