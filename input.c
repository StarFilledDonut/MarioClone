#include <SDL2/SDL.h>
#include "utils.h"

// Takes care of all the events of the game.
void handleEvents(GameState *state) {
  SDL_Event event;
  Player *player = &state->player;
  const short MAX_JUMP = -15, MAX_SPEED = 7;
  const float JUMP_FORCE = 2.5f, SPEED = 0.2f, FRIC = 0.85f;
  const ushort tile = state->screen.tile;

  if (player->onSurface) {
    player->gainingHeigth = false;
    player->jumping = false;
  }

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        quit(state, 0);
        break;
      case SDL_WINDOWEVENT_CLOSE:
        quit(state, 0);
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit(state, 0);
            break;
          case SDLK_f: {
            if (!player->fireForm || player->squatting)
              break;
            ushort ballCount = 0, emptySlot;
            for (ushort i = 0; i < player->fireballLimit; i++) {
              if (player->fireballs[i].visible)
                ballCount++;
              else
                emptySlot = i;
            }
            if (ballCount < player->fireballLimit) {
              Fireball *ball = &player->fireballs[emptySlot];
              if (player->facingRight) {
                ball->rect.x = player->rect.x + player->rect.w;
                ball->dx = MAX_SPEED;
              } else {
                ball->rect.x = player->rect.x;
                ball->dx = -MAX_SPEED;
              }
              ball->rect.y = player->rect.y;
              ball->dy = MAX_SPEED;
              ball->visible = true;
              if (player->firing)
                state->screen.firingTimer = 0;
              player->firing = true;
            }
            break;
          }
        }
      case SDL_KEYUP: {
        if (event.key.repeat != 0)
          break;
        const SDL_Keycode keyup = event.key.keysym.sym;
        if (keyup == SDLK_SPACE || keyup == SDLK_w || keyup == SDLK_UP) {
          if (player->dy < 0)
            player->dy *= FRIC;
          player->holdingJump = false;
          player->gainingHeigth = false;
        }
        if (keyup == SDLK_s || keyup == SDLK_DOWN) {
          if (player->squatting)
            player->rect.y -= tile;
          player->squatting = false;
        }
        break;
      }
    }
  }
  bool walkPressed = false;
  const Uint8 *key = SDL_GetKeyboardState(NULL);
  if (!player->squatting && (key[SDL_SCANCODE_LEFT] || key[SDL_SCANCODE_A])) {
    player->facingRight = false;
    player->walking = true;
    walkPressed = true;
    if (player->dx > 0)
      player->dx *= FRIC;
    if (player->dx > -MAX_SPEED)
      player->dx -= SPEED;
  } else if (!player->squatting &&
             (key[SDL_SCANCODE_RIGHT] || key[SDL_SCANCODE_D])) {
    player->facingRight = true;
    player->walking = true;
    walkPressed = true;
    if (player->dx < 0)
      player->dx *= FRIC;
    if (player->dx < MAX_SPEED)
      player->dx += SPEED;
  } else {
    if (player->dx) {
      player->dx *= FRIC;
      if (fabsf(player->dx) < 0.1f)
        player->dx = 0;
    } else
      player->walking = false;
    walkPressed = false;
  }
  if (player->onSurface && !walkPressed && (player->tall || player->fireForm) &&
      (key[SDL_SCANCODE_DOWN] || key[SDL_SCANCODE_S])) {
    if (!player->squatting)
      player->rect.y += tile;
    player->squatting = true;
  }
  if (((!player->holdingJump && player->onSurface) ||
       (!player->onSurface && player->gainingHeigth)) &&
      (key[SDL_SCANCODE_SPACE] || key[SDL_SCANCODE_W] ||
       key[SDL_SCANCODE_UP])) {
    player->dy -= JUMP_FORCE;
    if (player->dy < MAX_JUMP)
      player->gainingHeigth = false;
    else
      player->gainingHeigth = true;
    player->holdingJump = true;
    player->jumping = true;
  }
  // Size handling
  if (!player->squatting && (player->tall || player->fireForm))
    player->rect.h = tile * 2;
  else
    player->rect.h = tile;

  // NOTES: TEMPORARY CEILING AND LEFT WALL
  if (player->rect.y < 0)
    player->rect.y = 0;
  if (player->rect.x < 0)
    player->rect.x = 0;
}
