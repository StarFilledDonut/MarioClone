#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rect.h>
#include "gameState.h"
#include "utils.h"

// Create a block in state.blocks
void createBlock(
  GameState *state, const int x, const int y, const BlockState tBlock, const ItemType tItem) {
  BlockSprite sprite;
  if (tBlock == NOTHING || tItem == COINS)
    sprite = BRICK_SPRITE;
  else
    sprite = INTERROGATION_SPRITE;

  ushort iw = state->screen.tile;
  if (tItem == COINS)
    iw /= 2;

  Block *block = &state->blocks[state->blocksLenght];
  block->rect = (SDL_FRect) {x, y, state->screen.tile, state->screen.tile};
  block->initY = y;
  if (tBlock == NOTHING) {
    for (ushort i = 0; i < 8; i++) {
      float *bitX = &block->bitsX[i], *bitY = &block->bitsY[i];
      if (i == 0 || i == 2)
        *bitX = x;
      else if (i == 1 || i == 3)
        *bitX = x + state->screen.tile / 2.0f;
      if (i < 2)
        *bitY = y;
      else
        *bitY = y + state->screen.tile / 2.0f;
    }
    block->bitDy = 0;
    block->bitDx = 0;
    block->bitFall = false;
  }
  block->gotHit = false;
  block->broken = false;
  block->type = tBlock;
  block->sprite = sprite;
  state->blocksLenght++;

  if (tBlock == NOTHING) {
    block->item = (Item){{0, 0}, false, false, false, 0, {0, 0, 0, 0}};
    return;
  }
  else if (tItem == COINS) {
    block->maxCoins = 10;
    block->coinCount = block->maxCoins;

    for (ushort i = 0; i < block->maxCoins; i++) {
      block->coins[i] = (Coin) {
        .rect = {x + iw / 2.0, y, iw, state->screen.tile},
        .onAir = false,
        .willFall = false,
      };
    }
  }

  block->item = (Item) {
    .velocity = {0, 0},
    .rect = {x, y, iw, state->screen.tile},
    .type = tItem,
    .free = false,
    .visible = true,
  };
}

void initObjs(GameState *state) {
  Screen *screen = &state->screen;
  ushort tile = screen->tile;

  state->blocksLenght = 0;
  state->objsLength = 6;

  createBlock(state, tile, screen->h - tile * 3, NOTHING, false);
  createBlock(
    state, screen->w / 2 - tile * 2, screen->h - tile * 5, NOTHING, false);
  createBlock(
    state, screen->w / 2 - tile, screen->h - tile * 5, FULL, MUSHROOM);
  createBlock(state, screen->w / 2, screen->h - tile * 5, FULL, FIRE_FLOWER);
  createBlock(state, screen->w / 2 + tile, screen->h - tile * 5, FULL, COINS);
  createBlock(
    state, screen->w / 2 + tile * 2, screen->h - tile * 5, FULL, STAR);
}

// Initialize the textures on the state.sheets, as well as the srcs.
void initTextures(GameState *state) {
  const char *path = "./assets/sprites/";
  Sheets *sheets = &state->sheets;
  char *files[] = {"mario.png", "objs.png", "items.png", "effects.png"};

  for (uint i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
    char *filePath = catpath(state, path, files[i]);
    SDL_RWops *fileRW = SDL_RWFromFile(filePath, "r");
    if (!fileRW) {
      printf("Could not load the sprites! SDL_Error: %s\n", SDL_GetError());
      quit(state, 1);
    }
    SDL_Texture *fileTexture =
      IMG_LoadTextureTyped_RW(state->renderer, fileRW, 1, "PNG");
    if (i == 0)
      sheets->mario = fileTexture;
    else if (i == 1)
      sheets->objs = fileTexture;
    else if (i == 2)
      sheets->items = fileTexture;
    else if (i == 3)
      sheets->effects = fileTexture;
    if (!fileTexture) {
      printf("Could not place the sprites! SDL_Error: %s\n", SDL_GetError());
      quit(state, 1);
    }
    free(filePath);
    filePath = NULL;
  }
  ushort marioFCount = 0, objsFCount = 0, itemsFCount = 0, effectsFCount = 0;

  // Small Mario
  getsrcs(sheets->srcmario, 7, &marioFCount, 1, 1, 1, false, false);
  getsrcs(sheets->srcmario, 7, &marioFCount, 2, 1, 1, false, false);
  getsrcs(sheets->srcmario, 7, &marioFCount, 3, 1, 1, false, false);
  getsrcs(sheets->srcmario, 7, &marioFCount, 4, 1, 1, false, false);
  // Tall Mario
  getsrcs(sheets->srcmario, 7, &marioFCount, 5, 1, 2, false, false);
  getsrcs(sheets->srcmario, 7, &marioFCount, 7, 1, 2, false, false);
  getsrcs(sheets->srcmario, 7, &marioFCount, 9, 1, 2, false, false);
  getsrcs(sheets->srcmario, 7, &marioFCount, 11, 1, 2, false, false);
  // Fire Mario
  getsrcs(sheets->srcmario, 7, &marioFCount, 15, 1, 2, false, false);
  getsrcs(sheets->srcmario, 3 * 4, &marioFCount, 17, 1, 2, false, false);
  // Mid transformation
  getsrcs(sheets->srcmario, 6, &marioFCount, 13, 1, 2, false, false);
  getsrcs(sheets->srcsobjs, 4, &objsFCount, 1, 1, 1, false, false);
  getsrcs(sheets->srcitems, 10, &itemsFCount, 1, 1, 1, false, false);

  // Coin frames
  getsrcs(sheets->srcitems, 4, &itemsFCount, 3, 0.5f, 1, 8, false);
  for (ushort i = 0; i < 4; i++) {
    if (i < 2)
      getsrcs(sheets->srceffects,
              1,
              &effectsFCount,
              3,
              0.5f,
              0.5f,
              8 * (4 + i),
              false);
    else
      getsrcs(sheets->srceffects,
              1,
              &effectsFCount,
              false,
              0.5,
              0.5,
              8 * (4 + i - 2),
              32 + 8);
  }
  getsrcs(sheets->srceffects,
          4,
          &effectsFCount,
          1,
          0.5,
          0.5,
          8,
          8); // Fire ball
  // Fire explosion
  getsrcs(sheets->srceffects, 4, &effectsFCount, 2, 1, 1, false, false);
}

void initGame(GameState *state) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    printf("Could not initialize SDL! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }
  if (IMG_Init(IMG_INIT_PNG) < 0) {
    printf("Could not initialize IMG! IMG_Error: %s\n", SDL_GetError());
    exit(1);
  }
  Screen screen = {.w = 640, // TODO: Screen resizing
                   .h = 480,
                   .tile = 64,
                   .deltaTime = 0, // DeltaTime
                   .targetFps = 60,
                   .xformTimer = 0,
                   .starTimer = 0,
                   .firingTimer = 0};
  state->screen = screen;

  SDL_Window *window = SDL_CreateWindow("Mario Bros Demo",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        screen.w,
                                        screen.h,
                                        SDL_WINDOW_SHOWN);
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }
  state->window = window;

  // TODO: Have option to choose fps limit instead of vsync
  SDL_Renderer *renderer = SDL_CreateRenderer(
    window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    quit(state, 1);
  }
  state->renderer = renderer;

  // TODO: Alter fixed position start later
  ushort tile = screen.tile;
  SDL_FRect prect = {screen.w / 2.0 - tile, screen.h - tile * 3, tile, tile};
  Player player = {
    .rect = prect,
    .hitbox = {prect.x + tile / 4.0, prect.y, tile / 2.0, prect.h},
    .velocity = {0, 0},
    .tall = false,
    .fireForm = false,
    .invincible = false,
    .transforming = false,
    .facingRight = true,
    .frame = 0,
    .fireballLimit = 4};
  state->player = player;

  if (player.tall || player.fireForm) {
    player.rect.h += tile;
    player.rect.y -= tile;
  }
  initTextures(state);
  initObjs(state);
}
