CC = clang
CFLAGS = -Wall -g
SDL = -lSDL2 -lSDL2_image
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, build/%.o, $(SRCS))

game: $(OBJS)
	$(CC) $(CFLAGS) $(SDL) $^ -o $@ 2> log.txt
build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
run: game
	./game
clean:
	rm -f build/*.o game log.txt
.PHONY: clean run
