#pragma once

#include <optional>
#include <vector>

#include "board.h"
#include "table_entry.h"

class Table {
   public:
    Table();
    void put(const Board position, const TableEntry entry);
    std::optional<TableEntry> get(const Board position);
    std::optional<TableEntry> get(const uint64_t hash);

   private:
    std::vector<TableEntry> table;
};