#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"
#include "expr_list.h"
#include "expr_list_old.h"

using namespace icfpc;

DEFINE_int32(size, -1, "Size of the expression");
DEFINE_string(operators, "", "List of the operators");
DEFINE_bool(genallcheck, false, "Check by comparison with the old routine");

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  CHECK(FLAGS_size >= 3) << "--size should be specified";
  CHECK(!FLAGS_operators.empty()) << "--operators should be specified";

  int op_type_set = ParseOpTypeSet(FLAGS_operators);

  std::vector<std::shared_ptr<Expr> > result = ListExpr(FLAGS_size, op_type_set);
  if (FLAGS_genallcheck) {
    std::vector<std::shared_ptr<Expr> > result2 = old::ListExpr(FLAGS_size, op_type_set);
    CHECK_EQ(result.size(), result2.size());
    for (size_t i = 0; i < result2.size(); ++i)
      CHECK(result[i]->EqualTo(*result2[i]));
    std::cerr<<"CHECKED"<<std::endl;
  }

  for (const std::shared_ptr<Expr>& e : result) {
    std::cout << *e << std::endl;
  }
  return 0;
}
