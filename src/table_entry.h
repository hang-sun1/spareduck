#pragma once

#include "move.h"

typedef enum NodeType {
    exact = 0,
    upper,
    lower
} NodeType;

class TableEntry {
   public:
    TableEntry(uint64_t hash_, Move move_, NodeType type_, uint8_t depth_);

    uint64_t get_hash() const;
    Move get_move() const;
    NodeType get_type() const;
    int8_t get_depth() const;

   private:
    uint64_t hash;
    Move move;
    NodeType type;
    int8_t depth;
};