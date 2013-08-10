#include <glog/logging.h>
#include <gtest/gtest.h>

#include "cluster.h"
#include "expr.h"
#include "expr_list.h"
#include "simplify.h"

using namespace icfpc;

TEST(SimplifyTest, PlusXOne_Swapped) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateOne());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), PLUS);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  // Swapped.
  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyTest, PlusOneX_AsIs) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateOne(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), PLUS);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyTest, PlusXZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyTest, PlusZeroX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateZero(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyTest, PlusZeroOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> zero_one =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s_zero_one = Simplify(zero_one);
  ASSERT_EQ(s_zero_one->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_zero_one)->value(), 1);
}

TEST(SimplifyTest, Id) {
  std::shared_ptr<IdExpr> x = IdExpr::CreateX();
  std::shared_ptr<IdExpr> y = IdExpr::CreateY();
  std::shared_ptr<IdExpr> z = IdExpr::CreateZ();
  ASSERT_EQ(x->name(), IdExpr::Name::X);
  ASSERT_EQ(y->name(), IdExpr::Name::Y);
  ASSERT_EQ(z->name(), IdExpr::Name::Z);

  std::shared_ptr<Expr> s_x = Simplify(x);
  ASSERT_EQ(s_x->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_x)->name(), IdExpr::Name::X);
  std::shared_ptr<Expr> s_y = Simplify(y);
  ASSERT_EQ(s_y->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_y)->name(), IdExpr::Name::Y);
  std::shared_ptr<Expr> s_z = Simplify(z);
  ASSERT_EQ(s_z->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_z)->name(), IdExpr::Name::Z);
}

TEST(SimplifyTest, Constant) {
  std::shared_ptr<ConstantExpr> zero = ConstantExpr::CreateZero();
  std::shared_ptr<ConstantExpr> one = ConstantExpr::CreateOne();
  std::shared_ptr<ConstantExpr> deadbeef = ConstantExpr::Create((uint64_t)0xdeadbeef);
  ASSERT_EQ(zero->value(), 0);
  ASSERT_EQ(one->value(), 1);
  ASSERT_EQ(deadbeef->value(), 0xdeadbeef);

  std::shared_ptr<Expr> s_zero = Simplify(zero);
  ASSERT_EQ(s_zero->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_zero)->value(), 0);

  std::shared_ptr<Expr> s_one = Simplify(one);
  ASSERT_EQ(s_one->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_one)->value(), 1);

  std::shared_ptr<Expr> s_deadbeef = Simplify(deadbeef);
  ASSERT_EQ(s_deadbeef->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_deadbeef)->value(), 0xdeadbeef);
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
