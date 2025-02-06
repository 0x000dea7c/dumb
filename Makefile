# -fsanitize=address,leak,undefined -D_FORTIFY_SOURCE=2 (interferes with O0)
CC               := gcc
CC_FLAGS_WARN    := -Wall -Wextra -pedantic -Wformat -Wformat-security -Wconversion -Wshadow
CC_FLAGS_DEBUG   := -O0 -ggdb3 -fstack-clash-protection -fcf-protection=full -march=native -DDEBUG
CC_FLAGS_RELEASE := -O3 -g -ffast-math -funroll-loops -flto -march=native
# NOTE: Hidden symbols by default, I think that reduces the size of the
# generated binary, which is nice
SHARED_FLAGS     := -shared -fPIC -fvisibility=hidden
INCLUDE_DIRS     := $(shell find code/hyper -type d)
INCLUDE_FLAGS    := $(addprefix -I, $(INCLUDE_DIRS))

# TODO: this is temporary because I don't know how the structure will be

# these are the sources for hot reloading
GAME_LIB_SOURCES := code/hyper/game/hyper_game_logic.c \
code/hyper/renderer/hyper_renderer.c \
code/hyper/core/hyper_linear_arena.c \
code/hyper/core/hyper_math.c \
code/hyper/core/hyper_colour.c \
code/hyper/core/hyper_stack_arena.c

# all engine and game sources
SOURCES := code/hyper/game/hyper_game_logic.c \
code/hyper/renderer/hyper_renderer.c \
code/hyper/core/hyper_hot_reload.c \
code/hyper/core/hyper_linear_arena.c \
code/hyper/core/hyper_math.c \
code/hyper/core/hyper_colour.c \
code/hyper/core/hyper_stack_arena.c \
code/xcaliber_gnulinux.c

OBJECTS := $(SOURCES:code/%.c=obj/%.o)
TARGET  := xcaliber
GAME_LIB := libgamelogic.so
LD_FLAGS := -lSDL3 -ldl -lm

$(shell mkdir -p obj)

all: release

release: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_RELEASE) $(INCLUDE_FLAGS)
release: $(TARGET) $(GAME_LIB)

debug: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_DEBUG) $(INCLUDE_FLAGS)
debug: $(TARGET) $(GAME_LIB)

$(TARGET): $(OBJECTS)
	$(CC) $(CC_FLAGS) $(OBJECTS) -o $@ $(LD_FLAGS)

$(GAME_LIB): $(GAME_LIB_SOURCES)
	$(CC) $(CC_FLAGS) $(SHARED_FLAGS) $^ -o $@

obj/%.o: code/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c $< -o $@ $(LD_FLAGS)

# XXX: LD_LIBRARY_PATH is needed here because SDL3 installs in /usr/local/lib,
# also add current directory for my shared library.
run:
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:/usr/local/lib:. LSAN_OPTIONS="suppressions=./lsan_suppressions.txt" ./xcaliber

gdb:
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:/usr/local/lib:. LSAN_OPTIONS="suppressions=./lsan_suppressions.txt" gdb -q ./xcaliber

clean:
	rm -f $(TARGET) $(GAME_LIB)  obj/*.o
	rmdir obj

.PHONY: all clean release debug run gdb
