#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "expr.h"
#include "eugeo.h"

using namespace icfpc;

const int kListBodyMax = 9;

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

struct Key {
  bool has_fold;
  std::vector<uint64_t> arguments;
};

bool operator<(const Key& k1, const Key& k2) {
  return k1.has_fold != k2.has_fold ? k1.has_fold < k2.has_fold : k1.arguments < k2.arguments;
}


std::ostream& operator<<(std::ostream& os, const Key& key) {
  os << "{has_fold: " << (key.has_fold ? "true" : "false") << "} [";
  if (key.arguments.size() > 0) {
    os << key.arguments[0];
    for (size_t i = 1; i < key.arguments.size(); ++i) {
      os << ", " << key.arguments[i];
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
    abort();
  }
}

struct Back {
  OpType type;
  const Key* arg1;
  const Key* arg2;
  const Key* arg3;
  std::shared_ptr<Expr> fold_body;
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
  Key result { input.has_fold };
  result.arguments.reserve(input.arguments.size());
  for (size_t i = 0; i < input.arguments.size(); ++i) {
    result.arguments.push_back(op(input.arguments[i]));
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
      abort();
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
  DCHECK_EQ(input1.arguments.size(), input2.arguments.size());
  Key result { bool(input1.has_fold | input2.has_fold) };
  result.arguments.reserve(input1.arguments.size());
  for (size_t i = 0; i < input1.arguments.size(); ++i) {
    result.arguments.push_back(op(input1.arguments[i], input2.arguments[i]));
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
      abort();
  }
}

Key EvalIfImmediate(const Key& value1, const Key& value2, const Key& value3) {
  Key result { bool(value1.has_fold | value2.has_fold | value3.has_fold) };
  result.arguments.reserve(value1.arguments.size());
  for (size_t i = 0; i < value1.arguments.size(); ++i) {
    result.arguments.push_back(value1.arguments[i] == 0 ? value2.arguments[i] : value3.arguments[i]);
  }
  return result;
}

struct FoldOp {
  uint64_t operator()(uint64_t x, uint64_t v, uint64_t init, const Expr& body) {
    Env env;
    env.x = x;
    for (size_t i = 0; i < 8; ++i, v >>= 8) {
      env.y = (v & 0xFF);
      env.z = init;
      init = body.Eval(env);
    }
    return init;
  }
};

Key EvalFoldImmediate(
    const std::vector<uint64_t>& arguments, const Key& value1, const Key& value2, const Expr& body) {
  Key result { true };
  result.arguments.reserve(arguments.size());
  for (size_t i = 0; i < value1.arguments.size(); ++i) {
    result.arguments.push_back(
        FoldOp()(arguments[i], value1.arguments[i], value2.arguments[i], body));
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
    return FoldExpr::Create(
        MakeExpression(size_dict, expr_dicts, *found->second.arg1),
        MakeExpression(size_dict, expr_dicts, *found->second.arg2),
        found->second.fold_body);
  case TFOLD:
    LOG(FATAL) << "Unsupported: TFOLD";
  case LAMBDA:
    LOG(FATAL) << "Unexpected: Lambda";
  case CONSTANT:
    return found->first.arguments[0] ? ConstantExpr::CreateOne() : ConstantExpr::CreateZero();
  case ID:
    // TODO
    return IdExpr::CreateX();
  default:
    LOG(FATAL) << "Unknown OpType.";
    abort();
  }
}

enum CardinalMode {
  SOLVE, CONDITION, BONUS_CONDITION,
};

// TODO: The argument to be a vector to pass multiple arguments.
std::shared_ptr<Expr> Cardinal(const std::vector<uint64_t>& arguments,
                               const std::vector<uint64_t>& expecteds,
                               int max_size, int op_type_set,
                               CardinalMode mode,
                               int timeout_sec) {
  std::vector<OpType> unary_op_types;
  for (OpType type : ALL_UNARY_OP_TYPES) {
    if (op_type_set & type) unary_op_types.push_back(type);
  }

  std::vector<OpType> binary_op_types;
  for (OpType type : ALL_BINARY_OP_TYPES) {
    if (op_type_set & type) binary_op_types.push_back(type);
  }

  // TODO
  std::vector<std::vector<std::shared_ptr<Expr> > > fold_body_list
      = ListFoldBody(kListBodyMax);

  // { Output => Minimum Size }
  std::map<Key, int> size_dict;

  // TODO: To be std::map<std::vector<uint64_t>, std::shared_ptr<Expr> > dict;
  // [output when |x0| is given, output when |x1| is given, ...] => backtrack
  // { Size => { Output => Expr } }
  std::map<Key, Back> expr_dicts[30];

  // unstable.insert(make_pair([0, 0, 0], ConstantExpr::CreateZero()));
  std::vector<uint64_t> zeroes(arguments.size(), 0), ones(arguments.size(), 1);
  expr_dicts[1].insert(std::make_pair(Key { false, zeroes },
                                      Back { OpType::CONSTANT }));
  size_dict.insert(std::make_pair(Key { false, zeroes }, 1));
  // unstable.insert(make_pair([1, 1, 1], ConstantExpr::CreateOne()));
  expr_dicts[1].insert(std::make_pair(Key { false, ones },
                                      Back { OpType::CONSTANT }));
  size_dict.insert(std::make_pair(Key { false, ones }, 1));
  // unstable.insert(make_pair([a0, a1, a2], IdExpr::CreateX()));
  expr_dicts[1].insert(std::make_pair(Key { false, arguments },
                                      Back { OpType::ID })); // Assumes ID = X
  size_dict.insert(std::make_pair(Key { false, arguments }, 1));

  time_t x = time(NULL);
  for (int size = 2; size <= max_size; ++size) {
    LOG(INFO) << "Size = " << size;

    // Randomize operator order.
    std::random_shuffle(unary_op_types.begin(), unary_op_types.end());
    for (const auto& pair : expr_dicts[size - 1]) {
      if (pair.first.has_fold) break;
      for (OpType type : unary_op_types) {
        Key new_value = EvalUnaryImmediate(type, pair.first);
        // TODO
        if (size_dict.insert(std::make_pair(new_value, size)).second) {
          expr_dicts[size].insert(std::make_pair(new_value,
                                                 Back { type, &pair.first }));
          if ((size_dict.size() & 0x3FFF) == 0) {
            time_t y = time(NULL);
            if (y - x > timeout_sec) {
              return std::shared_ptr<Expr>();
            }
          }
        }
      }
    }
    LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : Unary done";

    // Randomize operator order.
    std::random_shuffle(binary_op_types.begin(), binary_op_types.end());
    for (int arg1_size = 1; arg1_size < size - 1; ++arg1_size) {
      int arg2_size = size - 1 - arg1_size;
      for (const auto& pair1 : expr_dicts[arg1_size]) {
        if (pair1.first.has_fold) break;
        for (const auto& pair2 : expr_dicts[arg2_size]) {
          if (pair2.first.has_fold) break;
          for (OpType type : binary_op_types) {
            Key new_value = EvalBinaryImmediate(type, pair1.first, pair2.first);
            // TODO
            if (size_dict.insert(std::make_pair(new_value, size)).second) {
              expr_dicts[size].insert(std::make_pair(new_value,
                                                     Back { type, &pair1.first, &pair2.first }));
              if ((size_dict.size() & 0x3FFF) == 0) {
                time_t y = time(NULL);
                if (y - x > timeout_sec) {
                  return std::shared_ptr<Expr>();
                }
              }
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
            if (pair1.first.has_fold) break;
            for (const auto& pair2 : expr_dicts[arg2_size]) {
              if (pair2.first.has_fold) break;
              for (const auto& pair3 : expr_dicts[arg3_size]) {
                if (pair3.first.has_fold) break;
                Key new_value = EvalIfImmediate(pair1.first, pair2.first, pair3.first);
                // TODO
                if (size_dict.insert(std::make_pair(new_value, size)).second) {
                  expr_dicts[size].insert(std::make_pair(new_value,
                                                         Back { OpType::IF0,
                                                                &pair1.first, &pair2.first, &pair3.first }));
                  if ((size_dict.size() & 0x3FFF) == 0) {
                    time_t y = time(NULL);
                    if (y - x > timeout_sec) {
                      return std::shared_ptr<Expr>();
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : If0 done";

    if (op_type_set & OpType::FOLD) {  // TODO size
      // Randomize operator order.
      std::random_shuffle(unary_op_types.begin(), unary_op_types.end());
      for (const auto& pair : expr_dicts[size - 1]) {
        if (!pair.first.has_fold) continue;
        for (OpType type : unary_op_types) {
          Key new_value = EvalUnaryImmediate(type, pair.first);

          // Found non-has-fold entry.
          if (size_dict.find(Key { false, new_value.arguments}) != size_dict.end()) continue;
          if (size_dict.insert(std::make_pair(new_value, size)).second) {
            expr_dicts[size].insert(std::make_pair(new_value,
                                                   Back { type, &pair.first }));
            if ((size_dict.size() & 0x3FFF) == 0) {
              time_t y = time(NULL);
              if (y - x > timeout_sec) {
                return std::shared_ptr<Expr>();
              }
            }
          }
        }
      }
      LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : Fold Unary done";

      // Randomize operator order.
      std::random_shuffle(binary_op_types.begin(), binary_op_types.end());
      for (int arg1_size = 1; arg1_size < size - 1; ++arg1_size) {
        int arg2_size = size - 1 - arg1_size;
        for (const auto& pair1 : expr_dicts[arg1_size]) {
          for (const auto& pair2 : expr_dicts[arg2_size]) {
            if (!(pair1.first.has_fold | pair2.first.has_fold)) continue;
            for (OpType type : binary_op_types) {
              Key new_value = EvalBinaryImmediate(type, pair1.first, pair2.first);

              // Found non-has-fold entry.
              if (size_dict.find(Key { false, new_value.arguments}) != size_dict.end()) continue;
              if (size_dict.insert(std::make_pair(new_value, size)).second) {
                expr_dicts[size].insert(std::make_pair(new_value,
                                                       Back { type, &pair1.first, &pair2.first }));
                if ((size_dict.size() & 0x3FFF) == 0) {
                  time_t y = time(NULL);
                  if (y - x > timeout_sec) {
                    return std::shared_ptr<Expr>();
                  }
                }
              }
            }
          }
        }
      }
      LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : Fold Binary done";

      if (op_type_set & IF0) {
        for (int arg1_size = 1; arg1_size < size - 2; ++arg1_size) {
          for (int arg2_size = 1; arg2_size < size - arg1_size - 1; ++arg2_size) {
            int arg3_size = size - 1 - arg1_size - arg2_size;
            for (const auto& pair1 : expr_dicts[arg1_size]) {
              for (const auto& pair2 : expr_dicts[arg2_size]) {
                for (const auto& pair3 : expr_dicts[arg3_size]) {
                  if (pair1.first.has_fold | pair2.first.has_fold | pair3.first.has_fold) continue;
                  Key new_value = EvalIfImmediate(pair1.first, pair2.first, pair3.first);

                  // Found non-has-fold entry.
                  if (size_dict.find(Key { false, new_value.arguments}) != size_dict.end()) continue;
                  if (size_dict.insert(std::make_pair(new_value, size)).second) {
                    expr_dicts[size].insert(std::make_pair(new_value,
                                                           Back { OpType::IF0,
                                                                 &pair1.first, &pair2.first, &pair3.first }));
                    if ((size_dict.size() & 0x3FFF) == 0) {
                      time_t y = time(NULL);
                      if (y - x > timeout_sec) {
                        return std::shared_ptr<Expr>();
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      LOG(INFO) << "  Dict[" << size << "] = " << expr_dicts[size].size() << " : If0 done";

      // Fold
      for (int body_size = 1; body_size < std::min(size - 2, kListBodyMax); ++body_size) {
        for (int arg1_size = 1; arg1_size < size - body_size - 1; ++arg1_size) {
          int arg2_size = size - 1 - body_size - arg1_size;
          for (const auto& pair1 : expr_dicts[arg1_size]) {
            if (pair1.first.has_fold) break;
            for (const auto& pair2 : expr_dicts[arg2_size]) {
              if (pair2.first.has_fold) break;
              for (const std::shared_ptr<Expr>& body : fold_body_list[body_size]) {
                Key new_value = EvalFoldImmediate(arguments, pair1.first, pair2.first, *body);

                // Found non-has-fold entry.
                if (size_dict.find(Key { false, new_value.arguments}) != size_dict.end()) continue;
                if (size_dict.insert(std::make_pair(new_value, size)).second) {
                  expr_dicts[size].insert(std::make_pair(new_value,
                                                         Back { OpType::FOLD,
                                                               &pair1.first, &pair2.first, NULL, body }));
                  if ((size_dict.size() & 0x3FFF) == 0) {
                    time_t y = time(NULL);
                    if (y - x > timeout_sec) {
                      return std::shared_ptr<Expr>();
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (mode == CONDITION || mode == BONUS_CONDITION) {
      for (auto& item : expr_dicts[size]) {
        const Key& k1 = item.first;
        bool mismatch = false;
        for (size_t i = 0; i < expecteds.size(); ++i) {
          if (mode == CONDITION) {
            if ((k1.arguments[i] == 0 && expecteds[i] != 0) ||
                (k1.arguments[i] != 0 && expecteds[i] == 0)) {
              mismatch = true;
              break;
            }
          } else {
            // !!!BONUS MODE!!!
            if ((k1.arguments[i] & 1) != expecteds[i]) {
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
      std::map<Key, Back>::iterator found = expr_dicts[size].find(Key { false, expecteds });
      if (found == expr_dicts[size].end())
        found = expr_dicts[size].find(Key { true, expecteds });
      if (found != expr_dicts[size].end()) {
        return MakeExpression(size_dict, expr_dicts, found->first);
      }
    }
  }

  return std::shared_ptr<Expr>();

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
}


void InitializeEugeo() {
  ListFoldBody(kListBodyMax);
}

int main(int argc, char* argv[]) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::ios::sync_with_stdio(false);

  InitializeEugeo();
  std::cout << "ready" << std::endl;

  int timeout_sec;
  int expr_size;
  int op_type_set;
  bool is_bonus;
  std::vector<uint64_t> arguments;
  std::vector<uint64_t> expecteds;
  std::vector<uint64_t> refinement_arguments;
  std::vector<uint64_t> refinement_expecteds;

  size_t tfold_position = 0;
  for (std::string line; std::getline(std::cin, line); ) {
    // READ REQUEST!
    CHECK_EQ("request1", line);
    CHECK(std::getline(std::cin, line));
    if (line == "0") {
      LOG(INFO) << "Net Problem!";
      tfold_position = 0;
    }

    // timeout_sec
    CHECK(std::getline(std::cin, line));
    timeout_sec = strtoull(line.c_str(), NULL, 0);
    // expr_size
    CHECK(std::getline(std::cin, line));
    expr_size = strtoull(line.c_str(), NULL, 0);
    // op_type_set / is_bonus
    CHECK(std::getline(std::cin, line));
    is_bonus = false;
    op_type_set = ParseOpTypeSetWithBonus(line, &is_bonus);
    // arguments
    CHECK(std::getline(std::cin, line));
    arguments = ParseNumberSet(line);
    // expecteds
    CHECK(std::getline(std::cin, line));
    expecteds = ParseNumberSet(line);
    // refinement_arguments
    CHECK(std::getline(std::cin, line));
    refinement_arguments = ParseNumberSet(line);
    // refinement_expecteds
    CHECK(std::getline(std::cin, line));
    refinement_expecteds = ParseNumberSet(line);
    // random_seed
    CHECK(std::getline(std::cin, line));
    std::srand(strtoull(line.c_str(), NULL, 0));

    if ((op_type_set & OpType::TFOLD) && tfold_position == 0) {
      std::vector<uint64_t> arguments2 = arguments;
      arguments2.insert(arguments2.end(), refinement_arguments.begin(), refinement_arguments.end());
      std::vector<uint64_t> expecteds2 = expecteds;
      expecteds2.insert(expecteds2.end(), refinement_expecteds.begin(), refinement_expecteds.end());

      bool should_quit = false;
      // time_t x = time(NULL);
      // int count = 0;
      for (auto& body_list : ListFoldBody(kListBodyMax)) {
        for (auto& body : body_list) {
          // ++count;
          bool mismatch = false;
          for (size_t i = 0; i < arguments2.size(); ++i) {
            if (expecteds2[i] != FoldOp()(arguments2[i], arguments2[i], 0, *body)) {
              mismatch = true;
              break;
            }
          }
          if (!mismatch) {
            should_quit = true;
            std::cout << *LambdaExpr::Create(FoldExpr::CreateTFold(body)) << std::endl;
            break;
          }
          // if ((count & 0x3FF) == 0) {
          //   time_t y = time(NULL);
          //   if (y - x > timeout_sec) {
          //     should_quit = true;
          //     std::cout << std::endl;
          //     break;
          //   }
          // }
        }
        if (should_quit) {
          break;
        }
      }
      if (should_quit) continue;

      // Fallback to the normal search with FOLD operation.
      op_type_set = (op_type_set & ~OpType::TFOLD) | OpType::FOLD;
    }
    tfold_position = 1;

    if (!refinement_arguments.empty()) {
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

      // TODO
      std::shared_ptr<Expr> cond_expr =
          Cardinal(condition_arguments, condition_expecteds, expr_size, (op_type_set & ~OpType::FOLD),
                   is_bonus ? BONUS_CONDITION : CONDITION,
                   timeout_sec);
      if (!cond_expr.get()) {
        std::cout << std::endl;
        continue;
      }
      std::shared_ptr<Expr> then_body =
          Cardinal(refinement_arguments, refinement_expecteds, expr_size, op_type_set, SOLVE,
                   timeout_sec);
      if (!then_body.get()) {
        std::cout << std::endl;
        continue;
      }
      std::shared_ptr<Expr> else_body =
          Cardinal(arguments, expecteds, expr_size, op_type_set, SOLVE,
                   timeout_sec);
      if (!else_body.get()) {
        std::cout << std::endl;
        continue;
      }
      std::shared_ptr<Expr> expr =
          is_bonus ?
          LambdaExpr::Create(If0Expr::Create(
              BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                                   cond_expr, ConstantExpr::CreateOne()),
              then_body, else_body)) :
          LambdaExpr::Create(If0Expr::Create(cond_expr, then_body, else_body));
      std::cout << *expr << std::endl;
    } else {
      std::shared_ptr<Expr> body =
          Cardinal(arguments, expecteds, expr_size, op_type_set, SOLVE, timeout_sec);
      if (!body.get()) {
        std::cout << std::endl;
        continue;
      }

      std::shared_ptr<Expr> expr = LambdaExpr::Create(body);
      std::cout << *expr << std::endl;
    }
  }  // end of request loop

  return 0;
}
