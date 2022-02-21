#include "table_entry.hpp"

/* 
    Entry in the transposition table:
    https://www.chessprogramming.org/Transposition_Table
*/

TableEntry::TableEntry() {}

TableEntry::TableEntry(uint32_t hash, Move move, int32_t eval, NodeType type, uint8_t depth) {
  this->hash_ = hash;
  this->move_ = move;
  this->eval_ = eval;
  this->type_ = type;
  this->depth_ = depth;
}

std::ostream& operator<<(std::ostream& strm, const TableEntry& entry) {
  return strm << " [" << entry.get_move() << ", " << entry.get_eval() << ", " << entry.get_depth() << "] ";
}

uint32_t TableEntry::get_upper_hash() const {
  return hash_;
}

Move TableEntry::get_move() const {
  return move_;
}

int32_t TableEntry::get_eval() const {
  return eval_;
}

NodeType TableEntry::get_type() const {
  return type_;
}

int8_t TableEntry::get_depth() const {
  return depth_;
}
