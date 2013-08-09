#include <iostream>
#include <memory>
#include "expr.h"
#include "expr_list.h"

using namespace icfpc;

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  size_t depth = static_cast<std::size_t>(atoi(argv[1]));
  int op_type_set = 0;
  for (int i = 2; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "not") {
      op_type_set |= OpType::NOT;
      continue;
    }
    if (arg == "shl1") {
      op_type_set |= OpType::SHL1;
      continue;
    }
    if (arg == "shr1") {
      op_type_set |= OpType::SHR1;
      continue;
    }
    if (arg == "shr4") {
      op_type_set |= OpType::SHR4;
      continue;
    }
    if (arg == "shr16") {
      op_type_set |= OpType::SHR16;
      continue;
    }
    if (arg == "and") {
      op_type_set |= OpType::AND;
      continue;
    }
    if (arg == "or") {
      op_type_set |= OpType::OR;
      continue;
    }
    if (arg == "xor") {
      op_type_set |= OpType::XOR;
      continue;
    }
    if (arg == "plus") {
      op_type_set |= OpType::PLUS;
      continue;
    }
    if (arg == "if0") {
      op_type_set |= OpType::IF0;
      continue;
    }
    if (arg == "fold") {
      op_type_set |= OpType::FOLD;
      continue;
    }
    if (arg == "tfold") {
      op_type_set |= OpType::TFOLD;
      continue;
    }
  }

  std::vector<std::shared_ptr<Expr> > result = ListExpr(depth, op_type_set);
  for (const std::shared_ptr<Expr>& e : result) {
    std::cout << *e << std::endl;
  }
  return 0;
}
