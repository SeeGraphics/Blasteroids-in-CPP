APP := build/game
SRC := src/main.cpp
OBJ := build/main.o

CXX := c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic

RAYLIB_PREFIX := $(shell brew --prefix raylib 2>/dev/null)
ifneq ($(RAYLIB_PREFIX),)
  CXXFLAGS += -I$(RAYLIB_PREFIX)/include
  LDFLAGS += -L$(RAYLIB_PREFIX)/lib
endif

LDLIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

.PHONY: all run clean

all: $(APP)

$(APP): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ): $(SRC) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(APP)
	./$(APP)

clean:
	rm -rf build
