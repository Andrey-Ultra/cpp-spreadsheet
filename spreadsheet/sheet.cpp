#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <stack>

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

Sheet::~Sheet() = default;

Cell* Sheet::GetOrCreateCell(Position pos) {
    auto it = cells_.find(pos);
    if (it != cells_.end()) return it->second.get();
    cells_[pos] = std::make_unique<Cell>(*this);
    return cells_[pos].get();
}

void Sheet::InvalidateDependents(Cell* cell) {
    for (auto* dep : cell->dependents_) {
        dep->InvalidateCache();
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position");

    std::vector<Position> new_deps;
    if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        auto formula = ParseFormula(text.substr(1));
        new_deps = formula->GetReferencedCells();
    }

    if (HasCycle(pos, new_deps)) {
        throw CircularDependencyException("circular dependency");
    }

    Cell* cell = GetOrCreateCell(pos);

    for (const auto& dep : cell->GetReferencedCells()) {
        auto dep_it = cells_.find(dep);
        if (dep_it != cells_.end()) {
            dep_it->second->dependents_.erase(cell);
        }
    }

    cell->Set(text);

    for (const auto& dep : new_deps) {
        GetOrCreateCell(dep)->dependents_.insert(cell);
    }

    InvalidateDependents(cell);

    if (!content_cells_.count(pos)) {
        content_cells_.insert(pos);
        rows_.insert(pos.row);
        cols_.insert(pos.col);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position");
    auto it = cells_.find(pos);
    if (it == cells_.end()) return nullptr;
    if (!content_cells_.count(pos) && it->second->dependents_.empty()) return nullptr;
    return it->second.get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position");
    auto it = cells_.find(pos);
    if (it == cells_.end()) return nullptr;
    if (!content_cells_.count(pos) && it->second->dependents_.empty()) return nullptr;
    return it->second.get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position");

    if (!content_cells_.count(pos)) return;

    Cell* cell = cells_[pos].get();

    for (const auto& dep : cell->GetReferencedCells()) {
        auto dep_it = cells_.find(dep);
        if (dep_it != cells_.end()) {
            dep_it->second->dependents_.erase(cell);
        }
    }

    content_cells_.erase(pos);
    rows_.erase(rows_.find(pos.row));
    cols_.erase(cols_.find(pos.col));

    InvalidateDependents(cell);
    cell->Clear();

    if (cell->dependents_.empty()) {
        cells_.erase(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    if (content_cells_.empty()) return {0, 0};
    return {*rows_.rbegin() + 1, *cols_.rbegin() + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    if (content_cells_.empty()) return;
    int max_col = *cols_.rbegin();
    int max_row = *rows_.rbegin();

    for (int y = 0; y <= max_row; ++y) {
        for (int x = 0; x <= max_col; ++x) {
            if (content_cells_.count(Position{y, x})) {
                std::visit([&output](const auto& val) { output << val; },
                           cells_.at(Position{y, x})->GetValue());
            }
            if (x != max_col) output << '\t';
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    if (content_cells_.empty()) return;
    int max_col = *cols_.rbegin();
    int max_row = *rows_.rbegin();

    for (int y = 0; y <= max_row; ++y) {
        for (int x = 0; x <= max_col; ++x) {
            if (content_cells_.count(Position{y, x})) {
                output << cells_.at(Position{y, x})->GetText();
            }
            if (x != max_col) output << '\t';
        }
        output << '\n';
    }
}

bool Sheet::HasCycle(Position pos, const std::vector<Position>& new_deps) const {
    std::set<Position> visited;
    std::stack<Position> stack;

    for (const auto& dep : new_deps) {
        stack.push(dep);
    }

    while (!stack.empty()) {
        Position current = stack.top();
        stack.pop();

        if (current == pos) return true;
        if (visited.count(current)) continue;
        visited.insert(current);

        auto it = cells_.find(current);
        if (it != cells_.end()) {
            for (const auto& dep : it->second->GetReferencedCells()) {
                stack.push(dep);
            }
        }
    }
    return false;
}
