#include <stdlib.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG(INFO) << "Hello, World!";

  // ＿人人 人人＿
  // ＞ 突然の死 ＜
  // ￣Y^Y^Y^Y￣
  abort();

  return 0;
}
