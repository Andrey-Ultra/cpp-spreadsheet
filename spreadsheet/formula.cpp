#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError::Category category) {
    switch (category) {
        case FormulaError::Category::Ref:        return output << "#REF!";
        case FormulaError::Category::Value:      return output << "#VALUE!";
        case FormulaError::Category::Arithmetic: return output << "#ARITHM!";
    }
    return output;
}

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
    return output << fe.GetCategory();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
    try : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError& e) {
            return e;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;
        for (const auto& cell : ast_.GetCells()) {
            if (result.empty() || result.back() != cell) {
                result.push_back(cell);
            }
        }
        return result;
    }

private:
    FormulaAST ast_;
};
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
