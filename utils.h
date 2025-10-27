#ifndef UTILS_H
#define UTILS_H

#include "gameState.h"

// Destroy everything that was initialized from SDL then exit the program.
// @param *state: Your instance of GameState
// @param __status: The status shown after exting
void quit(GameState *state, int __status);
void getsrcs(SDL_Rect srcs[],
             const ushort frames,
             ushort *startIndex,
             const uint row,
             const float w,
             const float h,
             uint x,
             uint y);
// Allocates memory and concatenates the path and file strings
// @param state: Current pre-initialized GameState
// @param path: A directory path
// @param file: A file in that directory
// @return Concatenation of the path string and file string
// @note Free the string after using it
char *catpath(GameState *state, const char *path, const char *file);

#endif
