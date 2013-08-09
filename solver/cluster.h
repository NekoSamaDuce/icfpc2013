#ifndef ICFPC_CLUSTER_H_
#define ICFPC_CLUSTER_H_

#include <map>
#include <vector>
#include <memory>
#include <random>

#include "expr.h"

namespace icfpc {

// Creates a key (vector of 256 uint64 values).
std::vector<uint64_t> CreateKey() {
  static std::mt19937 rand_engine(178);
  std::uniform_int_distribution<uint64_t> rand_distr(0, ~0ull);
  std::vector<uint64_t> keys;
  keys.reserve(256);
  for (int64_t i = -7; i <= 7; ++i) {
    keys.push_back(static_cast<uint64_t>(i));
  }
  for (int i = 0; i < 64; ++i) {
    keys.push_back(1ULL << i);
    keys.push_back(~(1ULL << i));
  }

  while (keys.size() < 256) {
    keys.emplace_back(rand_distr(rand_engine));
  }
  return keys;
}

// Classify by key.
std::map<std::vector<uint64_t>, std::vector<std::shared_ptr<Expr> > >
CreateCluster(const std::vector<uint64_t>& input,
              const std::vector<std::shared_ptr<Expr> >& expr_list) {
  std::map<std::vector<uint64_t>, std::vector<std::shared_ptr<Expr> > > result;
  for (std::shared_ptr<Expr> expr : expr_list) {
    std::vector<uint64_t> key;
    key.reserve(256);
    for (uint64_t v : input) {
      key.push_back(Eval(*expr, v));
    }
    result[key].push_back(expr);
  }
  return result;
}

}  // namespace icfpc

#endif  // ICFPC_CLUSTER_H_
