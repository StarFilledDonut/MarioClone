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
#include <stdio.h>
#include <stdlib.h>

#define GRAVITY 0.8f

typedef struct {
  float x, y;
  u_int w, h;
  float dx, dy;
  _Bool tall;
  u_short frame;
} Character;

typedef struct {
  SDL_Rect rect;
  float y, initY;
  _Bool gotHit;
} Block;

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
  Block blocks[2];
  SDL_Rect objs[20];
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
  const float SPEED = 0.1f, FRIC = 0.85f;

  if (player->dy < 0) player->frame = 5;
  else if (!player->dy && player->frame == 5) player->frame = 0;

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
        }
    }
  }
  const Uint8 *key = SDL_GetKeyboardState(NULL);
  if (key[SDL_SCANCODE_LEFT] || key[SDL_SCANCODE_A]) {
    if (player->dx > 0) player->dx *= FRIC;
    if (player->dx > MAX_SPEED * -1) player->dx -= SPEED;
  }
  else if (key[SDL_SCANCODE_RIGHT] || key [SDL_SCANCODE_D]) {
    if (player->dx < 0) player->dx *= FRIC;
    if (player->dx < MAX_SPEED) player->dx += SPEED;
  } else {
    if (player->dx) {
      player->dx *= FRIC;
      if (fabsf(player->dx) < 0.1f) player->dx = 0;
    }
  }
  if (key[SDL_SCANCODE_SPACE] || key[SDL_SCANCODE_W] || key[SDL_SCANCODE_UP]) {
    if (!player->dy) player->dy -= JUMP_FORCE;
  }
}

void handleCollision(GameState *state, u_int length, float dx, float dy) {
  Character *player = &state->player;
  Block *blocks = state->blocks;
  const u_int pw = player->w, ph = player->h;
  float *px = &player->x, *py = &player->y;

  for (u_int i = 0; i < length; i++) {
    SDL_Rect block = blocks[i].rect;
    float bx = block.x, by = blocks[i].y;
    _Bool *gotHit = &blocks[i].gotHit;
    const u_int bw = block.w, bh = block.h;
    const _Bool collX = *px < bx + bw && *px + pw > bx;
    const _Bool collY = *py < by + bh && *py + ph > by;
    const float BLOCK_SPEED = 1.5f, initY = blocks[i].initY;

    if (collX && collY) {
      if (dx > 0) *px = bx - pw;
      else if (dx < 0) *px = bx + bw;
      else if (dy > 0) *py = by - ph;
      else if (dy < 0) {
        *py = by + bh;
        *gotHit = true;
      }
      if (dx) player->dx = 0;
      else player->dy = 0;
      printf("gotHit = %s\n", *gotHit ? "True" : "False");
    }
    if (*gotHit) {
      const float bjump = initY + bh - state->screen.tile / 4.0f;
      if (by + bh > bjump) blocks[i].y -= BLOCK_SPEED;
      else *gotHit = false;
    } else if (by != initY) blocks[i].y += BLOCK_SPEED;
  }
}

void physics(GameState *state) {
  // if (!(state->player.dy && state->player.dx)) return;
  Character *player = &state->player;
  const float GROUND = state->screen.h - state->screen.tile * 2;
  player->x += player->dx;
  const u_int blocksLength = 2;
  handleCollision(state, blocksLength, player->dx, 0);
  // TODO: Not run this handle collision when !player->dx
  // I must create a separate physiscs func for non player related things for this optimization to work.
  player->y += player->dy;
  handleCollision(state, blocksLength, 0, player->dy);
  // This piece of code WILL be replaced
  if (player->y + player->h < GROUND) player->dy += GRAVITY;
  else if (player->dy) {
    player->frame = 0;
    player->dy = 0;
    player->y = GROUND - player->h;
  }
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
}

void initializeObjs(GameState *state) {
  Block *blocks = state->blocks;
  Screen *screen = &state->screen;
  u_int tile = screen->tile;
  blocks[0].y = screen->h - tile * 5;
  blocks[0].initY = blocks[0].y;
  blocks[0].gotHit = false;

  blocks[1].y = screen->h - tile * 3;
  blocks[1].rect.w = tile;
  blocks[1].rect.h = tile;
  blocks[1].rect.x = tile;
  blocks[1].rect.y = blocks[1].y;
  blocks[1].initY = blocks[1].y;
  blocks[1].gotHit = false;
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
  Block *blocks = state->blocks;
  u_int tile = screen->tile;

  SDL_SetRenderDrawColor(state->renderer,  92,  148,  252,  255);
  SDL_RenderClear(state->renderer);

  SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
  SDL_RenderDrawLine(state->renderer, 0, screen->h, screen->w, screen->h);

  SDL_Rect srcsplayer[24];
  getsrcs(srcsplayer, 7); // Replace this with a more generic func getsrcs
  SDL_Rect dstplayer = { player->x, player->y, player->w, player->h };

  SDL_Rect srcsobjs[4];
  getsrcs(srcsobjs, 4); // TODO: Make it so getsrcs only runs on initialization

  // Only refresh the blocks who may be interacted with
  SDL_Rect dstblock = { screen->w / 2.0f - screen->tile, blocks[0].y, tile, tile };
  blocks[0].rect = dstblock;

  SDL_Rect srcground = { 0, 16, 32, 32 };
  SDL_Rect dstground = { 0, screen->h - tile * 2, tile * 2, tile * 2 };

  SDL_RenderCopy(state->renderer, sheets->mario, &srcsplayer[player->frame], &dstplayer);
  SDL_RenderCopy(state->renderer, sheets->objs, &srcsobjs[1], &blocks[0].rect);
  SDL_RenderCopy(state->renderer, sheets->objs, &srcsobjs[1], &blocks[1].rect);
  // Temporary ground
  for (u_short i = 0; i < 6; i++) {
    SDL_RenderCopy(state->renderer, sheets->objs, &srcground, &dstground);
    dstground.x += dstground.w;
  }

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
  player.x = state.screen.w / 2.0f - player.w;
  player.y = state.screen.h - player.h - state.screen.tile * 2;
  player.dx = 0;
  player.dy = 0;
  player.tall = false;
  player.frame = 0;
  state.player = player;
  initializeTextures(&state);
  initializeObjs(&state);

  while (true) {
    processEvents(&state);
    render(&state);
    physics(&state);
  }
}
