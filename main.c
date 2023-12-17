#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

int main() {
  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  for (int i = KEY_RESERVED; i < KEY_MAX; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }

  struct uinput_user_dev uidev = {0};

  strcpy(uidev.name, "Kambing Proof Of Concept");
  // strcpy(uidev.name, "1234567890");
  uidev.id.bustype = BUS_VIRTUAL;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0x5678;
  uidev.id.version = 1;
  ioctl(fd, UI_SET_PHYS, "kambi-ng-device");
  ioctl(fd, UI_DEV_SETUP, &uidev);
  ioctl(fd, UI_DEV_CREATE);

  sleep(1);

  puts("When B is pressed, 'kontol' wil be typed. Press ESC to exit.");
  // TODO: change this device
  const char *device = "";
  int in_fd = open(device, O_RDONLY | O_NONBLOCK);

  if (ioctl(in_fd, EVIOCGRAB, 1) == -1) {
    perror("Error grabbing the device");
    goto cleanup;
    return 1;
  }
  struct input_event ev;

  while (1) {
    ssize_t n = read(in_fd, &ev, sizeof ev);
    if (n == -1) {
      if (errno == EAGAIN) {
        continue;
      }
      perror("Error reading input event");
      break;
    }
    if (n != sizeof ev) {
      perror("Error reading input event");
      break;
    }

    if (ev.type == EV_KEY) {
      if (ev.code == KEY_B && ev.value == 1) {
        // K O N T O L
        emit(fd, EV_KEY, KEY_K, 1);
        emit(fd, EV_KEY, KEY_K, 0);
        emit(fd, EV_KEY, KEY_O, 1);
        emit(fd, EV_KEY, KEY_O, 0);
        emit(fd, EV_KEY, KEY_N, 1);
        emit(fd, EV_KEY, KEY_N, 0);
        emit(fd, EV_KEY, KEY_T, 1);
        emit(fd, EV_KEY, KEY_T, 0);
        emit(fd, EV_KEY, KEY_O, 1);
        emit(fd, EV_KEY, KEY_O, 0);
        emit(fd, EV_KEY, KEY_L, 1);
        emit(fd, EV_KEY, KEY_L, 0);
        continue;
      }

      if (ev.code == KEY_ESC && ev.value == 1) {
        break;
      }
    }

    write(fd, &ev, sizeof ev);
  }

  if (ioctl(in_fd, EVIOCGRAB, NULL) == -1) {
    perror("\nError releasing the device");
  }
  puts("\nDone... Device deleted");

cleanup:
  ioctl(fd, UI_DEV_DESTROY);
  close(in_fd);
  close(fd);
}
