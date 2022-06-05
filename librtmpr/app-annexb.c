#include "log.h"
#include "rtmp.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

static const char hexdig[] = "0123456789abcdef";
static const uint8_t startcode[] = {0x00, 0x00, 0x00, 0x01};
static const char *nalu_types[10] = {[0] = "Unspecified", "non-IDR", [5] = "IDR", "SEI", "SPS", "PPS", "AU delimiter"};

typedef struct frame {
  uint8_t *address;
  uint8_t first_byte;
  uint8_t nal_unit_type;
  off_t offset;
  size_t size;
} frame_t;

typedef struct annexb_stream {
  FILE *fp;
  uint8_t *buffer;
  size_t buffer_size;
  uint8_t *sps;
  size_t sps_size;
  uint8_t *pps;
  size_t pps_size;
  frame_t *frames[4096];
  size_t frame_count;
  frame_t *video_frames[4096];
  size_t video_frame_count;
} annexb_stream_t;

static void print_hex(const uint8_t *data, unsigned long len);
static void annexb_parse(annexb_stream_t *);
static void parse_frames(annexb_stream_t *, uint8_t *buffer, size_t size);
static void die();
static void sigint(int);
static void send_flv_data_tag(RTMP *, annexb_stream_t *);
static void send_video_tag(RTMP *, frame_t *);
static size_t make_avcrecord(annexb_stream_t *, uint8_t *);

char *url;
RTMP rtmp;
annexb_stream_t stream;

int main() {
  RTMP_LogSetLevel(RTMP_LOGDEBUG);
  url = "rtmp://shgbit.xyz/live/5";
  // url = "rtmp://live.nonocast.cn/foo/1";
  printf("url: %s\n", url);
  signal(SIGINT, sigint);

  memset(&stream, 0x00, sizeof(annexb_stream_t));
  annexb_parse(&stream);

  /*
    // write back - NO PROBLEM
    FILE *outfp = fopen("back.h264", "wb");
    fwrite(startcode, 1, 4, outfp);
    fwrite(stream.sps, 1, stream.sps_size, outfp);
    fwrite(startcode, 1, 4, outfp);
    fwrite(stream.pps, 1, stream.pps_size, outfp);

    for (int i = 0; i < stream.video_frame_count; ++i) {
      frame_t *frame = stream.video_frames[i];
      fwrite(startcode, 1, 4, outfp);
      fwrite(frame->address, 1, frame->size, outfp);
    }
    fclose(outfp);
  */

  RTMP_Init(&rtmp);
  RTMP_SetupURL(&rtmp, url);
  RTMP_EnableWrite(&rtmp);
  RTMP_Connect(&rtmp, NULL);
  RTMP_ConnectStream(&rtmp, 0);

  send_flv_data_tag(&rtmp, &stream);

  while (true) {
    for (int i = 0; i < stream.video_frame_count; ++i) {
      if (RTMP_ctrlC) die();

      frame_t *frame = stream.video_frames[i];
      send_video_tag(&rtmp, frame);
      usleep(1000000 / 10); // fps: 10
    }
  }

  return 0;
}

static void die() {
  free(stream.buffer);
  RTMP_Close(&rtmp);
  exit(0);
}

static void sigint(int sig) {
  RTMP_ctrlC = TRUE;
}

static void send_video_tag(RTMP *rtmp, frame_t *frame) {
  // header: 16 bytes
  // nalu length: 4 bytes
  uint8_t *buffer = malloc(frame->size + 16 + 4);
  memcpy(buffer + 20, frame->address, frame->size);

  uint8_t *nalu_length_ptr = (uint8_t *) &frame->size;
  *(buffer + 16) = *(nalu_length_ptr + 3); // 17th byte
  *(buffer + 17) = *(nalu_length_ptr + 2); // 18th byte
  *(buffer + 18) = *(nalu_length_ptr + 1); // 19th byte
  *(buffer + 19) = *(nalu_length_ptr + 0); // 20th byte

  uint8_t *p = buffer;

  // byte 1: video (9)
  *p++ = 0x09;
  // byte 2-4: data size
  size_t len = frame->size + 9;
  uint8_t *data_size_ptr = (uint8_t *) &len;
  *p++ = *(data_size_ptr + 2);
  *p++ = *(data_size_ptr + 1);
  *p++ = *data_size_ptr;

  // timestamp: 4
  static uint32_t timestamp = 0;
  timestamp += 100;
  uint8_t *timestamp_ptr = (uint8_t *) &timestamp;
  *p++ = *(timestamp_ptr + 2);
  *p++ = *(timestamp_ptr + 1);
  *p++ = *timestamp_ptr;
  *p++ = *(timestamp_ptr + 3);

  // stream id: 3
  *p++ = 0;
  *p++ = 0;
  *p++ = 0;

  // VIDEODATA header: 1
  *p++ = 0x17;

  // AVCVIDEOPACKET: 4
  *p++ = 0x01; // AVC NALU

  // composition time
  *p++ = 0;
  *p++ = 0;
  *p++ = 0;

  RTMP_Write(rtmp, (char *) buffer, frame->size + 20);
  print_hex(buffer, frame->size + 20);

  free(buffer);
}

static size_t make_avcrecord(annexb_stream_t *stream, uint8_t *buffer) {
  uint8_t *p = buffer;
  // byte 1: version
  *p++ = 0x01;

  // byte 2-4: sps 1-3
  for (int i = 0; i < 3; ++i) {
    *p++ = *(stream->sps + 1 + i);
  }

  // byte 5
  *p++ = 0xff;
  // byte 6: 低4位表示sps num
  *p++ = 0xe1;

  // byte 7-8: sps_size
  // pps_size: 0x0f (15): 0f 00 00 00 00 00 00 00
  uint8_t *sps_size_ptr = (uint8_t *) &stream->sps_size;
  *p++ = *(sps_size_ptr + 1);
  *p++ = *sps_size_ptr;

  // byte 9-(9+sps_size): sps
  for (int i = 0; i < stream->sps_size; ++i) {
    *p++ = *(stream->sps + i);
  }

  // byte (9+sps_size+1): pps num
  *p++ = 0x01;

  uint8_t *pps_size_ptr = (uint8_t *) &stream->pps_size;
  *p++ = *(pps_size_ptr + 1);
  *p++ = *pps_size_ptr;

  // byte (9+sps_size+2 - 9+sps_size+2+pps_size)
  for (int i = 0; i < stream->pps_size; ++i) {
    *p++ = *(stream->pps + i);
  }
  return p - buffer;
}

static void send_flv_data_tag(RTMP *rtmp, annexb_stream_t *stream) {
  uint8_t buffer[512];
  size_t record_size = make_avcrecord(stream, buffer + 16);

  uint8_t *p = buffer;
  // byte 1: type
  *p++ = 0x09;

  // byte 2-4: data size
  size_t data_size = record_size + 5;
  uint8_t *data_size_ptr = (uint8_t *) &data_size;
  *p++ = *(data_size_ptr + 2);
  *p++ = *(data_size_ptr + 1);
  *p++ = *data_size_ptr;

  // byte 5-8: timestamp+ext
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x00;

  // byte 9-11: streamid
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x00;

  // byte 12: Video tag (VIDEODATA) header
  *p++ = 0x17;

  // byte 13: AVCVIDEOPACKET - type
  *p++ = 0;

  // byte 14-16
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x00;

  p += record_size;

  RTMP_LogHex(RTMP_LOGINFO, buffer, p - buffer);
  RTMP_Write(rtmp, (char *) buffer, p - buffer);
}

static void annexb_parse(annexb_stream_t *stream) {
  // 简化: 将文件一次load进缓冲区
  stream->fp = fopen("sample.h264", "rb");
  fseek(stream->fp, 0, SEEK_END);
  size_t filesize = ftell(stream->fp);
  printf("file size: %lu bytes\n", filesize);
  rewind(stream->fp);

  stream->buffer_size = filesize;
  stream->buffer = malloc(stream->buffer_size);
  fread(stream->buffer, 1, stream->buffer_size, stream->fp);
  parse_frames(stream, stream->buffer, stream->buffer_size);

  for (int i = 0; i < stream->frame_count; ++i) {
    frame_t *frame = stream->frames[i];
    if (stream->sps == NULL && frame->nal_unit_type == 0x07) {
      stream->sps_size = frame->size;
      stream->sps = malloc(frame->size);
      memcpy(stream->sps, frame->address, frame->size);
    }

    if (stream->pps == NULL && frame->nal_unit_type == 0x08) {
      stream->pps_size = frame->size;
      stream->pps = malloc(frame->size);
      memcpy(stream->pps, frame->address, frame->size);
    }

    if (frame->nal_unit_type == 0x01 || frame->nal_unit_type == 0x05) {
      stream->video_frames[stream->video_frame_count++] = frame;
    }
  }
  fclose(stream->fp);
}

void parse_frames(annexb_stream_t *stream, uint8_t *buffer, size_t size) {
  uint8_t *p = buffer;
  uint8_t *begin = NULL;

  while (buffer + size - p >= 0) {
    if ((p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01) || p == buffer + size) {
      if (begin == NULL) {
        p += 4;
        begin = p;
      } else {
        frame_t *frame = malloc(sizeof(frame_t));
        frame->first_byte = *begin;
        frame->nal_unit_type = *begin & 0x1f;
        frame->address = begin;
        frame->offset = begin - buffer;
        frame->size = p - begin;
        stream->frames[stream->frame_count++] = frame;

        // printf("frame(#%04lu) offset: 0x%08llx, size: %lu\n", frame_count, frame->offset, frame->size);
        // print_hex(frame->address, frame->size);
        begin = NULL;
      }
    } else {
      p++;
    }
  }
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
