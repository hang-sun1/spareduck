#pragma once

#include <optional>
#include <vector>
#include <stdlib.h>

#include "board.hpp"
#include "move.hpp"
#include "table_entry.hpp"

class Table {
   public:
    Table();
    void put(const Board& position, const TableEntry entry);
    void put(const Board& position, Move move, int16_t eval, NodeType type, uint8_t depth);
    std::optional<TableEntry> get(const Board& position);
    std::optional<TableEntry> get(const uint64_t hash);
    std::vector<Move> get_variation(Board& position);
    void clear();

   private:
    std::vector<TableEntry> table;
};
