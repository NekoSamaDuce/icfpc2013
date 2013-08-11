#include <map>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"

using namespace icfpc;


DEFINE_uint64(argument, 0, "");
DEFINE_uint64(expected, 0, "");
DEFINE_int32(size, 30, "");
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
    return value << 1;
  case UnaryOpExpr::Type::SHR1:
    return value >> 1;
  case UnaryOpExpr::Type::SHR4:
    return value >> 4;
  case UnaryOpExpr::Type::SHR16:
    return value >> 16;
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
void Cardinal(uint64_t argument, uint64_t expected, int max_size, int op_type_set) {
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
  // { Output => ( Representative Expression, Size ) }
  std::map<uint64_t, std::pair<std::shared_ptr<Expr>, int> > stable;
  std::map<uint64_t, std::pair<std::shared_ptr<Expr>, int> > unstable;

  // unstable.insert(make_pair([0, 0, 0], ConstantExpr::CreateZero()));
  unstable.insert(std::make_pair(0, std::make_pair(ConstantExpr::CreateZero(), 1)));
  // unstable.insert(make_pair([1, 1, 1], ConstantExpr::CreateOne()));
  unstable.insert(std::make_pair(1, std::make_pair(ConstantExpr::CreateOne(), 1)));
  // unstable.insert(make_pair([a0, a1, a2], IdExpr::CreateX()));
  unstable.insert(std::make_pair(argument, std::make_pair(IdExpr::CreateX(), 1)));

  while (!unstable.empty()) {
    std::map<uint64_t, std::pair<std::shared_ptr<Expr>, int> > fresh;

    for (const auto& pair : unstable) {
      for (UnaryOpExpr::Type type : UNARY_OP_TYPES) {
        uint64_t new_value = EvalUnaryImmediate(type, pair.first);
        if (max_size >= pair.second.second + 1 &&
            stable.find(new_value) == stable.end() &&
            unstable.find(new_value) == unstable.end()) {
          fresh.insert(make_pair(new_value,
                                 std::make_pair(UnaryOpExpr::Create(type,
                                                                    ConstantExpr::Create(pair.first)),
                                                pair.second.second + 1)));
        }
      }

      for (BinaryOpExpr::Type type : BINARY_OP_TYPES) {
        for (const auto& pair2 : unstable) {
          uint64_t new_value = EvalBinaryImmediate(type,
                                                   pair.first, pair2.first);
          if (max_size >= pair.second.second + pair2.second.second + 1 &&
              stable.find(new_value) == stable.end() &&
              unstable.find(new_value) == unstable.end()) {
            fresh.insert(std::make_pair(new_value,
                                        std::make_pair(BinaryOpExpr::Create(type,
                                                                            ConstantExpr::Create(pair.first),
                                                                            ConstantExpr::Create(pair2.first)),
                                                       pair.second.second + pair2.second.second + 1)));
          }
        }

        for (const auto& pair2 : stable) {
          uint64_t new_value = EvalBinaryImmediate(type,
                                                   pair.first, pair2.first);
          if (max_size >= pair.second.second + pair2.second.second + 1 &&
              stable.find(new_value) == stable.end() &&
              unstable.find(new_value) == unstable.end()) {
            fresh.insert(std::make_pair(new_value,
                                        std::make_pair(BinaryOpExpr::Create(type,
                                                                            ConstantExpr::Create(pair.first),
                                                                            ConstantExpr::Create(pair2.first)),
                                                       pair.second.second + pair2.second.second + 1)));
          }
        }
      }
    }
    // TODO: Find it earlier.
    std::map<uint64_t, std::pair<std::shared_ptr<Expr>, int> >::const_iterator found =
        fresh.find(expected);
    stable.insert(unstable.begin(), unstable.end());
    if (found == fresh.end()) {
      // TODO: Speed-up.
      unstable.clear();
      unstable.insert(fresh.begin(), fresh.end());
    } else {
      stable.insert(fresh.begin(), fresh.end());
      break;
    }
  }

  std::vector<std::pair<int, uint64_t> > stack;
  int depth = 0;
  uint64_t value = expected;
  while (true) {
    std::map<uint64_t, std::pair<std::shared_ptr<Expr>, int> >::const_iterator found = stable.find(value);
    if (found == stable.end()) {
      std::cout << "Not found." << std::endl;
      break;
    }

    for (int i = 0; i < depth; ++i) {
      std::cout << " ";
    }
    std::cout << found->first << " <== "
              << *found->second.first << " [size="
              << found->second.second << "]" << std::endl;

    if (found->second.first->op_type() == CONSTANT ||
        found->second.first->op_type() == ID) {
      if (stack.empty()) {
        break;
      }
      std::pair<int, uint64_t> popped = stack.back();
      stack.pop_back();
      depth = popped.first;
      value = popped.second;
    } else if (found->second.first->op_type() == AND ||
               found->second.first->op_type() == OR ||
               found->second.first->op_type() == XOR ||
               found->second.first->op_type() == PLUS) {
      value = std::static_pointer_cast<ConstantExpr>(
          std::static_pointer_cast<BinaryOpExpr>(found->second.first)->arg1())->value();
      ++depth;
      uint64_t value2 = std::static_pointer_cast<ConstantExpr>(
          std::static_pointer_cast<BinaryOpExpr>(found->second.first)->arg2())->value();
      stack.push_back(std::make_pair(depth, value2));
    } else {
      value = std::static_pointer_cast<ConstantExpr>(
           std::static_pointer_cast<UnaryOpExpr>(found->second.first)->arg())->value();
      ++depth;
    }
  }
}


int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  int op_type_set = ParseOpTypeSet(FLAGS_operators);

  Cardinal(FLAGS_argument, FLAGS_expected, FLAGS_size, op_type_set);
  // std::cout << *expr << std::endl;

  return 0;
}
