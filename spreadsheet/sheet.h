#pragma once

#include "cell.h"
#include "common.h"

#include <map>
#include <memory>
#include <ostream>
#include <set>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;
    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    void ClearCell(Position pos) override;
    Size GetPrintableSize() const override;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    Cell* GetOrCreateCell(Position pos);
    bool HasCycle(Position pos, const std::vector<Position>& new_deps) const;
    void InvalidateDependents(Cell* cell);

    std::map<Position, std::unique_ptr<Cell>> cells_;
    std::set<Position> content_cells_;
    std::multiset<int> rows_;
    std::multiset<int> cols_;
};
