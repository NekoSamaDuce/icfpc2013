#ifndef ICFPC_SIMPLIFY_H_
#define ICFPC_SIMPLIFY_H_

#include <vector>
#include <set>

#include "expr.h"

namespace icfpc {

std::shared_ptr<Expr> Simplify(const std::shared_ptr<Expr>& expr);

std::shared_ptr<Expr> SimplifyLambda(const std::shared_ptr<LambdaExpr>& expr) {
  std::shared_ptr<Expr> body = Simplify(expr->body());
  if (body.get() == expr->body().get()) {
    return expr;
  }

  return LambdaExpr::Create(body);
}

std::shared_ptr<Expr> SimplifyIf0(const std::shared_ptr<If0Expr>& expr) {
  std::shared_ptr<Expr> cond = Simplify(expr->cond());
  if (cond->op_type() == OpType::CONSTANT) {
    if (static_cast<const ConstantExpr&>(*cond).value() == 0) {
      return Simplify(expr->then_body());
    } else {
      return Simplify(expr->else_body());
    }
  }

  std::shared_ptr<Expr> then_body = Simplify(expr->then_body());
  std::shared_ptr<Expr> else_body = Simplify(expr->else_body());
  if (cond.get() == expr->cond().get() &&
      then_body.get() == expr->then_body().get() &&
      else_body.get() == expr->else_body().get()) {
    return expr;
  }

  return If0Expr::Create(cond, then_body, else_body);
}

std::shared_ptr<Expr> SimplifyFold(const std::shared_ptr<FoldExpr>& expr) {
  if (expr->op_type_set() & OpType::TFOLD) {
    std::shared_ptr<Expr> body = Simplify(expr->body());
    if (body.get() == expr->body().get()) {
      return expr;
    }
    return FoldExpr::CreateTFold(body);
  }

  std::shared_ptr<Expr> value = Simplify(expr->value());
  std::shared_ptr<Expr> init_value = Simplify(expr->init_value());
  std::shared_ptr<Expr> body = Simplify(expr->body());

  if (value.get() == expr->value().get() &&
      init_value.get() == expr->init_value().get() &&
      body.get() == expr->body().get()) {
    return expr;
  }

  return FoldExpr::Create(value, init_value, body);
}

std::shared_ptr<Expr> SimplifyNot(const std::shared_ptr<UnaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg = Simplify(expr->arg());
  if (arg->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(~static_cast<const ConstantExpr&>(*arg).value());
  }
  // TODO ~~.
  if (arg.get() == expr->arg().get()) {
    return expr;
  }
  return UnaryOpExpr::Create(UnaryOpExpr::Type::NOT, arg);
}

std::shared_ptr<Expr> SimplifyShl1(const std::shared_ptr<UnaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg = Simplify(expr->arg());
  if (arg->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(static_cast<const ConstantExpr&>(*arg).value() << 1);
  }

  if (arg.get() == expr->arg().get()) {
    return expr;
  }
  return UnaryOpExpr::Create(UnaryOpExpr::Type::SHL1, arg);
}

std::shared_ptr<Expr> SimplifyShr1(const std::shared_ptr<UnaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg = Simplify(expr->arg());
  if (arg->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(static_cast<const ConstantExpr&>(*arg).value() >> 1);
  }

  if (arg.get() == expr->arg().get()) {
    return expr;
  }
  return UnaryOpExpr::Create(UnaryOpExpr::Type::SHR1, arg);
}

std::shared_ptr<Expr> SimplifyShr4(const std::shared_ptr<UnaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg = Simplify(expr->arg());
  if (arg->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(static_cast<const ConstantExpr&>(*arg).value() >> 4);
  }

  if (arg.get() == expr->arg().get()) {
    return expr;
  }

  return UnaryOpExpr::Create(UnaryOpExpr::Type::SHR4, arg);
}

std::shared_ptr<Expr> SimplifyShr16(const std::shared_ptr<UnaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg = Simplify(expr->arg());
  if (arg->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(static_cast<const ConstantExpr&>(*arg).value() >> 16);
  }

  if (arg.get() == expr->arg().get()) {
    return expr;
  }
  return UnaryOpExpr::Create(UnaryOpExpr::Type::SHR16, arg);
}

std::shared_ptr<Expr> SimplifyAnd(const std::shared_ptr<BinaryOpExpr>& expr) {
  if ((expr->arg1()->op_type() == OpType::CONSTANT &&
       static_cast<const ConstantExpr&>(*expr->arg1()).value() == 0) ||
      (expr->arg2()->op_type() == OpType::CONSTANT &&
       static_cast<const ConstantExpr&>(*expr->arg2()).value() == 0)) {
    return ConstantExpr::Create(0);
  }
  if (expr->arg1()->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*expr->arg1()).value() == 0xFFFFFFFFFFFFFFFF) {
    return Simplify(expr->arg2());
  }
  if (expr->arg2()->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*expr->arg2()).value() == 0xFFFFFFFFFFFFFFFF) {
    return Simplify(expr->arg1());
  }

  std::shared_ptr<Expr> arg1 = Simplify(expr->arg1());
  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0) {
    return ConstantExpr::Create(0);
  }
  std::shared_ptr<Expr> arg2 = Simplify(expr->arg2());
  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0) {
    return ConstantExpr::Create(0);
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      arg2->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(
        static_cast<const ConstantExpr&>(*arg1).value() &
        static_cast<const ConstantExpr&>(*arg2).value());
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0xFFFFFFFFFFFFFFFF) {
    return arg2;
  }
  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0xFFFFFFFFFFFFFFFF) {
    return arg1;
  }

  if (arg1->CompareTo(*arg2) > 0) {
    arg1.swap(arg2);
  }

  if (arg1.get() == expr->arg1().get() &&
      arg2.get() == expr->arg2().get()) {
    return expr;
  }
  return BinaryOpExpr::Create(BinaryOpExpr::Type::AND, arg1, arg2);
}

std::shared_ptr<Expr> SimplifyOr(const std::shared_ptr<BinaryOpExpr>& expr) {
  if ((expr->arg1()->op_type() == OpType::CONSTANT &&
       static_cast<const ConstantExpr&>(*expr->arg1()).value() == 0xFFFFFFFFFFFFFFFF) ||
      (expr->arg2()->op_type() == OpType::CONSTANT &&
       static_cast<const ConstantExpr&>(*expr->arg2()).value() == 0xFFFFFFFFFFFFFFFF)) {
    return ConstantExpr::Create(0xFFFFFFFFFFFFFFFF);
  }
  if (expr->arg1()->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*expr->arg1()).value() == 0) {
    return Simplify(expr->arg2());
  }
  if (expr->arg2()->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*expr->arg2()).value() == 0) {
    return Simplify(expr->arg1());
  }

  std::shared_ptr<Expr> arg1 = Simplify(expr->arg1());
  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0xFFFFFFFFFFFFFFFF) {
    return ConstantExpr::Create(0xFFFFFFFFFFFFFFFF);
  }
  std::shared_ptr<Expr> arg2 = Simplify(expr->arg2());
  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0xFFFFFFFFFFFFFFFF) {
    return ConstantExpr::Create(0xFFFFFFFFFFFFFFFF);
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      arg2->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(
        static_cast<const ConstantExpr&>(*arg1).value() |
        static_cast<const ConstantExpr&>(*arg2).value());
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0) {
    return arg2;
  }
  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0) {
    return arg1;
  }

  if (arg1->CompareTo(*arg2) > 0) {
    arg1.swap(arg2);
  }

  if (arg1.get() == expr->arg1().get() &&
      arg2.get() == expr->arg2().get()) {
    return expr;
  }
  return BinaryOpExpr::Create(BinaryOpExpr::Type::OR, arg1, arg2);
}

std::shared_ptr<Expr> SimplifyXor(const std::shared_ptr<BinaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg1 = Simplify(expr->arg1());
  std::shared_ptr<Expr> arg2 = Simplify(expr->arg2());

  if (arg1->EqualTo(*arg2)) {
    return ConstantExpr::Create(0);
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      arg2->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(
        static_cast<const ConstantExpr&>(*arg1).value() ^
        static_cast<const ConstantExpr&>(*arg2).value());
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0) {
    return arg2;
  }
  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0) {
    return arg1;
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0xFFFFFFFFFFFFFFFF) {
    return UnaryOpExpr::Create(UnaryOpExpr::Type::NOT, arg2);
  }

  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0xFFFFFFFFFFFFFFFF) {
    return UnaryOpExpr::Create(UnaryOpExpr::Type::NOT, arg1);
  }

  if (arg1->CompareTo(*arg2) > 0) {
    arg1.swap(arg2);
  }

  if (arg1.get() == expr->arg1().get() &&
      arg2.get() == expr->arg2().get()) {
    return expr;
  }
  return BinaryOpExpr::Create(BinaryOpExpr::Type::XOR, arg1, arg2);
}

std::shared_ptr<Expr> SimplifyPlus(const std::shared_ptr<BinaryOpExpr>& expr) {
  std::shared_ptr<Expr> arg1 = Simplify(expr->arg1());
  std::shared_ptr<Expr> arg2 = Simplify(expr->arg2());

  if (arg1->op_type() == OpType::CONSTANT &&
      arg2->op_type() == OpType::CONSTANT) {
    return ConstantExpr::Create(
        static_cast<const ConstantExpr&>(*arg1).value() +
        static_cast<const ConstantExpr&>(*arg2).value());
  }

  if (arg1->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg1).value() == 0) {
    return arg2;
  }
  if (arg2->op_type() == OpType::CONSTANT &&
      static_cast<const ConstantExpr&>(*arg2).value() == 0) {
    return arg1;
  }

  if (arg1->CompareTo(*arg2) > 0) {
    arg1.swap(arg2);
  }

  if (arg1.get() == expr->arg1().get() &&
      arg2.get() == expr->arg2().get()) {
    return expr;
  }
  return BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS, arg1, arg2);
}

std::shared_ptr<Expr> Simplify(const std::shared_ptr<Expr>& expr) {
  switch (expr->op_type()) {
    case OpType::LAMBDA:
      return SimplifyLambda(std::static_pointer_cast<LambdaExpr>(expr));
    case OpType::CONSTANT:
    case OpType::ID:
      // No simplification is available.
      return expr;
    case OpType::IF0:
      return SimplifyIf0(std::static_pointer_cast<If0Expr>(expr));
    case OpType::FOLD:
      return SimplifyFold(std::static_pointer_cast<FoldExpr>(expr));
    case OpType::NOT:
      return SimplifyNot(std::static_pointer_cast<UnaryOpExpr>(expr));
    case OpType::SHL1:
      return SimplifyShl1(std::static_pointer_cast<UnaryOpExpr>(expr));
    case OpType::SHR1:
      return SimplifyShr1(std::static_pointer_cast<UnaryOpExpr>(expr));
    case OpType::SHR4:
      return SimplifyShr4(std::static_pointer_cast<UnaryOpExpr>(expr));
    case OpType::SHR16:
      return SimplifyShr16(std::static_pointer_cast<UnaryOpExpr>(expr));
    case OpType::AND:
      return SimplifyAnd(std::static_pointer_cast<BinaryOpExpr>(expr));
    case OpType::OR:
      return SimplifyOr(std::static_pointer_cast<BinaryOpExpr>(expr));
    case OpType::XOR:
      return SimplifyXor(std::static_pointer_cast<BinaryOpExpr>(expr));
    case OpType::PLUS:
      return SimplifyPlus(std::static_pointer_cast<BinaryOpExpr>(expr));
    default:
      NOTREACHED();
  }
  return expr;
}

std::vector<std::shared_ptr<Expr> > SimplifyExprList(
    const std::vector<std::shared_ptr<Expr> >& expr_list) {
  std::set<std::string> expr_repr;
  std::vector<std::shared_ptr<Expr> > result_list;
  result_list.reserve(expr_list.size());
  for (const std::shared_ptr<Expr>& e : expr_list) {
    auto result = expr_repr.insert(Simplify(e)->ToString());
    if (result.second) {
      result_list.push_back(e);
    } else {
      LOG(INFO) << e->ToString() << " is reduced to " << *result.first;
    }
  }
  return result_list;
}

}  // icfpc

#endif  // ICFPC_SIMPLIFY_H_
