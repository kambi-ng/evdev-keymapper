CXX=g++
APP=evdev-remapper
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
	sudo $(APP) config.example.toml

clean:
	rm -f $(OBJ) $(DEP) $(APP)

lsp: clean
	bear -- make

install: $(APP)
	install -Dm755 $(APP) /usr/local/bin/$(APP)
	install -Dm644 config.example.toml /etc/$(APP)/config.toml
# install systemd service file
	install -Dm644 systemd/evdev-remapper.service /usr/lib/systemd/system/evdev-remapper.service
# install udev rule
	install -Dm644 udev/64-evdev-remapper.rules /usr/lib/udev/rules.d/64-evdev-remapper.rules

.PHONY: run clean lsp
