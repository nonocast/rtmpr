#ifndef __RTMP_X_H__
#define __RTMP_X_H__

#include "log.h"
#include "rtmp.h"
#include <stdlib.h>
#include <string.h>

int RTMPx_SetupURL(RTMP *r, const char *url);
void RTMPx_Close(RTMP *r);
size_t RTMPx_MakeVideoNALUTag(const int32_t timestamp, const uint8_t *nalu, size_t nalu_length, uint8_t *buffer, size_t buffer_size);
size_t RTMPx_MakeVideoMetadataTag(const uint8_t *sps, size_t sps_size, const uint8_t *pps, size_t pps_size, uint8_t *buffer, size_t buffer_size);
size_t RTMPx_MakeAVCDecoderConfigurationRecord(const uint8_t *sps, size_t sps_size, const uint8_t *pps, size_t pps_size, uint8_t *buffer, size_t buffer_size);

#endif