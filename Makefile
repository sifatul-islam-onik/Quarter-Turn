# Makefile - for the MSYS2 MinGW64 shell.  Windows users: prefer .\build.ps1
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g
LDLIBS   := -lfreeglut -lopengl32 -lglu32
HDRS     := $(wildcard src/*.h)
SRC      := src/main.cpp
OUT      := build/quarterturn.exe

.PHONY: all release run clean
all: $(OUT)

$(OUT): $(SRC) $(HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDLIBS)

release: CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -DNDEBUG
release: clean $(OUT)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
