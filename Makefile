CC               := gcc
CC_FLAGS_WARN    := -Wall -Werror -Wextra -pedantic -Wformat -Wformat-security -Wconversion -Wshadow
CC_FLAGS_DEBUG   := -O1 -ggdb -D_FORTIFY_SOURCE=2 -fsanitize=address,leak,undefined -fstack-clash-protection -fcf-protection=full
CC_FLAGS_RELEASE := -O3 -g -ffast-math -funroll-loops -flto
SOURCES          := $(wildcard code/*.c)
TARGETS          := $(patsubst code/%.c, %, $(SOURCES))
LD_FLAGS         := -lSDL2

all: release

release: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_RELEASE)
release: $(TARGETS)

debug: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_DEBUG)
debug: $(TARGETS)

%: code/%.c
	$(CC) $(CC_FLAGS) $< -o $@ $(LD_FLAGS)

run:
	LSAN_OPTIONS="suppressions=./lsan_suppressions.txt" ./xcaliber

gdb:
	gdb -q ./xcaliber

clean:
	rm -f $(TARGETS)

.PHONY: all clean
