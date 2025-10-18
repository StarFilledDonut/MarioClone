#ifndef UTILS_H
#define UTILS_H

#include "gameState.h"

void quit(GameState *state, u_short __status);
void getsrcs(SDL_Rect srcs[], const u_short frames, u_short *startIndex,
             const uint row, const float w, const float h, uint x, uint y);
char *catpath(GameState *state, const char *path, const char *file);

#endif
