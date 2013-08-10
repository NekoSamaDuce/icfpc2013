#ifndef ICFPC_SIMPLIFY_H_
#define ICFPC_SIMPLIFY_H_

#include <vector>
#include <set>

#include "expr.h"

namespace icfpc {

std::shared_ptr<Expr> Simplify(std::shared_ptr<Expr> expr) {
  return expr->simplified();
}

std::vector<std::shared_ptr<Expr> > SimplifyExprList(
    const std::vector<std::shared_ptr<Expr> >& expr_list) {
  std::set<std::string> expr_repr;
  std::vector<std::shared_ptr<Expr> > result_list;
  result_list.reserve(expr_list.size());
  for (std::shared_ptr<Expr> e : expr_list) {
    auto result = expr_repr.insert(Simplify(e)->ToString());
    if (result.second) {
      result_list.push_back(e);
    } else {
      //LOG(INFO) << e->ToString() << " is reduced to " << *result.first;
    }
  }
  return result_list;
}

}  // icfpc

#endif  // ICFPC_SIMPLIFY_H_
