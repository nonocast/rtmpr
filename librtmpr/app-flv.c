#include "flv.h"
#include "log.h"
#include "rtmp.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *url;
RTMP rtmp;
FLV flv;

void init();
void die();
static void sigint(int sig);
static void print_hex(const uint8_t *data, unsigned long len);

int main() {
  printf("app-flv\n");

  init();

  FLV_Open(&flv, "sample.flv");
  FLV_Parse(&flv);
  FLV_Dump(&flv);

  RTMP_Init(&rtmp);
  RTMP_SetupURL(&rtmp, url);
  RTMP_EnableWrite(&rtmp);
  RTMP_Connect(&rtmp, NULL);
  RTMP_ConnectStream(&rtmp, 0);

  byte buffer[1024 * 1024 * 8];
  fseek(flv.file, flv.metadata->offset, SEEK_SET);
  fread(buffer, 1, flv.metadata->size, flv.file);
  RTMP_Write(&rtmp, (char *) buffer, flv.metadata->size);

  while (true) {
    for (int i = 0; i < flv.videoTagCount; ++i) {
      if (RTMP_ctrlC) die();
      FLVTag *t = flv.videoTags[i];
      fseek(flv.file, t->offset, SEEK_SET);
      fread(buffer, 1, t->size, flv.file);
      printf("send size: %lu\n", t->size);

      print_hex(buffer, t->size);
      // RTMP_Write(&rtmp, (char *) buffer, t->size);

      // printf(".");
      // fflush(stdout);
      usleep(1000000 / 10); // fps: 10
    }
  }

  return 0;
}

void init() {
  RTMP_LogSetLevel(RTMP_LOGDEBUG);
  url = "rtmp://shgbit.xyz/live/7";
  signal(SIGINT, sigint);
}

void die() {
  RTMP_Close(&rtmp);
  FLV_Close(&flv);
  exit(0);
}

void sigint(int sig) {
  RTMP_ctrlC = TRUE;
}

static void print_hex(const uint8_t *data, unsigned long len) {
  if (len <= 32) {
    for (int i = 0; i < len; ++i) {
      printf("%02x ", data[i]);
    }
    printf("\n");
  } else {
    for (int i = 0; i < 24; ++i) {
      printf("%02x ", data[i]);
    }
    printf("...... ");
    for (int i = 4; i > 0; --i) {
      printf("%02x ", data[len - i]);
    }
    printf("\n");
  }
}
