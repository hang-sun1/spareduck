#include "table_entry.hpp"

/* 
    Entry in the transposition table:
    https://www.chessprogramming.org/Transposition_Table
*/

TableEntry::TableEntry(uint32_t hash, Move move, int16_t eval, NodeType type, uint8_t depth) {
    this->hash = hash;
    this->move = move;
    this->eval = eval;
    this->type = type;
    this->depth = depth;
}

std::ostream& operator<<(std::ostream& strm, const TableEntry& entry) {
    return strm << " [" << entry.get_move() << ", " << entry.get_eval() << ", " << entry.get_depth() << "] ";
}

uint32_t TableEntry::get_upper_hash() const {
    return hash;
}

Move TableEntry::get_move() const {
    return move;
}

int16_t TableEntry::get_eval() const {
    return eval;
}

NodeType TableEntry::get_type() const {
    return type;
}

int8_t TableEntry::get_depth() const {
    return depth;
}
