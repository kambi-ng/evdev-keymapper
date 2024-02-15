CXX=g++
APP=evdev-keymapper
CXXFLAGS=-MMD -MP -I. -Isrc -std=c++17

SRC = $(wildcard src/*.cpp)
OBJ=$(SRC:.cpp=.o)
DEP=$(SRC:.cpp=.d)

$(APP): src/keycodes.hpp vendor/toml.hpp $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(APP) $(OBJ) $(LDFLAGS)

-include $(DEP)

*.o: *.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)

vendor/toml.hpp: vendor
	curl https://raw.githubusercontent.com/marzer/tomlplusplus/30172438cee64926dc41fdd9c11fb3ba5b2ba9de/toml.hpp > vendor/toml.hpp

vendor:
	mkdir vendor

src/keycodes.hpp: keygen.sh
	./keygen.sh > src/keycodes.hpp

run: $(APP)
	sudo ./$(APP) config.example.toml


clean:
	rm -f $(OBJ) $(DEP) $(APP)

lsp: clean
	bear -- make

install: $(APP)
	install -Dm755 ./$(APP) /usr/local/bin/$(APP)
	[ ! -f /etc/testing/config.toml ] && install -Dm644 config.example.toml /etc/testing/config.toml || true
# install systemd service file
	[ ! -f /usr/lib/systemd/system/evdev/evdev-keymapper.service ] && install -Dm644 systemd/evdev-keymapper.service /usr/lib/systemd/system/evdev-keymapper.service || true
# install udev rule
	[ ! -f /usr/lib/udev/rules.d/64-evdev-keymapper.rules ] && install -Dm644 udev/64-evdev-keymapper.rules /usr/lib/udev/rules.d/64-evdev-keymapper.rules || true

uninstall:
	rm -f /usr/local/bin/$(APP)
#	rm -f /etc/testing/config.toml
	rm -f /usr/lib/systemd/system/evdev-keymapper.service
	rm -f /usr/lib/udev/rules.d/64-evdev-keymapper.rules

.PHONY: run clean lsp
