#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
  ioctl(fd, UI_SET_KEYBIT, KEY_SPACE);
  ioctl(fd, UI_SET_KEYBIT, KEY_A);

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

  puts("Device created: Waiting for 1 second... Then start spamming space and a");

  sleep(1);

  for (int i = 0; i < 100; i++) {
    emit(fd, EV_KEY, KEY_SPACE, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    emit(fd, EV_KEY, KEY_SPACE, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    emit(fd, EV_KEY, KEY_A, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    emit(fd, EV_KEY, KEY_A, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(1500);
  }
  sleep(1);

  ioctl(fd, UI_DEV_DESTROY);
  puts("\nDone... Device deleted");
  close(fd);
}
