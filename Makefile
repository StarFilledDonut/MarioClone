SHELL := /bin/sh

CC := clang
CFLAGS := -Wall -Wextra -g -MMD -MP
SDL := -lSDL2 -lSDL2_image

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,build/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
LOG := log.txt

define report_log
	@if [ -s $(1) ]; then \
		rm -rf build/ game; \
		cat $(1) >&2; \
		exit 1; \
	else \
		rm -f $(1); \
	fi
endef

game: $(OBJS)
	-$(CC) $(CFLAGS) $(SDL) $^ -o $@ 2>> $(LOG)
	$(call report_log,$(LOG))

build/%.o: %.c | build
	-$(CC) $(CFLAGS) -c $< -o $@ 2>> $(LOG)
	$(call report_log,$(LOG))

build:
	@mkdir -p build

run: game
	./game

clean:
	rm -rf build $(LOG) game

-include $(DEPS)

.PHONY: clean run
