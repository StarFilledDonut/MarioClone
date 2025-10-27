#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <math.h>
#include "gameState.h"

#define BLOCK_SPEED 3

// Shattering animation when player breaks a block
void blockBreakAnimation(struct Particle *particle, const ushort index) {
  if (particle->velocity.x == 0) {
    if (index == 0 || index == 2)
      particle->velocity.x -= SPEED * 1.2f;
    else
      particle->velocity.x += SPEED * 1.2f;
  }

  if (index < 2 && particle->velocity.y < MAX_GRAVITY * 1.5f)
    particle->velocity.y += GRAVITY;
  else if (index >= 2 && particle->velocity.y < MAX_GRAVITY * 1.5f)
    particle->velocity.y += GRAVITY * 1.25f;

  particle->rect.x += particle->velocity.x;
  particle->rect.y += particle->velocity.y;
}

// Bump animation when player hits a Block from below
void blockAnimation(Block *block, const ushort tile) {
  if (block->gotHit) {
    const float goal = block->initY + block->rect.h - tile / 4.0f;
    if (block->rect.y + block->rect.h > goal)
      block->rect.y -= BLOCK_SPEED;
    else
      block->gotHit = false;
  } else if (block->rect.y != block->initY)
    block->rect.y += BLOCK_SPEED;
}

// Animate items and coins comming out of the block
void itemAnimation(Block *block, const ushort tile) {
  if (block->item.type > COINS && block->item.free) {
    if (block->item.rect.y > block->initY - tile)
      block->item.rect.y -= BLOCK_SPEED;
    else
      block->type = EMPTY;
  }

  // When the type Coin be made into type of Coin, make it so onAir is
  // unnecessary Coins coming out of block
  if (block->item.type == COINS && block->item.free) {
    const float COIN_SPEED = BLOCK_SPEED * 3;
    for (ushort i = 0; i < block->maxCoins; i++) {
      Coin *coin = &block->coins[i];
      if (!coin->onAir)
        continue;

      if (!coin->willFall && coin->rect.y > block->initY - tile * 3)
        coin->rect.y -= COIN_SPEED;
      else
        coin->willFall = true;

      if (coin->willFall && coin->rect.y < block->initY)
        coin->rect.y += COIN_SPEED;
      else if (coin->rect.y == block->initY) {
        coin->willFall = false;
        coin->onAir = false;
      }
    }
  }
}

// Handles animations and wich frames all moving parts of the game to be in.
void handlePlayerFrames(GameState *state) {
  Player *player = &state->player;
  const bool isSmall = !player->tall && !player->fireForm,
             jump = player->jumping && !player->crounching,
             walking = player->walking && !player->jumping;

  int animationSpeed = fabsf(player->velocity.x * 0.3f);

  if (!animationSpeed)
    animationSpeed = 1;

  const uint walkFrame = SDL_GetTicks() * animationSpeed / 180 % 3;

  // Transofrmation animation
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
    player->rect.h = state->screen.tile * 2;

    if (elapsedTime >= 2000) {
      player->transforming = false;
      player->tall = true;
    } else
      return;
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
  if (player->firing) {
    if (!state->screen.firingTimer)
      state->screen.firingTimer = SDL_GetTicks();

    if (SDL_GetTicks() - state->screen.firingTimer > 200) {
      state->screen.firingTimer = 0;
      player->firing = false;
    }
  }

  if (isSmall) {
    if (jump)
      player->frame = JUMP;
    else if (!walking)
      player->frame = STILL;
    else
      player->frame = walkFrame + WALK;
  } else if (player->tall && !player->fireForm) {
    if (jump)
      player->frame = TALL_JUMP;
    else if (!walking)
      player->frame = TALL_STILL;
    else
      player->frame = walkFrame + TALL_WALK;

    if (player->crounching)
      player->frame = TALL_CROUNCHING;
  } else {
    if (jump)
      player->frame = FIRE_JUMP;
    else if (!walking)
      player->frame = FIRE_STILL;
    else
      player->frame = walkFrame + FIRE_WALK;

    if (player->crounching)
      player->frame = FIRE_CROUNCHING;
    if (player->firing && !walking && !jump)
      player->frame = FIRE_FIRING;
    else if (player->firing && walking)
      player->frame = walkFrame + FIRE_FIRING;
    else if (player->firing && jump)
      player->frame = FIRE_FIRING + 1;
  }

  // Star form animation
  if (player->invincible) {
    const uint starFrame = SDL_GetTicks() / 90 % 4;
    if (!player->fireForm)
      player->frame += starFrame * 7;
    else if (!player->firing) {
      ushort fireStarFrames[4] = {
        0, TALL_STILL - 7, TALL_STILL - 7 * 2, TALL_STILL - 7 * 3};
      player->frame -= fireStarFrames[starFrame];
    } else {
      player->frame += 3 * starFrame;
    }
  }
}

// If it is not free, then it will be static
ushort handleItemFrames(Item *item) {
  enum { FLOWER_FRAME = 2, STAR_FRAME = 6, COIN_FRAME = 10 };
  const ushort velocity = item->type == COINS ? 100 : 180;
  ushort itemFrame = item->free ? SDL_GetTicks() / velocity % 4 : 0;

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
  ushort tile = screen->tile;

  SDL_SetRenderDrawColor(state->renderer, 92, 148, 252, 255);
  SDL_RenderClear(state->renderer);

  SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
  // NOTES: Delmiter of the bottom of the screen
  SDL_RenderDrawLine(state->renderer, 0, screen->h, screen->w, screen->h);

  SDL_Rect srcground = {0, 16, 32, 32};
  SDL_FRect dstground = {0, screen->h - tile * 2, tile * 2, tile * 2};

  // Rendering ground
  // TODO: This method is very limitating, because it does not follow a map
  // NOTE: This must be behind the block breaking bits
  for (uint i = 0; i < state->objsLength; i++) {
    state->objs[i] = dstground;
    SDL_RenderCopyF(state->renderer, sheets->objs, &srcground, &dstground);
    dstground.x += dstground.w;
  }

  // Rendering blocks
  for (uint i = 0; i < state->blocksLenght; i++) {
    Block *block = &state->blocks[i];

    // Handling item frames
    if (block->type != NOTHING && block->item.visible &&
        block->item.type != COINS) {
      Item *item = &block->item;
      ushort frame;

      if (item->type == MUSHROOM)
        frame = 0;
      else
        frame = handleItemFrames(item);

      // Rendering Items
      SDL_RenderCopyF(
        state->renderer, sheets->items, &sheets->srcitems[frame], &item->rect);
    } else if (block->type != NOTHING && block->item.type == COINS) {
      for (ushort j = 0; j < block->maxCoins; j++) {
        Coin *coin = &block->coins[j];
        if (!coin->onAir)
          continue;
        ushort frame = handleItemFrames(&block->item);

        SDL_RenderCopyF(state->renderer,
                        sheets->items,
                        &sheets->srcitems[frame],
                        &coin->rect);
      }
    }

    // Animating blocks and items
    if ((!player->tall && block->type == NOTHING) ||
        block->item.type == COINS) {
      blockAnimation(block, screen->tile);
    }
    if (block->type != NOTHING)
      itemAnimation(block, screen->tile);

    // Rendering blocks or broken block's particles
    if (!block->broken) {
      SDL_RenderCopyF(state->renderer,
                      sheets->objs,
                      &sheets->srcsobjs[block->sprite],
                      &block->rect);
    } else {
      for (ushort j = 0; j < MAX_BLOCK_PARTICLES; j++) {
        struct Particle *particle = &block->particles[j];

        if (particle->rect.y >= screen->h)
          continue;

        blockBreakAnimation(particle, j);

        SDL_RenderCopyF(state->renderer,
                        sheets->effects,
                        &sheets->srceffects[j],
                        &particle->rect);
      }
    }
  }

  // Size handling
  if (player->tall || player->fireForm)
    player->rect.h = tile * 2;
  else
    player->rect.h = tile;

  // TODO: Add a debug mode to see all collisions
  // SDL_RenderDrawRectF(state->renderer, &player->hitbox);

  SDL_RenderCopyExF(state->renderer,
                    sheets->mario,
                    &sheets->srcmario[player->frame],
                    &player->rect,
                    0,
                    NULL,
                    player->facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);

  // Rendering fireballs
  for (ushort i = 0; i < MAX_FIREBALLS; i++) {
    Fireball *ball = &player->fireballs[i];
    if (!ball->visible)
      continue;

    const ushort frame = SDL_GetTicks() / 180 % 4 + 4;

    SDL_RenderCopyF(state->renderer,
                    sheets->effects,
                    &sheets->srceffects[frame],
                    &ball->rect);
  }
  SDL_RenderPresent(state->renderer);
}
