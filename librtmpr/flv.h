#ifndef __FLV_H__
#define __FLV_H__

// written by nonocast
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef unsigned char byte;

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define MAX_TAG 4096
#define FLV_HEAD_SIZE 9
#define FLV_TAG_HEAD_SIZE 11

enum tag_types { TAGTYPE_AUDIODATA = 8, TAGTYPE_VIDEODATA = 9, TAGTYPE_SCRIPTDATAOBJECT = 18 };
static const char *flv_tag_types[] = {"", "", "", "", "", "", "", "", "audio", "video", "", "", "", "", "", "", "", "", "data"};

typedef struct FLVTag {
  byte type;
  byte head[11];
  off_t offset;
  size_t size;
  off_t data_offset;
  size_t data_size;
  int32_t timestamp;
} FLVTag;

typedef struct FLV {
  char *path;
  char *filename;
  FILE *file;
  FLVTag *metadata;
  FLVTag **tags;
  size_t tagCount;
  FLVTag **videoTags;
  size_t videoTagCount;
  FLVTag **audioTags;
  size_t audioTagCount;
} FLV;

void FLV_Open(FLV *flv, char *filename);
void FLV_Parse(FLV *flv);
void FLV_Close(FLV *flv);
void FLV_Dump(FLV *flv);

FLVTag *FLV_ReadTag(FLV *flv);

static void Read_UI24(uint32_t *out, void *ptr);
static void Read_UI32(uint32_t *out, void *ptr);
static void DumpHex(FILE *file, const uint8_t *data, unsigned long len, const char *prefix);
#endif