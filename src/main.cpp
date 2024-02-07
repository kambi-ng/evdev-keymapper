#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.hpp"
#include "device.hpp"

#define CHECK(x)                                                               \
  if (x < 0) {                                                                 \
    perror(#x);                                                                \
    exit(1);                                                                   \
  }

void emit(int fd, int type, int code, int val) {
  struct input_event ie;

  ie.type = type;
  ie.code = code;
  ie.value = val;
  /* timestamp values below are ignored */
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;

  write(fd, &ie, sizeof(ie));
}


int main(int argc, char ** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s /path/to/config.toml", argv[0]);
    exit(1);
  }

  auto conf = read_config(std::string(argv[1]));
  if (!conf.has_value()){
    fprintf(stderr, "Error reading config");
    exit(2);
  }


  listen_and_remap(setup_devices(conf.value()));
}
