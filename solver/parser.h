#ifndef ICFPC_PARSER_H_
#define ICFPC_PARSER_H_

#include <memory>
#include <sstream>
#include <string>
#include <assert.h>

#include "expr.h"

namespace icfpc {

std::string ParseToken(std::istream* is) {
  char c = is->get();
  while (c == ' ') {
    c = is->get();
  }

  if (c == '(' || c == ')') {
    return std::string(1, c);
  }

  std::string s(1, c);
  while (true) {
    c = is->get();
    if (!is || c == ' ') return s;
    if (c == '(' || c == ')') {
      is->unget();
      return s;
    }
    s.push_back(c);
  }
}

struct ParseEnv {
  std::string x;
  std::string y;
  std::string z;
};

std::shared_ptr<Expr> ParseInternal(std::istream* is, const ParseEnv& env) {
  std::string token = ParseToken(is);
  if (token != "(") {
    // Should be id or constant.
    if (token == "0") return ConstantExpr::CreateZero();
    if (token == "1") return ConstantExpr::CreateOne();
    if (token == env.x) return IdExpr::CreateX();
    if (token == env.y) return IdExpr::CreateY();
    if (token == env.z) return IdExpr::CreateZ();
    assert(false);
  }

  assert(token == "(");
  token = ParseToken(is);
  if (token == "if0") {
    std::shared_ptr<Expr> cond = ParseInternal(is, env);
    std::shared_ptr<Expr> then_body = ParseInternal(is, env);
    std::shared_ptr<Expr> else_body = ParseInternal(is, env);
    token = ParseToken(is);
    assert(token == ")");
    return If0Expr::Create(cond, then_body, else_body);
  }
  if (token == "fold") {
    std::shared_ptr<Expr> value = ParseInternal(is, env);
    std::shared_ptr<Expr> init_value = ParseInternal(is, env);
    token = ParseToken(is);
    assert(token == "(");
    token = ParseToken(is);
    assert(token == "lambda");
    token = ParseToken(is);
    assert(token == "(");
    std::string y = ParseToken(is);
    std::string z = ParseToken(is);
    token = ParseToken(is);
    assert(token == ")");

    ParseEnv env2 = env;
    env2.y = y;
    env2.z = z;
    std::shared_ptr<Expr> body = ParseInternal(is, env2);
    token = ParseToken(is);
    assert(token == ")");
    token = ParseToken(is);
    assert(token == ")");
    return FoldExpr::Create(value, init_value, body);
  }

  if (token == "not" || token == "shl1" || token == "shr1" || token == "shr4" || token == "shr16") {
    std::shared_ptr<Expr> arg = ParseInternal(is, env);
    UnaryOpExpr::Type type =
        (token == "not") ? UnaryOpExpr::Type::NOT :
        (token == "shl1") ? UnaryOpExpr::Type::SHL1 :
        (token == "shr1") ? UnaryOpExpr::Type::SHR1 :
        (token == "shr4") ? UnaryOpExpr::Type::SHR4 :
        UnaryOpExpr::Type::SHR16;
    token = ParseToken(is);
    assert(token == ")");
    return UnaryOpExpr::Create(type, arg);
  }

  if (token == "and" || token == "or" || token == "xor" || token == "plus") {
    std::shared_ptr<Expr> arg1 = ParseInternal(is, env);
    std::shared_ptr<Expr> arg2 = ParseInternal(is, env);
    BinaryOpExpr::Type type =
        (token == "and") ? BinaryOpExpr::Type::AND :
        (token == "or") ? BinaryOpExpr::Type::OR :
        (token == "xor") ? BinaryOpExpr::Type::XOR :
        BinaryOpExpr::Type::PLUS;
    token = ParseToken(is);
    assert(token == ")");
    return BinaryOpExpr::Create(type, arg1, arg2);
  }

  assert(token == "lambda");
  token = ParseToken(is);
  assert(token == "(");
  std::string x = ParseToken(is);
  token = ParseToken(is);
  assert(token == ")");
  ParseEnv env2 = env;
  env2.x = x;
  std::shared_ptr<Expr> expr = ParseInternal(is, env2);
  token = ParseToken(is);
  assert(token == ")");
  return LambdaExpr::Create(expr);
}

std::shared_ptr<Expr> Parse(const std::string& str) {
  std::istringstream is(str);
  ParseEnv env;
  env.x = "x";
  env.y = "y";
  env.z = "z";
  return ParseInternal(&is, env);
}

}  // icfpc

#endif  // ICFPC_PARSER_H_
