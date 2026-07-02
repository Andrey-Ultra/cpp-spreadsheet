#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    enum class Type {
        Empty,
        Text,
        Formula,
    };

    void InvalidateCache();

    SheetInterface& sheet_;
    std::string text_;
    Type type_ = Type::Empty;
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<Value> cache_;
    std::unordered_set<Cell*> dependents_;

    friend class Sheet;
};
