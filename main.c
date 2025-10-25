#include <SDL2/SDL.h>
#include <stdbool.h>
#include "gameState.h"
#include "init.h"
#include "input.h"
#include "player.h"
#include "render.h"

static bool _collision_rr(const SDL_Rect *a, const SDL_Rect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}
static bool _collision_rf(const SDL_Rect *a, const SDL_FRect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}
static bool _collision_fr(const SDL_FRect *a, const SDL_Rect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}
static bool _collision_ff(const SDL_FRect *a, const SDL_FRect *b) {
  return ((a->x < b->x + b->w && a->x + a->w > b->x) &&
          (a->y < b->y + b->h && a->y + a->h > b->y));
}

// Calculates the overlaping area of two rectanlge,
// supporting both SDL_Rect and SDL_FRect
// @param a The first rectangle
// @param b The second rectangle
// @return true if there is a intersection, false otherwise
#define collision(a, b)                                                        \
  _Generic((a),                                                                \
    SDL_Rect *: _Generic((b),                                                  \
      SDL_Rect *: _collision_rr,                                               \
      SDL_FRect *: _collision_rf,                                              \
      const SDL_Rect *: _collision_rr,                                         \
      const SDL_FRect *: _collision_rf),                                       \
    SDL_FRect *: _Generic((b),                                                 \
      SDL_Rect *: _collision_fr,                                               \
      SDL_FRect *: _collision_ff,                                              \
      const SDL_Rect *: _collision_fr,                                         \
      const SDL_FRect *: _collision_ff),                                       \
    const SDL_Rect *: _Generic((b),                                            \
      SDL_Rect *: _collision_rr,                                               \
      SDL_FRect *: _collision_rf,                                              \
      const SDL_Rect *: _collision_rr,                                         \
      const SDL_FRect *: _collision_rf),                                       \
    const SDL_FRect *: _Generic((b),                                           \
      SDL_Rect *: _collision_fr,                                               \
      SDL_FRect *: _collision_ff,                                              \
      const SDL_Rect *: _collision_fr,                                         \
      const SDL_FRect *: _collision_ff))(a, b)

// Takes care of the collision of moving items with non-player entities.
void itemCollision(GameState *state, const ushort index, float dx, float dy) {
  Item *item = &state->blocks[index].item;
  const ushort isize = state->screen.tile;

  if (!item->free || !item->visible || item->type == FIRE_FLOWER)
    return;
  if (((item->rect.x + isize < 0 && item->rect.x > state->screen.w) &&
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
    const SDL_Rect *obj = &state->objs[i];
    if (!collision(&item->rect, obj))
      continue;

    if (dx > 0)
      item->rect.x = obj->w - isize;
    else if (dx < 0)
      item->rect.x = obj->w + obj->w;
    else if (dy > 0)
      item->rect.y = obj->y - isize;
    else if (dy < 0)
      item->rect.y = obj->y + obj->h;

    if (dx)
      item->velocity.x *= -1;
    else if (item->type == STAR)
      item->velocity.y = -ITEM_JUMP_FORCE;
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
  const float fs = state->screen.tile / 2.0;

  if (!ball->visible)
    return;
  else if (!((ball->rect.x + fs > 0 && ball->rect.x < state->screen.w) &&
             (ball->rect.y + fs > 0 && ball->rect.y < state->screen.h))) {
    ball->visible = false;
    return;
  }

  for (uint i = 0; i < state->blocksLenght; i++) {
    const SDL_FRect *block = &state->blocks[i].rect;
    const float bs = state->screen.tile;

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
    const SDL_Rect *obj = &state->objs[i];
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

  if (player->crounching && player->hitbox.h == state->screen.tile * 2) {
    player->hitbox.y += state->screen.tile;
    player->hitbox.h = state->screen.tile;
  } else if (!player->crounching && player->hitbox.h != state->screen.tile * 2) {
    player->hitbox.y -= state->screen.tile;
    player->hitbox.h = state->screen.tile * 2;
  }

  if (player->velocity.x) {
    player->hitbox.x += player->velocity.x * TARGET_FPS * dt;
    playerCollision(state, player->velocity.x, 0);
  }

  if (player->velocity.y < MAX_GRAVITY)
    player->velocity.y += GRAVITY;
  player->hitbox.y += player->velocity.y * TARGET_FPS * dt;
  playerCollision(state, 0, player->velocity.y);

  // Fireballs collision
  for (ushort i = 0; i < MAX_FIREBALLS; i++) {
    Fireball *ball = &player->fireballs[i];
    if (!ball->visible)
      continue;

    ball->rect.x += ball->velocity.x * TARGET_FPS * dt;
    fireballCollision(state, i, ball->velocity.x, 0);

    ball->rect.y += ball->velocity.y * TARGET_FPS * dt;
    fireballCollision(state, i, 0, ball->velocity.y);
  }

  // Items collision
  for (uint i = 0; i < state->blocksLenght; i++) {
    Block *block = &state->blocks[i];
    Item *item = &state->blocks[i].item;

    if ((!item->free || !item->visible) || item->type == FIRE_FLOWER ||
        collision(&item->rect, &block->rect))
      continue;
    // In the original, default direction is always right
    // On the following games, the starting direction of an
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

  player->rect.y = player->crounching ? player->hitbox.y - state->screen.tile
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
// ERROR: After an item collide with a object, its rect.y decreases by 2 after
// each loop ERROR: When the STAR collides with a block by the bottom side ledge
// it will ignore gravity and sideways bounce to climb the block
// TODO: Add a delay from wich is possible the player to fire fireballs with the
// key held
// TODO: Make so that items jump when the player triggers a block bump besides
// it ERROR: Player will phase upward through the block when it is on the last
// coin FIX: Make so that the block only update to EMPTY in the last fram of
// block bump
