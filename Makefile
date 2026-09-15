# Makefile - for the MSYS2 MinGW64 shell.  Windows users: prefer .\build.ps1
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g
LDLIBS   := -lglew32 -lfreeglut -lopengl32 -lglu32
HDRS     := src/config.h src/kinematics.h src/prim.h src/scene.h
SRC      := src/main.cpp
OUT      := build/quarterturn.exe
TEST     := build/mathcheck.exe

.PHONY: all release run check clean
all: $(OUT)

$(OUT): $(SRC) $(HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDLIBS)

$(TEST): tests/mathcheck.cpp src/config.h src/kinematics.h
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -O2 $< -o $@

release: CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -DNDEBUG
release: clean $(OUT)

run: $(OUT)
	./$(OUT)

# Verifies every derived number in PRD section 10 against the shipping headers.
check: $(TEST)
	./$(TEST)

clean:
	rm -f $(OUT) $(TEST)
