CC               := gcc
CC_FLAGS_WARN    := -Wall -Werror -Wextra -pedantic -Wformat -Wformat-security -Wconversion -Wshadow
CC_FLAGS_DEBUG   := -O1 -ggdb3 -D_FORTIFY_SOURCE=2 -fsanitize=address,leak,undefined -fstack-clash-protection -fcf-protection=full
CC_FLAGS_RELEASE := -O3 -g -ffast-math -funroll-loops -flto
# NOTE: Hidden symbols by default, I think that reduces the size of the
# generated binary, which is nice.
SHARED_FLAGS     := -shared -fPIC -fvisibility=hidden
# NOTE: I want to not include the game logic file here because that one will be
# hot reloaded and treated as a shared library!
SOURCES          := $(filter-out code/xcaliber_game_logic.c, $(wildcard code/*.c))
OBJECTS          := $(SOURCES:code/%.c=obj/%.o)
TARGET           := xcaliber
GAME_LIB         := libgamelogic.so
LD_FLAGS         := -lSDL3 -ldl

$(shell mkdir -p obj)

all: release

release: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_RELEASE)
release: $(TARGET) $(GAME_LIB)

debug: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_DEBUG)
debug: $(TARGET) $(GAME_LIB)

$(TARGET): $(OBJECTS)
	$(CC) $(CC_FLAGS) $(OBJECTS) -o $@ $(LD_FLAGS)

$(GAME_LIB): code/xcaliber_game_logic.c
	$(CC) $(CC_FLAGS) $(SHARED_FLAGS) $< -o $@

obj/%.o: code/%.c
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
