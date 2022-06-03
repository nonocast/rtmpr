#include <gtest/gtest.h>
extern "C" {
#include "log.h"
#include "rtmpwrapper.h"
#include <stdio.h>
#include <string.h>
}

typedef uint8_t byte;

namespace {
class RTMPWrapperTest : public testing::Test {
protected:
  void SetUp() override {
    RTMP_LogSetLevel(RTMP_LOGALL);
  }
};

TEST_F(RTMPWrapperTest, string) {
  const char *name = "nonocast";
  EXPECT_EQ(strlen(name), 8);
}
} // namespace