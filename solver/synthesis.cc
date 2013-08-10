#include <map>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"

using namespace icfpc;

DEFINE_uint64(argument, 0, "");
DEFINE_uint64(expected, 0, "");

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

std::shared_ptr<Expr> Synthesis(uint64_t argument, uint64_t expected) {
  // Size -> Output -> Expr
  std::map<int, std::map<uint64_t, std::shared_ptr<Expr> > > memo;
  memo[1][0] = ConstantExpr::CreateZero();
  memo[1][1] = ConstantExpr::CreateOne();
  memo[1][argument] = IdExpr::CreateX();

  for (int expr_size = 2; ; ++expr_size) {
    LOG(INFO) << "expr_size = " << expr_size;

    for (UnaryOpExpr::Type type : UNARY_OP_TYPES) {
      int arg_size = expr_size - 1;
      for (const auto& pair : memo[arg_size]) {
        std::shared_ptr<Expr> arg = pair.second;
        std::shared_ptr<Expr> expr = UnaryOpExpr::Create(type, arg);
        uint64_t output = Eval(*expr, argument);
        if (output == expected) {
          return expr;
        }
        memo[expr_size][output] = expr;
      }
    }

    for (BinaryOpExpr::Type type : BINARY_OP_TYPES) {
      for (int arg1_size = 1; expr_size - 1 - arg1_size >= 1; ++arg1_size) {
        int arg2_size = expr_size - 1 - arg1_size;
        for (const auto& pair1 : memo[arg1_size]) {
          std::shared_ptr<Expr> arg1 = pair1.second;
          for (const auto& pair2 : memo[arg2_size]) {
            std::shared_ptr<Expr> arg2 = pair2.second;
            std::shared_ptr<Expr> expr = BinaryOpExpr::Create(type, arg1, arg2);
            uint64_t output = Eval(*expr, argument);
            if (output == expected) {
              return expr;
            }
            memo[expr_size][output] = expr;
          }
        }
      }
    }

    LOG(INFO) << "memo[" << expr_size << "].size = " << memo[expr_size].size();
  }
}

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  std::shared_ptr<Expr> expr = Synthesis(FLAGS_argument, FLAGS_expected);
  std::cout << *expr << std::endl;

  return 0;
}
