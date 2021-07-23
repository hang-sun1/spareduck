#pragma once

#include <vector>

#include "board.h"
#include "table_entry.h"

class Table {
   public:
    Table();
    void put(const Board position, const TableEntry entry);
    TableEntry get(const Board position);
    TableEntry get(const uint64_t hash);

   private:
    std::vector<TableEntry> table;
};