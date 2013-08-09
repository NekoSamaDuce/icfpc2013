#ifndef ICFPC_EXPR_H_
#define ICFPC_EXPR_H_

#include <iostream>
#include <sstream>
#include <memory>
#include <string>

#include "util.h"

namespace icfpc {

#define DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete; \
  void operator=(const type&) = delete;

enum OpType {
  NOT = 1 << 0,
  SHL1 = 1 << 1,
  SHR1 = 1 << 2,
  SHR4 = 1 << 3,
  SHR16 = 1 << 4,
  AND = 1 << 5,
  OR = 1 << 6,
  XOR = 1 << 7,
  PLUS = 1 << 8,
  IF0 = 1 << 9,
  FOLD = 1 << 10,
  TFOLD = 1 << 11,

  // Virtual type.
  LAMBDA = 1 << 12,
  CONSTANT = 1 << 13,
  ID = 1 << 14,
};

struct Env {
  uint64_t x;
  uint64_t y;
  uint64_t z;
};

// An interface of the expression.
class Expr {
 public:
  virtual ~Expr() {
  }

  std::string ToString() const {
    std::stringstream stream;
    stream << *this;
    return stream.str();
  }

  std::size_t depth() const { return depth_; }
  bool in_fold() const { return in_fold_; }
  bool has_fold() const { return has_fold_; }

  // Returns the OpType of this expression. Maybe virtual type (e.g. CONSTANT or ID).
  OpType op_type() const { return op_type_; }

  // Returns the set of OpType, including the ones for subtrees.
  int op_type_set() const { return op_type_set_; }

  uint64_t Eval(const Env& env) const {
    return EvalImpl(env);
  }

  bool EqualTo(const Expr& other) const {
    if (op_type() != other.op_type()) return false;
    return EqualToImpl(other);
  }

  int CompareTo(const Expr& other) const {
    if (this == &other) return 0;
    if (op_type() != other.op_type())
      return op_type() < other.op_type() ? -1 : 1;
    return CompareToImpl(other);
  }

 protected:
  friend std::ostream& operator<<(std::ostream&, const Expr&);

  Expr(OpType op_type, int op_type_set, std::size_t depth,
       bool in_fold, bool has_fold)
      : op_type_(op_type), op_type_set_(op_type_set), depth_(depth),
        in_fold_(in_fold), has_fold_(has_fold) {}
  virtual void Output(std::ostream* os) const = 0;
  virtual uint64_t EvalImpl(const Env& env) const = 0;
  virtual bool EqualToImpl(const Expr& other) const = 0;
  virtual int CompareToImpl(const Expr& other) const = 0;

  OpType op_type_;
  int op_type_set_;

  std::size_t depth_;
  bool in_fold_;
  bool has_fold_;

  DISALLOW_COPY_AND_ASSIGN(Expr);
};

std::ostream& operator<<(std::ostream& os, const Expr& e) {
  e.Output(&os);
  return os;
}

// The arg name is fixed to "x" as this is only appeared only at the top of a valid
// expr.
class LambdaExpr : public Expr {
 public:
  LambdaExpr(std::shared_ptr<Expr> body)
      : Expr(OpType::LAMBDA, body->op_type_set(), 1 + body->depth(),
             body->in_fold(), body->has_fold()),
        body_(body) {
  }

  static std::shared_ptr<LambdaExpr> Create(std::shared_ptr<Expr> body) {
    return std::make_shared<LambdaExpr>(body);
  }

  const std::shared_ptr<Expr>& body() const { return body_; }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(lambda (x) " << *body_ << ")";
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    return body_->Eval(env);
  }

  virtual bool EqualToImpl(const Expr& expr) const {
    return body_->EqualTo(*static_cast<const LambdaExpr&>(expr).body_);
  }

  virtual int CompareToImpl(const Expr& expr) const {
    return body_->CompareTo(*static_cast<const LambdaExpr&>(expr).body_);
  }

 private:
  std::shared_ptr<Expr> body_;
};

class ConstantExpr : public Expr {
 public:
  explicit ConstantExpr(uint64_t value)
      : Expr(OpType::CONSTANT, 0, 1, false, false), value_(value) {}

  static std::shared_ptr<ConstantExpr> Create(uint64_t value) {
    if (value == 0) return CreateZero();
    if (value == 1) return CreateOne();
    return std::make_shared<ConstantExpr>(value);
  }

  static std::shared_ptr<ConstantExpr> CreateZero() {
    static std::shared_ptr<ConstantExpr> instance(new ConstantExpr(0));
    return instance;
  }

  static std::shared_ptr<ConstantExpr> CreateOne() {
    static std::shared_ptr<ConstantExpr> instance(new ConstantExpr(1));
    return instance;
  }

  uint64_t value() const { return value_; }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << value_;
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    return static_cast<uint64_t>(value_);
  }

  virtual bool EqualToImpl(const Expr& other) const {
    return value_ == static_cast<const ConstantExpr&>(other).value_;
  }

  virtual int CompareToImpl(const Expr& other) const {
    uint64_t v = static_cast<const ConstantExpr&>(other).value_;
    if (value_ != v)
      return value_ < v ? -1 : 1;
    return 0;
  }

 private:
  uint64_t value_;
};

class IdExpr : public Expr {
 public:
  enum Name { X, Y, Z, };

  explicit IdExpr(Name name) :
      Expr(OpType::ID, 0, 1, name != Name::X, false), name_(name) {
  }

  static std::shared_ptr<IdExpr> Create(Name name) {
    switch (name) {
      case Name::X: return CreateX();
      case Name::Y: return CreateY();
      case Name::Z: return CreateZ();
    }
    NOTREACHED();
  }

  static std::shared_ptr<IdExpr> CreateX() {
    static std::shared_ptr<IdExpr> instance(new IdExpr(Name::X));
    return instance;
  }

  static std::shared_ptr<IdExpr> CreateY() {
    static std::shared_ptr<IdExpr> instance(new IdExpr(Name::Y));
    return instance;
  }

  static std::shared_ptr<IdExpr> CreateZ() {
    static std::shared_ptr<IdExpr> instance(new IdExpr(Name::Z));
    return instance;
  }

  Name name() const { return name_; }

 protected:
  virtual void Output(std::ostream* os) const {
    switch (name_) {
      case Name::X: *os << "x"; break;
      case Name::Y: *os << "y"; break;
      case Name::Z: *os << "z"; break;
    }
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    switch (name_) {
      case Name::X: return env.x;
      case Name::Y: return env.y;
      case Name::Z: return env.z;
    }
    return 0;
  }

  virtual bool EqualToImpl(const Expr& other) const {
    return name_ == static_cast<const IdExpr&>(other).name_;
  }

  virtual int CompareToImpl(const Expr& other) const {
    Name n = static_cast<const IdExpr&>(other).name_;
    if (name_ != n)
      return name_ < n ? -1 : 1;
    return 0;
  }

 private:
  Name name_;
};

class If0Expr : public Expr {
 public:
  If0Expr(std::shared_ptr<Expr> cond,
          std::shared_ptr<Expr> then_body,
          std::shared_ptr<Expr> else_body)
      : Expr(OpType::IF0,
             OpType::IF0 | cond->op_type_set() | then_body->op_type_set() | else_body->op_type_set(),
             1 + cond->depth() + then_body->depth() + else_body->depth(),
             cond->in_fold() | then_body->in_fold() | else_body->in_fold(),
             cond->has_fold() | then_body->has_fold() | else_body->has_fold()),
        cond_(cond), then_body_(then_body), else_body_(else_body) {
  }

  static std::shared_ptr<If0Expr> Create(std::shared_ptr<Expr> cond,
                                         std::shared_ptr<Expr> then_body,
                                         std::shared_ptr<Expr> else_body) {
    return std::make_shared<If0Expr>(cond, then_body, else_body);
  }

  const std::shared_ptr<Expr>& cond() const { return cond_; }
  const std::shared_ptr<Expr>& then_body() const { return then_body_; }
  const std::shared_ptr<Expr>& else_body() const { return else_body_; }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(if0 " << *cond_ << " " << *then_body_ << " " << *else_body_ << ")";
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    uint64_t cond = cond_->Eval(env);
    if (cond == 0) {
      return then_body_->Eval(env);
    } else {
      return else_body_->Eval(env);
    }
  }

  virtual bool EqualToImpl(const Expr& other) const {
    const If0Expr& expr = static_cast<const If0Expr&>(other);
    return cond_->EqualTo(*expr.cond_)
        && then_body_->EqualTo(*expr.then_body_)
        && else_body_->EqualTo(*expr.else_body_);
  }

  virtual int CompareToImpl(const Expr& other) const {
    const If0Expr& expr = static_cast<const If0Expr&>(other);
    int cmp = cond_->CompareTo(*expr.cond_);
    if (cmp != 0) return cmp;
    cmp = then_body_->CompareTo(*expr.then_body_);
    if (cmp != 0) return cmp;
    return else_body_->CompareTo(*expr.else_body_);
  }

 private:
  std::shared_ptr<Expr> cond_;
  std::shared_ptr<Expr> then_body_;
  std::shared_ptr<Expr> else_body_;
};

// Fix id to Y and Z, as Fold appears at most once in the valid expr.
class FoldExpr : public Expr {
 public:
  FoldExpr(std::shared_ptr<Expr> value,
           std::shared_ptr<Expr> init_value,
           std::shared_ptr<Expr> body)
      : Expr(OpType::FOLD,
             OpType::FOLD | value->op_type_set() | init_value->op_type_set() | body->op_type_set(),
             2 + value->depth() + init_value->depth() + body->depth(), false, true),
        value_(value), init_value_(init_value), body_(body) {
  }

  explicit FoldExpr(std::shared_ptr<Expr> body)
      : Expr(OpType::FOLD,  // Use FOLD. not TFOLD.
             OpType::TFOLD | body->op_type_set(),
             2 + 1 + 1 + body->depth(), false, true),
        value_(IdExpr::CreateX()), init_value_(ConstantExpr::CreateZero()), body_(body) {
  }

  static std::shared_ptr<FoldExpr> Create(std::shared_ptr<Expr> value,
                                          std::shared_ptr<Expr> init_value,
                                          std::shared_ptr<Expr> body) {
    return std::make_shared<FoldExpr>(value, init_value, body);
  }

  static std::shared_ptr<FoldExpr> CreateTFold(std::shared_ptr<Expr> body) {
    return std::make_shared<FoldExpr>(body);
  }

  const std::shared_ptr<Expr>& value() const { return value_; }
  const std::shared_ptr<Expr>& init_value() const { return init_value_; }
  const std::shared_ptr<Expr>& body() const { return body_; }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(fold " << *value_ << " " << *init_value_
        << " (lambda (y z) " << *body_ << "))";
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    uint64_t value = value_->Eval(env);
    uint64_t acc = init_value_->Eval(env);

    Env env2 = env;
    for (size_t i = 0; i < 8; ++i, value >>= 8) {
      env2.y = (value & 0xFF);
      env2.z = acc;
      acc = body_->Eval(env2);
    }
    return acc;
  }

  virtual bool EqualToImpl(const Expr& other) const {
    const FoldExpr& expr = static_cast<const FoldExpr&>(other);
    return value_->EqualTo(*expr.value_)
        && init_value_->EqualTo(*expr.init_value_)
        && body_->EqualTo(*expr.body_);
  }

  virtual int CompareToImpl(const Expr& other) const {
    const FoldExpr& expr = static_cast<const FoldExpr&>(other);
    int cmp = value_->CompareTo(*expr.value_);
    if (cmp != 0) return cmp;
    cmp = init_value_->CompareTo(*expr.init_value_);
    if (cmp != 0) return cmp;
    return body_->CompareTo(*expr.body_);
  }

 private:
  std::shared_ptr<Expr> value_;
  std::shared_ptr<Expr> init_value_;
  std::shared_ptr<Expr> body_;
};

class UnaryOpExpr : public Expr {
 public:
  enum class Type {
    NOT, SHL1, SHR1, SHR4, SHR16,
  };

  UnaryOpExpr(Type type, std::shared_ptr<Expr> arg)
      : Expr(ToOpType(type), ToOpType(type) | arg->op_type_set(),
             1 + arg->depth(), arg->in_fold(), arg->has_fold()),
        type_(type), arg_(arg) {
  }

  static std::shared_ptr<UnaryOpExpr> Create(Type type, std::shared_ptr<Expr> arg) {
    return std::make_shared<UnaryOpExpr>(type, arg);
  }

  Type type() const { return type_; }
  const std::shared_ptr<Expr>& arg() const { return arg_; }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(";
    switch (type_) {
      case Type::NOT: *os << "not"; break;
      case Type::SHL1: *os << "shl1"; break;
      case Type::SHR1: *os << "shr1"; break;
      case Type::SHR4: *os << "shr4"; break;
      case Type::SHR16: *os << "shr16"; break;
      default: NOTREACHED();
    }
    *os << " " << *arg_ << ")";
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    uint64_t v1 = arg_->Eval(env);
    switch (type_) {
      case Type::NOT: return ~v1;
      case Type::SHL1: return v1 << 1;
      case Type::SHR1: return v1 >> 1;
      case Type::SHR4: return v1 >> 4;
      case Type::SHR16: return v1 >> 16;
      default: NOTREACHED();
    }
    return 0;
  }

  virtual bool EqualToImpl(const Expr& other) const {
    const UnaryOpExpr& expr = static_cast<const UnaryOpExpr&>(other);
    return type_ == expr.type_ && arg_->EqualTo(*expr.arg_);
  }

  virtual int CompareToImpl(const Expr& other) const {
    const UnaryOpExpr& expr = static_cast<const UnaryOpExpr&>(other);
    if (type_ != expr.type_)
      return type_ < expr.type_ ? -1 : 1;
    return arg_->CompareTo(*expr.arg_);
  }

 private:
  static OpType ToOpType(Type type) {
    switch (type) {
      case Type::NOT: return OpType::NOT;
      case Type::SHL1: return OpType::SHL1;
      case Type::SHR1: return OpType::SHR1;
      case Type::SHR4: return OpType::SHR4;
      case Type::SHR16: return OpType::SHR16;
      default: NOTREACHED();
    }
    return static_cast<OpType>(-1);
  }
  Type type_;
  std::shared_ptr<Expr> arg_;
};

class BinaryOpExpr : public Expr {
 public:
  enum class Type {
    AND, OR, XOR, PLUS,
  };

  BinaryOpExpr(Type type, std::shared_ptr<Expr> arg1, std::shared_ptr<Expr> arg2)
      : Expr(ToOpType(type), ToOpType(type) | arg1->op_type_set() | arg2->op_type_set(),
             1 + arg1->depth() + arg2->depth(),
             arg1->in_fold() | arg2->in_fold(),
             arg1->has_fold() | arg2->has_fold()),
        type_(type), arg1_(arg1), arg2_(arg2) {
  }

  static std::shared_ptr<BinaryOpExpr> Create(
      Type type, std::shared_ptr<Expr> arg1, std::shared_ptr<Expr> arg2) {
    return std::make_shared<BinaryOpExpr>(type, arg1, arg2);
  }

  Type type() const { return type_; }
  const std::shared_ptr<Expr>& arg1() const { return arg1_; }
  const std::shared_ptr<Expr>& arg2() const { return arg2_; }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(";
    switch (type_) {
      case Type::AND: *os << "and"; break;
      case Type::OR: *os << "or"; break;
      case Type::XOR: *os << "xor"; break;
      case Type::PLUS: *os << "plus"; break;
      default: NOTREACHED();
    }
    *os << " " << *arg1_ << " " << *arg2_ << ")";
  }

  virtual uint64_t EvalImpl(const Env& env) const {
    uint64_t v1 = arg1_->Eval(env);
    uint64_t v2 = arg2_->Eval(env);
    switch (type_) {
      case Type::AND: return v1 & v2;
      case Type::OR: return v1 | v2;
      case Type::XOR: return v1 ^ v2;
      case Type::PLUS: return v1 + v2;
      default: NOTREACHED();
    }
    return 0;
  }

  virtual bool EqualToImpl(const Expr& other) const {
    const BinaryOpExpr& expr = static_cast<const BinaryOpExpr&>(other);
    return type_ == expr.type_ && arg1_->EqualTo(*expr.arg1_) && arg2_->EqualTo(*expr.arg2_);
  }

  virtual int CompareToImpl(const Expr& other) const {
    const BinaryOpExpr& expr = static_cast<const BinaryOpExpr&>(other);
    if (type_ != expr.type_)
      return type_ < expr.type_ ? -1 : 1;
    int cmp = arg1_->CompareTo(*expr.arg1_);
    if (cmp != 0) return cmp;
    return arg2_->CompareTo(*expr.arg2_);
  }

 private:
  static OpType ToOpType(Type type) {
    switch(type) {
      case Type::AND: return OpType::AND;
      case Type::OR: return OpType::OR;
      case Type::XOR: return OpType::XOR;
      case Type::PLUS: return OpType::PLUS;
      default: NOTREACHED();
    }
    return static_cast<OpType>(-1);
  }

  Type type_;
  std::shared_ptr<Expr> arg1_;
  std::shared_ptr<Expr> arg2_;
};

uint64_t Eval(const Expr& e, uint64_t x) {
  Env env;
  env.x = x;
  return e.Eval(env);
}

OpType ParseOpType(const std::string& s) {
  if (s == "not") {
    return OpType::NOT;
  }
  if (s == "shl1") {
    return OpType::SHL1;
  }
  if (s == "shr1") {
    return OpType::SHR1;
  }
  if (s == "shr4") {
    return OpType::SHR4;
  }
  if (s == "shr16") {
    return OpType::SHR16;
  }
  if (s == "and") {
    return OpType::AND;
  }
  if (s == "or") {
    return OpType::OR;
  }
  if (s == "xor") {
    return OpType::XOR;
  }
  if (s == "plus") {
    return OpType::PLUS;
  }
  if (s == "if0") {
    return OpType::IF0;
  }
  if (s == "fold") {
    return OpType::FOLD;
  }
  if (s == "tfold") {
    return OpType::TFOLD;
  }
  NOTREACHED();
}

int ParseOpTypeSet(const std::string& s) {
  int op_type_set = 0;
  std::string param = s;
  for (auto& c : param) {
    if (c == ',') {
      c = ' ';
    }
  }
  std::stringstream ss(param);
  for(std::string op; ss >> op; ) {
    op_type_set |= ParseOpType(op);
  }
  return op_type_set;
}

}  // namespace icpfc

#endif  // ICFPC_EXPR_H_
