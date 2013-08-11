#include <stdio.h>
#include <sys/file.h>

#include <sstream>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"
#include "expr_list.h"
#include "cluster.h"
#include "simplify.h"
#include "util.h"

using namespace icfpc;

DEFINE_int32(size, -1, "Size of the expression");
DEFINE_string(operators, "", "List of the operators");
DEFINE_string(simplify, "global", "{no,each,global}");
DEFINE_bool(quiet, false, "suppress outputs");
DEFINE_string(cache_dir, "", "Path to cache dir");


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

  if (!FLAGS_quiet) {
    std::cout << "argument: ";
    PrintCollection(&std::cout, key, ",");
    std::cout << "\n";
  
    int i = 0;
    for (auto iter = cluster.cbegin(); iter != cluster.cend(); ++iter) {
      VLOG(1) << "Output Cluster " << i++ << ": " << iter->second.size();
      std::cout << "expected: ";
      PrintCollection(&std::cout, iter->first, ",");
      std::cout << "\n";
  
      for (const std::shared_ptr<Expr>& e : iter->second) {
        std::cout << *e << "\n";
      }
    }
  }

  if (!FLAGS_cache_dir.empty()) {
    for (auto iter = cluster.cbegin(); iter != cluster.cend(); ++iter) {
      uint64_t hash = HashKey(iter->first);
      char filename[256];
      MaybeMakeDir(FLAGS_cache_dir.c_str());
      sprintf(filename, "%s/%02x",
              FLAGS_cache_dir.c_str(),
              static_cast<int>(hash & 0xff));
      MaybeMakeDir(filename);
      sprintf(filename, "%s/%02x/%016llx.sxp",
              FLAGS_cache_dir.c_str(),
              static_cast<int>(hash & 0xff),
              static_cast<unsigned long long>(hash));
      FILE* fp = fopen(filename, "a+");
      flock(fileno(fp), LOCK_EX);
      fseek(fp, 0, SEEK_END);
      if (ftell(fp) == 0) {
        for (const std::shared_ptr<Expr>& e : iter->second) {
          std::ostringstream st;
          st << *e << "\n";
          std::string s(st.str());
          fwrite(s.c_str(), 1, s.size(), fp);
        }
      }
      fclose(fp);
    }
  }

  return 0;
}
