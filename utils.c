#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gameState.h"

// Destroy everything that was initialized from SDL then exit the program.
// @param *state Your instance of GameState
// @param __status The status shown after exting
void quit(GameState *state, u_short __status) {
  Sheets *sheets = &state->sheets;
  if (sheets->effects)
    SDL_DestroyTexture(sheets->effects);
  if (sheets->mario)
    SDL_DestroyTexture(sheets->mario);
  if (sheets->objs)
    SDL_DestroyTexture(sheets->objs);
  if (sheets->items)
    SDL_DestroyTexture(sheets->items);
  if (state->renderer)
    SDL_DestroyRenderer(state->renderer);
  if (state->window)
    SDL_DestroyWindow(state->window);
  IMG_Quit();
  SDL_Quit();
  exit(__status);
}

// Get the srcs of the specific frames of a spritesheet.
// The row is an index, which starts at 0.
void getsrcs(SDL_Rect srcs[],
             const u_short frames,
             u_short *startIndex,
             const uint row,
             const float w,
             const float h,
             uint x,
             uint y) {
  if (!frames) {
    printf("Invalid number of frames!\n");
    return;
  }
  const u_short tile = 16;
  if (!x)
    x = tile;
  if (!y && row)
    y = tile * (row - 1);
  u_short j = *startIndex;
  *startIndex += frames;
  for (u_short i = 0; i < frames; i++) {
    SDL_Rect *frame = &srcs[j];
    frame->x = frames != 1 ? x * i : x;
    frame->y = y;
    frame->w = tile * w;
    frame->h = tile * h;
    j++;
  }
}

// Remember to free the str after using it, needs state in case of an allocation
// error
// @return char * path + file
char *catpath(GameState *state, const char *path, const char *file) {
  char *result = malloc(strlen(path) + strlen(file) + 1);
  if (result == NULL) {
    printf("Could not allocate memory for the file path\n");
    quit(state, 1);
  }
  strcpy(result, path);
  strcat(result, file);
  return result;
}
