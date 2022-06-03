#include "rtmpwrapper.h"

char *mutable_url;

int RTMP_SetupURLEx(RTMP *r, const char *url) {
  mutable_url = malloc(strlen(url) + 1);
  strcpy(mutable_url, url);
  return RTMP_SetupURL(r, mutable_url);
}

void RTMP_CloseEx(RTMP *r) {
  RTMP_Close(r);
  if (mutable_url) {
    free(mutable_url);
    mutable_url = NULL;
  }
}