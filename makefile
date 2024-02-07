CXX=g++
APP=evdev
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
	sudo ./evdev config.example.toml

clean:
	rm -f $(OBJ) $(DEP) $(APP)

lsp: clean
	bear -- make

.PHONY: run clean lsp
