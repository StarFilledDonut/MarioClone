#ifndef UTILS_H
#define UTILS_H

#include "gameState.h"

void quit(GameState *state, ushort __status);
void getsrcs(SDL_Rect srcs[], const ushort frames, ushort *startIndex,
             const uint row, const float w, const float h, uint x, uint y);
char *catpath(GameState *state, const char *path, const char *file);

#endif
