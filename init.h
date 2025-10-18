#ifndef INIT_H
#define INIT_H

#include "gameState.h"

typedef enum {
  SHINY_SPRITE,
  BRICK_SPRITE,
  EMPTY_SPRITE,
  INTERROGATION_SPRITE
} BlockSprite;

void createBlock(GameState *state, int x, int y, BlockState tBlock,
                 ItemType tItem);
void initObjs(GameState *state);
void initTextures(GameState *state);
void initGame(GameState *state);

#endif
