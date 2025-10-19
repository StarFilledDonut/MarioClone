#ifndef INIT_H
#define INIT_H

#include "gameState.h"

void createBlock(GameState *state, int x, int y, BlockState tBlock,
                 ItemType tItem);
void initObjs(GameState *state);
void initTextures(GameState *state);
void initGame(GameState *state);

#endif
