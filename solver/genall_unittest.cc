#include <glog/logging.h>
#include <gtest/gtest.h>

#include "expr.h"
#include "expr_list.h"
#include "expr_list_old.h"

using namespace icfpc;

TEST(GenAllTest, A) {
  int size = 3;
  int op_type_set = ParseOpTypeSet("not");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

TEST(GenAllTest, B) {
  int size = 10;
  int op_type_set = ParseOpTypeSet("not,if0,fold,plus");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

TEST(GenAllTest, C) {
  int size = 10;
  int op_type_set = ParseOpTypeSet("shr1,if0,shl1,or");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

TEST(GenAllTest, D) {
  int size = 10;
  int op_type_set = ParseOpTypeSet("shl1,if0,tfold");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

TEST(GenAllTest, E) {
  int size = 5;
  int op_type_set = ParseOpTypeSet("shl1,if0,tfold");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

TEST(GenAllTest, F) {
  int size = 6;
  int op_type_set = ParseOpTypeSet("shl1,if0,tfold");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

TEST(GenAllTest, G) {
  int size = 7;
  int op_type_set = ParseOpTypeSet("shl1,if0,tfold");
  std::vector<std::shared_ptr<Expr> > result = ListExpr(size, op_type_set, NO_SIMPLIFY);
  std::vector<std::shared_ptr<Expr> > result_old = old::ListExpr(size, op_type_set);
  EXPECT_EQ(result_old.size(), result.size());
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_TRUE(result[i]->EqualTo(*result_old[i]));
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
