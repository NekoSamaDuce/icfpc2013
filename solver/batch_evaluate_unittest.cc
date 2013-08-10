#include <glog/logging.h>
#include <gtest/gtest.h>

#include "expr.h"
#include "expr_list.h"
#include "parser.h"

using namespace icfpc;

TEST(EvalTest, Foo) {
  ASSERT_TRUE(false) << "kaite mita dake";
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
