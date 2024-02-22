CC = clang
CFLAGS = -Wall -g
SDL2 = `pkg-config --libs --cflags sdl2 SDL2_image`

game.o: game.c
	$(CC) $(CFLAGS) $(SDL2) $^ -o $@
