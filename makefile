
C_FILES = $(wildcard *.c)

evdev: $(C_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: run clean
run: evdev
	sudo ./evdev

clean:
	rm -f evdev
