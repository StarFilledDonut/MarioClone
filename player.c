#include <SDL2/SDL.h>
#include "gameState.h"

// Handles the collision a axis per time, call this first with dx only,
// then call it again for the dy.
// This function will only run for things that are displayed on screen.
void playerCollision(GameState *state, float dx, float dy) {
  Player *player = &state->player;
  float *px = &player->hitbox.x, *py = &player->hitbox.y;
  const ushort pw = player->hitbox.w, ph = player->hitbox.h;

  bool onBlock = false, onObj = false;
  const ushort tile = state->screen.tile;

  for (uint i = 0; i < state->blocksLenght; i++) {
    Block *block = &state->blocks[i];
    Item *item = &block->item;

    // NOTES: Don't use these if block->type == NOTHING
    const float ix = item->rect.x, iy = item->rect.y;
    const ushort iw = item->rect.w, ih = item->rect.h;

    const float bx = block->rect.x, by = block->rect.y;
    const ushort bw = block->rect.w, bh = block->rect.h;

    // If a block has been broken or is off-screen, skip its collision check
    if (block->broken || (bx + bw < 0 || bx > state->screen.w) ||
        (by + bh < 0 || by > state->screen.h))
      continue;

    const bool blockCollision =
      (*px < bx + bw && *px + pw > bx) && (*py < by + bh && *py + ph > by);

    if (blockCollision) {
      if (dx > 0)
        *px = bx - pw;
      else if (dx < 0)
        *px = bx + bw;
      else if (dy > 0) {
        *py = by - ph;
        onBlock = true;
      } else if (dy < 0) {
        // ERROR: When going for the last block a collision bug happens
        // Block *neighbour = NULL;
        // if (i + 1 <= state->blocksLenght && player->facingRight) {
        //   neighbour = &state->blocks[i + 1];
        //   if (neighbour->rect.x <= player->rect.x + player->rect.w / 2.0)
        //     continue;
        // }
        *py = by + bh;

        if (block->type != EMPTY)
          block->gotHit = true;
        if (block->type == NOTHING && player->tall)
          block->broken = true;
        if (block->type == FULL) {
          item->free = true;
          if (item->type > COINS || !block->coinCount) {
            block->type = EMPTY;
            block->sprite = EMPTY_SPRITE;
          }
        }
        // TODO: Later add this coin to player->coinCount
        if (item->type == COINS && block->coinCount) {
          block->coinCount--;
          for (ushort j = 0; j < block->maxCoins; j++) {
            Coin *coin = &block->coins[j];
            if (!coin->onAir) {
              coin->onAir = true;
              break;
            }
          }
        }
      }
      if (dx)
        player->velocity.x = 0;
      else {
        player->velocity.y = 0;
        player->gainingHeigth = false;
      }
    }

    // Item collison
    if (item->visible && item->free && item->type > COINS) {
      const bool itemCollision =
        (*px < ix + iw && *px + pw > ix) && (*py < iy + ih && *py + ph > iy);
      if (itemCollision) {
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
  }

  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    const bool collision = (*px < obj.x + obj.w && *px + pw > obj.x) &&
                           (*py < obj.y + obj.h && *py + ph > obj.y);
    if ((obj.x + obj.w < 0 || obj.x > (int)state->screen.w) ||
        (obj.y + obj.h < 0 || obj.y > (int)state->screen.h))
      continue;

    if (collision) {
      if (dx > 0)
        *px = obj.x - pw;
      else if (dx < 0)
        *px = obj.x + obj.w;
      else if (dy > 0) {
        *py = obj.y - ph;
        onObj = true;
      } else if (dy < 0)
        *py = obj.y + obj.h;
      if (dx)
        player->velocity.x = 0;
      else
        player->velocity.y = 0;
    }
  }
  player->onSurface = onBlock || onObj;
}
// TODO: Make it so all changes to sprite in this function are moved to render.c
