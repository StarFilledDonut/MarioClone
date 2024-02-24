#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>

#define GRAVITY 0.8f

typedef struct {
  float x, y;
  u_int w, h;
  float dx, dy; // Stands for delta x and delta y, terminal velocity.
  _Bool tall;
  u_short frame;
} Character;

typedef struct {
  u_int w, h, tile;
} Screen;

typedef struct {
  SDL_Texture *mario;
  SDL_Texture *objs;
} Sheets;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Rect blocks[100];
  Sheets sheets;
  Screen screen;
  Character player;
} GameState;

/* Destroy everything that was initialized from SDL then exit the program.
 * @param *state Your instance of GameState
 * @param __status The status shown after exting
*/
void quit(GameState *state, u_short __status) {
  Sheets *sheets = &state->sheets;
  if (sheets->mario) SDL_DestroyTexture(sheets->mario);
  if (state->renderer) SDL_DestroyRenderer(state->renderer);
  if (state->window) SDL_DestroyWindow(state->window);
  IMG_Quit();
  SDL_Quit();
  exit(__status);
}

// Remember to free the str after using it
// @return char * path + file
char *catpath(const char *path, const char *file) {
  char *result = malloc(strlen(path) + strlen(file) + 1);
  if (result == NULL) return NULL; // Search for "handling memory allocation failure"
  strcpy(result, path);
  strcat(result, file);
  return result;
}

// Takes care of all the events of the game.
void processEvents(GameState *state) {
  SDL_Event event;
  Character *player = &state->player;
  const u_short JUMP_FORCE = 15, MAX_SPEED = 7;
  const float SPEED = 0.2f, FRIC = 0.85f;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT | SDL_WINDOWEVENT_CLOSE:
        quit(state, 0);
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit(state, 0);
            break;
          case SDLK_f: // case temporaria
            if (player->frame < 6) player->frame++;
            else player->frame--;
            break;
          case SDLK_SPACE:
            if (player->frame != 5) player->frame = 5;
            break;
        }
    }
  }
  const Uint8 *key = SDL_GetKeyboardState(NULL);
  if (key[SDL_SCANCODE_LEFT]) {
    if (player->dx > MAX_SPEED * -1) player->dx -= SPEED;
  }
  else if (key[SDL_SCANCODE_RIGHT]) {
    if (player->dx < MAX_SPEED) player->dx += SPEED; 
  } else {
    /* OLD FRIC
    if (player->dx > 0) {
      player->dx -= SPEED;
      if (player->dx < 0) player->dx = 0;
    } else if (player->dx < 0) {
      player->dx += SPEED;
      if (player->dx > 0) player->dx = 0;
    }
    */ // NEW FRIC, maybe it does not suit the mario feel?
    if (player->dx) {
      player->dx *= FRIC;
      if (fabsf(player->dx) < 0.1f) player->dx = 0;
    }
  }
  if (key[SDL_SCANCODE_SPACE]) {
    if (!player->dy) player->dy -= JUMP_FORCE;
  }
}

void handleCollision(GameState *state) {
  SDL_Rect *blocks = state->blocks;
  const u_int length = sizeof(state->blocks) / sizeof(blocks[0]);
  Character *player = &state->player;
  for (u_int i = 0; i < length; i++) {
    SDL_Rect block = blocks[i];
    // Using AABB
    // This basiccaly meas if it is in collsion range
    if (
      (player->y <= block.y + block.h && player->y + player->h >= block.y)
      && (player->x <= block.x + block.w && player->x + player->w >= block.x)
    ) {
      printf("IN COLLISION!\n");
    }
  }
}

void physics(GameState *state) {
  Character *player = &state->player;
  const float GROUND = state->screen.h - state->screen.tile * 3;
  player->x += player->dx;
  player->y += player->dy;
  // This piece of code WILL be replaced
  if (player->y < GROUND) player->dy += GRAVITY;
  else if (player->dy) {
    player->frame = 0;
    player->dy = 0;
    player->y = GROUND;
  }
  handleCollision(state);
}

void initializeTextures(GameState *state) {
  Sheets *sheets = &state->sheets;
  const char *path = "../../../../game/assets/sprites/";
  const char *mode = "r";
  char *marioFile = catpath(path, "mario.png");
  char *objsFile = catpath(path, "objs.png");

  SDL_RWops *marioRW = SDL_RWFromFile(marioFile, mode);
  SDL_RWops *objsRW = SDL_RWFromFile(objsFile, mode);
  if (!marioRW || !objsRW) {
    printf("Could not load the sprites! SDL_Error: %s\n", SDL_GetError());
    quit(state, 1);
  }
  free(marioFile);
  free(objsFile);
  sheets->mario = IMG_LoadTextureTyped_RW(state->renderer, marioRW, 1, "PNG");
  sheets->objs = IMG_LoadTextureTyped_RW(state->renderer, objsRW, 1, "PNG");
  if (!sheets->mario || !sheets->objs) {
    printf("Could not place the sprites! SDL_Error: %s\n", SDL_GetError());
    quit(state, 1);
  }
  // SDL_RWFromFile()
  // IMG_LoadTextureTyped_RW()
}

void getsrcs(SDL_Rect srcs[], u_short frames) {
  u_int w = 16, h = 16, x = 16, y = 0;
  for (u_short i = 0; i < frames; i++) {
    srcs[i].x = i * x;
    srcs[i].y = y;
    srcs[i].w = w;
    srcs[i].h = h;
  }
}

// Renders to the screen
void render(GameState *state) {
  Character *player = &state->player;
  Sheets *sheets = &state->sheets;
  Screen *screen = &state->screen;
  u_int tile = screen->tile;
  initializeTextures(state);

  SDL_SetRenderDrawColor(state->renderer,  92,  148,  252,  255); // Draw to Blue
  SDL_RenderClear(state->renderer); // Clears to Blue

  SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
  SDL_RenderDrawLine(state->renderer, 0, screen->h, screen->w, screen->h);

  SDL_Rect srcsplayer[24];
  getsrcs(srcsplayer, 7); // Replace this with a more generic func getsrcs
  SDL_Rect dstplayer = { player->x, player->y, player->w, player->h };

  SDL_Rect srcsobjs[4];
  getsrcs(srcsobjs, 4);
  SDL_Rect dstobj = { screen->w / 2.0f - screen->tile, screen->h - tile * 4, tile, tile };
  state->blocks[0] = dstobj;

  SDL_Rect srcground = { 0, 16, 32, 32 };
  SDL_Rect dstground = { screen->w / 2.0f - tile, screen->h - tile * 2, tile * 2, tile * 2 };

  SDL_RenderCopy(state->renderer, sheets->mario, &srcsplayer[player->frame], &dstplayer);
  SDL_RenderCopy(state->renderer, sheets->objs, &srcsobjs[1], &dstobj);
  SDL_RenderCopy(state->renderer, sheets->objs, &srcground, &dstground);

  SDL_RenderPresent(state->renderer); // Presents the drawings made
}

// REPLACE THIS WITH A SPRITESHEET ANIMATION FRAMES FUNC
// This may still be used for static pictures though
int main(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not initialize SDL! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }
  if (IMG_Init(IMG_INIT_PNG) < 0) {
    printf("Could not initialize IMG! IMG_Error: %s\n", IMG_GetError);
    return 1;
  }
  GameState state;
  state.screen.w = 640; // For later screen resizing
  state.screen.h = 480;
  state.screen.tile = 64;

  SDL_Window *window = SDL_CreateWindow(
    "Mario copy",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    state.screen.w, state.screen.h,
    SDL_WINDOW_SHOWN
  );
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  state.window = window;
  
  SDL_Renderer *renderer = SDL_CreateRenderer(
    window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  if (!renderer) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    quit(&state, 1);
  }
  state.renderer = renderer;

  Character player;
  player.w = 64;
  player.h = 64;
  player.x = state.screen.w - player.w;
  player.y = state.screen.h - player.h;
  player.dx = 0.0f;
  player.dy = 0.0f;
  player.tall = 0;
  player.frame = 0;
  state.player = player;

  while (1) {
    processEvents(&state);
    physics(&state);
    render(&state);
  }
}
