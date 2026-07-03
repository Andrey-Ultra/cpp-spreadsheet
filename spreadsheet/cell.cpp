#include "cell.h"

#include <cassert>
#include <sstream>
#include <string>

Cell::Cell(SheetInterface& sheet) : sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text_ == text) return;

    cache_.reset();
    text_ = text;
    formula_.reset();

    if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        type_ = Type::Formula;
        formula_ = ParseFormula(text.substr(1));
    } else if (text.empty()) {
        type_ = Type::Empty;
    } else {
        type_ = Type::Text;
    }
}

void Cell::Clear() {
    cache_.reset();
    text_.clear();
    type_ = Type::Empty;
    formula_.reset();
}

Cell::Value Cell::GetValue() const {
    if (cache_) return *cache_;

    Value result;
    switch (type_) {
        case Type::Empty:
            result = std::string{};
            break;
        case Type::Text:
            result = (!text_.empty() && text_[0] == ESCAPE_SIGN) ? text_.substr(1) : text_;
            break;
        case Type::Formula: {
            auto eval = formula_->Evaluate(sheet_);
            if (std::holds_alternative<double>(eval)) {
                result = std::get<double>(eval);
            } else {
                result = std::get<FormulaError>(eval);
            }
            break;
        }
    }
    cache_ = result;
    return result;
}

std::string Cell::GetText() const {
    if (type_ == Type::Formula) {
        return std::string(1, FORMULA_SIGN) + formula_->GetExpression();
    }
    return text_;
}

std::vector<Position> Cell::GetReferencedCells() const {
    if (!formula_) return {};
    return formula_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !dependents_.empty();
}

void Cell::InvalidateCache() {
    if (!cache_) return;
    cache_.reset();
    for (auto* dep : dependents_) {
        dep->InvalidateCache();
    }
}
