#ifndef ICFPC_EXPR_H_
#define ICFPC_EXPR_H_

#include <iostream>
#include <sstream>
#include <memory>
#include <string>

namespace icfpc {

#define DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete; \
  void operator=(const type&) = delete;

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

 protected:
  friend std::ostream& operator<<(std::ostream&, const Expr&);

  Expr(std::size_t depth, bool in_fold, bool has_fold)
      : depth_(depth), in_fold_(in_fold), has_fold_(has_fold) {}
  virtual void Output(std::ostream* os) const = 0;

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
      : Expr(1 + body->depth(), body->in_fold(), body->has_fold()), body_(body) {
  }

  static std::shared_ptr<LambdaExpr> Create(std::shared_ptr<Expr> body) {
    return std::make_shared<LambdaExpr>(body);
  }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(lambda (x) " << *body_ << ")";
  }

 private:
  std::shared_ptr<Expr> body_;
};

class ConstantExpr : public Expr {
 public:
  enum class Value {
    ZERO = 0,
    ONE = 1,
  };
  explicit ConstantExpr(Value value) : Expr(1, false, false), value_(value) {}

  static std::shared_ptr<ConstantExpr> Create(Value value) {
    return value == Value::ZERO ? CreateZero() : CreateOne();
  }

  static std::shared_ptr<ConstantExpr> CreateZero() {
    static std::shared_ptr<ConstantExpr> instance(new ConstantExpr(Value::ZERO));
    return instance;
  }

  static std::shared_ptr<ConstantExpr> CreateOne() {
    static std::shared_ptr<ConstantExpr> instance(new ConstantExpr(Value::ONE));
    return instance;
  }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << (value_ == Value::ZERO ? "0" : "1");
  }

 private:
  Value value_;
};

class IdExpr : public Expr {
 public:
  enum Name { X, Y, Z, };

  explicit IdExpr(Name name) : Expr(1, name != Name::X, false) ,name_(name) {
  }

  static std::shared_ptr<IdExpr> Create(Name name) {
    switch (name) {
      case Name::X: return CreateX();
      case Name::Y: return CreateY();
      case Name::Z: return CreateZ();
    }

    // Not reached
    return std::shared_ptr<IdExpr>();
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

 protected:
  virtual void Output(std::ostream* os) const {
    switch (name_) {
      case Name::X: *os << "x"; break;
      case Name::Y: *os << "y"; break;
      case Name::Z: *os << "z"; break;
    }
  }

 private:
  Name name_;
};

class If0Expr : public Expr {
 public:
  If0Expr(std::shared_ptr<Expr> cond,
          std::shared_ptr<Expr> then_body,
          std::shared_ptr<Expr> else_body)
      : Expr(1 + cond->depth() + then_body->depth() + else_body->depth(),
             cond->in_fold() | then_body->in_fold() | else_body->in_fold(),
             cond->has_fold() | then_body->has_fold() | else_body->has_fold()),
        cond_(cond), then_body_(then_body), else_body_(else_body) {
  }

  static std::shared_ptr<If0Expr> Create(std::shared_ptr<Expr> cond,
                                         std::shared_ptr<Expr> then_body,
                                         std::shared_ptr<Expr> else_body) {
    return std::make_shared<If0Expr>(cond, then_body, else_body);
  }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(if0 " << *cond_ << " " << *then_body_ << " " << *else_body_ << ")";
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
      : Expr(2 + value->depth() + init_value->depth() + body->depth(), false, true),
        value_(value), init_value_(init_value), body_(body) {
  }

  static std::shared_ptr<FoldExpr> Create(std::shared_ptr<Expr> value,
                                          std::shared_ptr<Expr> init_value,
                                          std::shared_ptr<Expr> body) {
    return std::make_shared<FoldExpr>(value, init_value, body);
  }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(fold " << *value_ << " " << *init_value_
        << " (lambda (y z) " << *body_ << "))";
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
      : Expr(1 + arg->depth(), arg->in_fold(), arg->has_fold()), type_(type), arg_(arg) {
  }

  static std::shared_ptr<UnaryOpExpr> Create(Type type, std::shared_ptr<Expr> arg) {
    return std::make_shared<UnaryOpExpr>(type, arg);
  }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(";
    switch (type_) {
      case Type::NOT: *os << "not"; break;
      case Type::SHL1: *os << "shl1"; break;
      case Type::SHR1: *os << "shr1"; break;
      case Type::SHR4: *os << "shr4"; break;
      case Type::SHR16: *os << "shrt16"; break;
    }
    *os << " " << *arg_ << ")";
  }

 private:
  Type type_;
  std::shared_ptr<Expr> arg_;
};

class BinaryOpExpr : public Expr {
 public:
  enum class Type {
    AND, OR, XOR, PLUS,
  };

  BinaryOpExpr(Type type, std::shared_ptr<Expr> arg1, std::shared_ptr<Expr> arg2)
      : Expr(1 + arg1->depth() + arg2->depth(),
             arg1->in_fold() | arg2->in_fold(),
             arg1->has_fold() | arg2->has_fold()),
        type_(type), arg1_(arg1), arg2_(arg2) {
  }

  static std::shared_ptr<BinaryOpExpr> Create(
      Type type, std::shared_ptr<Expr> arg1, std::shared_ptr<Expr> arg2) {
    return std::make_shared<BinaryOpExpr>(type, arg1, arg2);
  }

 protected:
  virtual void Output(std::ostream* os) const {
    *os << "(";
    switch (type_) {
      case Type::AND: *os << "and"; break;
      case Type::OR: *os << "or"; break;
      case Type::XOR: *os << "xor"; break;
      case Type::PLUS: *os << "plus"; break;
    }
    *os << " " << *arg1_ << " " << *arg2_ << ")";
  }

 private:
  Type type_;
  std::shared_ptr<Expr> arg1_;
  std::shared_ptr<Expr> arg2_;
};

}  // namespace icpfc

#endif  // ICFPC_EXPR_H_
