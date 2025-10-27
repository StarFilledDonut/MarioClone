#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_rect.h>
#include <stdbool.h>
#include "gameState.h"
#include "init.h"
#include "input.h"
#include "render.h"
#include "collision.h"

// Apply physics to the player, the objects, and the enemies
void physics(GameState *state) {
  Player *player = &state->player;
  float dt = state->screen.deltaTime;
  const ushort TARGET_FPS = state->screen.targetFps;

  // Resolve player hitbox
  if (player->crounching && player->hitbox.h == state->screen.tile * 2) {
    player->hitbox.y += state->screen.tile;
    player->hitbox.h = state->screen.tile;
  } else if (!player->crounching &&
             player->hitbox.h != state->screen.tile * 2) {
    player->hitbox.y -= state->screen.tile;
    player->hitbox.h = state->screen.tile * 2;
  }

  // Player collision
  if (player->velocity.y < MAX_GRAVITY)
    player->velocity.y += GRAVITY;
  playerCollision(state);
  player->hitbox.x += player->velocity.x * TARGET_FPS * dt;
  player->hitbox.y += player->velocity.y * TARGET_FPS * dt;

  // Resolve player rectangle
  player->rect.y = player->crounching ? player->hitbox.y - state->screen.tile
                                      : player->hitbox.y;
  player->rect.x = player->hitbox.x;
  player->rect.x -= state->screen.tile / 4.0;

  // Fireballs collision
  for (ushort i = 0; i < MAX_FIREBALLS; i++) {
    Fireball *ball = &player->fireballs[i];
    if (!ball->visible)
      continue;

    fireballCollision(state, i);
    ball->rect.x += ball->velocity.x * TARGET_FPS * dt;
    ball->rect.y += ball->velocity.y * TARGET_FPS * dt;
  }

  // Items collision
  for (uint i = 0; i < state->blocksLenght; i++) {
    Block *block = &state->blocks[i];
    Item *item = &state->blocks[i].item;

    if ((!item->free || !item->visible) || item->type == FIRE_FLOWER ||
        SDL_HasIntersectionF(&item->rect, &block->rect))
      continue;
    // In the original, default direction is always right
    // On the following games, the starting direction of an
    // item depends on your position in relation to the block

    if (item->velocity.y < MAX_GRAVITY)
      item->velocity.y += GRAVITY;
    itemCollision(state, i);
    item->rect.x += item->velocity.x * TARGET_FPS * dt;
    item->rect.y += item->velocity.y * TARGET_FPS * dt;
  };
}

int main(void) {
  GameState state;
  initGame(&state);
  uint currentTime = SDL_GetTicks(), lastTime;

  while (true) {
    lastTime = currentTime;
    currentTime = SDL_GetTicks();
    state.screen.deltaTime = (currentTime - lastTime) / 1000.0f;

    if (!state.player.transforming) {
      handleEvents(&state);
      physics(&state);
    }
    render(&state);
  }
}
// ERROR: After an item collide with a object, its rect.y decreases by 2 after
// each loop
// ERROR: When the STAR collides with a block by the bottom side ledge
// it will ignore gravity and sideways bounce to climb the block
// TODO: Add a delay from wich is possible the player to fire fireballs with the
// key held
// ERROR: Player will phase upward through the block when it is on the last
// coin
