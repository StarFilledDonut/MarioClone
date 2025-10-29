#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>
#include "gameState.h"
#include "utils.h"

// Takes care of all the events of the game.
void handleEvents(GameState *state) {
  SDL_Event event;
  Player *player = &state->player;
  static bool holdingJump = false;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        quit(state, 0);
        break;
      case SDL_WINDOWEVENT_CLOSE:
        quit(state, 0);
        break;
      case SDL_KEYDOWN:
        if (event.key.repeat != 0)
          break;
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit(state, 0);
            break;
          case SDLK_f: {
            if (!player->fireForm || player->crounching || player->firing)
              break;

            // Finding an available fireball slot
            ushort ballCount = 0, emptySlot = 0;
            for (ushort i = 0; i < MAX_FIREBALLS; i++) {
              if (player->fireballs[i].visible)
                ballCount++;
              else {
                emptySlot = i;
                break;
              }
            }

            if (ballCount < MAX_FIREBALLS) {
              Fireball *ball = &player->fireballs[emptySlot];

              if (player->facingRight) {
                ball->rect.x = player->rect.x + player->rect.w;
                ball->velocity.x = MAX_SPEED;
              } else {
                ball->rect.x = player->rect.x;
                ball->velocity.x = -MAX_SPEED;
              }

              ball->rect.y = player->rect.y;
              ball->velocity.y = MAX_SPEED;
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
        if ((player->jumping || !player->velocity.y) &&
            (keyup == SDLK_SPACE || keyup == SDLK_w || keyup == SDLK_UP)) {
          if (player->velocity.y < 0)
            player->velocity.y = player->velocity.y * 0.5;
          else
            holdingJump = false;
        }

        if (keyup == SDLK_s || keyup == SDLK_DOWN) {
          player->crounching = false;
        }
        break;
      }
    }
  }

  bool walkPressed = false;
  const Uint8 *key = SDL_GetKeyboardState(NULL);

  if (!player->crounching && (key[SDL_SCANCODE_LEFT] || key[SDL_SCANCODE_A])) {
    player->facingRight = false;
    player->walking = true;
    walkPressed = true;

    if (player->velocity.x > 0)
      player->velocity.x *= FRIC;
    if (player->velocity.x > -MAX_SPEED)
      player->velocity.x -= SPEED;
  } else if (!player->crounching &&
             (key[SDL_SCANCODE_RIGHT] || key[SDL_SCANCODE_D])) {
    player->facingRight = true;
    player->walking = true;
    walkPressed = true;

    if (player->velocity.x < 0)
      player->velocity.x *= FRIC;
    if (player->velocity.x < MAX_SPEED)
      player->velocity.x += SPEED;
  } else {
    if (player->velocity.x) {
      player->velocity.x *= FRIC;

      if (fabsf(player->velocity.x) < 0.1f)
        player->velocity.x = 0;
    } else
      player->walking = false;
    walkPressed = false;
  }

  if (!player->velocity.y && !walkPressed && player->tall &&
      (key[SDL_SCANCODE_DOWN] || key[SDL_SCANCODE_S])) {
    player->crounching = true;
  }

  if (player->onSurface && !holdingJump && key[SDL_SCANCODE_UP]) {
    player->velocity.y = MAX_JUMP * 1.25;
    player->jumping = true;
    holdingJump = true;
  }

  // NOTES: TEMPORARY CEILING AND LEFT WALL
  if (player->rect.y < 0)
    player->rect.y = 0;
  if (player->rect.x < 0)
    player->rect.x = 0;
}
