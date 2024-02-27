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
  float x, y, dx, dy;
  u_int w, h;
  _Bool tall, onSurface, holdingJump, onJump;
  u_short frame;
} Character;

typedef enum {
  NOTHING,
  COINS,
  MUSHROOM,
  FIRE_FLOWER,
  EMPTY
} ItemType;

typedef struct {
  float x, y, dx, dy; // NOTES: Do not make him come out of the block with dy, it will break the collision
  _Bool isFree;
} Item;

typedef struct {
  SDL_Rect rect;
  float y, initY;
  _Bool gotHit;
  ItemType type;
  Item item;
  u_int frame;
} Block;

typedef struct {
  u_int w, h, tile;
  float dt;
} Screen;

typedef struct {
  SDL_Texture *mario;
  SDL_Texture *objs;
  SDL_Texture *items;
  SDL_Texture *effects;
  SDL_Rect srcmario[10];
  SDL_Rect srcsobjs[4];
  SDL_Rect srcitems[10];
} Sheets;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  Block blocks[20];
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

// Remember to free the str after using it, needs state in case of an allocation error
// @return char * path + file
char *catpath(GameState *state, const char *path, const char *file) {
  char *result = malloc(strlen(path) + strlen(file) + 1);
  if (result == NULL) {
    printf("Could not allocate memory for the file path\n");
    quit(state, 1);
  }
  strcpy(result, path);
  strcat(result, file);
  return result;
}

// Get the srcs of the specific frames of a spritesheet
void getsrcs(SDL_Rect srcs[], u_short frames) {
  u_int w = 16, h = 16, x = 16, y = 0;
  for (u_short i = 0; i < frames; i++) {
    srcs[i].x = i * x;
    srcs[i].y = y;
    srcs[i].w = w;
    srcs[i].h = h;
  }
}

// Initialize the textures on the state.sheets, as well as the srcs.
void initTextures(GameState *state) {
  const char *path = "./assets/sprites/";
  Sheets *sheets = &state->sheets;
  char *files[] = {
    "mario.png",
    "objs.png",
    "items.png",
    "effects.png"
  };

  for (u_int i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
    char *filePath = catpath(state, path, files[i]);
    SDL_RWops *fileRW = SDL_RWFromFile(filePath, "r");
    if (!fileRW) {
      printf("Could not load the sprites! SDL_Error: %s\n", SDL_GetError());
      quit(state, 1);
    }
    SDL_Texture *fileTexture = IMG_LoadTextureTyped_RW(state->renderer, fileRW, 1, "PNG");
    if (i == 0) sheets->mario = fileTexture;
    else if (i == 1) sheets->objs = fileTexture;
    else if (i == 2) sheets->items = fileTexture;
    else if (i == 3) sheets->effects = fileTexture;
    if (!fileTexture) {
      printf("Could not place the sprites! SDL_Error: %s\n", SDL_GetError());
      quit(state, 1);
    }
    free(filePath);
  }
  getsrcs(state->sheets.srcmario, 7);
  getsrcs(state->sheets.srcsobjs, 4);
  getsrcs(state->sheets.srcitems, 6);
}

void initObjs(GameState *state) {
  Block *blocks = state->blocks;
  Screen *screen = &state->screen;
  u_int tile = screen->tile;
  blocks[0].y = screen->h - tile * 5;
  blocks[0].initY = blocks[0].y;
  blocks[0].gotHit = false;
  blocks[0].type = MUSHROOM;
  blocks[0].item.x = 0;
  blocks[0].item.y = blocks[0].initY;
  blocks[0].item.isFree = false;
  blocks[0].frame = 3;

  blocks[1].y = screen->h - tile * 3;
  blocks[1].rect.w = tile;
  blocks[1].rect.h = tile;
  blocks[1].rect.x = tile;
  blocks[1].rect.y = blocks[1].y;
  blocks[1].initY = blocks[1].y;
  blocks[1].gotHit = false;
  blocks[1].type = NOTHING;
}

void initGame(GameState *state) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not initialize SDL! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }
  if (IMG_Init(IMG_INIT_PNG) < 0) {
    printf("Could not initialize IMG! IMG_Error: %s\n", SDL_GetError());
    exit(1);
  }
  state->screen.w = 640; // For later screen resizing
  state->screen.h = 480;
  state->screen.tile = 64;
  state->screen.dt = 0; // DeltaTime

  SDL_Window *window = SDL_CreateWindow(
    "Mario copy",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    state->screen.w, state->screen.h,
    SDL_WINDOW_SHOWN
  );
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }
  state->window = window;
  
  SDL_Renderer *renderer = SDL_CreateRenderer(
    window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
  );
  if (!renderer) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    quit(state, 1);
  }
  state->renderer = renderer;

  Character player;
  player.w = 64;
  player.h = 64;
  player.x = state->screen.w / 2.0f - player.w;
  player.y = state->screen.h - player.h - state->screen.tile * 2;
  player.dx = 0;
  player.dy = 0;
  player.tall = false;
  player.onSurface = true;
  player.holdingJump = false;
  player.onJump = false;
  player.frame = 0;
  state->player = player;
  initTextures(state);
  initObjs(state);
}

// Takes care of all the events of the game.
void handleEvents(GameState *state) {
  SDL_Event event;
  Character *player = &state->player;
  const short MAX_JUMP = -15, MAX_SPEED = 7;
  const float JUMP_FORCE = 2.5f, SPEED = 0.2f, FRIC = 0.85f;

  if (player->dy < 0) player->frame = 5;
  else if (player->onSurface && player->frame == 5)
    player->frame = 0;
  if (player->onSurface) player->onJump = false;

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
        }
      case SDL_KEYUP:
        if (event.key.repeat == 0) {
          const SDL_Keycode keyup = event.key.keysym.sym;
          if (keyup == SDLK_SPACE || keyup == SDLK_w || keyup == SDLK_UP) {
            if (player->dy < 0) player->dy *= FRIC;
            player->holdingJump = false;
            player->onJump = false;
          }
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
  if (
    ((!player->holdingJump && player->onSurface)
    || (!player->onSurface && player->onJump)) && (
    key[SDL_SCANCODE_SPACE] || key[SDL_SCANCODE_W] || key[SDL_SCANCODE_UP]
  )) {
    player->dy -= JUMP_FORCE;
    if (player->dy < MAX_JUMP) player->onJump = false;
    else player->onJump = true;
    player->holdingJump = true;
  }
  // NOTES: TEMPORARY CEILING
  if (player->y < 0) player->y = 0;
}

// Handles the collision a axis per time, call this first with dx only,
// then call it again for the dy.
void handleCollision(
  float dx,
  float dy,
  GameState *state,
  const u_int blength,
  const u_int objslength
) {
  Character *player = &state->player;
  Block *blocks = state->blocks;
  const u_int pw = player->w, ph = player->h;
  _Bool onBlock, onObj;
  float *px = &player->x, *py = &player->y;

  for (u_int i = 0; i < blength; i++) {
    SDL_Rect block = blocks[i].rect;
    float bx = block.x, by = blocks[i].y;
    _Bool *gotHit = &blocks[i].gotHit;
    Item *item = &blocks[i].item;
    const u_int bw = block.w, bh = block.h;
    const _Bool collision = (*px < bx + bw && *px + pw > bx)
      && (*py < by + bh && *py + ph > by);
    const float BLOCK_SPEED = 1.5f, initY = blocks[i].initY;

    if (collision) {
      if (dx > 0) *px = bx - pw;
      else if (dx < 0) *px = bx + bw;
      else if (dy > 0) {
        *py = by - ph;
        onBlock = true;
      }
      else if (dy < 0) {
        const ItemType inside = blocks[i].type;
        *py = by + bh;
        if ((!player->tall && inside == NOTHING)
          || (inside != EMPTY && inside != NOTHING)) {
          *gotHit = true;
        }
        // TODO: Add a mechanic to type == COINS
        // to only become empty after 10 coins/hits
        if (inside != NOTHING) {
          item->isFree = true;
          blocks[i].type = EMPTY;
          blocks[i].frame = 2;
        }
      }
      if (dx) player->dx = 0;
      else player->dy = 0;
      break;
    } else onBlock = false;
    if (*gotHit) {
      const float bjump = initY + bh - state->screen.tile / 4.0f;
      if (by + bh > bjump) blocks[i].y -= BLOCK_SPEED;
      else *gotHit = false;
    } else if (by != initY) blocks[i].y += BLOCK_SPEED;
    if (item->isFree) {
      const float iy = initY - state->screen.tile;
      if (item->y > iy) item->y -= BLOCK_SPEED;
    }
  }
  for (u_int i = 0; i < objslength; i++) {
    SDL_Rect obj = state->objs[i];
    const _Bool collision = (*px < obj.x + obj.w && *px + pw > obj.x)
      && (*py < obj.y + obj.h && *py + ph > obj.y);

    if (collision) {
      if (dx > 0) *px = obj.x - pw;
      else if (dx < 0) *px = obj.x + obj.w;
      else if (dy > 0) {
        *py = obj.y - ph;
        onObj = true;
      }
      else if (dy < 0) *py = obj.y + obj.h;
      if (dx) player->dx = 0;
      else player->dy = 0;
      break;
    } else onObj = false;
  }
  player->onSurface = onBlock || onObj;
}

// Apply physics to the player, the objects, and the eneyms.
void physics(GameState *state) {
  Character *player = &state->player;
  const float *dt = &state->screen.dt;
  const float MAX_GRAVITY = 20,
  TARGET_FRAMERATE = 60,
  MAX_DELTA_TIME = 1/TARGET_FRAMERATE;
  const u_int blength = 2; // NOTES: Update these accordingly
  const u_int objsLength = 6; // NOTES: Update these accordingly

  if (*dt && *dt < MAX_DELTA_TIME)
    player->x += player->dx * TARGET_FRAMERATE * *dt;
  else
    player->x += player->dx * TARGET_FRAMERATE * MAX_DELTA_TIME;
  handleCollision(player->dx, 0, state, blength, objsLength);

  if (player->dy < MAX_GRAVITY && *dt && *dt < MAX_DELTA_TIME) {
    player->dy += GRAVITY * TARGET_FRAMERATE * *dt;
    player->y += player->dy * TARGET_FRAMERATE * *dt;
  } else {
    player->dy += GRAVITY * TARGET_FRAMERATE * MAX_DELTA_TIME;
    player->y += player->dy * TARGET_FRAMERATE * MAX_DELTA_TIME;
  }
  handleCollision(0, player->dy, state, blength, objsLength);

  // NOTES: Placeholder code below, prevent from falling into endeless pit
  if (player->y - player->h > state->screen.h) {
    player->y = (float) 0 - player->h;
    player->x = state->screen.h / 2.0f - player->w;
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

  SDL_Rect dstplayer = { player->x, player->y, player->w, player->h };
  SDL_Rect dstblock = { screen->w / 2.0f - screen->tile, blocks[0].y, tile, tile };
  blocks[0].rect = dstblock;
  const float itemY = blocks[0].item.isFree ? blocks[0].item.y : blocks[0].y;
  SDL_Rect dstitem = { blocks[0].item.x + dstblock.x, itemY, tile, tile };

  SDL_Rect srcground = { 0, 16, 32, 32 };
  SDL_Rect dstground = { 0, screen->h - tile * 2, tile * 2, tile * 2 };

  SDL_RenderCopy(state->renderer, sheets->mario, &sheets->srcmario[player->frame], &dstplayer);
  SDL_RenderCopy(state->renderer, sheets->items, &sheets->srcitems[0], &dstitem);
  SDL_RenderCopy(state->renderer, sheets->objs, &sheets->srcsobjs[blocks[0].frame], &blocks[0].rect);
  SDL_RenderCopy(state->renderer, sheets->objs, &sheets->srcsobjs[1], &blocks[1].rect);
  for (u_int i = 0; i < 6; i++) {
    state->objs[i] = dstground;
    SDL_RenderCopy(state->renderer, sheets->objs, &srcground, &dstground);
    dstground.x += dstground.w;
  }

  SDL_RenderPresent(state->renderer);
}

int main(void) {
  GameState state;
  initGame(&state);
  Uint32 currentTimeT = SDL_GetTicks(), lastTimeT = 0;

  while (true) {
    lastTimeT = currentTimeT;
    currentTimeT = SDL_GetTicks();
    state.screen.dt = (currentTimeT - lastTimeT) / 1000.0f;
    handleEvents(&state);
    physics(&state);
    render(&state);
  }
}
