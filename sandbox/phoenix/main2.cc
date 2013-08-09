#include <iostream>
#include <memory>
#include "expr.h"
#include "expr_list.h"
#include "parser.h"

using namespace icfpc;

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  std::shared_ptr<Expr> expr = Parse(argv[1]);
  uint64_t v = strtoull(argv[2], NULL, 0);
  std::cout << Eval(*expr, v) << std::endl;
  return 0;
}
