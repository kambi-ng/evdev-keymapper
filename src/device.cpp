

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <memory>
#include <string.h>
#include <string>
#include <unistd.h>

#include "config.hpp"
#include "device.hpp"

struct device_state {
  int in_fd;
  int out_fd;

  // state
  config conf;

  device_state(config conf) {
    in_fd = open(conf.device.c_str(), O_RDONLY | O_NONBLOCK);

    this->conf = conf;

    if (in_fd < 0) {
      perror("\nError opening device");
      exit(1);
    }

    if (ioctl(in_fd, EVIOCGRAB, 1) == -1) {
      perror("Error grabbing the device");
      exit(1);
    }

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
  }

  ~device_state() {
    puts("cleaning up..");
    if (ioctl(in_fd, EVIOCGRAB, NULL) == -1) {
      perror("\nError releasing the device");
    }
    close(in_fd);

    ioctl(out_fd, UI_DEV_DESTROY);
    close(out_fd);
  }
};

device_state_ptr setup_devices(config conf) {
  return std::make_shared<device_state>(conf);
}

void listen_and_remap(device_state_ptr state) {
  struct input_event curr_key = {0};

  bool press_toggle = !state->conf.toggle;
  int toggle_key = state->conf.layerkey;

  bool layer_enabled = false;

  while (1) {
    ssize_t n = read(state->in_fd, &curr_key, sizeof(curr_key));
    if (n == -1) {
      if (errno == EAGAIN) {
        continue;
      }
      perror("Error reading input event");
      break;
    }

    bool tk = curr_key.code == toggle_key;
    bool tk_pressed = tk && curr_key.value == 1;
    bool tk_released = tk && curr_key.value == 0;

    if (press_toggle) {
      // If press_toggle is enabled, set isToggled based on the key press and
      // release
      if (tk_pressed) {
        layer_enabled = true; // Set true when the key is pressed
      } else if (tk_released) {
        layer_enabled = false; // Set false when the key is released
      }
    } else {
      // Original toggle functionality
      if (tk_pressed) {
        // Flip the toggle state only when the key is pressed
        layer_enabled = !layer_enabled;
        continue;
      }
    }

    // TODO do remap layer0
    if (!layer_enabled) {
      write(state->out_fd, &curr_key, sizeof(curr_key));
      continue;
    }

    auto layer1_map = state->conf.keymap;
    int keycode = curr_key.code;
    if (layer1_map->find(keycode) != layer1_map->end()) {
      keycode = (*layer1_map)[keycode];
    }
    curr_key.code = keycode;
    write(state->out_fd, &curr_key, sizeof(curr_key));
  }
}
