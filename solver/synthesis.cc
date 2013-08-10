#include <map>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"
#include "simplify.h"

using namespace icfpc;

DEFINE_uint64(argument, 0, "");
DEFINE_uint64(expected, 0, "");
DEFINE_int32(size, 0, "");
DEFINE_string(operators, "", "List of the operators");


static const UnaryOpExpr::Type UNARY_OP_TYPES[] = {
  UnaryOpExpr::Type::NOT,
  UnaryOpExpr::Type::SHL1,
  UnaryOpExpr::Type::SHR1,
  UnaryOpExpr::Type::SHR4,
  UnaryOpExpr::Type::SHR16,
};

static const BinaryOpExpr::Type BINARY_OP_TYPES[] = {
  BinaryOpExpr::Type::AND,
  BinaryOpExpr::Type::OR,
  BinaryOpExpr::Type::XOR,
  BinaryOpExpr::Type::PLUS,
};

typedef std::pair<std::string, std::shared_ptr<Expr> > MapPair;
MapPair MakeMapPair(std::shared_ptr<Expr> expr) {
  return std::make_pair(Simplify(expr)->ToString(), expr);
}

std::vector<std::shared_ptr<Expr> > Synthesis(
    uint64_t argument, uint64_t expected, int max_size, int op_type_set) {
  // Size -> Output -> Expr
  //std::map<int, std::map<uint64_t, std::shared_ptr<Expr> > > memo;
  // Simple form -> Expr
  std::map<std::string, std::shared_ptr<Expr> > stable;
  std::map<std::string, std::shared_ptr<Expr> > unstable;

  unstable.insert(MakeMapPair(ConstantExpr::CreateZero()));
  unstable.insert(MakeMapPair(ConstantExpr::CreateOne()));
  unstable.insert(MakeMapPair(IdExpr::CreateX()));

  while (!unstable.empty()) {
    LOG(INFO) << stable.size() << " " << unstable.size();
    for (const auto& pair : stable) {
      VLOG(1) << pair.second->ToString();
    }
    std::map<std::string, std::shared_ptr<Expr> > fresh;

    for (UnaryOpExpr::Type type : UNARY_OP_TYPES) {
      if ((op_type_set & UnaryOpExpr::ToOpType(type)) == 0) {
        continue;
      }
      for (const auto& pair : unstable) {
        std::shared_ptr<Expr> arg = pair.second;
        std::shared_ptr<Expr> expr = UnaryOpExpr::Create(type, arg);
        if (static_cast<int>(expr->depth()) > max_size) {
          continue;
        }
        MapPair map_pair = MakeMapPair(expr);
        const std::string& fingerprint = map_pair.first;
        if (stable.count(fingerprint) == 0 &&
            unstable.count(fingerprint) == 0 &&
            fresh.count(fingerprint) == 0) {
          fresh.insert(map_pair);
        }
      }
      // LOG(INFO) << "unary" << type;
    }

    for (BinaryOpExpr::Type type : BINARY_OP_TYPES) {
      if ((op_type_set & BinaryOpExpr::ToOpType(type)) == 0) {
        continue;
      }

      // stable x unstable
      for (const auto& pair1 : stable) {
        std::shared_ptr<Expr> arg1 = pair1.second;
        for (const auto& pair2 : unstable) {
          std::shared_ptr<Expr> arg2 = pair2.second;
          std::shared_ptr<Expr> expr = BinaryOpExpr::Create(type, arg1, arg2);
          if (static_cast<int>(expr->depth()) > max_size) {
            continue;
          }
          MapPair map_pair = MakeMapPair(expr);
          const std::string& fingerprint = map_pair.first;
          if (stable.count(fingerprint) == 0 &&
              unstable.count(fingerprint) == 0 &&
              fresh.count(fingerprint) == 0) {
            fresh.insert(map_pair);
          }
        }
      }
      
      // unstable x unstable
      for (const auto& pair1 : unstable) {
        std::shared_ptr<Expr> arg1 = pair1.second;
        for (const auto& pair2 : unstable) {
          std::shared_ptr<Expr> arg2 = pair2.second;
          std::shared_ptr<Expr> expr = BinaryOpExpr::Create(type, arg1, arg2);
          if (static_cast<int>(expr->depth()) > max_size) {
            continue;
          }
          MapPair map_pair = MakeMapPair(expr);
          const std::string& fingerprint = map_pair.first;
          if (stable.count(fingerprint) == 0 &&
              unstable.count(fingerprint) == 0 &&
              fresh.count(fingerprint) == 0) {
            fresh.insert(map_pair);
          }
        }
      }
    }

    for (const auto& pair : unstable) {
      stable.insert(pair);
    }
    std::swap(unstable, fresh);
  }

  std::vector<std::shared_ptr<Expr> > results;
  for (const auto& pair : stable) {
    std::shared_ptr<Expr> expr = pair.second;
    if (Eval(*expr, argument) == expected) {
      results.push_back(expr);
    }
  }

  return results;
}

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  int op_type_set = ParseOpTypeSet(FLAGS_operators);

  std::vector<std::shared_ptr<Expr> > exprs = Synthesis(
      FLAGS_argument, FLAGS_expected, FLAGS_size, op_type_set);
  for (std::shared_ptr<Expr> expr : exprs) {
    std::cout << *expr << std::endl;
  }

  return 0;
}
