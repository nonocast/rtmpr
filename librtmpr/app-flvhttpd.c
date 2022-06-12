#include "flv.h"
#include "flvhttpd.h"
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#define kBLACK "\x1B[0m"
#define kRED "\x1B[31m"
#define kGREEN "\x1B[32m"

FLV flv;

void error_die(const char *);
void accept_request(void *arg);
static void print_hex(const uint8_t *data, unsigned long len);

int main(void) {

  uint16_t port = 3000;
  struct sockaddr_in name = {.sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_ANY)};
  int on = 1;

  int httpd = socket(PF_INET, SOCK_STREAM, 0);
  if (httpd == -1) error_die("socket");
  if (setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) { error_die("setsocket failed"); }
  if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0) error_die("bind");
  if (listen(httpd, 5) < 0) error_die("listen");

  printf("server start at %d\n", port);

  int client = -1;
  struct sockaddr_in client_name;
  socklen_t client_name_len = sizeof(client_name);
  pthread_t newthread;

  while (true) {
    int client = accept(httpd, (struct sockaddr *)&client_name, &client_name_len);
    if (client == -1) error_die("accept");
    if (pthread_create(&newthread, NULL, (void *)accept_request, (void *)(intptr_t)client) != 0)
      perror("pthread_create");
  }

  close(httpd);
  return 0;
}

void error_die(const char *sc) {
  perror("error: ");
  exit(1);
}

void accept_request(void *arg) {
  int client = (intptr_t)arg;

  char request[1024], method[10], uri[255];
  read(client, request, sizeof(request));
  char *p = request;
  while (*p++ != ' ') {}
  memcpy(method, request, p - request - 1);
  while (*p++ != ' ') {}
  memcpy(uri, request + strlen(method) + 1, p - request - strlen(method) - 1 - 1);

  printf("%s%s %s%s\n", kGREEN, method, kBLACK, uri);

  // send header
  char head[] = "HTTP/1.1 200 OK\nConnection: Keep-Alive\nContent-Type: video/x-flv\n\
Server: flvhttpd\nTransfer-Encoding: chunked\r\n\r\n";
  write(client, head, strlen(head));

  if (flv.file == NULL) {
    FLV_Open(&flv, "/Users/nonocast/desktop/teams.flv");
    // FLV_Open(&flv, "sample.flv");
    FLV_Parse(&flv);
    // FLV_Dump(&flv);
    printf("parse flv ok\n");
  }

  char chunkline[64];
  uint8_t *buffer = malloc(1024 * 1024 * 10);
  uint8_t *prev = malloc(4);
  memset(prev, 0x00, 4);

  for (int i = 0; i < flv.tagCount; ++i) {
    FLVTag *t = flv.tags[i];

    if (i == 0) {
      // send FLV 9 bytes head
      sprintf(chunkline, "%x\r\n", 9);
      printf("%s", chunkline);
      write(client, chunkline, strlen(chunkline));
      fseek(flv.file, 0, SEEK_SET);
      fread(buffer, 1, 9, flv.file);
      print_hex(buffer, 9);
      write(client, buffer, 9);
      write(client, "\r\n", 2);

      sprintf(chunkline, "%x\r\n", 4);
      printf("%s", chunkline);
      write(client, chunkline, strlen(chunkline));
      print_hex(prev, 4);
      write(client, prev, 4);
      write(client, "\r\n", 2);
    }

    // write tag
    sprintf(chunkline, "%lx\r\n", t->size);
    printf("%s", chunkline);
    write(client, chunkline, strlen(chunkline));

    fseek(flv.file, t->offset, SEEK_SET);
    fread(buffer, 1, t->size, flv.file);

    print_hex(buffer, t->size);
    write(client, buffer, t->size);
    write(client, "\r\n", 2);

    // write prev
    sprintf(chunkline, "%x\r\n", 4);
    printf("%s", chunkline);
    write(client, chunkline, strlen(chunkline));
    fread(prev, 1, 4, flv.file);
    print_hex(prev, 4);
    write(client, prev, 4);
    write(client, "\r\n", 2);

    usleep(1000000 / 30);
  }

  write(client, "0\r\n\r\n", 5);
  close(client);
  free(buffer);
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
