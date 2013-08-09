#ifndef ICFPC_EXPR_LIST_H_
#define ICFPC_EXPR_LIST_H_

#include <algorithm>
#include <vector>
#include "expr.h"

namespace icfpc {

class KeyExpr : public Expr {
 public:
  KeyExpr(std::size_t depth) : Expr(static_cast<OpType>(-1), 0, depth, false, false) {
  }

 protected:
  virtual void Output(std::ostream*) const {}
  virtual uint64_t EvalImpl(const Env& e) const { return 0; }
  virtual bool EqualToImpl(const Expr& e) const { return false; }
};

struct DepthComp {
  bool operator()(const std::shared_ptr<Expr>& e1, const std::shared_ptr<Expr>& e2) {
    return e1->depth() < e2->depth();
  }
};

std::vector<std::shared_ptr<Expr> > ListExprInternal(
    std::size_t depth, bool is_initial, int op_type_set) {
  if (depth == 1) {
    std::vector<std::shared_ptr<Expr> > result;
    result.push_back(ConstantExpr::CreateZero());
    result.push_back(ConstantExpr::CreateOne());
    result.push_back(IdExpr::CreateX());
    if (op_type_set & (OpType::FOLD | OpType::TFOLD)) {
      result.push_back(IdExpr::CreateY());
      result.push_back(IdExpr::CreateZ());
    }
    return result;
  }

  std::vector<std::shared_ptr<Expr> > result_list = ListExprInternal(depth - 1, false, op_type_set);
  std::vector<std::shared_ptr<Expr> > new_list;

  // Unary.
  if (depth >= 2 &&
      (op_type_set & (OpType::NOT | OpType::SHL1 | OpType::SHR1 | OpType::SHR4 | OpType::SHR16))) {
    auto range = std::equal_range(
        result_list.begin(), result_list.end(),
        std::make_shared<KeyExpr>(depth - 1), DepthComp());
    for (auto it = range.first; it != range.second; ++it) {
      if (op_type_set & OpType::NOT)
        new_list.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::NOT, *it));
      if (op_type_set & OpType::SHL1)
        new_list.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHL1, *it));
      if (op_type_set & OpType::SHR1)
        new_list.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR1, *it));
      if (op_type_set & OpType::SHR4)
        new_list.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR4, *it));
      if (op_type_set & OpType::SHR16)
        new_list.push_back(UnaryOpExpr::Create(UnaryOpExpr::Type::SHR16, *it));
    }
  }

  // Binary.
  if (depth >= 3 &&
      (op_type_set & (OpType::AND | OpType::OR | OpType::XOR | OpType::PLUS))) {
    for (std::size_t i = 1; i < depth - 1; ++i) {
      auto lhs_range = std::equal_range(
          result_list.begin(), result_list.end(),
          std::make_shared<KeyExpr>(i), DepthComp());
      auto rhs_range = std::equal_range(
          result_list.begin(), result_list.end(),
          std::make_shared<KeyExpr>(depth - 1 - i), DepthComp());
      for (auto lit = lhs_range.first; lit != lhs_range.second; ++lit) {
        for (auto rit = rhs_range.first; rit != rhs_range.second; ++rit) {
          if (op_type_set & OpType::AND)
            new_list.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::AND, *lit, *rit));
          if (op_type_set & OpType::OR)
            new_list.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::OR, *lit, *rit));
          if (op_type_set & OpType::XOR)
            new_list.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::XOR, *lit, *rit));
          if (op_type_set & OpType::PLUS)
            new_list.push_back(BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS, *lit, *rit));
        }
      }
    }
  }

  if (depth >= 4 && (op_type_set & OpType::IF0)) {
    for (size_t i = 1; i < depth - 2; ++i) {
      auto cond_range = std::equal_range(
          result_list.begin(), result_list.end(),
          std::make_shared<KeyExpr>(i), DepthComp());
      for (size_t j = 1; j < depth - i - 1; ++j) {
        auto then_range = std::equal_range(
            result_list.begin(), result_list.end(),
            std::make_shared<KeyExpr>(j), DepthComp());
        auto else_range = std::equal_range(
            result_list.begin(), result_list.end(),
            std::make_shared<KeyExpr>(depth - 1 - i - j), DepthComp());
        for (auto cit = cond_range.first; cit != cond_range.second; ++cit) {
          for (auto tit = then_range.first; tit != then_range.second; ++tit) {
            for (auto eit = else_range.first; eit != else_range.second; ++eit) {
              new_list.push_back(If0Expr::Create(*cit, *tit, *eit));
            }
          }
        }
      }
    }
  }

  // TODO do not create TFold expression here.
  if (depth >= 5 && (op_type_set & OpType::FOLD)) {
    for (size_t i = 1; i < depth - 3; ++i) {
      auto value_range = std::equal_range(
          result_list.begin(), result_list.end(),
          std::make_shared<KeyExpr>(i), DepthComp());
      for (size_t j = 1; j < depth - i - 2; ++j) {
        auto init_value_range = std::equal_range(
            result_list.begin(), result_list.end(),
            std::make_shared<KeyExpr>(j), DepthComp());
        auto body_range = std::equal_range(
            result_list.begin(), result_list.end(),
            std::make_shared<KeyExpr>(depth - 2 - i - j), DepthComp());
        for (auto vit = value_range.first; vit != value_range.second; ++vit) {
          if ((*vit)->has_fold() || (*vit)->in_fold()) continue;
          for (auto iit = init_value_range.first; iit != init_value_range.second; ++iit) {
            if ((*iit)->has_fold() || (*iit)->in_fold()) continue;

            if (is_initial &&
                (*vit) == IdExpr::CreateX() &&
                (*iit) == ConstantExpr::CreateZero()) {
              // Do not create TFOLD.
              continue;
            }

            for (auto bit = body_range.first; bit != body_range.second; ++bit) {
              if ((*bit)->has_fold()) continue;
              new_list.push_back(FoldExpr::Create(*vit, *iit, *bit));
            }
          }
        }
      }
    }
  }

  if (is_initial && depth >= 5 && (op_type_set & OpType::TFOLD)) {
    auto body_range = std::equal_range(
        result_list.begin(), result_list.end(),
        std::make_shared<KeyExpr>(depth - 4), DepthComp());
    for (auto bit = body_range.first; bit != body_range.second; ++bit) {
      if ((*bit)->has_fold()) continue;
      new_list.push_back(FoldExpr::CreateTFold(*bit));
    }
  }

  result_list.insert(result_list.end(), new_list.begin(), new_list.end());
  return result_list;
}

std::vector<std::shared_ptr<Expr> > ListExpr(std::size_t depth, int op_type_set) {
  std::vector<std::shared_ptr<Expr> > child_list =
      ListExprInternal(depth - 1, true, op_type_set);
  auto range = std::equal_range(
      child_list.begin(), child_list.end(), std::make_shared<KeyExpr>(depth - 1), DepthComp());
  std::vector<std::shared_ptr<Expr> > result;
  while (range.first != range.second) {
    if (!(*range.first)->in_fold() && (*range.first)->op_type_set() == op_type_set)
      result.push_back(LambdaExpr::Create(*range.first));
    ++range.first;
  }
  return result;
}

}  // namespace icpfc

#endif  // ICFPC_EXPR_LIST_H_
