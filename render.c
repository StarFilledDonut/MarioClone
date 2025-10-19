#include <SDL2/SDL.h>
#include "gameState.h"

// Handles animations and wich frames all moving parts of the game to be in.
void handlePlayerFrames(GameState *state) {
  Player *player = &state->player;
  const _Bool isSmall = !player->tall && !player->fireForm,
              isJumping = player->onJump && !player->isSquatting,
              isWalking =
                player->isWalking && !player->isSquatting && !player->onJump;
  const uint tile = state->screen.tile;
  int animSpeed = fabsf(player->dx * 0.3f);
  if (!animSpeed)
    animSpeed = 1;
  const uint walkFrame = SDL_GetTicks() * animSpeed / 180 % 3;
  enum {
    STILL,
    WALK,
    TURNING = 4,
    JUMP,
    DYING,
    TALL_STILL = 7 * 4,
    TALL_WALK,
    TALL_TURNING = 32,
    TALL_JUMP,
    TALL_SQUATTING,
    STAR_TALL_MARIO,
    FIRE_STILL = 35 + 7 * 3,
    FIRE_WALK,
    FIRE_TURNING = 60,
    FIRE_JUMP,
    FIRE_SQUATTING,
    FIRE_FIRING,
    SMALL_TO_TALL = 75,
    SMALL_TO_FIRE = 78
  };

  if (player->transforming && !player->tall) {
    if (!state->screen.xformTimer)
      state->screen.xformTimer = SDL_GetTicks();

    const uint elapsedTime = SDL_GetTicks() - state->screen.xformTimer;
    const uint xformFrame = elapsedTime / 180 % 3;
    int xformTo;

    if (!player->fireForm)
      xformTo = SMALL_TO_TALL;
    else
      xformTo = SMALL_TO_FIRE;
    player->frame = xformFrame + xformTo;
    player->rect.h = tile * 2;

    if (elapsedTime >= 2000) {
      player->transforming = false;
      // TODO: Make an animation for TALL_TO_FIRE
      player->tall = true;
    } else
      return;
    // TODO: test if this return cancels the rendering of later objs
  }
  // Star timer
  if (player->invincible) {
    if (!state->screen.starTimer)
      state->screen.starTimer = SDL_GetTicks();

    if (SDL_GetTicks() - state->screen.starTimer > 20 * 1000) {
      state->screen.starTimer = 0;
      player->invincible = false;
    }
  }

  // Firing timer
  if (player->isFiring) {
    if (!state->screen.firingTimer)
      state->screen.firingTimer = SDL_GetTicks();

    if (SDL_GetTicks() - state->screen.firingTimer > 200) {
      state->screen.firingTimer = 0;
      player->isFiring = false;
    }
  }

  if (isSmall) {
    if (isJumping)
      player->frame = JUMP;
    else if (!isWalking)
      player->frame = STILL;
    else
      player->frame = walkFrame + WALK;
  } else if (player->tall && !player->fireForm) {
    if (isJumping)
      player->frame = TALL_JUMP;
    else if (!isWalking)
      player->frame = TALL_STILL;
    else
      player->frame = walkFrame + TALL_WALK;

    if (player->isSquatting)
      player->frame = TALL_SQUATTING;
  } else {
    if (isJumping)
      player->frame = FIRE_JUMP;
    else if (!isWalking)
      player->frame = FIRE_STILL;
    else
      player->frame = walkFrame + FIRE_WALK;

    if (player->isSquatting)
      player->frame = FIRE_SQUATTING;
    if (player->isFiring && !isWalking && !isJumping)
      player->frame = FIRE_FIRING;
    else if (player->isFiring && isWalking)
      player->frame = walkFrame + FIRE_FIRING;
    else if (player->isFiring && isJumping)
      player->frame = FIRE_FIRING + 1;
  }

  if (player->invincible) {
    const uint starFrame = SDL_GetTicks() / 90 % 4;
    if (!player->fireForm)
      player->frame += starFrame * 7;
    else if (!player->isFiring) {
      u_short fireStarFrames[4] = {
        0, TALL_STILL - 7, TALL_STILL - 7 * 2, TALL_STILL - 7 * 3};
      player->frame -= fireStarFrames[starFrame];
    } else {
      player->frame += 3 * starFrame;
    }
  }
}

// If it is not free, then it will be static
u_short handleItemFrames(Item *item) {
  enum { FLOWER_FRAME = 2, STAR_FRAME = 6, COIN_FRAME = 10 };
  const u_short velocity = item->type == COINS ? 100 : 180;
  u_short itemFrame = item->isFree ? SDL_GetTicks() / velocity % 4 : 0;

  if (item->type == FIRE_FLOWER)
    return itemFrame + FLOWER_FRAME;
  else if (item->type == STAR)
    return itemFrame + STAR_FRAME;
  else
    return itemFrame + COIN_FRAME;
}

// Renders to the screen
void render(GameState *state) {
  handlePlayerFrames(state);
  Player *player = &state->player;
  Sheets *sheets = &state->sheets;
  Screen *screen = &state->screen;
  Block *blocks = state->blocks;
  u_short tile = screen->tile;

  SDL_SetRenderDrawColor(state->renderer, 92, 148, 252, 255);
  SDL_RenderClear(state->renderer);

  SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
  // NOTES: Delmiter of the bottom of the screen
  SDL_RenderDrawLine(state->renderer, 0, screen->h, screen->w, screen->h);

  SDL_Rect srcground = {0, 16, 32, 32};
  SDL_Rect dstground = {0, screen->h - tile * 2, tile * 2, tile * 2};

  // Rendering ground
  // TODO: This method is very limitating, because it does not follow a map
  // NOTE:  This must be behind the block breaking bits
  for (uint i = 0; i < state->objsLength; i++) {
    state->objs[i] = dstground;
    SDL_RenderCopy(state->renderer, sheets->objs, &srcground, &dstground);
    dstground.x += dstground.w;
  }
  // Rendering blocks
  for (uint i = 0; i < state->blocksLenght; i++) {
    if (blocks[i].type != NOTHING && blocks[i].item.isVisible &&
        blocks[i].item.type != COINS) {
      Item *item = &blocks[i].item;
      SDL_Rect dstitem = {
        item->rect.x, item->rect.y, item->rect.w, item->rect.h};
      u_short frame;

      if (item->type == MUSHROOM)
        frame = 0;
      else
        frame = handleItemFrames(item);
      SDL_RenderCopy(
        state->renderer, sheets->items, &sheets->srcitems[frame], &dstitem);
    } else if (blocks[i].type != NOTHING && blocks[i].item.type == COINS) {
      for (u_short j = 0; j < blocks[i].maxCoins; j++) {
        Coin *coin = &blocks[i].coins[j];
        if (!coin->onAir)
          continue;
        SDL_Rect dstcoin = {
          coin->rect.x, coin->rect.y, coin->rect.w, coin->rect.h};
        u_short frame = handleItemFrames(&blocks[i].item);

        SDL_RenderCopy(
          state->renderer, sheets->items, &sheets->srcitems[frame], &dstcoin);
      }
    }
    if (!blocks[i].gotDestroyed) {
      SDL_RenderCopyF(state->renderer,
                      sheets->objs,
                      &sheets->srcsobjs[blocks[i].sprite],
                      &blocks[i].rect);
    } else {
      u_short bitSize = screen->tile / 2;
      for (u_short j = 0; j < 4; j++) {
        float *bitDx = &blocks[i].bitDx, *bitDy = &blocks[i].bitDy,
              *bitX = &blocks[i].bitsX[j], *bitY = &blocks[i].bitsY[j];
        SDL_Rect bit = {*bitX, *bitY, bitSize, bitSize};

        if (blocks[i].bitsY[0] > (int)screen->h + 1)
          break;
        else if (bit.y > (int)screen->h + 1)
          continue;

        const float SPEED = 1.2f * screen->targetFps * screen->deltaTime;
        const u_short MAX_SPEED = 6, LIMIT = blocks[i].initY - tile;

        if (*bitDy > -MAX_SPEED && !blocks[i].bitFall)
          *bitDy -= SPEED;
        else if (*bitY <= LIMIT)
          blocks[i].bitFall = true;
        if (*bitDx < MAX_SPEED && !blocks[i].bitFall)
          *bitDx += SPEED * screen->targetFps * screen->deltaTime;
        if (*bitDy < MAX_SPEED * 1.25f)
          *bitDy += GRAVITY * screen->targetFps * screen->deltaTime;
        *bitY += *bitDy;

        if (j < 2 && *bitDy < 0)
          *bitY += *bitDy;
        if (j == 1 || j == 3)
          *bitX += *bitDx * 0.5f;
        else
          *bitX -= *bitDx * 0.5f;

        SDL_RenderCopy(
          state->renderer, sheets->effects, &sheets->srceffects[j], &bit);
      }
    }
  }

  if (player->isSquatting) {
    player->rect.h += tile;
    player->rect.y -= tile;
  }
  SDL_RenderCopyExF(state->renderer,
                    sheets->mario,
                    &sheets->srcmario[player->frame],
                    &player->rect,
                    0,
                    NULL,
                    player->facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);

  for (u_short i = 0; i < player->fireballLimit && player->fireForm; i++) {
    Fireball *ball = &player->fireballs[i];
    if (!ball->visible)
      continue;
    const u_short fs = tile / 2;
    SDL_Rect fireballrect = {ball->rect.x, ball->rect.y, fs, fs};
    const u_short frame = SDL_GetTicks() / 180 % 4 + 4;
    SDL_RenderCopy(state->renderer,
                   sheets->effects,
                   &sheets->srceffects[frame],
                   &fireballrect);
  }
  SDL_RenderPresent(state->renderer);
}
