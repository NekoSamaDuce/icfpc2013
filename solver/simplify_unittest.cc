// TODO(dmikurube): Fold: y, z is not included
// TODO(dmikurube): Fold: body is z
// TODO(dmikurube): Try Eval both for simplified and naive.

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "cluster.h"
#include "expr.h"
#include "expr_list.h"
#include "parser.h"
#include "simplify.h"

using namespace icfpc;

const uint64_t g_fullbits = ~((uint64_t)0);


// Combined

TEST(SimplifyCombinedTest, Xor_NotX_Full_ReducedToX) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         UnaryOpExpr::Create(UnaryOpExpr::Type::NOT,
                                             IdExpr::CreateX()),
                         ConstantExpr::CreateFull());
  std::shared_ptr<Expr> s = Simplify(e);

  // Reduced
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}


// Fold

TEST(SimplifyFoldTest, ZeroZero_OrYZ_AsIs) {
  std::shared_ptr<Expr> e = Parse("(fold 0 0 (lambda (y z) (or y z)))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyFoldTest, XZero_OrYZ_AsIs) {
  std::shared_ptr<Expr> e = Parse("(fold x 0 (lambda (y z) (or y z)))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyFoldTest, XOne_OrYZ_AsIs) {
  std::shared_ptr<Expr> e = Parse("(fold x 1 (lambda (y z) (or y z)))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyFoldTest, XZero_BodyZero_NonYZ) {
  std::shared_ptr<Expr> e = Parse("(fold x 0 (lambda (y z) 0))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateZero()));
}

TEST(SimplifyFoldTest, XOne_BodyZero_NonYZ) {
  std::shared_ptr<Expr> e = Parse("(fold x 1 (lambda (y z) 0))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateZero()));
}

TEST(SimplifyFoldTest, XZero_BodyX_NonYZ) {
  std::shared_ptr<Expr> e = Parse("(fold x 0 (lambda (y z) x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*IdExpr::CreateX()));
}

TEST(SimplifyFoldTest, XOne_BodyX_NonYZ) {
  std::shared_ptr<Expr> e = Parse("(fold x 1 (lambda (y z) x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*IdExpr::CreateX()));
}

TEST(SimplifyFoldTest, XZero_BodyY_AsIs) {
  std::shared_ptr<Expr> e = Parse("(fold x 0 (lambda (y z) z))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateZero()));
}

TEST(SimplifyFoldTest, XOne_BodyY_AsIs) {
  std::shared_ptr<Expr> e = Parse("(fold x 1 (lambda (y z) z))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateOne()));
}

TEST(SimplifyFoldTest, XZero_BodyZ_ReducedToZero) {
  std::shared_ptr<Expr> e = Parse("(fold x 0 (lambda (y z) z))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateZero()));
}

TEST(SimplifyFoldTest, XOne_BodyZ_ReducedToOne) {
  std::shared_ptr<Expr> e = Parse("(fold x 1 (lambda (y z) z))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateOne()));
}


// Shift

TEST(SimplifyShiftTest, Shl1_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shl1 x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shl1Shr1_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shl1 (shr1 x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr1Shl1_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr1 (shl1 x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr1_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr1 x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr1Shr4_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr1 (shr4 x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr4Shr1_Swapped) {
  std::shared_ptr<Expr> e = Parse("(shr4 (shr1 x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(shr1 (shr4 x))")));
}

TEST(SimplifyShiftTest, Shr4_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr4 x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr4Shr16_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr4 (shr16 x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr16Shr4_Swapped) {
  std::shared_ptr<Expr> e = Parse("(shr16 (shr4 x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(shr4 (shr16 x))")));
}

TEST(SimplifyShiftTest, Shr16_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr16 x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyShiftTest, Shr1Shr1Shr1Shr1_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr1 (shr1 (shr1 (shr1 x))))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(shr4 x)")));
}

TEST(SimplifyShiftTest, Shr4Shr4Shr4Shr4_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr4 (shr4 (shr4 (shr4 x))))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(shr16 x)")));
}

TEST(SimplifyShiftTest, Shr16Shr16Shr16Shr16_AsIs) {
  std::shared_ptr<Expr> e = Parse("(shr16 (shr16 (shr16 (shr16 x))))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}


// Not

TEST(SimplifyNotTest, X_AsIs) {
  std::shared_ptr<Expr> e = Parse("(not x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*e));
}

TEST(SimplifyNotTest, NotX_AsIs) {
  std::shared_ptr<Expr> e = Parse("(not (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*IdExpr::CreateX()));
}


// Or

TEST(SimplifyOrTest, NotXNotX_ReducedToZero) {
  std::shared_ptr<Expr> e = Parse("(or (not x) (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(not x)")));
}

TEST(SimplifyOrTest, NotXX_ReducedToFull) {
  std::shared_ptr<Expr> e = Parse("(or (not x) x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, XNotX_ReducedToFull) {
  std::shared_ptr<Expr> e = Parse("(or x (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, XX_ReducedToX) {
  std::shared_ptr<Expr> e = Parse("(or x x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*IdExpr::CreateX()));
}

TEST(SimplifyOrTest, XFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, FullX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateFull(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, XOne_Swapped) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateOne());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), OR);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  // Swapped.
  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyOrTest, OneX_AsIs) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateOne(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), OR);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyOrTest, XZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyOrTest, ZeroX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateZero(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyOrTest, FullFull_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, OneFull_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, FullOne_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, ZeroFull_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, FullZero_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyOrTest, OneOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyOrTest, OneZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyOrTest, ZeroOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyOrTest, ZeroZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::OR,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}


// And

TEST(SimplifyAndTest, NotXNotX_ReducedToNotX) {
  std::shared_ptr<Expr> e = Parse("(and (not x) (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(not x)")));
}

TEST(SimplifyAndTest, NotXX_ReducedToZero) {
  std::shared_ptr<Expr> e = Parse("(and (not x) x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, XNotX_ReducedToZero) {
  std::shared_ptr<Expr> e = Parse("(and x (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, XX_ReducedToX) {
  std::shared_ptr<Expr> e = Parse("(and x x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*IdExpr::CreateX()));
}

TEST(SimplifyAndTest, XOne_Swapped) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateOne());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), AND);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  // Swapped.
  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyAndTest, OneX_AsIs) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateOne(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), AND);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyAndTest, XZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, ZeroX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateZero(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, FullX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateFull(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyAndTest, XFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyAndTest, FullFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyAndTest, OneFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyAndTest, FullOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyAndTest, ZeroFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, FullZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, OneOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyAndTest, OneZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, ZeroOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyAndTest, ZeroZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::AND,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}


// Xor

TEST(SimplifyXorTest, NotXNotX_ReducedToZero) {
  std::shared_ptr<Expr> e = Parse("(xor (not x) (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateZero()));
}

TEST(SimplifyXorTest, NotXX_ReducedToFull) {
  std::shared_ptr<Expr> e = Parse("(xor (not x) x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyXorTest, XNotX_ReducedToFull) {
  std::shared_ptr<Expr> e = Parse("(xor x (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyXorTest, XX_ReducedToZero) {
  std::shared_ptr<Expr> e = Parse("(xor x x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*ConstantExpr::CreateZero()));
}

TEST(SimplifyXorTest, XFull_ReducedToNot) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateFull());
  std::shared_ptr<Expr> s = Simplify(e);

  // Reduced to NOT.
  ASSERT_EQ(s->op_type(), NOT);
  std::shared_ptr<UnaryOpExpr> t = std::static_pointer_cast<UnaryOpExpr>(s);
  std::shared_ptr<Expr> s_arg = t->arg();
  ASSERT_EQ(s_arg->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyXorTest, FullX_ReducedToNot) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateFull(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);

  // Reduced to NOT.
  ASSERT_EQ(s->op_type(), NOT);
  std::shared_ptr<UnaryOpExpr> t = std::static_pointer_cast<UnaryOpExpr>(s);
  std::shared_ptr<Expr> s_arg = t->arg();
  ASSERT_EQ(s_arg->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyXorTest, XOne_Swapped) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateOne());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), XOR);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  // Swapped.
  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyXorTest, OneX_AsIs) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateOne(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), XOR);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyXorTest, XZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyXorTest, ZeroX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateZero(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyXorTest, FullFull_ReducedToZero) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyXorTest, OneFull_ReducedToFullMinus1) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits - 1);
}

TEST(SimplifyXorTest, FullOne_ReducedToFullMinus1) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits - 1);
}

TEST(SimplifyXorTest, ZeroFull_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyXorTest, FullZero_ReducedToFull) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyXorTest, OneOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyXorTest, OneZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyXorTest, ZeroOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyXorTest, ZeroZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::XOR,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}


// Plus

TEST(SimplifyPlusTest, NotXNotX_AsIs) {
  std::shared_ptr<Expr> e = Parse("(plus (not x) (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(shl1 (not x))")));
}

TEST(SimplifyPlusTest, NotXX_ReducedToFull) {
  std::shared_ptr<Expr> e = Parse("(plus (not x) x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyPlusTest, XNotX_ReducedToFull) {
  std::shared_ptr<Expr> e = Parse("(plus x (not x))");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyPlusTest, XX_AsIs) {
  std::shared_ptr<Expr> e = Parse("(plus x x)");
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_TRUE(s->EqualTo(*Parse("(shl1 x)")));
}

TEST(SimplifyPlusTest, XFull_Swapped) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateFull());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), PLUS);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  // Swapped.
  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), g_fullbits);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyPlusTest, FullX_AsIs) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateFull(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), PLUS);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), g_fullbits);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyPlusTest, XOne_Swapped) {
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
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyPlusTest, OneX_AsIs) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateOne(),
                         IdExpr::CreateX());
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), PLUS);
  std::shared_ptr<BinaryOpExpr> t = std::static_pointer_cast<BinaryOpExpr>(s);

  std::shared_ptr<Expr> s_arg1 = t->arg1();
  ASSERT_EQ(s_arg1->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_arg1)->value(), 1U);
  std::shared_ptr<Expr> s_arg2 = t->arg2();
  ASSERT_EQ(s_arg2->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s_arg2)->name(), IdExpr::Name::X);
}

TEST(SimplifyPlusTest, XZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         IdExpr::CreateX(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyPlusTest, ZeroX_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateZero(),
                         IdExpr::CreateX());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), ID);
  ASSERT_EQ(std::static_pointer_cast<IdExpr>(s)->name(), IdExpr::Name::X);
}

TEST(SimplifyPlusTest, FullFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits - 1);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0xFFFFFFFFFFFFFFFE);
}

TEST(SimplifyPlusTest, OneFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyPlusTest, FullOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}

TEST(SimplifyPlusTest, ZeroFull_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateFull());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyPlusTest, FullZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateFull(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), g_fullbits);
}

TEST(SimplifyPlusTest, OneOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 2U);
}

TEST(SimplifyPlusTest, OneZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateOne(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyPlusTest, ZeroOne_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateOne());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 1U);
}

TEST(SimplifyPlusTest, ZeroZero_Reduced) {
  std::shared_ptr<BinaryOpExpr> e =
    BinaryOpExpr::Create(BinaryOpExpr::Type::PLUS,
                         ConstantExpr::CreateZero(),
                         ConstantExpr::CreateZero());

  // Reduced.
  std::shared_ptr<Expr> s = Simplify(e);
  ASSERT_EQ(s->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s)->value(), 0U);
}


// Id, Constant

TEST(SimplifyIdTest, Simple) {
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

TEST(SimplifyConstantTest, Simple) {
  std::shared_ptr<ConstantExpr> zero = ConstantExpr::CreateZero();
  std::shared_ptr<ConstantExpr> one = ConstantExpr::CreateOne();
  std::shared_ptr<ConstantExpr> full = ConstantExpr::CreateFull();
  std::shared_ptr<ConstantExpr> deadbeef = ConstantExpr::Create(0xdeadbeefU);
  ASSERT_EQ(zero->value(), 0U);
  ASSERT_EQ(one->value(), 1U);
  ASSERT_EQ(full->value(), 0xFFFFFFFFFFFFFFFFU);
  ASSERT_EQ(full->value(), g_fullbits);
  ASSERT_EQ(deadbeef->value(), 0xdeadbeefU);

  std::shared_ptr<Expr> s_zero = Simplify(zero);
  ASSERT_EQ(s_zero->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_zero)->value(), 0U);

  std::shared_ptr<Expr> s_one = Simplify(one);
  ASSERT_EQ(s_one->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_one)->value(), 1U);

  std::shared_ptr<Expr> s_full = Simplify(full);
  ASSERT_EQ(s_one->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_full)->value(), 0xFFFFFFFFFFFFFFFFU);

  std::shared_ptr<Expr> s_deadbeef = Simplify(deadbeef);
  ASSERT_EQ(s_deadbeef->op_type(), CONSTANT);
  ASSERT_EQ(std::static_pointer_cast<ConstantExpr>(s_deadbeef)->value(), 0xdeadbeefU);
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
