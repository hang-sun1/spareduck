#pragma once

#include <optional>
#include <vector>

#include "board.h"
#include "move.h"
#include "table_entry.h"

class Table {
   public:
    Table();
    void put(const Board position, const TableEntry entry);
    void put(Board position, Move move, int16_t eval, NodeType type, uint8_t depth);
    std::optional<TableEntry> get(const Board position);
    std::optional<TableEntry> get(const uint64_t hash);

   private:
    std::vector<TableEntry> table;
};
