#include <SDL2/SDL.h>
#include "gameState.h"

// Handles the collision a axis per time, call this first with dx only,
// then call it again for the dy.
// This function will only run for things that are displayed on screen.
void handlePlayerColl(float dx, float dy, GameState *state) {
  Player *player = &state->player;
  Block *blocks = state->blocks;
  _Bool onBlock = false, onObj = false;

  float *px = &player->hitbox.x, *py = &player->hitbox.y;
  const float pw = player->hitbox.w, ph = player->hitbox.h;
  const float BLOCK_SPEED = 1.5f;

  for (uint i = 0; i < state->blocksLenght; i++) {
    Item *item = &blocks[i].item;
    const BlockState blockType = blocks[i].type;
    _Bool *gotHit = &blocks[i].gotHit, *isVisible = &item->isVisible;

    const ItemType itemType = item->type;
    float ix = item->rect.x, iy = item->rect.y;

    // NOTES: Don't use these if blockType == NOTHING
    const u_short iw = item->rect.w, ih = item->rect.h;

    SDL_FRect brect = blocks[i].rect;
    float bx = brect.x, by = blocks[i].rect.y;
    const u_short bw = brect.w, bh = brect.h;

    // NOTES: This reads as: If a brick block, or a empty coin block,
    //        or an empty fire flower block is off screen, skip it.
    // TODO: Test if this is working later
    if (blocks[i].gotDestroyed)
      continue;
    else if ((blockType == NOTHING ||
              (blockType == EMPTY && itemType == COINS) ||
              (blockType == EMPTY && itemType == FIRE_FLOWER)) &&
             ((bx + bw < 0 || bx > state->screen.w) ||
              (by + bh < 0 || by > state->screen.h)))
      continue;

    const _Bool blockCollision =
      (*px < bx + bw && *px + pw > bx) && (*py < by + bh && *py + ph > by);
    const float initY = blocks[i].initY;

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
        if (itemType == COINS && blocks[i].coinCount) {
          blocks[i].coinCount--;
          for (u_short j = 0; j < blocks[i].maxCoins; j++) {
            Coin *coin = &blocks[i].coins[j];
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
        if (blockType != EMPTY)
          *gotHit = true;
        if (blockType == NOTHING && player->tall)
          blocks[i].gotDestroyed = true;
        if (blockType == FULL) {
          item->isFree = true;
          if (itemType > COINS || !blocks[i].coinCount) {
            blocks[i].type = EMPTY;
            blocks[i].sprite = EMPTY_SPRITE;
          }
        }
      }
      if (dx)
        player->dx = 0;
      else {
        player->dy = 0;
        player->gainingHeigth = false;
      }
    }

    // Block bump
    if (*gotHit &&
        (!player->tall && (blockType == NOTHING || itemType == COINS))) {
      const float bjump = initY + bh - state->screen.tile / 4.0f;
      if (by + bh > bjump)
        blocks[i].rect.y -= BLOCK_SPEED;
      else
        *gotHit = false;
    } else if (by != initY)
      blocks[i].rect.y += BLOCK_SPEED;

    // Item coming out of block
    if (itemType > COINS && item->isFree) {
      if (item->rect.y > initY - state->screen.tile)
        item->rect.y -= BLOCK_SPEED;
      else
        blocks[i].type = EMPTY;
    }

    // Coins coming out of block
    if (itemType == COINS && item->isFree) {
      const float COIN_SPEED = BLOCK_SPEED * 3;
      for (u_short j = 0; j < blocks[i].maxCoins; j++) {
        Coin *coin = &blocks[i].coins[j];
        if (!coin->onAir)
          continue;
        if (!coin->willFall && coin->rect.y > initY - state->screen.tile * 3)
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
    if (*isVisible && item->isFree && itemType > COINS) {
      const _Bool itemCollision =
        (*px < ix + iw && *px + pw > ix) && (*py < iy + ih && *py + ph > iy);
      if (itemCollision) {
        *isVisible = false;
        if (itemType == MUSHROOM && !player->tall) {
          // TODO: When TALL_TO_FIRE is made uncomment this
          //       player->tall = true;
          player->rect.y -= state->screen.tile;
          player->rect.h += state->screen.tile;
          player->transforming = true;
        } else if (itemType == FIRE_FLOWER && !player->fireForm) {
          if (!player->tall) {
            player->rect.y -= state->screen.tile;
            player->rect.h += state->screen.tile;
            // TODO: Move this when TALL_TO_FIRE is made
            player->transforming = true;
          }
          player->fireForm = true;
        } else if (itemType == STAR)
          player->invincible = true;
      }
    }
  }
  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    const _Bool collision = (*px < obj.x + obj.w && *px + pw > obj.x) &&
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
        player->dx = 0;
      else
        player->dy = 0;
    }
  }
  player->onSurface = onBlock || onObj;
}
