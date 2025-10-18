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
#include "render.h"
#include "init.h"
#include "input.h"

// Handles the collision a axis per time, call this first with dx only,
// then call it again for the dy.
// This function will only run for things that are displayed on screen.
void handlePlayerColl(float dx, float dy, GameState *state) {
  Player *player = &state->player;
  Block *blocks = state->blocks;
  const u_short pw = player->w, ph = player->h;
  _Bool onBlock = false, onObj = false;
  float *px = &player->x, *py = &player->y;
  const float BLOCK_SPEED = 1.5f;

  for (uint i = 0; i < state->blocksLenght; i++) {
    Item *item = &blocks[i].item;
    _Bool *gotHit = &blocks[i].gotHit, *isVisible = &item->isVisible;
    const BlockState blockType = blocks[i].type;
    const ItemType itemType = item->type;
    float ix = item->x, iy = item->y;
    // NOTES: Don't use these if blockType == NOTHING
    const u_short iw = item->w, ih = item->h;

    SDL_Rect brect = blocks[i].rect;
    float bx = brect.x, by = blocks[i].y;
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
        blocks[i].y -= BLOCK_SPEED;
      else
        *gotHit = false;
    } else if (by != initY)
      blocks[i].y += BLOCK_SPEED;

    // Item coming out of block
    if (itemType > COINS && item->isFree) {
      if (item->y > initY - state->screen.tile)
        item->y -= BLOCK_SPEED;
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
        if (!coin->willFall && coin->y > initY - state->screen.tile * 3)
          coin->y -= COIN_SPEED;
        else
          coin->willFall = true;
        if (coin->willFall && coin->y < initY)
          coin->y += COIN_SPEED;
        else if (coin->y == initY) {
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
          player->y -= state->screen.tile;
          player->h += state->screen.tile;
          player->transforming = true;
        } else if (itemType == FIRE_FLOWER && !player->fireForm) {
          if (!player->tall) {
            player->y -= state->screen.tile;
            player->h += state->screen.tile;
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

// Takes care of the collision of the fireballs with non-player entities.
void handleFireballColl(GameState *state, u_short index, float dx, float dy) {
  Fireball *ball = &state->player.fireballs[index];
  const u_short fs = state->screen.tile / 2;

  if (!ball->visible)
    return;
  if (!((ball->x + fs > 0 && ball->x < state->screen.w) &&
        (ball->y + fs > 0 && ball->y < state->screen.h))) {
    ball->visible = false;
    return;
  }

  for (uint i = 0; i < state->blocksLenght; i++) {
    const float bx = state->blocks[i].rect.x, by = state->blocks[i].y;
    const u_short bs = state->screen.tile;
    if ((ball->x < bx + bs && ball->x + fs > bx) &&
        (ball->y < by + bs && ball->y + fs > by)) {
      if (dx > 0)
        ball->x = bx - fs;
      else if (dx < 0)
        ball->x = bx + bs;
      else if (dy > 0)
        ball->y = by - fs;
      else if (dy < 0)
        ball->y = by + bs;
      if (dx)
        ball->dx *= -1;
      else
        ball->dy *= -1;
    }
  }
  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    if ((ball->x < obj.x + obj.w && ball->x + fs > obj.x) &&
        (ball->y < obj.y + obj.h && ball->y + fs > obj.y)) {
      if (dx > 0)
        ball->dx = obj.w - obj.w;
      else if (dx < 0)
        ball->dx = obj.w + obj.w;
      else if (dy > 0)
        ball->y = obj.y - fs;
      else if (dy < 0)
        ball->y = obj.y + obj.h;
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
  const u_short TARGET_FPS = state->screen.targetFps;
  const float MAX_GRAVITY = 20;

  player->x += player->dx * TARGET_FPS * dt;
  handlePlayerColl(player->dx, 0, state);

  if (player->dy < MAX_GRAVITY) {
    player->dy += GRAVITY * TARGET_FPS * dt;
    player->y += player->dy * TARGET_FPS * dt;
  }
  handlePlayerColl(0, player->dy, state);

  for (u_short i = 0; i < player->fireballLimit; i++) {
    Fireball *ball = &player->fireballs[i];
    ball->x += ball->dx * TARGET_FPS * dt;
    handleFireballColl(state, i, ball->dx, 0);
    ball->y += ball->dy * TARGET_FPS * dt;
    handleFireballColl(state, i, 0, ball->dy);
  }

  // NOTES: Placeholder code below, prevent from falling into endeless pit
  if (player->y - player->h > state->screen.h) {
    player->y = 0 - player->h;
    player->x = state->screen.h / 2.0f - player->w;
  }
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
