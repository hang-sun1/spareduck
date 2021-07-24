#pragma once

#include "move.h"

typedef enum NodeType {
    exact = 0,
    upper,
    lower
} NodeType;

class TableEntry {
   public:
    TableEntry(uint32_t hash, Move move, NodeType type, uint8_t depth);

    uint32_t get_upper_hash() const;
    Move get_move() const;
    NodeType get_type() const;
    int8_t get_depth() const;

   private:
    uint32_t hash;
    Move move;
    NodeType type;
    int8_t depth;
};