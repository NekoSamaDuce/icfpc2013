#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"
#include "expr_list.h"
#include "cluster.h"
#include "simplify.h"

using namespace icfpc;

DEFINE_int32(size, -1, "Size of the expression");
DEFINE_string(operators, "", "List of the operators");
DEFINE_string(simplify, "global", "{no,each,global}");

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  CHECK(FLAGS_size >= 3) << "--size should be specified";
  CHECK(!FLAGS_operators.empty()) << "--operators should be specified";

  int op_type_set = ParseOpTypeSet(FLAGS_operators);

  GenAllSimplifyMode simp_mode =
     FLAGS_simplify=="global" ? GLOBAL_SIMPLIFY :
       FLAGS_simplify=="each" ? SIMPLIFY_EACH_STEP : NO_SIMPLIFY;

  std::vector<std::shared_ptr<Expr> > result = ListExpr(FLAGS_size, op_type_set, simp_mode);
  if (simp_mode == NO_SIMPLIFY)
    result = SimplifyExprList(result);
  LOG(INFO) << "SIZE[FIN] " <<  result.size();
  std::vector<uint64_t> key = CreateKey();
  std::map<std::vector<uint64_t>, std::vector<std::shared_ptr<Expr> > > cluster =
      CreateCluster(key, result);

  for (auto iter = cluster.cbegin(); iter != cluster.cend(); ++iter) {
    if (iter->second.size() > 1u) {
      std::cout << "-----------------------------\n";
      for (const std::shared_ptr<Expr>& e : iter->second) {
        std::cout << *e << "\n";
      }
    }
  }

  return 0;
}
