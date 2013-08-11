#include <map>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"

using namespace icfpc;

DEFINE_string(argument, "", "");
DEFINE_string(expected, "", "");
DEFINE_int32(size, 30, "");
DEFINE_string(operators, "", "List of the operators");
DEFINE_string(refinement_argument, "", "");
DEFINE_string(refinement_expected, "", "");

int ParseOpTypeSetWithBonus(const std::string& s, bool* is_bonus) {
  int op_type_set = 0;
  std::string param = s;
  for (auto& c : param) {
    if (c == ',') {
      c = ' ';
    }
  }
  std::stringstream ss(param);
  for(std::string op; ss >> op; ) {
    if (op == "bonus") {
      *is_bonus = true;
      continue;
    }
    op_type_set |= ParseOpType(op);
  }
  return op_type_set;
}

typedef std::vector<uint64_t> Key;
std::ostream& operator<<(std::ostream& os, const Key& key) {
  os << "[";
  if (key.size() > 0) {
    os << key[0];
    for (size_t i = 1; i < key.size(); ++i) {
      os << ", " << key[i];
    }
  }
  os << "]";
  return os;
}

std::string OpTypeToString(OpType type) {
  switch (type) {
  case NOT:
    return "NOT";
  case SHL1:
    return "SHL1";
  case SHR1:
    return "SHR1";
  case SHR4:
    return "SHR4";
  case SHR16:
    return "SHR16";
  case AND:
    return "AND";
  case OR:
    return "OR";
  case XOR:
    return "XOR";
  case PLUS:
    return "PLUS";
  case IF0:
    return "IF0";
  case FOLD:
    return "FOLD";
  case TFOLD:
    return "TFOLD";
  case LAMBDA:
    return "LAMBDA";
  case CONSTANT:
    return "CONSTANT";
  case ID:
    return "ID";
  default:
    LOG(FATAL) << "Unknown OpType.";
  }
}

struct Back {
  OpType type;
  const std::vector<uint64_t>* arg1;
  const std::vector<uint64_t>* arg2;
  const std::vector<uint64_t>* arg3;
};

static const OpType ALL_UNARY_OP_TYPES[] = {
  OpType::NOT,
  OpType::SHL1,
  OpType::SHR1,
  OpType::SHR1,
  OpType::SHR4,
  OpType::SHR16,
};

static const OpType ALL_BINARY_OP_TYPES[] = {
  OpType::AND,
  OpType::OR,
  OpType::XOR,
  OpType::PLUS,
};

std::vector<uint64_t> ParseNumberSet(const std::string& s) {
  std::vector<uint64_t> results;
  std::string param = s;
  for (auto& c : param) {
    if (c == ',') {
      c = ' ';
    }
  }
  std::stringstream ss(param);
  for(std::string op; ss >> op; ) {
    results.push_back(strtoull(op.c_str(), NULL, 0));
  }
  return results;
}

struct OpNot {
  uint64_t operator()(uint64_t value) const { return ~value; }
};
struct OpShl1 {
  uint64_t operator()(uint64_t value) const { return value << 1; }
};
struct OpShr1 {
  uint64_t operator()(uint64_t value) const { return value >> 1; }
};
struct OpShr4 {
  uint64_t operator()(uint64_t value) const { return value >> 4; }
};
struct OpShr16 {
  uint64_t operator()(uint64_t value) const { return value >> 16; }
};

template<typename T>
Key EvalUnaryInternal(const Key& input, T op) {
  Key result;
  result.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    result.push_back(op(input[i]));
  }
  return result;
}

Key EvalUnaryImmediate(OpType type, const Key& value) {
  switch (type) {
    case OpType::NOT: return EvalUnaryInternal(value, OpNot());
    case OpType::SHL1: return EvalUnaryInternal(value, OpShl1());
    case OpType::SHR1: return EvalUnaryInternal(value, OpShr1());
    case OpType::SHR4: return EvalUnaryInternal(value, OpShr4());
    case OpType::SHR16: return EvalUnaryInternal(value, OpShr16());
    default:
      LOG(FATAL) << "Unknown UnaryOpType.";
  }
}

struct AndOp {
  uint64_t operator()(uint64_t value1, uint64_t value2) const { return value1 & value2; }
};
struct OrOp {
  uint64_t operator()(uint64_t value1, uint64_t value2) const { return value1 | value2; }
};
struct XorOp {
  uint64_t operator()(uint64_t value1, uint64_t value2) const { return value1 ^ value2; }
};
struct PlusOp {
  uint64_t operator()(uint64_t value1, uint64_t value2) const { return value1 + value2; }
};

template<typename T>
Key EvalBinaryInternal(const Key& input1, const Key& input2, T op) {
  DCHECK_EQ(input1.size(), input2.size());
  Key result;
  result.reserve(input1.size());
  for (size_t i = 0; i < input1.size(); ++i) {
    result.push_back(op(input1[i], input2[i]));
  }
  return result;
}

Key EvalBinaryImmediate(OpType type, const Key& value1, const Key& value2) {
  switch (type) {
    case OpType::AND: return EvalBinaryInternal(value1, value2, AndOp());
    case OpType::OR: return EvalBinaryInternal(value1, value2, OrOp());
    case OpType::XOR: return EvalBinaryInternal(value1, value2, XorOp());
    case OpType::PLUS: return EvalBinaryInternal(value1, value2, PlusOp());
    default:
      LOG(FATAL) << "Unknown BinaryOpType.";
  }
}

Key EvalIfImmediate(const Key& value1, const Key& value2, const Key& value3) {
  Key result;
  result.reserve(value1.size());
  for (size_t i = 0; i < value1.size(); ++i) {
    result.push_back(value1[i] == 0 ? value2[i] : value3[i]);
  }
  return result;
}

std::shared_ptr<Expr> MakeExpression(const std::map<Key, int>& size_dict,
                                     const std::map<Key, Back> expr_dicts[],
                                     const Key& key) {
  auto found_size = size_dict.find(key);
  if (found_size == size_dict.end()) {
    LOG(ERROR) << "Not found";
    return std::shared_ptr<Expr>();
  }
  auto found = expr_dicts[found_size->second].find(key);
  switch (found->second.type) {
  case NOT:
    return UnaryOpExpr::Create(
        UnaryOpExpr::Type::NOT,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1));
  case SHL1:
    return UnaryOpExpr::Create(
        UnaryOpExpr::Type::SHL1,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1));
  case SHR1:
    return UnaryOpExpr::Create(
        UnaryOpExpr::Type::SHR1,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1));
  case SHR4:
    return UnaryOpExpr::Create(
        UnaryOpExpr::Type::SHR4,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1));
  case SHR16:
    return UnaryOpExpr::Create(
        UnaryOpExpr::Type::SHR16,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1));
  case AND:
    return BinaryOpExpr::Create(
        BinaryOpExpr::Type::AND,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1),
        MakeExpression(size_dict, expr_dicts, *found->second.arg2));
  case OR:
    return BinaryOpExpr::Create(
        BinaryOpExpr::Type::OR,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1),
        MakeExpression(size_dict, expr_dicts, *found->second.arg2));
  case XOR:
    return BinaryOpExpr::Create(
        BinaryOpExpr::Type::XOR,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1),
        MakeExpression(size_dict, expr_dicts, *found->second.arg2));
  case PLUS:
    return BinaryOpExpr::Create(
        BinaryOpExpr::Type::PLUS,
        MakeExpression(size_dict, expr_dicts, *found->second.arg1),
        MakeExpression(size_dict, expr_dicts, *found->second.arg2));
  case IF0:
    return If0Expr::Create(
        MakeExpression(size_dict, expr_dicts, *found->second.arg1),
        MakeExpression(size_dict, expr_dicts, *found->second.arg2),
        MakeExpression(size_dict, expr_dicts, *found->second.arg3));
  case FOLD:
    LOG(FATAL) << "Unsupported: FOLD";
  case TFOLD:
    LOG(FATAL) << "Unsupported: TFOLD";
  case LAMBDA:
    LOG(FATAL) << "Unexpected: Lambda";
  case CONSTANT:
    return found->first[0] ? ConstantExpr::CreateOne() : ConstantExpr::CreateZero();
  case ID:
    // TODO
    return IdExpr::CreateX();
  default:
    LOG(FATAL) << "Unknown OpType.";
  }
}

enum CardinalMode {
  SOLVE, CONDITION, BONUS_CONDITION,
};

// TODO: The argument to be a vector to pass multiple arguments.
std::shared_ptr<Expr> Cardinal(const Key& arguments,
                               const Key& expecteds,
                               int max_size, int op_type_set,
                               CardinalMode mode) {
  std::vector<OpType> unary_op_types;
  for (OpType type : ALL_UNARY_OP_TYPES) {
    if (op_type_set & type) unary_op_types.push_back(type);
  }

  std::vector<OpType> binary_op_types;
  for (OpType type : ALL_BINARY_OP_TYPES) {
    if (op_type_set & type) binary_op_types.push_back(type);
  }

  // { Output => Minimum Size }
  std::map<Key, int> size_dict;

  // TODO: To be std::map<std::vector<uint64_t>, std::shared_ptr<Expr> > dict;
  // [output when |x0| is given, output when |x1| is given, ...] => backtrack
  // { Size => { Output => Expr } }
  std::map<Key, Back> expr_dicts[30];

  // unstable.insert(make_pair([0, 0, 0], ConstantExpr::CreateZero()));
  Key zeroes, ones;
  for (size_t i = 0; i < arguments.size(); ++i) {
    zeroes.push_back(0);
    ones.push_back(1);
  }
  expr_dicts[1].insert(std::make_pair(zeroes,
                                      Back { OpType::CONSTANT }));
  size_dict.insert(std::make_pair(zeroes, 1));
  // unstable.insert(make_pair([1, 1, 1], ConstantExpr::CreateOne()));
  expr_dicts[1].insert(std::make_pair(ones,
                                      Back { OpType::CONSTANT }));
  size_dict.insert(std::make_pair(ones, 1));
  // unstable.insert(make_pair([a0, a1, a2], IdExpr::CreateX()));
  expr_dicts[1].insert(std::make_pair(arguments,
                                      Back { OpType::ID })); // Assumes ID = X
  size_dict.insert(std::make_pair(arguments, 1));

  for (int size = 2; size <= max_size; ++size) {
    LOG(INFO) << "Size = " << size;

    for (const auto& pair : expr_dicts[size - 1]) {
      for (OpType type : unary_op_types) {
        Key new_value = EvalUnaryImmediate(type, pair.first);
        if (size_dict.insert(std::make_pair(new_value, size)).second) {
          expr_dicts[size].insert(std::make_pair(new_value,
                                                 Back { type, &pair.first }));
        }
      }
    }
    LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : Unary done";

    for (int arg1_size = 1; arg1_size < size - 1; ++arg1_size) {
      int arg2_size = size - 1 - arg1_size;
      for (const auto& pair1 : expr_dicts[arg1_size]) {
        for (const auto& pair2 : expr_dicts[arg2_size]) {
          for (OpType type : binary_op_types) {
            Key new_value = EvalBinaryImmediate(type, pair1.first, pair2.first);
            if (size_dict.insert(std::make_pair(new_value, size)).second) {
              expr_dicts[size].insert(std::make_pair(new_value,
                                                     Back { type, &pair1.first, &pair2.first }));
            }
          }
        }
      }
    }
    LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : Binary done";

    if (op_type_set & IF0) {
      for (int arg1_size = 1; arg1_size < size - 2; ++arg1_size) {
        for (int arg2_size = 1; arg2_size < size - arg1_size - 1; ++arg2_size) {
          int arg3_size = size - 1 - arg1_size - arg2_size;
          for (const auto& pair1 : expr_dicts[arg1_size]) {
            for (const auto& pair2 : expr_dicts[arg2_size]) {
              for (const auto& pair3 : expr_dicts[arg3_size]) {
                Key new_value = EvalIfImmediate(pair1.first, pair2.first, pair3.first);
                if (size_dict.insert(std::make_pair(new_value, size)).second) {
                  expr_dicts[size].insert(std::make_pair(new_value,
                                                         Back { OpType::IF0,
                                                                &pair1.first, &pair2.first, &pair3.first }));
                }
              }
            }
          }
        }
      }
    }
    LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : If0 done";

    if (mode == CONDITION || mode == BONUS_CONDITION) {
      for (auto& item : expr_dicts[size]) {
        const Key& k1 = item.first;
        bool mismatch = false;
        for (size_t i = 0; i < expecteds.size(); ++i) {
          if (mode == CONDITION) {
            if ((k1[i] == 0 && expecteds[i] != 0) ||
                (k1[i] != 0 && expecteds[i] == 0)) {
              mismatch = true;
              break;
            }
          } else {
            // !!!BONUS MODE!!!
            if ((k1[i] & 1) != expecteds[i]) {
              mismatch = true;
              break;
            }
          }
        }
        if (!mismatch) {
          return MakeExpression(size_dict, expr_dicts, k1);
        }
      }

    } else {
      // TODO: Find it earlier.
      std::map<Key, Back>::iterator found = expr_dicts[size].find(expecteds);
      if (found != expr_dicts[size].end()) {
        break;
      }
    }
  }

#if 0
  std::vector<std::pair<int, Key> > stack;
  int depth = 0;
  Key value = expecteds;

  while (true) {
    std::map<Key, int>::const_iterator found_size = size_dict.find(value);
    if (found_size == size_dict.end()) {
      std::cout << "Not found." << std::endl;
      break;
    }
    int size = found_size->second;
    std::map<Key, Back>::iterator found = expr_dicts[size].find(value);

    for (int i = 0; i < depth; ++i) {
      std::cout << " ";
    }
    std::cout << found->first << " <== "
              << OpTypeToString(found->second.type) << " [size="
              << size << "]" << std::endl;

    if (found->second.type == CONSTANT ||
        found->second.type == ID) {
      if (stack.empty()) {
        break;
      }
      std::pair<int, Key> popped = stack.back();
      stack.pop_back();
      depth = popped.first;
      value = popped.second;
    } else if (found->second.type == AND ||
               found->second.type == OR ||
               found->second.type == XOR ||
               found->second.type == PLUS) {
      ++depth;
      value = *found->second.arg1;
      stack.push_back(std::make_pair(depth, *found->second.arg2));
    } else if (found->second.type == IF0) {
      ++depth;
      value = *found->second.arg1;
      stack.push_back(std::make_pair(depth, *found->second.arg2));
      stack.push_back(std::make_pair(depth, *found->second.arg3));
    } else { // Unary
      ++depth;
      value = *found->second.arg1;
    }
  }
#endif
  return MakeExpression(size_dict, expr_dicts, expecteds);
}


int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  std::vector<uint64_t> arguments = ParseNumberSet(FLAGS_argument);
  std::vector<uint64_t> expecteds = ParseNumberSet(FLAGS_expected);
  bool is_bonus = false;
  int op_type_set = ParseOpTypeSetWithBonus(FLAGS_operators, &is_bonus);

  if (!FLAGS_refinement_argument.empty()) {
    std::vector<uint64_t> refinement_arguments = ParseNumberSet(FLAGS_refinement_argument);
    std::vector<uint64_t> refinement_expecteds = ParseNumberSet(FLAGS_refinement_expected);
    std::vector<uint64_t> condition_arguments;
    std::vector<uint64_t> condition_expecteds;
    for (size_t i = 0; i < arguments.size(); ++i) {
      condition_arguments.push_back(arguments[i]);
      condition_expecteds.push_back(1);
    }
    for (size_t i = 0; i < refinement_arguments.size(); ++i) {
      condition_arguments.push_back(refinement_arguments[i]);
      condition_expecteds.push_back(0);
    }

    std::shared_ptr<Expr> cond_expr =
        Cardinal(condition_arguments, condition_expecteds, FLAGS_size, op_type_set,
                 is_bonus ? BONUS_CONDITION : CONDITION);
    std::shared_ptr<Expr> then_body =
        Cardinal(refinement_arguments, refinement_expecteds, FLAGS_size, op_type_set, SOLVE);
    std::shared_ptr<Expr> else_body =
        Cardinal(arguments, expecteds, FLAGS_size, op_type_set, SOLVE);
    std::shared_ptr<Expr> expr =
        is_bonus ?
        LambdaExpr::Create(If0Expr::Create(
            BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                                 cond_expr, ConstantExpr::CreateOne()),
            then_body, else_body)) :
        LambdaExpr::Create(If0Expr::Create(cond_expr, then_body, else_body));
    std::cout << *expr << std::endl;
    return 0;
  }

  std::shared_ptr<Expr> expr =
      LambdaExpr::Create(Cardinal(arguments, expecteds, FLAGS_size, op_type_set, SOLVE));
  std::cout << *expr << std::endl;

  return 0;
}
