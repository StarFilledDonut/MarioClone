#ifndef COLLISION_H
#define COLLISION_H

#include <SDL2/SDL.h>
#include "gameState.h"

void itemCollision(GameState *state, const ushort index);
void fireballCollision(GameState *state, const ushort index);
void playerCollision(GameState *state);

#endif
