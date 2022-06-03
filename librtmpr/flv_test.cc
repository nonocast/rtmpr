#include <gtest/gtest.h>
extern "C" {
#include "flv.h"
#include "log.h"
#include <libgen.h>
#include <stdio.h>
#include <string.h>
}

typedef uint8_t byte;

namespace {
class FLVTest : public testing::Test {
protected:
  void SetUp() override {
    RTMP_LogSetLevel(RTMP_LOGALL);
  }
};

TEST_F(FLVTest, address) {
  size_t len = 54556;
  uint8_t *payload = (uint8_t *) malloc(4);
  memset(payload, 0xff, 4);

  uint8_t *buffer = (uint8_t *) malloc(20);
  memset(buffer, 0x00, 20);
  uint8_t *len_ptr = (uint8_t *) &len;
  *(buffer + 0) = *(len_ptr + 3);
  *(buffer + 1) = *(len_ptr + 2);
  *(buffer + 2) = *(len_ptr + 1);
  *(buffer + 3) = *(len_ptr + 0);

  memcpy(buffer + 4, payload, 4);

  RTMP_LogHex(RTMP_LOGINFO, buffer, 20);

  free(buffer);
}

TEST_F(FLVTest, basename1) {
  char path[] = "sample.flv";
  char *name = basename(path);
  EXPECT_EQ(strcmp(name, "sample.flv"), 0);
}

TEST_F(FLVTest, basename2) {
  char path[] = "assert/sample.flv";
  char *name = basename(path);
  EXPECT_EQ(strcmp(name, "sample.flv"), 0);
}

TEST_F(FLVTest, ftell) {
  FILE *fp = fopen(__FILE__, "rb");
  fseek(fp, 5, SEEK_SET);
  EXPECT_EQ(5, ftell(fp));

  fseek(fp, 3, SEEK_SET);
  EXPECT_EQ(3, ftell(fp));

  fseek(fp, 7, SEEK_CUR);
  EXPECT_EQ(10, ftell(fp));
}
} // namespace