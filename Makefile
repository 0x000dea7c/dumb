CC               := gcc
CC_FLAGS_WARN    := -Wall -Werror -Wextra -pedantic -Wformat -Wformat-security -Wconversion -Wshadow
CC_FLAGS_DEBUG   := -O1 -ggdb3 -D_FORTIFY_SOURCE=2 -fsanitize=address,leak,undefined -fstack-clash-protection -fcf-protection=full
CC_FLAGS_RELEASE := -O3 -g -ffast-math -funroll-loops -flto
SOURCES          := $(wildcard code/*.c)
OBJECTS          := $(SOURCES:code/%.c=obj/%.o)
TARGET           := xcaliber
LD_FLAGS         := -lSDL3

# FIXME: this ain't pretty
$(shell mkdir -p obj)

all: release

release: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_RELEASE)
release: $(TARGET)

debug: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_DEBUG)
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CC_FLAGS) $(OBJECTS) -o $@ $(LD_FLAGS)

obj/%.o: code/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ $(LD_FLAGS)

run:
	LSAN_OPTIONS="suppressions=./lsan_suppressions.txt" ./xcaliber

gdb:
	gdb -q ./xcaliber

clean:
	rm -f $(TARGET) obj/*.o
	rmdir obj

.PHONY: all clean release debug run gdb
