#include <iostream>
#include <memory>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"
#include "expr_list.h"
#include "parser.h"

using namespace icfpc;

DEFINE_uint64(argument, 0, "Argument substituted to the expression.");

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  std::string line;
  while (std::getline(std::cin, line)) {
    std::shared_ptr<Expr> expr = Parse(line);
    std::cout << Eval(*expr, FLAGS_argument) << std::endl;
  }
  return 0;
}
