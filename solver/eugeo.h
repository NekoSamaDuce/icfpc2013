#ifndef EUGEO_H_
#define EUGEO_H_

// Expression lister for Alice.
//
// ListFoldBody(max_size = default-is-9):
//     Returns |table|, where table[s] is vector<shared_ptr<Expr>> of size |s| bodies.
//     E.g., table[3] = {(or y z), (and y z), ...}
//
// EvalFoldBody(e, x, value, init):
//     Evaluates e with the given arguments.
//

#include <algorithm>
#include <vector>
#include "expr.h"
#include "simplify.h"

namespace icfpc {

std::vector<std::shared_ptr<Expr> > ListExprInternal(
    const std::vector<std::vector<std::shared_ptr<Expr> > >& table,
    std::size_t depth) {
  if (depth == 1)
    return {
      ConstantExpr::CreateZero(),
      ConstantExpr::CreateOne(),
      IdExpr::CreateY(),
      IdExpr::CreateZ(),
    };

  std::vector<std::shared_ptr<Expr> > result;

  if (depth >= 2)
    for (auto& e : table[depth - 1]) {
      result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::NOT, e));
      result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHL1, e));
      result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR1, e));
      result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR4, e));
      result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR16, e));
    }
  
  if (depth >= 3)
    for (std::size_t i = 1; i < depth - 1; ++i) if (i <= depth - 1 - i)
      for (auto& lhs : table[i])
        for (auto& rhs : table[depth - 1 - i]) {
          result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::AND, lhs, rhs));
          result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::OR, lhs, rhs));
          result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::XOR, lhs, rhs));
          result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS, lhs, rhs));
        }

  if (depth >= 4)
    for (size_t i = 1; i < depth - 2; ++i)
      for (size_t j = 1; j < depth - i - 1; ++j)
        for (auto& e_cond : table[i])
          for (auto& e_then : table[j])
            for (auto& e_else : table[depth - 1 - i - j]) {
              result.push_back(If0Expr::Create(e_cond, e_then, e_else));
            }

  return result;
}

std::vector<std::vector<std::shared_ptr<Expr> > > PreComputeTable(std::size_t depth) {
  std::vector<std::vector<std::shared_ptr<Expr> > > table(1);
  std::vector<std::vector<std::shared_ptr<Expr> > > filtered_table(1);
  std::set<std::string> already_known;

  for (size_t d = 1; d <= depth; ++d) {
    auto es = ListExprInternal(table, d);

    table.emplace_back();
    for (auto& e: es)
      if (already_known.insert(Simplify(e)->ToString()).second)
        table.back().push_back(e);

    filtered_table.emplace_back();
    for (auto& e: es) {
      if (!e->has_x() && !e->has_y() && !e->has_z()) continue;  // Constant is always useless.
      if (!e->has_y()) continue;  // No y may useless: BODY[z/INIT] has same effect w/o fold.
      if (!e->has_z()) continue;  // No z may be: BODY[y/(VALUE>>16>>16>>16>>4>>4)]
      filtered_table.back().push_back(e);
    }

    LOG(INFO) << "[SIZE " << d << "] done. " << table.back().size() << " => " <<
        filtered_table.back().size() << " exprs.";
    for (size_t i = 0; i < filtered_table.back().size(); ++i)
      if (i < 2 || i+2 >= filtered_table.back().size())
        LOG(INFO) << " => " << *filtered_table.back()[i];
  }

  return filtered_table;
}

std::vector<std::vector<std::shared_ptr<Expr> > >& ListFoldBody(size_t max_size = 9) {
  static std::vector<std::vector<std::shared_ptr<Expr> > > table =
      PreComputeTable(max_size);
  return table;
}

uint64_t EvalFoldBody(const Expr& body, uint64_t x, uint64_t value, uint64_t acc) {
  for (size_t i = 0; i < 8; ++i, value >>= 8) {
    Env env = {x, (value & 0xFF), acc};
    acc = body.Eval(env);
  }
  return acc;
}

}  // namespace icpfc

#endif  // EUGEO_H_
