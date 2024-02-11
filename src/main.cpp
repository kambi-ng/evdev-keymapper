#include <asm-generic/errno-base.h>
#include <csignal>
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

bool running = true;

struct devices {
  int in_fd;
  int out_fd;

  // state

  devices(std::string keyboard_dev) {

    out_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (out_fd < 0) {
      perror("\nError opening device");
      exit(1);
    }

    ioctl(out_fd, UI_SET_EVBIT, EV_KEY);
    for (int i = KEY_RESERVED; i < KEY_MAX; i++) {
      ioctl(out_fd, UI_SET_KEYBIT, i);
    }

    struct uinput_user_dev uidev = {0};
    strcpy(uidev.name, "Kambing udev remapper");
    // strcpy(uidev.name, "1234567890");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor = 0x69;
    uidev.id.product = 0x420;
    uidev.id.version = 1;

    ioctl(out_fd, UI_SET_PHYS, "kambi-ng-udev");
    ioctl(out_fd, UI_DEV_SETUP, &uidev);
    ioctl(out_fd, UI_DEV_CREATE);

  open_file:
    in_fd = open(keyboard_dev.c_str(), O_RDONLY | O_NONBLOCK);
    if (errno == ENOENT) {
      fprintf(stderr, "Device %s not found, retrying in 5 seconds\n",
              keyboard_dev.c_str());
      sleep(5);
      if (running)
        goto open_file;
      else
        exit(1);
    }

    if (in_fd < 0) {
      perror("\nError opening device");
      exit(1);
    }

    if (ioctl(in_fd, EVIOCGRAB, 1) == -1) {
      perror("Error grabbing the device");
      exit(1);
    }
  }

  ~devices() {
    puts("cleaning up..");
    if (ioctl(in_fd, EVIOCGRAB, NULL) == -1) {
      perror("\nError releasing the device");
    }
    close(in_fd);

    ioctl(out_fd, UI_DEV_DESTROY);
    close(out_fd);
  }
};

void listen_and_remap(devices &dev, config &conf);

int main(int argc, char **argv) {

  signal(SIGINT, [](int) { running = false; });

  signal(SIGTERM, [](int) { running = false; });

  if (argc < 2) {
    fprintf(stderr, "Usage: %s /path/to/config.toml\n", argv[0]);
    exit(1);
  }

  auto conf = read_config(std::string(argv[1]));
  if (!conf.has_value()) {
    fprintf(stderr, "Error reading config\n");
    exit(2);
  }

  devices dev(conf.value().device);

  listen_and_remap(dev, conf.value());
}

void listen_and_remap(devices &dev, config &conf) {

  auto curr_layer = conf.keymap;
  auto curr_layer_code = -1;
  bool toggle = conf.toggle;

  struct input_event ev = {0};

  while (running) {
    if (read(dev.in_fd, &ev, sizeof(struct input_event)) < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      perror("Error reading key");
      running = false;
      break;
    }

    bool tk_key = conf.layermap->find(ev.code) != conf.layermap->end();
    if (!toggle) {
      // this will switch to default layer when key is released
      // unless we are on another layer
      if (tk_key && ev.value == 1 && curr_layer_code == -1) {
        curr_layer_code = ev.code;
        curr_layer = conf.layermap->at(ev.code);
      }
      if (tk_key && ev.value == 0 && curr_layer_code == ev.code) {
        curr_layer_code = -1;
        curr_layer = conf.keymap;
      }

      if (ev.code == curr_layer_code)
        continue;
    }

    if (toggle) {
      if (tk_key && ev.value == 1) {
        // if curr_layer_code = -1, we are on default layer then switch to layer
        if (curr_layer_code == -1) {
          curr_layer = conf.layermap->at(ev.code);
          curr_layer_code = ev.code;
          continue;
        } else if (curr_layer_code == ev.code) {
          // if curr_layer_code = ev.code, we are on the layer then switch to
          // default layer
          curr_layer = conf.keymap;
          curr_layer_code = -1;
          continue;
        }
      }
    }

    if (curr_layer->find(ev.code) != curr_layer->end()) {
      auto ev_codes = (*curr_layer)[ev.code];
      for (auto code : ev_codes) {
        ev.code = code;
        if (write(dev.out_fd, &ev, sizeof(struct input_event)) < 0) {
          perror("Error writing key");
          running = false;
          break;
        }
      }
      continue;
    }

    if (write(dev.out_fd, &ev, sizeof(struct input_event)) < 0) {
      perror("Error writing key");
      running = false;
      break;
    }
  }
}
