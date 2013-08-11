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
  std::vector<std::vector<uint64_t> > keys(expr_list.size(), std::vector<uint64_t>(input.size()));
  for (size_t i = 0; i < input.size(); ++i) {
    uint64_t x = input[i];
    for (size_t k = 0; k < expr_list.size(); ++k) {
      auto& e = expr_list[k];
      uint64_t y = Eval(*e, x);
      keys[k][i] = y;
    }
  }

  std::map<std::vector<uint64_t>, std::vector<std::shared_ptr<Expr> > > result;
  for (size_t k = 0; k < expr_list.size(); ++k)
    result[keys[k]].push_back(expr_list[k]);
  return result;
}

}  // namespace icfpc

#endif  // ICFPC_CLUSTER_H_
