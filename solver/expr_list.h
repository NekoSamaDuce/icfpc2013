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
    for (std::size_t i = 1; i < depth - 1; ++i) if (i <= depth - 1 - i) { // Only emit smaller left
      for (auto& lhs : table[i]) {
        for (auto& rhs : table[depth - 1 - i]) {
          int has_fold_cnt = (lhs->has_fold() ? 1 : 0) + (rhs->has_fold() ? 1 : 0);
          int in_fold_cnt  = (lhs->in_fold() ? 1 : 0) + (rhs->in_fold() ? 1 : 0);
          if(has_fold_cnt > 1) break;
          if(has_fold_cnt == 1 && in_fold_cnt >= 1) break;

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
              int has_fold_cnt = (e_cond->has_fold() ? 1 : 0) + (e_then->has_fold() ? 1 : 0) + (e_else->has_fold() ? 1 : 0);
              int  in_fold_cnt = (e_cond->in_fold()  ? 1 : 0) + (e_then->in_fold()  ? 1 : 0) + (e_else->in_fold()  ? 1 : 0);
              if(has_fold_cnt > 1) break;
              if(has_fold_cnt == 1 && in_fold_cnt >= 1) break;

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
  GLOBAL_SIMPLIFY,
};

std::vector<std::shared_ptr<Expr> > RemoveInFold(
    const std::vector<std::shared_ptr<Expr> >& exprs) {
  std::vector<std::shared_ptr<Expr> > result;
  for (auto& e : exprs)
    if (!e->in_fold())
      result.push_back(e);
  return result;
}

std::vector<std::shared_ptr<Expr> > ListExpr(
    std::size_t depth, int op_type_set, GenAllSimplifyMode mode) {
  std::vector<std::vector<std::shared_ptr<Expr> > > table(1);

  // For TFOLD, the size of the body is by |lambda|+|fold|+|x|+|0| = 5 smaller.
  std::size_t table_gen_limit =
    (op_type_set & OpType::TFOLD ? (depth >= 6 ? depth - 5 : 1) : depth - 1);

  std::set<std::string> already_known;

  // Generate size=1 expressions.
  table.push_back(ListExprDepth1(op_type_set));
  for (auto& e: table.back())
    already_known.insert(Simplify(e)->ToString());

  // Generate size=d expressions.
  for (size_t d = 2; d <= table_gen_limit; ++d) {
    auto table_d = ListExprInternal(table, d, op_type_set);

    // If it is to late to form fold, discard in_fold elements.
    if (d + 5 > depth)
      table_d = RemoveInFold(table_d);

    // Simplify
    switch (mode) {
      case NO_SIMPLIFY:
        break;
      case SIMPLIFY_EACH_STEP:
        table_d = SimplifyExprList(table_d);
        break;
      case GLOBAL_SIMPLIFY: {
        std::vector<std::shared_ptr<Expr> > global_simplified;
        for (auto& e: table_d) {
          std::string key = Simplify(e)->ToString();
          if (already_known.insert(Simplify(e)->ToString()).second)
            global_simplified.push_back(e);
        }
        table_d = std::move(global_simplified);
        break;
      }
    }

    table.push_back(table_d);
    LOG(INFO) << "SIZE[" << d << "] " << table_d.size();
  }

  std::vector<std::shared_ptr<Expr> > result;
  if (mode == GLOBAL_SIMPLIFY) {
    // GLOBAL_SIMPLIFY ==> Take all the possible sizes.
    if (op_type_set & OpType::TFOLD) {
      for (auto& table_d : table)
        for (auto& e : table_d)
          if (!e->in_fold())
            result.push_back(LambdaExpr::Create(FoldExpr::CreateTFold(e)));
    } else {
      for (auto& table_d : table)
        for (auto& e : table_d)
          if (!e->in_fold())
            result.push_back(LambdaExpr::Create(e));
    }
  } else {
    // NO_SIMPLIFY & SIMPLIFY_EACH_STEP ==> Take the exact size.
    if (op_type_set & OpType::TFOLD) {
      table.resize(depth);
      if (depth >= 5)
        for (auto& e_body : table[depth - 5])
          if (!e_body->has_fold())
            table[depth - 1].push_back(FoldExpr::CreateTFold(e_body));
    }

    for (auto& e: table[depth - 1]) {
      if (e->in_fold())
        continue;
      if (mode != NO_SIMPLIFY || e->op_type_set() == op_type_set)
        result.push_back(LambdaExpr::Create(e));
    }
  }
  LOG(INFO) << "SIZE[GEN] " << result.size();
  return result;
}

}  // namespace icpfc

#endif  // ICFPC_EXPR_LIST_H_
