#ifndef __RTMPWRAPPER_H__
#define __RTMPWRAPPER_H__

#include "rtmp.h"
#include <stdlib.h>
#include <string.h>

int RTMP_SetupURLEx(RTMP *r, const char *url);
void RTMP_CloseEx(RTMP *r);

#endif