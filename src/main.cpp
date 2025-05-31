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
#define _GNU_SOURCE
#include <poll.h>

#include "config.hpp"
#include "print.hpp"
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
        print("Waiting for device", keyboard_dev.c_str());
      }
      usleep(1000 * 100);
    }

    print("Device", keyboard_dev, "found");

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
    strcpy(uidev.name, "vDevice Kambing Evdev Keymapper");
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
    print("Closing devices");
    if (ioctl(in_fd, EVIOCGRAB, NULL) == -1) {
      perror("\nError releasing the device");
    }
    close(in_fd);

    ioctl(out_fd, UI_DEV_DESTROY);
    close(out_fd);
  }
};

struct State {
  std::shared_ptr<std::map<int, std::vector<int>>> active_layer;  // Current active keymap layer
  int layer_key;                                   // Key that activated current layer (-1 for default)
  std::set<int> pressed_keys;                     // Currently pressed keys
  
  State(config& conf) : 
    active_layer(conf.keymap),
    layer_key(-1) {}

  void handle_key_event(const input_event& ev, config& conf, int out_fd) {
    bool is_layer_key = conf.layermap->find(ev.code) != conf.layermap->end();

    // Handle layer switching
    if (is_layer_key) {
      if (ev.value == 1) {  // Press
        active_layer = conf.layermap->at(ev.code);
        layer_key = ev.code;
      } else if (ev.value == 0) {  // Release
        active_layer = conf.keymap;
        layer_key = -1;
        release_all_keys(out_fd);
      }
      return;
    }

    // Handle regular keys
    auto it = active_layer->find(ev.code);
    if (it != active_layer->end()) {
      // Send mapped key events
      struct input_event mapped_ev = ev;
      for (int code : it->second) {
        mapped_ev.code = code;
        if (ev.value == 1) {
          pressed_keys.insert(code);
        } else if (ev.value == 0) {
          pressed_keys.erase(code);
        }
        write_event(mapped_ev, out_fd);
      }
    } else {
      // Pass through unmapped keys
      if (ev.value == 1) {
        pressed_keys.insert(ev.code);
      } else if (ev.value == 0) {
        pressed_keys.erase(ev.code);
      }
      write_event(ev, out_fd);
    }

    // Always send a sync event after key events
    struct input_event sync_ev = {0};
    sync_ev.type = EV_SYN;
    sync_ev.code = SYN_REPORT;
    write_event(sync_ev, out_fd);
  }

private:
  void release_all_keys(int fd) {
    struct input_event ev = {0};
    ev.type = EV_KEY;
    ev.value = 0;

    for (int code : pressed_keys) {
      ev.code = code;
      write_event(ev, fd);
    }
    
    // Send final sync
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    write_event(ev, fd);
    
    pressed_keys.clear();
  }

  void write_event(const input_event& ev, int fd) {
    write(fd, &ev, sizeof(ev));
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
    printerr("Error reading config");
    exit(2);
  }

  devices dev(conf.value().device);

  listen_and_remap(dev, conf.value());
}

void listen_and_remap(devices &dev, config &conf) {
  State state(conf);
  struct pollfd pfd = {dev.in_fd, POLL_IN};
  struct input_event ev;

  while (running) {
    int ret = poll(&pfd, 1, -1);
    if (ret < 0) {
      if (errno == EINTR) continue;
      break;
    }

    if (read(dev.in_fd, &ev, sizeof(ev)) < 0) {
      if (errno == EINTR || errno == EAGAIN) continue;
      if (errno == ENODEV) {
        dev.wait_keyboard();
        continue;
      }
      break;
    }

    if (ev.type == EV_KEY) {
      state.handle_key_event(ev, conf, dev.out_fd);
    } else {
      write(dev.out_fd, &ev, sizeof(ev));
    }
  }
}
