#pragma once

#include <ostream>

#include "move.hpp"

typedef enum NodeType {
    EXACT = 0,
    UPPER,
    LOWER
} NodeType;

class TableEntry {
   public:
    TableEntry(uint32_t hash, Move move, int16_t eval, NodeType type, uint8_t depth);

    // to string
    friend std::ostream& operator<<(std::ostream&, const TableEntry&);

    // getters
    uint32_t get_upper_hash() const;
    Move get_move() const;
    int16_t get_eval() const;
    NodeType get_type() const;
    int8_t get_depth() const;

   private:
    uint32_t hash;
    Move move;
    int16_t eval;
    NodeType type;
    int8_t depth;
};