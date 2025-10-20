#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include "gameState.h"
#include "init.h"
#include "input.h"
#include "render.h"
#include "player.h"

// Takes care of the collision of the fireballs with non-player entities.
void handleFireballColl(GameState *state, ushort index, float dx, float dy) {
  Fireball *ball = &state->player.fireballs[index];
  const ushort fs = state->screen.tile / 2;

  if (!ball->visible)
    return;
  if (!((ball->rect.x + fs > 0 && ball->rect.x < state->screen.w) &&
        (ball->rect.y + fs > 0 && ball->rect.y < state->screen.h))) {
    ball->visible = false;
    return;
  }

  for (uint i = 0; i < state->blocksLenght; i++) {
    const float bx = state->blocks[i].rect.x, by = state->blocks[i].rect.y;
    const ushort bs = state->screen.tile;
    if ((ball->rect.x < bx + bs && ball->rect.x + fs > bx) &&
        (ball->rect.y < by + bs && ball->rect.y + fs > by)) {
      if (dx > 0)
        ball->rect.x = bx - fs;
      else if (dx < 0)
        ball->rect.x = bx + bs;
      else if (dy > 0)
        ball->rect.y = by - fs;
      else if (dy < 0)
        ball->rect.y = by + bs;
      if (dx)
        ball->dx *= -1;
      else
        ball->dy *= -1;
    }
  }
  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    if ((ball->rect.x < obj.x + obj.w && ball->rect.x + fs > obj.x) &&
        (ball->rect.y < obj.y + obj.h && ball->rect.y + fs > obj.y)) {
      if (dx > 0)
        ball->dx = obj.w - obj.w;
      else if (dx < 0)
        ball->dx = obj.w + obj.w;
      else if (dy > 0)
        ball->rect.y = obj.y - fs;
      else if (dy < 0)
        ball->rect.y = obj.y + obj.h;
      if (dx)
        ball->dx *= -1;
      else
        ball->dy *= -1;
    }
  }
}

// Apply physics to the player, the objects, and the eneyms.
void physics(GameState *state) {
  Player *player = &state->player;
  float dt = state->screen.deltaTime;
  const ushort TARGET_FPS = state->screen.targetFps;
  const float MAX_GRAVITY = 20;

  if (player->dx) {
    player->hitbox.x += player->dx * TARGET_FPS * dt;
    handlePlayerColl(player->dx, 0, state);
  }

  if (player->dy < MAX_GRAVITY) {
    player->dy += GRAVITY * TARGET_FPS * dt;
    player->hitbox.y += player->dy * TARGET_FPS * dt;
  }
  handlePlayerColl(0, player->dy, state);

  for (ushort i = 0; i < player->fireballLimit; i++) {
    Fireball *ball = &player->fireballs[i];
    ball->rect.x += ball->dx * TARGET_FPS * dt;
    handleFireballColl(state, i, ball->dx, 0);
    ball->rect.y += ball->dy * TARGET_FPS * dt;
    handleFireballColl(state, i, 0, ball->dy);
  }

  // NOTES: Placeholder code below, prevent from falling into endeless pit
  if (player->rect.y - player->rect.h > state->screen.h) {
    player->rect.y = 0 - player->rect.h;
    player->rect.x = state->screen.h / 2.0f - player->rect.w;
  }

  player->rect.y = player->hitbox.y;
  player->rect.x = player->hitbox.x;
  player->rect.x -= state->screen.tile / 4.0;
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
// TODO: Make mushroom and star to move arround
// TODO: Have an delay on player events on start of the game
// TODO: Have the bitsX[2] and bitsY[2] since there are only
//       two X and Y positions the 4 bits can go to
