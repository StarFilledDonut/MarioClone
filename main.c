#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include "gameState.h"
#include "init.h"
#include "input.h"
#include "player.h"
#include "render.h"

bool collision_rr(const SDL_Rect *a, const SDL_Rect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}
bool collision_rf(const SDL_Rect *a, const SDL_FRect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}
bool collision_fr(const SDL_FRect *a, const SDL_Rect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}
bool collision_ff(const SDL_FRect *a, const SDL_FRect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}

#define collision(a, b)                                                        \
  _Generic((a),                                                                \
    SDL_Rect *: _Generic((b),                                                  \
      SDL_Rect *: collision_rr,                                                \
      SDL_FRect *: collision_rf,                                               \
      const SDL_Rect *: collision_rr,                                          \
      const SDL_FRect *: collision_rf),                                        \
    SDL_FRect *: _Generic((b),                                                 \
      SDL_Rect *: collision_fr,                                                \
      SDL_FRect *: collision_ff,                                               \
      const SDL_Rect *: collision_fr,                                          \
      const SDL_FRect *: collision_ff),                                        \
    const SDL_Rect *: _Generic((b),                                            \
      SDL_Rect *: collision_rr,                                                \
      SDL_FRect *: collision_rf,                                               \
      const SDL_Rect *: collision_rr,                                          \
      const SDL_FRect *: collision_rf),                                        \
    const SDL_FRect *: _Generic((b),                                           \
      SDL_Rect *: collision_fr,                                                \
      SDL_FRect *: collision_ff,                                               \
      const SDL_Rect *: collision_fr,                                          \
      const SDL_FRect *: collision_ff))(a, b)

void itemCollision(GameState *state, const ushort index, float dx, float dy) {
  Item *item = &state->blocks[index].item;
  const ushort isize = state->screen.tile;

  if (!item->free || !item->visible || item->type == FIRE_FLOWER)
    return;
  else if (((item->rect.x + isize < 0 && item->rect.x > state->screen.w) &&
            (item->rect.y > 0 && item->rect.y + isize < state->screen.h))) {
    item->visible = false;
    return;
  }

  for (uint i = 0; i < state->blocksLenght; i++) {
    const SDL_FRect *block = &state->blocks[i].rect;
    if (!collision(&item->rect, block) || state->blocks[i].broken)
      continue;

    if (dx > 0)
      item->rect.x = block->x - isize;
    else if (dx < 0)
      item->rect.x = block->x + block->w;
    else if (dy > 0)
      item->rect.y = block->y - isize;
    else if (dy < 0)
      item->rect.y = block->y + block->h;
    if (dx)
      item->velocity.x = -item->velocity.x;
    else if (item->type == STAR)
      item->velocity.y = -ITEM_JUMP_FORCE;
    else
      item->velocity.y = 0;
  }

  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    if (!collision(&item->rect, &obj))
      continue;

    if (dx > 0)
      item->rect.x = obj.w - isize;
    else if (dx < 0)
      item->rect.x = obj.w + obj.w;
    else if (dy > 0)
      item->rect.y = obj.y - isize;
    else if (dy < 0)
      item->rect.y = obj.y + obj.h;
    if (dx)
      item->velocity.x *= -1;
    else if (item->type == STAR)
      item->velocity.y = -ITEM_JUMP_FORCE;
    // ERROR: flickering when colliding with object
    else
      item->velocity.y = 0;
  }

  if (item->rect.x + item->rect.w > state->screen.w)
    item->velocity.x = -ITEM_SPEED;
  else if (item->rect.x < 0)
    item->velocity.x = ITEM_SPEED;
  else if (item->rect.y < 0 || item->rect.y + isize > state->screen.h)
    item->velocity.y = 0;
}

// Takes care of the collision of the fireballs with non-player entities.
void fireballCollision(GameState *state, ushort index, float dx, float dy) {
  Fireball *ball = &state->player.fireballs[index];
  const ushort fs = state->screen.tile / 2;

  if (!ball->visible)
    return;
  else if (!((ball->rect.x + fs > 0 && ball->rect.x < state->screen.w) &&
            (ball->rect.y + fs > 0 && ball->rect.y < state->screen.h))) {
    ball->visible = false;
    return;
  }

  for (uint i = 0; i < state->blocksLenght; i++) {
    const SDL_FRect *block = &state->blocks[i].rect;
    const ushort bs = state->screen.tile;

    if (!collision(&ball->rect, block) || state->blocks[i].broken)
      continue;

    if (dx > 0)
      ball->rect.x = block->x - fs;
    else if (dx < 0)
      ball->rect.x = block->x + bs;
    else if (dy > 0)
      ball->rect.y = block->y - fs;
    else if (dy < 0)
      ball->rect.y = block->y + bs;
    if (dx)
      ball->velocity.x *= -1;
    else
      ball->velocity.y *= -1;
  }

  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect *obj = &state->objs[i];
    if (!collision(&ball->rect, obj))
      continue;

    if (dx > 0)
      ball->rect.x = obj->w - fs;
    else if (dx < 0)
      ball->rect.x = obj->w + obj->w;
    else if (dy > 0)
      ball->rect.y = obj->y - fs;
    else if (dy < 0)
      ball->rect.y = obj->y + obj->h;
    if (dx)
      ball->velocity.x *= -1;
    else
      ball->velocity.y *= -1;
  }
}

// Apply physics to the player, the objects, and the enemies
void physics(GameState *state) {
  Player *player = &state->player;
  float dt = state->screen.deltaTime;
  const ushort TARGET_FPS = state->screen.targetFps;
  const float MAX_GRAVITY = 20;

  if (player->squatting && player->hitbox.h == state->screen.tile) {
    player->hitbox.y += state->screen.tile;
    player->hitbox.h -= state->screen.tile;
  } else if (player->tall && player->hitbox.h != state->screen.tile * 2) {
    player->hitbox.y -= state->screen.tile;
    player->hitbox.h += state->screen.tile;
  }

  if (player->velocity.x) {
    player->hitbox.x += player->velocity.x * TARGET_FPS * dt;
    playerCollision(state, player->velocity.x, 0);
  }

  if (player->velocity.y < MAX_GRAVITY)
    player->velocity.y += GRAVITY;
  player->hitbox.y += player->velocity.y * TARGET_FPS * dt;
  playerCollision(state, 0, player->velocity.y);

  for (ushort i = 0; i < player->fireballLimit; i++) {
    Fireball *ball = &player->fireballs[i];
    if (!ball->visible)
      continue;

    ball->rect.x += ball->velocity.x * TARGET_FPS * dt;
    fireballCollision(state, i, ball->velocity.x, 0);

    ball->rect.y += ball->velocity.y * TARGET_FPS * dt;
    fireballCollision(state, i, 0, ball->velocity.y);
  }

  for (uint i = 0; i < state->blocksLenght; i++) {
    Block *block = &state->blocks[i];
    Item *item = &state->blocks[i].item;

    if ((!item->free || !item->visible) || item->type == FIRE_FLOWER ||
        SDL_HasIntersectionF(&item->rect, &block->rect))
      continue;
    // In the original, default direction is always right
    // on the following games, the starting direction of an
    // item depends on your position in relation to the block

    item->rect.x += item->velocity.x * TARGET_FPS * dt;
    itemCollision(state, i, item->velocity.x, 0);

    if (item->velocity.y < MAX_GRAVITY)
      item->velocity.y += GRAVITY;
    item->rect.y += item->velocity.y * TARGET_FPS * dt;
    itemCollision(state, i, 0, item->velocity.y);
  };

  // NOTES: Placeholder code below, prevent from falling into endeless pit
  if (player->rect.y - player->rect.h > state->screen.h) {
    player->rect.y = 0 - player->rect.h;
    player->rect.x = state->screen.h / 2.0f - player->rect.w;
  }

  player->rect.y = player->squatting ? player->hitbox.y + state->screen.tile
                                     : player->hitbox.y;
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
// FIX: Item collision with objects
// FIX: Broken block animation
// FIX: First fireball falling with no collision
