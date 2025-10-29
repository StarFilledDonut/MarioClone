#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_stdinc.h>
#include "gameState.h"

// Uses CCD to calculate acurately where and who is colliding
// @param a: The collider rectangle
// @param velocity: The collider velocity
// @param b: The object rectangle
// @param step: The positive number of steps incremented each iteration
// @return 0 for no collision, 1 for X collision and -1 for Y collision
int collision(SDL_FRect a,
              const Velocity velocity,
              const SDL_FRect b,
              const int step) {
  if (step < 1 || SDL_FRectEmpty(&a) || SDL_FRectEmpty(&b))
    return 0;

  const bool xforward = velocity.x > 0, yforward = velocity.y > 0;
  const float xgoal = a.x + velocity.x, ygoal = a.y + velocity.y;

  while (a.x != xgoal) {
    a.x += xforward ? step : -step;

    if ((xforward && a.x > xgoal) || (!xforward && a.x < xgoal))
      a.x = xgoal;

    if (!SDL_HasIntersectionF(&a, &b))
      continue;

    return 1;
  }

  while (a.y != ygoal) {
    a.y += yforward ? step : -step;

    if ((yforward && a.y > ygoal) || (!yforward && a.y < ygoal))
      a.y = ygoal;

    if (!SDL_HasIntersectionF(&a, &b))
      continue;

    return -1;
  }

  return 0;
}

// Repositions the collider based from where it hit the object
// @param a: The collider rectangle
// @param b: The object rectangle
// @param axis: The axis from wich collision was detected
void resolveCollision(SDL_FRect *const a,
                      const SDL_FRect *const b,
                      const int axis) {
  if (a == NULL || b == NULL || SDL_FRectEmpty(a) || SDL_FRectEmpty(b) || !axis)
    return;

  if (axis > 0) {
    if (a->x < b->x)
      a->x = b->x - a->w;
    else
      a->x = b->x + b->w;
  } else {
    if (a->y < b->y)
      a->y = b->y - a->h;
    else
      a->y = b->y + b->h;
  }
}

// Takes care of the collision of moving items with non-player entities.
void itemCollision(GameState *state, const ushort index) {
  if (state == NULL || index < 0)
    return;

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
    const Block *const block = &state->blocks[i];
    if (state->blocks[i].broken)
      continue;

    const int result = collision(
      item->rect, item->velocity, block->rect, state->screen.tile / 2);

    if (!result)
      continue;

    if (block->gotHit && block->rect.y > item->rect.y) {
      item->velocity.y = -ITEM_JUMP_FORCE;
      continue;
    }

    // EROR: On the frame before star hopping starts, the resolve is not
    // properly made FIX: The idiot here just did not account for a start
    // hitting the top of a block
    resolveCollision(&item->rect, &block->rect, result);

    if (result > 0)
      item->velocity.x = -item->velocity.x;
    else if (item->type == STAR && item->rect.y + item->rect.h == block->rect.y)
      item->velocity.y = -ITEM_JUMP_FORCE;
    else
      item->velocity.y = 0;
  }

  for (uint i = 0; i < state->objsLength; i++) {
    const SDL_FRect *const object = &state->objs[i];
    // ERROR: Using collision() in a if statement causes weird behaviour
    // ERROR: Using its value in !result or resolveCollision() causes weirder
    // behaviour const int result = collision(item->rect, item->velocity,
    // *object, state->screen.tile / 2); NOTE: Temporary trick to stop item
    // hopping
    const int result = item->rect.y > object->y - item->rect.h * 1.25 ? -1 : 0;

    if (!result)
      continue;

    resolveCollision(&item->rect, object, result);

    if (result > 0)
      item->velocity.x = -item->velocity.x;
    else if (item->type == STAR && item->rect.y + item->rect.h == object->y)
      item->velocity.y = -ITEM_JUMP_FORCE;
    else
      item->velocity.y = 0;
  }

  if (item->rect.x < 0 || item->rect.x + item->rect.w > state->screen.w)
    item->velocity.x *= -1;
  else if (item->rect.y + item->rect.w > state->screen.w)
    item->velocity.y = 0;
  else if (item->rect.y < 0)
    item->velocity.y = GRAVITY * 2;
}

// Takes care of the collision of the fireballs with non-player entities.
void fireballCollision(GameState *state, const ushort index) {
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
    (void)block;

    if (state->blocks[i].broken)
      continue;

    const int result =
      collision(ball->rect, ball->velocity, *block, state->screen.tile / 2);

    if (!result)
      continue;

    resolveCollision(&ball->rect, block, result);

    if (result > 0)
      ball->velocity.x *= -1;
    else
      ball->velocity.y *= -1;
  }

  for (uint i = 0; i < state->objsLength; i++) {
    const SDL_FRect *const object = &state->objs[i];

    const int result =
      collision(ball->rect, ball->velocity, *object, state->screen.tile / 2);

    if (!result)
      continue;

    resolveCollision(&ball->rect, object, result);

    if (result > 0)
      ball->velocity.x *= -1;
    else
      ball->velocity.y *= -1;
  }
}

// Handles the collision a axis per time, call this first with dx only,
// then call it again for the dy.
// This function will only run for things that are displayed on screen.
void playerCollision(GameState *state) {
  Player *player = &state->player;
  const ushort tile = state->screen.tile;

  for (uint i = 0; i < state->blocksLenght; i++) {
    Block *block = &state->blocks[i];
    Item *item = &block->item;

    // If a block has been broken or is off-screen, skip its collision check
    if (block->broken ||
        (block->rect.x + block->rect.w < 0 ||
         block->rect.x > state->screen.w) ||
        (block->rect.y + block->rect.h < 0 || block->rect.y > state->screen.h))
      continue;

    int result = collision(
      player->hitbox, player->velocity, block->rect, state->screen.tile);

    if (!result && !item->free)
      continue;

    if (result < 0 && player->velocity.y > 0 &&
        block->rect.y > player->hitbox.y)
      player->onSurface = true;

    // ERROR: Player hitbox is resolved uncorrectly when the collider is the
    // block
    if (block->rect.y != block->initY &&
        block->rect.y + block->rect.h > player->hitbox.y + player->velocity.y &&
        result > 0) {
      result = -1;
    }

    resolveCollision(&player->hitbox, &block->rect, result);

    // ERROR: When going for the last block a collision bug happens
    // Block *neighbour = NULL;
    // if (i + 1 <= state->blocksLenght && player->facingRight) {
    //   neighbour = &state->blocks[i + 1];
    //   if (neighbour->rect.x <= player->rect.x + player->rect.w / 2.0)
    //     continue;
    // }

    if (result > 0)
      player->velocity.x = 0;
    else if (result < 0) {
      if (player->velocity.y < 0) {
        if (block->type == NOTHING && player->tall)
          block->broken = true;

        if (!item->free || block->coinCount)
          block->gotHit = true;

        if (block->type == FULL)
          item->free = true;

        // TODO: Later add this coin to player->coinCount
        if (item->type == COINS && block->coinCount) {
          block->coinCount--;
          if (!block->coinCount)
            block->type = EMPTY;
          for (ushort j = 0; j < block->maxCoins; j++) {
            Coin *coin = &block->coins[j];
            if (!coin->onAir) {
              coin->onAir = true;
              break;
            }
          }
        }
      }
      player->velocity.y = 0;
    }

    // Item collison
    if (item->visible && item->free && item->type > COINS) {
      if (!collision(
            player->hitbox, player->velocity, item->rect, state->screen.tile))
        continue;

      item->visible = false;
      if ((item->type == MUSHROOM || item->type == FIRE_FLOWER) &&
          !player->tall) {
        player->rect.y -= tile;
        player->rect.h += tile;
        player->hitbox.h = player->rect.h;
        player->transforming = true;
      } else if (item->type == FIRE_FLOWER && !player->fireForm)
        player->fireForm = true;
      else if (item->type == STAR)
        player->invincible = true;
    }
  }

  // Player with object collision
  for (uint i = 0; i < state->objsLength; i++) {
    const SDL_FRect *const object = &state->objs[i];

    if ((object->x + object->w < 0 || object->x > state->screen.w) ||
        (object->y + object->h < 0 || object->y > state->screen.h))
      continue;

    const int result = collision(
      player->hitbox, player->velocity, *object, state->screen.tile / 2);

    if (!result)
      continue;

    if (result < 0 && player->velocity.y > 0 && object->y > player->hitbox.y)
      player->onSurface = true;

    resolveCollision(&player->hitbox, object, result);

    if (result > 0)
      player->velocity.x = 0;
    else
      player->velocity.y = 0;
  }

  if (player->velocity.y)
    player->onSurface = false;

  if (player->onSurface)
    player->jumping = false;
}
