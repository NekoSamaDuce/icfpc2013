#ifndef ICFPC_EXPR_LIST_H_
#define ICFPC_EXPR_LIST_H_

#include <algorithm>
#include <vector>
#include "expr.h"
#include "simplify.h"

namespace icfpc {

std::vector<std::shared_ptr<Expr> > ListExprDepth1(int op_type_set) {
  std::vector<std::shared_ptr<Expr> > result = {
    ConstantExpr::CreateZero(),
    ConstantExpr::CreateOne(),
  };
  if (!(op_type_set & OpType::TFOLD)) {
    result.push_back(IdExpr::CreateX());
  }
  if (op_type_set & (OpType::FOLD | OpType::TFOLD)) {
    result.push_back(IdExpr::CreateY());
    result.push_back(IdExpr::CreateZ());
  }
  return result;
}

std::vector<std::shared_ptr<Expr> > ListExprInternal(
    const std::vector<std::vector<std::shared_ptr<Expr> > >& table,
    std::size_t depth, int op_type_set) {
  std::vector<std::shared_ptr<Expr> > result;

  // Unary.
  if (depth >= 2 &&
      (op_type_set & (OpType::NOT | OpType::SHL1 | OpType::SHR1 | OpType::SHR4 | OpType::SHR16))) {
    for (auto& e : table[depth - 1]) {
      if (op_type_set & OpType::NOT)
        result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::NOT, e));
      if (op_type_set & OpType::SHL1)
        result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHL1, e));
      if (op_type_set & OpType::SHR1)
        result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR1, e));
      if (op_type_set & OpType::SHR4)
        result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR4, e));
      if (op_type_set & OpType::SHR16)
        result.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR16, e));
    }
  }

  // Binary.
  if (depth >= 3 &&
      (op_type_set & (OpType::AND | OpType::OR | OpType::XOR | OpType::PLUS))) {
    for (std::size_t i = 1; i < depth - 1; ++i) {
      for (auto& lhs : table[i]) {
        for (auto& rhs : table[depth - 1 - i]) {
          if (op_type_set & OpType::AND)
            result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::AND, lhs, rhs));
          if (op_type_set & OpType::OR)
            result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::OR, lhs, rhs));
          if (op_type_set & OpType::XOR)
            result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::XOR, lhs, rhs));
          if (op_type_set & OpType::PLUS)
            result.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS, lhs, rhs));
        }
      }
    }
  }

  // Ternary: if0.
  if (depth >= 4 && (op_type_set & OpType::IF0)) {
    for (size_t i = 1; i < depth - 2; ++i) {
      for (size_t j = 1; j < depth - i - 1; ++j) {
        for (auto& e_cond : table[i]) {
          for (auto& e_then : table[j]) {
            for (auto& e_else : table[depth - 1 - i - j]) {
              result.push_back(If0Expr::Create(e_cond, e_then, e_else));
            }
          }
        }
      }
    }
  }

  // Ternary: fold.
  if (depth >= 5 && (op_type_set & OpType::FOLD)) {
    for (size_t i = 1; i < depth - 3; ++i) {
      for (size_t j = 1; j < depth - i - 2; ++j) {
        for (auto& e_value: table[i]) {
          if (e_value->has_fold() || e_value->in_fold()) continue;
          for (auto& e_init: table[j]) {
            if (e_init->has_fold() || e_init->in_fold()) continue;
            for (auto& e_body: table[depth - 2 - i - j]) {
              if (e_body->has_fold()) continue;
              result.push_back(FoldExpr::Create(e_value, e_init, e_body));
            }
          }
        }
      }
    }
  }

  return result;
}

enum GenAllSimplifyMode {
  NO_SIMPLIFY,
  SIMPLIFY_EACH_STEP,
};

std::vector<std::shared_ptr<Expr> > ListExpr(
    std::size_t depth, int op_type_set, GenAllSimplifyMode mode) {
  std::vector<std::vector<std::shared_ptr<Expr> > > table(1);

  // For TFOLD, the size of the body is by |lambda|+|fold|+|x|+|0| = 5 smaller.
  std::size_t table_gen_limit =
    (op_type_set & OpType::TFOLD ? (depth >= 6 ? depth - 5 : 1) : depth - 1);

  table.push_back(ListExprDepth1(op_type_set));
  for (size_t d = 2; d <= table_gen_limit; ++d) {
    auto table_d = ListExprInternal(table, d, op_type_set);
    if (mode == SIMPLIFY_EACH_STEP)
      table_d = SimplifyExprList(table_d);
    table.push_back(table_d);
    LOG(INFO) << "SIZE[" << d << "] " << table_d.size();
  }

  if (op_type_set & OpType::TFOLD) {
    table.resize(depth);
    if (depth >= 5)
      for (auto& e_body : table[depth - 5])
        if (!e_body->has_fold())
          table[depth - 1].push_back(FoldExpr::CreateTFold(e_body));
  }

  std::vector<std::shared_ptr<Expr> > result;
  for (auto& e: table[depth - 1]) {
    // Caution: in SIMPLIFY_EACH_STEP mode, operator set may not be complete.
    if (mode == SIMPLIFY_EACH_STEP ||
        (!e->in_fold() && e->op_type_set() == op_type_set))
      result.push_back(LambdaExpr::Create(e));
  }
  LOG(INFO) << "SIZE[GEN] " << result.size();
  return result;
}

}  // namespace icpfc

#endif  // ICFPC_EXPR_LIST_H_
