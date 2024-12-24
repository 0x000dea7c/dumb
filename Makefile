CXX         := clang++
CXX_FLAGS   := -Wall -Wextra -pedantic -std=c++17 -fno-exceptions -fno-rtti
CXX_RELEASE := -ffast-math -O3 -march=native
CXX_DEBUG   := -O0 -DDUMB_DEBUG -ggdb -fsanitize=address,leak,undefined
LD_FLAGS    := -lxcb -lxcb-keysyms
MODULE      := -shared -fPIC

all: rel

rel:
	$(CXX) $(CXX_FLAGS) $(CXX_RELEASE) code/dumb_gnulinux_xcb.cpp -o dumb $(LD_FLAGS)
	$(CXX) $(CXX_FLAGS) $(CXX_RELEASE) $(MODULE) code/dumb_game_logic.cpp -o libdumb.so $(LD_FLAGS)

rel_mod:
	$(CXX) $(CXX_FLAGS) $(CXX_RELEASE) $(MODULE) code/dumb_game_logic.cpp -o libdumb.so $(LD_FLAGS)

dbg:
	$(CXX) $(CXX_FLAGS) $(CXX_DEBUG) code/dumb_gnulinux_xcb.cpp -o dumb $(LD_FLAGS)
	$(CXX) $(CXX_FLAGS) $(CXX_DEBUG) $(MODULE) code/dumb_game_logic.cpp -o libdumb.so $(LD_FLAGS)

dbg_mod:
	$(CXX) $(CXX_FLAGS) $(CXX_DEBUG) $(MODULE) code/dumb_game_logic.cpp -o libdumb.so $(LD_FLAGS)

run:
	./dumb

gdb:
	gdb -q ./dumb
