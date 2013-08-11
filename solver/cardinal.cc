#include <map>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"

using namespace icfpc;


DEFINE_string(argument, "", "");
DEFINE_string(expected, "", "");
DEFINE_int32(size, 30, "");
DEFINE_string(operators, "", "List of the operators");


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


Key EvalUnaryImmediate(UnaryOpExpr::Type type, const Key& value) {
  Key result;
  switch (type) {
  case UnaryOpExpr::Type::NOT:
    for (auto x : value)
      result.push_back(~x);
    return result;
  case UnaryOpExpr::Type::SHL1:
    for (auto x : value)
      result.push_back(x << 1);
    return result;
  case UnaryOpExpr::Type::SHR1:
    for (auto x : value)
      result.push_back(x >> 1);
    return result;
  case UnaryOpExpr::Type::SHR4:
    for (auto x : value)
      result.push_back(x >> 4);
    return result;
  case UnaryOpExpr::Type::SHR16:
    for (auto x : value)
      result.push_back(x >> 16);
    return result;
  default:
    LOG(FATAL) << "Unknown UnaryOpType.";
  }
}


Key EvalBinaryImmediate(BinaryOpExpr::Type type,
                             const Key& value1, const Key& value2) {
  Key result;
  switch (type) {
  case BinaryOpExpr::Type::AND:
    for (size_t i = 0; i < value1.size(); ++i)
      result.push_back(value1[i] & value2[i]);
    return result;
  case BinaryOpExpr::Type::OR:
    for (size_t i = 0; i < value1.size(); ++i)
      result.push_back(value1[i] | value2[i]);
    return result;
  case BinaryOpExpr::Type::XOR:
    for (size_t i = 0; i < value1.size(); ++i)
      result.push_back(value1[i] ^ value2[i]);
    return result;
  case BinaryOpExpr::Type::PLUS:
    for (size_t i = 0; i < value1.size(); ++i)
      result.push_back(value1[i] + value2[i]);
    return result;
  default:
    LOG(FATAL) << "Unknown BinaryOpType.";
  }
}


// TODO: The argument to be a vector to pass multiple arguments.
void Cardinal(const Key& arguments,
              const Key& expecteds,
              int max_size, int op_type_set) {
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
  size_dict.insert(std::make_pair(Key {0, 0, 0}, 1));
  // unstable.insert(make_pair([1, 1, 1], ConstantExpr::CreateOne()));
  expr_dicts[1].insert(std::make_pair(ones,
                                      Back { OpType::CONSTANT }));
  size_dict.insert(std::make_pair(Key {1, 1, 1}, 1));
  // unstable.insert(make_pair([a0, a1, a2], IdExpr::CreateX()));
  expr_dicts[1].insert(std::make_pair(arguments,
                                      Back { OpType::ID })); // Assumes ID = X
  size_dict.insert(std::make_pair(arguments, 1));

  for (int size = 2; size <= max_size; ++size) {
    for (const auto& pair : expr_dicts[size - 1]) {
      for (UnaryOpExpr::Type type : UNARY_OP_TYPES) {
        Key new_value = EvalUnaryImmediate(type, pair.first);
        if (size_dict.insert(std::make_pair(new_value, size)).second) {
          expr_dicts[size].insert(std::make_pair(new_value,
                                                 Back { UnaryOpExpr::ToOpType(type), &pair.first }));
        }
      }
    }

    for (int arg1_size = 1; arg1_size < size - 1; ++arg1_size) {
      int arg2_size = size - 1 - arg1_size;
      for (const auto& pair1 : expr_dicts[arg1_size]) {
        for (const auto& pair2 : expr_dicts[arg2_size]) {
          for (BinaryOpExpr::Type type : BINARY_OP_TYPES) {
            Key new_value = EvalBinaryImmediate(type, pair1.first, pair2.first);
            if (size_dict.insert(std::make_pair(new_value, size)).second) {
              expr_dicts[size].insert(std::make_pair(new_value,
                                                     Back { BinaryOpExpr::ToOpType(type),
                                                            &pair1.first, &pair2.first }));
            }
          }
        }
      }
    }

    // TODO: Find it earlier.
    std::map<Key, Back>::iterator found = expr_dicts[size].find(expecteds);
    if (found != expr_dicts[size].end()) {
      break;
    }
  }

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
    } else {
      value = *found->second.arg1;
      ++depth;
    }
  }
}


int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  std::vector<uint64_t> arguments = ParseNumberSet(FLAGS_argument);
  std::vector<uint64_t> expecteds = ParseNumberSet(FLAGS_expected);
  int op_type_set = ParseOpTypeSet(FLAGS_operators);

  Cardinal(arguments, expecteds, FLAGS_size, op_type_set);
  // std::cout << *expr << std::endl;

  return 0;
}
