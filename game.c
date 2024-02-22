#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>

typedef struct {
  int x, y;
  short life;
  char *name;
} Character;

typedef struct {
  Character player;
  SDL_Texture *playerSprite;
} GameState;

// Takes care of all the events of the game.
_Bool processEvents(const SDL_Window *window, GameState *state) {
  SDL_Event event;
  _Bool running = true;
  short movement = 10; // Maybe this will be part of Character

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT | SDL_WINDOWEVENT_CLOSE:
        running = false;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            running = false;
            break;
        }
    }
  }
  const Uint8 *key = SDL_GetKeyboardState(NULL);
  if (key[SDL_SCANCODE_LEFT]) state->player.x -= movement; 
  if (key[SDL_SCANCODE_RIGHT]) state->player.x += movement; 
  if (key[SDL_SCANCODE_UP]) state->player.x -= movement;
  if (key[SDL_SCANCODE_DOWN]) state->player.x += movement;

  return running;
}

// Renders to the screen
void render(SDL_Renderer *renderer, const GameState *state) {
  SDL_SetRenderDrawColor(renderer,  0,  0,  255,  255); // Draw to Blue
  SDL_RenderClear(renderer); // Clears to Blue

  SDL_Rect playerSprite = { state->player.x, state->player.y, 16, 16 };
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, state->playerSprite, NULL, &playerSprite);

  SDL_RenderPresent(renderer); // Presents the drawings made
}

// Pass NULL to what you don't want to destroy.
// Will do IMG_Quit() and SDL_Quit() independently.
void quit(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *playerSprite) {
  if (playerSprite) SDL_DestroyTexture(playerSprite);
  if (renderer) SDL_DestroyRenderer(renderer);
  if (window) SDL_DestroyWindow(window);
  IMG_Quit();
  SDL_Quit();
}

int main(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not initialize SDL! SDL_Error: %s\n", SDL_GetError());
    return 0;
  }

  SDL_Window *window = SDL_CreateWindow(
    "My first game",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    640,  480,
    SDL_WINDOW_SHOWN
  );
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 0;
  }
  
  SDL_Renderer *renderer = SDL_CreateRenderer(
    window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  if (!renderer) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    quit(window, NULL, NULL);
    return 0;
  }
  SDL_RWops *playerSpriteRW = SDL_RWFromFile("./assets/sprites/player/Mario_still.png", "rw");
  if (!playerSpriteRW) {
    printf("Could not load the sprites! SDL_Error: %s\n", SDL_GetError());
    quit(window, renderer, NULL);
    return 0;
  }

  SDL_Texture *playerSprite = IMG_LoadTextureTyped_RW(renderer, playerSpriteRW, 1, "PNG");
  if (!playerSprite) {
    printf("Could not place the sprites! SDL_Error: %s\n", SDL_GetError());
    quit(window, renderer, NULL);
    return 0;
  }

  Character player = { 340 - 16, 480 - 16, 5, "Mario" };

  GameState state;
  state.playerSprite = playerSprite;
  state.player = player;

  _Bool running = true;

  while (running) {
    // Event handling
    if (!processEvents(window, &state)) running = false;
    // Rendering
    render(renderer, &state);
  }

  quit(window, renderer, playerSprite);
  return 1;
}
