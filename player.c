#include <SDL2/SDL.h>
#include "gameState.h"

#define BLOCK_SPEED 1.5f

// Handles the collision a axis per time, call this first with dx only,
// then call it again for the dy.
// This function will only run for things that are displayed on screen.
void handlePlayerColl(float dx, float dy, GameState *state) {
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

    // NOTES: This reads as: If a brick block, or a empty coin block,
    //        or an empty fire flower block is off screen, skip it.
    // TODO: Test if this is working later
    if (block->broken)
      continue;
    else if ((block->type == NOTHING ||
              (block->type == EMPTY && item->type == COINS) ||
              (block->type == EMPTY && item->type == FIRE_FLOWER)) &&
             ((bx + bw < 0 || bx > state->screen.w) ||
              (by + bh < 0 || by > state->screen.h)))
      continue;

    const bool blockCollision =
      (*px < bx + bw && *px + pw > bx) && (*py < by + bh && *py + ph > by);
    const float initY = block->initY;

    if (blockCollision) {
      if (dx > 0)
        *px = bx - pw;
      else if (dx < 0)
        *px = bx + bw;
      else if (dy > 0) {
        *py = by - ph;
        onBlock = true;
      } else if (dy < 0) {
        *py = by + bh;
        // TODO: Later add this coin to player->coinCount
        if (item->type == COINS && block->coinCount) {
          block->coinCount--;
          for (ushort j = 0; j < block->maxCoins; j++) {
            Coin *coin = &block->coins[j];
            if (!coin->onAir) {
              coin->onAir = true;
              break;
            } else
              continue;
          }
        }
        // TODO: Make this condition be done with the side he is oriented
        //       with, AKA make his fist collide with the bottom half of the
        //       block for it to get hit
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
      }
      if (dx)
        player->velocity.x = 0;
      else {
        player->velocity.y = 0;
        player->gainingHeigth = false;
      }
    }

    // Block bump
    if (block->gotHit &&
        ((!player->tall && block->type == NOTHING) || item->type == COINS)) {
      const float goal = initY + bh - tile / 4.0f;
      if (by + bh > goal)
        block->rect.y -= BLOCK_SPEED;
      else
        block->gotHit = false;
    } else if (by != initY)
      block->rect.y += BLOCK_SPEED;

    // Item coming out of block
    if (item->type > COINS && item->free) {
      if (item->rect.y > initY - tile)
        item->rect.y -= BLOCK_SPEED;
      else
        block->type = EMPTY;
    }

    // ERROR: For a reason that gods know why, coins come at a Item speed
    //        when player.velocity.x == 0
    // Coins coming out of block
    if (item->type == COINS && item->free) {
      const float COIN_SPEED = BLOCK_SPEED * 3;
      for (ushort j = 0; j < block->maxCoins; j++) {
        Coin *coin = &block->coins[j];
        if (!coin->onAir)
          continue;

        if (!coin->willFall && coin->rect.y > initY - tile * 3)
          coin->rect.y -= COIN_SPEED;
        else
          coin->willFall = true;

        if (coin->willFall && coin->rect.y < initY)
          coin->rect.y += COIN_SPEED;
        else if (coin->rect.y == initY) {
          coin->willFall = false;
          coin->onAir = false;
        }
      }
    }

    // Item collison
    if (item->visible && item->free && item->type > COINS) {
      const bool itemCollision =
        (*px < ix + iw && *px + pw > ix) && (*py < iy + ih && *py + ph > iy);
      if (itemCollision) {
        item->visible = false;
        if (item->type == MUSHROOM && !player->tall) {
          player->rect.y -= tile;
          player->rect.h += tile;
          player->hitbox.h = player->rect.h;
          // TODO: When TALL_TO_FIRE is made uncomment this
          //       player->tall = true;
          player->transforming = true;
        } else if (item->type == FIRE_FLOWER && !player->fireForm) {
          if (!player->tall) {
            player->rect.y -= tile;
            player->rect.h += tile;
            // TODO: Move this when TALL_TO_FIRE is made
            player->transforming = true;
          }
          player->fireForm = true;
        } else if (item->type == STAR)
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
