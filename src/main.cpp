#include <asm-generic/errno-base.h>
#include <csignal>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <thread>

#include "config.hpp"

bool running = true;

struct devices {
  int in_fd;
  int out_fd;

  std::string keyboard_dev;
  // state
  void wait_keyboard() {
    bool printed = false;
    while (true) {
      if (access(keyboard_dev.c_str(), F_OK) != -1) {
        break;
      }
      if (!printed) {
        printed = true;
        printf("Waiting for device %s\n", keyboard_dev.c_str());
      }
      usleep(1000 * 100);
    }
    printf("Device %s found\n", keyboard_dev.c_str());

    in_fd = open(keyboard_dev.c_str(), O_RDONLY | O_NONBLOCK);

    if (in_fd < 0) {
      perror("\nError opening device");
      exit(1);
    }

    if (ioctl(in_fd, EVIOCGRAB, 1) == -1) {
      perror("Error grabbing the device");
      exit(1);
    }
  }

  devices(std::string keyboard_dev) {
    this->keyboard_dev = keyboard_dev;

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

    wait_keyboard();
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

  // HACK: flush stdout and stderr every 100ms
  std::thread t([]() {
    while (running) {
      fflush(stdout);
      fflush(stderr);
      usleep(1000 * 100);
    }
  });

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

  puts("Listening for key events..");
  auto curr_layer = conf.keymap;
  auto curr_layer_code = -1;
  bool toggle = conf.toggle;

  struct input_event ev = {0};

  std::set<int> pressed_keys = {};

  while (running) {

    if (read(dev.in_fd, &ev, sizeof(struct input_event)) < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      if (errno == ENODEV) {
        printf("Device %s lost\n", dev.keyboard_dev.c_str());
        dev.wait_keyboard();
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

        // release all pressed keys
        for (auto code : pressed_keys) {
          ev.code = code;
          ev.value = 0;
          if (write(dev.out_fd, &ev, sizeof(struct input_event)) < 0) {
            perror("Error writing key");
            running = false;
            break;
          }
        }
        pressed_keys.clear();
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

          // release all pressed keys
          for (auto code : pressed_keys) {
            ev.code = code;
            ev.value = 0;
            if (write(dev.out_fd, &ev, sizeof(struct input_event)) < 0) {
              perror("Error writing key");
              running = false;
              break;
            }
          }
          pressed_keys.clear();

          curr_layer = conf.keymap;
          curr_layer_code = -1;
          continue;
        }
      }
    }

    if (curr_layer->find(ev.code) != curr_layer->end()) {
      auto ev_codes = (*curr_layer)[ev.code];
      for (auto code : ev_codes) {
        if (ev.value == 1) {
          pressed_keys.insert(code);
        } else if (ev.value == 0) {
          pressed_keys.erase(code);
        }

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
