CXX=g++
APP=evdev
CXXFLAGS=-MMD -MP -I. -std=c++17

SRC = $(wildcard src/*.cpp)
OBJ=$(SRC:.cpp=.o)
DEP=$(SRC:.cpp=.d)


$(APP): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

*.o: *.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)

vendor/toml.hpp: vendor
	curl https://raw.githubusercontent.com/marzer/tomlplusplus/30172438cee64926dc41fdd9c11fb3ba5b2ba9de/toml.hpp > vendor/toml.hpp

vendor:
	mkdir vendor

-include $(DEP)

run: $(APP)
	sudo ./evdev

clean:
	rm -f $(OBJ) $(DEP) $(APP)

lsp: clean
	bear -- make

.PHONY: run clean lsp
