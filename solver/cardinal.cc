#include <map>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"

using namespace icfpc;

DEFINE_uint64(argument, 0, "");
DEFINE_uint64(expected, 0, "");
DEFINE_int32(size, 0, "");
DEFINE_string(operators, "", "List of the operators");


static const UnaryOpExpr::Type ALL_UNARY_OP_TYPES[] = {
  UnaryOpExpr::Type::NOT,
  UnaryOpExpr::Type::SHL1,
  UnaryOpExpr::Type::SHR1,
  UnaryOpExpr::Type::SHR4,
  UnaryOpExpr::Type::SHR16,
};

static std::vector<UnaryOpExpr::Type> UNARY_OP_TYPES;

static const BinaryOpExpr::Type ALL_BINARY_OP_TYPES[] = {
  BinaryOpExpr::Type::AND,
  BinaryOpExpr::Type::OR,
  BinaryOpExpr::Type::XOR,
  BinaryOpExpr::Type::PLUS,
};

static std::vector<BinaryOpExpr::Type> BINARY_OP_TYPES;


uint64_t EvalUnaryImmediate(UnaryOpExpr::Type type, uint64_t value) {
  switch (type) {
  case UnaryOpExpr::Type::NOT:
    return ~value;
  case UnaryOpExpr::Type::SHL1:
    return value >> 1;
  case UnaryOpExpr::Type::SHR1:
    return value << 1;
  case UnaryOpExpr::Type::SHR4:
    return value << 4;
  case UnaryOpExpr::Type::SHR16:
    return value << 16;
  default:
    return 0xdeadbeef;
  }
}


uint64_t EvalBinaryImmediate(BinaryOpExpr::Type type,
                             uint64_t value1, uint64_t value2) {
  switch (type) {
  case BinaryOpExpr::Type::AND:
    return value1 & value2;
  case BinaryOpExpr::Type::OR:
    return value1 | value2;
  case BinaryOpExpr::Type::XOR:
    return value1 ^ value2;
  case BinaryOpExpr::Type::PLUS:
    return value1 + value2;
  default:
    return 0xdeadbeef;
  }
}

// TODO: The argument to be a vector to pass multiple arguments.
std::shared_ptr<Expr> Cardinal(
    uint64_t argument, uint64_t expected, int max_size, int op_type_set) {
  for (UnaryOpExpr::Type type : ALL_UNARY_OP_TYPES) {
    if ((op_type_set & UnaryOpExpr::ToOpType(type)) == 0) {
      continue;
    }
    UNARY_OP_TYPES.push_back(type);
  }

  for (BinaryOpExpr::Type type : ALL_BINARY_OP_TYPES) {
    if ((op_type_set & BinaryOpExpr::ToOpType(type)) == 0) {
      continue;
    }
    BINARY_OP_TYPES.push_back(type);
  }

  // TODO: To be std::map<std::vector<uint64_t>, std::shared_ptr<Expr> > dict;
  // [output when |x0| is given, output when |x1| is given, ...] => backtrack
  std::map<uint64_t, std::shared_ptr<Expr> > stable;
  std::map<uint64_t, std::shared_ptr<Expr> > unstable;

  // unstable.insert(make_pair([0, 0, 0], ConstantExpr::CreateZero()));
  unstable.insert(make_pair(0, ConstantExpr::CreateZero()));
  // unstable.insert(make_pair([1, 1, 1], ConstantExpr::CreateOne()));
  unstable.insert(make_pair(1, ConstantExpr::CreateOne()));
  // unstable.insert(make_pair([a0, a1, a2], IdExpr::CreateX()));
  unstable.insert(make_pair(argument, IdExpr::CreateX()));

  while (!unstable.empty()) {
    std::map<uint64_t, std::shared_ptr<Expr> > fresh;

    for (const auto& pair : unstable) {
      for (UnaryOpExpr::Type type : UNARY_OP_TYPES) {
        uint64_t new_value = EvalUnaryImmediate(type, pair.first);
        if (stable.find(new_value) == stable.end() &&
            unstable.find(new_value) == unstable.end()) {
          fresh.insert(make_pair(new_value, UnaryOpExpr::Create(type,
              ConstantExpr::Create(pair.first))));
        }
      }

      for (BinaryOpExpr::Type type : BINARY_OP_TYPES) {
        for (const auto& pair2 : unstable) {
          uint64_t new_value = EvalBinaryImmediate(type,
                                                   pair.first, pair2.first);
          if (stable.find(new_value) == stable.end() &&
              unstable.find(new_value) == unstable.end()) {
            fresh.insert(make_pair(new_value, BinaryOpExpr::Create(type,
                ConstantExpr::Create(pair.first),
                ConstantExpr::Create(pair2.first))));
          }
        }

        for (const auto& pair2 : stable) {
          uint64_t new_value = EvalBinaryImmediate(type,
                                                   pair.first, pair2.first);
          if (stable.find(new_value) == stable.end() &&
              unstable.find(new_value) == unstable.end()) {
            fresh.insert(make_pair(new_value, BinaryOpExpr::Create(type,
                ConstantExpr::Create(pair.first),
                ConstantExpr::Create(pair2.first))));
          }
        }
      }
    }
    // TODO: Find it earlier.
    std::map<uint64_t, std::shared_ptr<Expr> >::const_iterator found =
        fresh.find(expected);
    stable.insert(unstable.begin(), unstable.end());
    if (found == fresh.end()) {
      unstable.insert(fresh.begin(), fresh.end());
    } else {
      stable.insert(fresh.begin(), fresh.end());
      break;
    }
  }

  std::map<uint64_t, std::shared_ptr<Expr> >::const_iterator it =
      stable.find(expected);
  while (true) {
    std::cout << it->first << " <== " << *it->second << std::endl;
    if (it->second->op_type() == CONSTANT ||
        it->second->op_type() == ID) {
      break;
    } else if (it->second->op_type() == AND ||
               it->second->op_type() == OR ||
               it->second->op_type() == XOR ||
               it->second->op_type() == PLUS) {
      uint64_t value = std::static_pointer_cast<ConstantExpr>(
           std::static_pointer_cast<BinaryOpExpr>(it->second)->arg1())->value();
      it = stable.find(value);
    } else {
      uint64_t value = std::static_pointer_cast<ConstantExpr>(
           std::static_pointer_cast<UnaryOpExpr>(it->second)->arg())->value();
      it = stable.find(value);
    }
  }
  return it->second;
}

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  int op_type_set = ParseOpTypeSet(FLAGS_operators);

  std::shared_ptr<Expr> expr = Cardinal(
      FLAGS_argument, FLAGS_expected, FLAGS_size, op_type_set);
  std::cout << *expr << std::endl;

  return 0;
}
