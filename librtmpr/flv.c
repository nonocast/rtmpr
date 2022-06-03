#include "flv.h"

static const char hexdig[] = "0123456789abcdef";

void FLV_Open(FLV *flv, char *path) {
  flv->path = path;
  flv->filename = basename(path);
  flv->file = fopen(path, "rb");
  flv->tags = malloc(sizeof(FLVTag *) * 4096);
  flv->videoTags = malloc(sizeof(FLVTag *) * 4096);
  flv->audioTags = malloc(sizeof(FLVTag *) * 4096);
}

void FLV_Parse(FLV *flv) {
  fseek(flv->file, FLV_HEAD_SIZE + 4, SEEK_SET);

  FLVTag *current;
  while ((current = FLV_ReadTag(flv)) != NULL) {
    flv->tags[flv->tagCount++] = current;
    if (current->type == TAGTYPE_VIDEODATA) {
      flv->videoTags[flv->videoTagCount++] = current;
    } else if (current->type == TAGTYPE_AUDIODATA) {
      flv->audioTags[flv->audioTagCount++] = current;
    }
  }

  flv->metadata = flv->tags[0];
}

FLVTag *FLV_ReadTag(FLV *flv) {
  // save offset
  off_t offset = ftell(flv->file);

  byte head[FLV_TAG_HEAD_SIZE];
  if (FLV_TAG_HEAD_SIZE != fread(head, 1, FLV_TAG_HEAD_SIZE, flv->file)) return NULL;

  FLVTag *tag = malloc(sizeof(FLVTag));
  memcpy(tag->head, head, FLV_TAG_HEAD_SIZE);

  tag->type = *tag->head;
  tag->offset = offset;
  tag->data_offset = tag->offset + FLV_TAG_HEAD_SIZE;
  Read_UI24((uint32_t *) &tag->data_size, tag->head + 1);
  Read_UI24((uint32_t *) &tag->timestamp, tag->head + 4);
  tag->timestamp |= *(tag->head + 7) << 24;
  tag->size = tag->data_size + FLV_TAG_HEAD_SIZE;

  if (fseek(flv->file, tag->data_size + 4, SEEK_CUR) != 0) return NULL;
  return tag;
}

void FLV_Close(FLV *flv) {
  fclose(flv->file);
}

void FLV_Dump(FLV *flv) {
  char path[255];
  byte video_head[5];
  strcpy(path, flv->filename);
  strcat(path, ".meta");

  FILE *fp = fopen(path, "w");

  fprintf(fp, "tags: %lu, data: 1, video: %lu, audio: %lu\n", flv->tagCount, flv->videoTagCount, flv->audioTagCount);
  // data
  FLVTag *meta = flv->metadata;
  fprintf(fp, "  tag id     type      address   tag size     payload size    timestamp\n");
  fprintf(fp, "-------- -------- ------------ ---------- ---------------- ------------\n");
  fprintf(fp, " #%06d %8s     0x%06llx %10lu %16lu %12d\n", 1, flv_tag_types[meta->type], meta->offset, meta->size, meta->data_size, meta->timestamp);
  DumpHex(fp, meta->head, FLV_TAG_HEAD_SIZE, "..");
  fprintf(fp, "\n\n");

  fprintf(fp, "  tag id     type      address   tag size     payload size    timestamp video type pkt type composition time\n");
  fprintf(fp, "-------- -------- ------------ ---------- ---------------- ------------ ---------- -------- ----------------\n");
  // video
  for (int i = 0; i < min(10, flv->videoTagCount); ++i) {
    FLVTag *t = flv->videoTags[i];
    fseek(flv->file, t->data_offset, SEEK_SET);
    fread(video_head, 1, 5, flv->file);

    uint32_t composition = 0;
    Read_UI24(&composition, video_head + 2);

    fprintf(fp, " #%06d %8s     0x%06llx %10lu %16lu %12d       0x%x      0x%x %16d\n", i + 1, flv_tag_types[t->type], t->offset, t->size, t->data_size, t->timestamp, video_head[0], video_head[1],
            composition);
    DumpHex(fp, t->head, FLV_TAG_HEAD_SIZE, "..");
    DumpHex(fp, video_head, 5, ".... .. .. .. .. .. .. .. .. .. .. ");
    fprintf(fp, "\n");
  }

  fclose(fp);
}

static void DumpHex(FILE *file, const uint8_t *data, unsigned long len, const char *prefix) {
  fprintf(file, "%s", prefix);

  unsigned long i;
  char line[50], *ptr;

  ptr = line;

  for (i = 0; i < len; i++) {
    *ptr++ = hexdig[0x0f & (data[i] >> 4)];
    *ptr++ = hexdig[0x0f & data[i]];
    if ((i & 0x0f) == 0x0f) {
      *ptr = '\0';
      ptr = line;
      fprintf(file, "%s", line);
    } else {
      *ptr++ = ' ';
    }
  }
  if (i & 0x0f) {
    *ptr = '\0';
    fprintf(file, "%s\n", line);
  }
}

static void Read_UI24(uint32_t *out, void *ptr) {
  uint8_t *bytes = (uint8_t *) ptr;
  *out = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
}

static void Read_UI32(uint32_t *out, void *ptr) {
  uint8_t *bytes = (uint8_t *) ptr;
  *out = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}