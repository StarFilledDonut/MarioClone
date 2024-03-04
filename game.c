#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define GRAVITY 0.8f

typedef enum { COINS, MUSHROOM, FIRE_FLOWER, STAR } ItemType;
typedef enum { NOTHING, FULL, EMPTY } BlockState;
typedef enum {
  SHINY_SPRITE, BRICK_SPRITE, EMPTY_SPRITE, INTERROGATION_SPRITE
} BlockSprite;

typedef struct {
  float x, y, dx, dy; 
  _Bool visible;
} Fireball;

typedef struct {
  float x, y, dx, dy;
  u_short w, h, frame, fireballLimit;
  _Bool tall, firePower, invincible, transforming, onSurface, holdingJump,
        onJump, facingRight, isSquatting;
  Fireball fireballs[3];
} Player;

typedef struct {
  float x, y, dx, dy;
  u_short w, h;
  ItemType type;
  _Bool isFree, isVisible;
} Item;

typedef struct {
  SDL_Rect rect;
  float y, initY, bitDx, bitDy, bitsX[4], bitsY[4];
  _Bool gotHit, gotDestroyed, bitFall;
  BlockState type;
  Item item;
  u_short sprite;
} Block;

typedef struct {
  uint w, h, timerStart;
  u_short tile, targetFps;
  float deltaTime;
  _Bool timerOn;
} Screen;

typedef struct {
  SDL_Texture *mario, *objs, *items, *effects;
  SDL_Rect srcmario[30], srcsobjs[4], srcitems[20], srceffects[20];
} Sheets;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  Block blocks[20];
  SDL_Rect objs[20];
  uint objsLength, blocksLenght;
  Sheets sheets;
  Screen screen;
  Player player;
} GameState;

/* Destroy everything that was initialized from SDL then exit the program.
 * @param *state Your instance of GameState
 * @param __status The status shown after exting
 */
void quit(GameState *state, u_short __status) {
  Sheets *sheets = &state->sheets;
  if (sheets->effects) SDL_DestroyTexture(sheets->effects);
  if (sheets->mario) SDL_DestroyTexture(sheets->mario);
  if (sheets->objs) SDL_DestroyTexture(sheets->objs);
  if (sheets->items) SDL_DestroyTexture(sheets->items);
  if (state->renderer) SDL_DestroyRenderer(state->renderer);
  if (state->window) SDL_DestroyWindow(state->window);
  IMG_Quit();
  SDL_Quit();
  exit(__status);
}

// Remember to free the str after using it, needs state in case of an allocation
// error
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

// Get the srcs of the specific frames of a spritesheet.
// The row is an index, which starts at 0.
void getsrcs(
  SDL_Rect srcs[],
  const u_short frames,
  const u_short startIndex,
  const char row,
  const float w,
  const float h,
  u_short x,
  u_short y
) {
  if (!frames) {
    printf("Invalid number of frames!\n");
    return;
  }
  const u_short tile = 16;
  if (!x) x = tile;
  if (!y && row) y = tile * (row - 1);
  u_short j = startIndex;
  for (u_short i = 0; i < frames; i++) {
    SDL_Rect *frame = &srcs[j];
    frame->x = frames != 1 ? x * i : x;
    frame->y = y;
    frame->w = tile * w;
    frame->h = tile * h;
    j++;
  }
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
    if (i == 0) sheets->mario = fileTexture;
    else if (i == 1) sheets->objs = fileTexture;
    else if (i == 2) sheets->items = fileTexture;
    else if (i == 3) sheets->effects = fileTexture;
    if (!fileTexture) {
      printf("Could not place the sprites! SDL_Error: %s\n", SDL_GetError());
      quit(state, 1);
    }
    free(filePath);
    filePath = NULL;
  }
  getsrcs(sheets->srcmario, 7, 0, 1, 1, 1, false, false); // Small Mario
  getsrcs(sheets->srcmario, 6, 7, 2, 1, 2, false, 0); // Tall Mario
  SDL_Rect *tallSquatting = &sheets->srcmario[13];
  tallSquatting->x = 16 * 6;
  tallSquatting->y = 16 + 16 / 2;
  tallSquatting->w = 16;
  tallSquatting->h = 16 + 16 / 2;
  getsrcs(sheets->srcmario, 6, 14, false, 1, 2, false, 32 * 2 + 16); // Fire Mario
  SDL_Rect *fireSquatting = &sheets->srcmario[20];
  fireSquatting->x = 16 * 6;
  fireSquatting->y = tallSquatting->y + 32 * 2;
  fireSquatting->w = 16;
  fireSquatting->h = 16 + 16 / 2;
  getsrcs(sheets->srcmario, 6, 21, false, 1, 2, false, 48); // Mid transformation
  getsrcs(sheets->srcsobjs, 4, 0, 1, 1, 1, false, false);
  getsrcs(sheets->srcitems, 10, 0, 1, 1, 1, false, false);
  getsrcs(sheets->srcitems, 4, 10, 3, 1, 0.5f, false, false); // Coin frames
  for (u_short i = 0; i < 4; i++) {
    if (i < 2)
      getsrcs(sheets->srceffects, 1, i, 3, 0.5f,
              0.5f, 8 * (4 + i), false);
    else
      getsrcs(sheets->srceffects, 1, i, false, 0.5f,
              0.5f, 8 * (4 + i - 2), 32 + 8);
  }
  getsrcs(sheets->srceffects, 4, 4, 1, 0.5f, 0.5f, 8, 8); // Fire ball
  getsrcs(sheets->srceffects, 4, 8, 2, 1, 1, false, false); // Fire explosion
}

// This function alone does not create interactive blocks, make sure
// to create a dstrect in the render function to work
void createBlock(GameState *state, int x, int y,
                 BlockState tBlock, ItemType tItem) {
  BlockSprite sprite;
  if (tBlock == NOTHING || tItem == COINS) sprite = BRICK_SPRITE;
  else sprite = INTERROGATION_SPRITE;
  u_short iw;
  if (tBlock != COINS) iw = state->screen.tile;
  else iw = state->screen.tile * 2;
  Block *block = &state->blocks[state->blocksLenght];
  block->rect.x = x;
  block->y = y;
  if (tBlock == NOTHING) {
    for (u_short i = 0; i < 8; i++) {
      float *bitX = &block->bitsX[i], *bitY = &block->bitsY[i];
      if (i == 0 || i == 2) *bitX = x;
      else if (i == 1 || i == 3) *bitX = x + state->screen.tile / 2.0f;
      if (i < 2) *bitY = y;
      else *bitY = y + state->screen.tile / 2.0f;
    }
    block->bitDy = 0;
    block->bitDx = 0;
    block->bitFall = false;
  }
  block->rect.y = y;
  block->rect.w = state->screen.tile;
  block->rect.h = state->screen.tile;
  block->initY = y;
  block->gotHit = false;
  block->gotDestroyed = false;
  block->type = tBlock;
  block->sprite = sprite;
  state->blocksLenght++;
  if (tBlock == NOTHING) return;
  block->item.x = x;
  block->item.y = y;
  block->item.w = iw;
  block->item.h = state->screen.tile;
  block->item.type = tItem;
  block->item.isFree = false;
  block->item.isVisible = true;
}

void initObjs(GameState *state) {
  Screen *screen = &state->screen;
  u_short tile = screen->tile;
  state->blocksLenght = 0;
  state->objsLength = 6;
  createBlock(state, tile, screen->h - tile * 3, NOTHING, false);
  createBlock(state, screen->w / 2 - tile * 2, screen->h - tile * 5,
              NOTHING, false);
  createBlock(state, screen->w / 2 - tile, screen->h - tile * 5,
              FULL, MUSHROOM);
  createBlock(state, screen->w / 2, screen->h - tile * 5, FULL, FIRE_FLOWER);
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
  state->screen.w = 640; // TODO: Screen resizing
  state->screen.h = 480;
  state->screen.tile = 64;
  state->screen.deltaTime = 0; // DeltaTime
  state->screen.targetFps = 60;
  state->screen.timerOn = false;
  state->screen.timerStart = 0;

  SDL_Window *window = SDL_CreateWindow(
      "Mario copy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      state->screen.w, state->screen.h, SDL_WINDOW_SHOWN);
  if (!window) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }
  state->window = window;

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    quit(state, 1);
  }
  state->renderer = renderer;

  Player player;
  player.w = state->screen.tile;
  player.h = state->screen.tile;
  player.x = state->screen.w / 2.0f - player.w;
  player.y = state->screen.h - player.h - state->screen.tile * 2;
  player.dx = 0;
  player.dy = 0;
  player.tall = false;
  player.firePower = false;
  player.invincible = false;
  player.transforming = false;
  player.facingRight = true;
  player.frame = 0;
  player.fireballLimit = 4;
  state->player = player;
  // NOTES: This is for testing
  if (player.tall || player.firePower) {
    player.h += state->screen.tile;
    player.y -= state->screen.tile;
  }
  initTextures(state);
  initObjs(state);
}

// Takes care of all the events of the game.
void handleEvents(GameState *state) {
  SDL_Event event;
  Player *player = &state->player;
  const short MAX_JUMP = -15, MAX_SPEED = 7;
  const float JUMP_FORCE = 2.5f, SPEED = 0.2f, FRIC = 0.85f;
  const u_short tile = state->screen.tile;

  if (player->onSurface) player->onJump = false;

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
            if (!player->firePower) break;
            u_short ballCount = 0, emptySlot;
            for (u_short i = 0; i < player->fireballLimit; i++) {
              if (player->fireballs[i].visible) ballCount++;
              else emptySlot = i;
            }
            if (ballCount < player->fireballLimit) {
              Fireball *ball = &player->fireballs[emptySlot];
              if (player->facingRight) {
                ball->x = player->x + player->w;
                ball->dx = MAX_SPEED;
              }
              else {
                ball->x = player->x;
                ball->dx = -MAX_SPEED;
              }
              ball->y = player->y;
              ball->dy = MAX_SPEED;
              ball->visible = true;
            }
            break;
          }
        }
      case SDL_KEYUP:
        if (event.key.repeat != 0) break;
        const SDL_Keycode keyup = event.key.keysym.sym;
        if (keyup == SDLK_SPACE || keyup == SDLK_w || keyup == SDLK_UP) {
          if (player->dy < 0)
            player->dy *= FRIC;
          player->holdingJump = false;
          player->onJump = false;
        }
        if (keyup == SDLK_s || keyup == SDLK_DOWN) {
          if (player->isSquatting)
            player->y -= (float) tile / 2;
          player->isSquatting = false;
        }
    }
  }
  _Bool walkPressed = false;
  // TODO: Implement deltaTime in here
  const Uint8 *key = SDL_GetKeyboardState(NULL);
  if (
    !player->isSquatting && (key[SDL_SCANCODE_LEFT] || key[SDL_SCANCODE_A])
  ) {
    player->facingRight = false;
    walkPressed = true;
    if (player->dx > 0) player->dx *= FRIC;
    if (player->dx > -MAX_SPEED) player->dx -= SPEED;
  } else if (
    !player->isSquatting && (key[SDL_SCANCODE_RIGHT] || key[SDL_SCANCODE_D])
  ) {
    player->facingRight = true;
    walkPressed = true;
    if (player->dx < 0) player->dx *= FRIC;
    if (player->dx < MAX_SPEED) player->dx += SPEED;
  } else {
    if (player->dx) {
      player->dx *= FRIC;
      if (fabsf(player->dx) < 0.1f) player->dx = 0;
    }
  }
  if (
    player->onSurface && !walkPressed && (player->tall || player->firePower) &&
    (key[SDL_SCANCODE_DOWN] || key[SDL_SCANCODE_S])
  ) {
    if (!player->isSquatting) player->y += (float) tile / 2;
    player->isSquatting = true;
  }
  if (
    ((!player->holdingJump && player->onSurface) ||
    (!player->onSurface && player->onJump)) &&
    (key[SDL_SCANCODE_SPACE] || key[SDL_SCANCODE_W] || key[SDL_SCANCODE_UP])
  ) {
    player->dy -= JUMP_FORCE;
    if (player->dy < MAX_JUMP) player->onJump = false;
    else player->onJump = true;
    player->holdingJump = true;
  }
  // Size handling
  if (!player->isSquatting && (player->tall || player->firePower))
    player->h = tile * 2;
  else if (player->isSquatting)
    player->h = tile + tile / 2;
  else player->h = tile;

  // NOTES: TEMPORARY CEILING AND LEFT WALL
  if (player->y < 0) player->y = 0;
  if (player->x < 0) player->x = 0;
}

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
    if (blocks[i].gotDestroyed) continue;
    else if ((
      blockType == NOTHING ||
      (blockType == EMPTY && itemType == COINS) ||
      (blockType == EMPTY && itemType == FIRE_FLOWER)) && (
      (bx + bw < 0 || bx > state->screen.w) ||
      (by + bh < 0 || by > state->screen.h)
    )) continue;

    const _Bool blockCollision =
        (*px < bx + bw && *px + pw > bx) &&
        (*py < by + bh && *py + ph > by);
    const float initY = blocks[i].initY;

    if (blockCollision) {
      if (dx > 0) *px = bx - pw;
      else if (dx < 0) *px = bx + bw;
      else if (dy > 0) {
        *py = by - ph;
        onBlock = true;
      } else if (dy < 0) {
        *py = by + bh;
        // TODO: Make this condition be done with the side he is oriented
        //       with, AKA make his fist collide with the bottom half of the
        //       block for it to get hit
        if (blockType != EMPTY) *gotHit = true;
        if (blockType == NOTHING && player->tall)
          blocks[i].gotDestroyed = true;
        // TODO: Add a mechanic to type == COINS
        //       to only become empty after 10 coins/hits
        if (blockType != NOTHING && itemType > COINS) {
          item->isFree = true;
          blocks[i].sprite = EMPTY_SPRITE;
        }
      }
      if (dx) player->dx = 0;
      else {
        player->dy = 0;
        player->onJump = false;
      }
    }
    if (*gotHit && (!player->tall && blockType == NOTHING)) {
      const float bjump = initY + bh - state->screen.tile / 4.0f;
      if (by + bh > bjump) blocks[i].y -= BLOCK_SPEED;
      else *gotHit = false;
    } else if (by != initY) blocks[i].y += BLOCK_SPEED;
    if (itemType > COINS && item->isFree) {
      if (item->y > initY - state->screen.tile) item->y -= BLOCK_SPEED;
      else blocks[i].type = EMPTY;
    }
    if (*isVisible && item->isFree) {
      const _Bool itemCollision =
          (*px < ix + iw && *px + pw > ix) &&
          (*py < iy + ih && *py + ph > iy);
      if (itemCollision) {
        *isVisible = false;
        if (itemType == MUSHROOM && !player->tall) {
          // TODO: When TALL_TO_FIRE is made uncomment this
          //       player->tall = true;
          player->y -= state->screen.tile;
          player->h += state->screen.tile;
          player->transforming = true;
        } else if (itemType == FIRE_FLOWER && !player->firePower) {
          if (!player->tall) {
            player->y -= state->screen.tile;
            player->h += state->screen.tile;
            // TODO: Move this when TALL_TO_FIRE is made
            player->transforming = true;
          }
          player->firePower = true;
        } else if (itemType == STAR) player->invincible = true;
      }
    }
  }
  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    const _Bool collision = (*px < obj.x + obj.w && *px + pw > obj.x) &&
                            (*py < obj.y + obj.h && *py + ph > obj.y);
    if ((obj.x + obj.w < 0 || obj.x > (int) state->screen.w) ||
        (obj.y + obj.h < 0 || obj.y > (int) state->screen.h)
    ) continue;

    if (collision) {
      if (dx > 0) *px = obj.x - pw;
      else if (dx < 0) *px = obj.x + obj.w;
      else if (dy > 0) {
        *py = obj.y - ph;
        onObj = true;
      } else if (dy < 0) *py = obj.y + obj.h;
      if (dx) player->dx = 0;
      else player->dy = 0;
    }
  }
  player->onSurface = onBlock || onObj;
}

// Takes care of the collision of the fireballs with everything.
// Except the player
void handleFireballColl(GameState *state, u_short index, float dx, float dy) {
  Fireball *ball = &state->player.fireballs[index];
  const u_short fs = state->screen.tile / 2;

  if (!ball->visible) return;
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
      if (dx > 0) ball->x = bx - fs;
      else if (dx < 0) ball->x = bx + bs;
      else if (dy > 0) ball->y = by - fs;
      else if (dy < 0) ball->y = by + bs;
      if (dx) ball->dx *= -1;
      else ball->dy *= -1;
    }
  }
  for (uint i = 0; i < state->objsLength; i++) {
    SDL_Rect obj = state->objs[i];
    if ((ball->x < obj.x + obj.w && ball->x + fs > obj.x) &&
        (ball->y < obj.y + obj.h && ball->y + fs > obj.y)) {
      if (dx > 0) ball->dx = obj.w - obj.w;
      else if (dx < 0) ball->dx = obj.w + obj.w;
      else if (dy > 0) ball->y = obj.y - fs;
      else if (dy < 0) ball->y = obj.y + obj.h;
      if (dx) ball->dx *= -1;
      else ball->dy *= -1;
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

// Handles animations and wich frames all moving parts of the game to be in.
void handlePlayerFrames(GameState *state) {
  Player *player = &state->player;
  const _Bool isSmall = !player->tall && !player->firePower;
  const uint tile = state->screen.tile;
  // TODO: Only do this calculation conditionally
  int animSpeed = fabsf(player->dx * 0.3f);
  if (!animSpeed) animSpeed = 1;
  const uint walkFrame = SDL_GetTicks() * animSpeed / 180 % 3;
  enum {
    STILL,
    WALK_1, WALK_2, WALK_3,
    TURNING, JUMP, DYING,
    TALL_STILL,
    TALL_WALK_1, TALL_WALK_2, TALL_WALK_3,
    TALL_TURNING, TALL_JUMP, TALL_SQUATTING,
    FIRE_STILL,
    FIRE_WALK_1, FIRE_WALK_2, FIRE_WALK_3,
    FIRE_TURNING, FIRE_JUMP, FIRE_SQUATTING,
    SMALL_TO_TALL_1, SMALL_TO_TALL_2, SMALL_TO_TALL_3,
    SMALL_TO_FIRE_1, SMALL_TO_FIRE_2, SMALL_TO_FIRE_3
  };

  if (player->transforming && !player->tall) {
    if (!state->screen.timerOn)
      state->screen.timerStart = SDL_GetTicks();
    state->screen.timerOn = true;
    const uint elapsedTime = SDL_GetTicks() - state->screen.timerStart;
    const uint xformFrame = elapsedTime / 180 % 3;
    int xformTo;
    if (!player->firePower) xformTo = SMALL_TO_TALL_1;
    else xformTo = SMALL_TO_FIRE_1;
    player->frame = xformFrame + xformTo;
    if (player->h < tile * 2) player->h = tile * 2;
    if (elapsedTime >= 2000) {
      player->transforming = false;
      // TODO: Make an animation for TALL_TO_FIRE
      player->tall = true;
      state->screen.timerOn = false;
    }
    else return;
  }

  if (isSmall) {
    if (player->holdingJump)
      player->frame = JUMP;
    else if (player->onSurface) {
      if (!player->dx) player->frame = STILL;
      else player->frame = walkFrame + WALK_1;
    }
  } else if (player->tall && !player->firePower) {
    if (player->holdingJump)
      player->frame = TALL_JUMP;
    else if (player->onSurface) {
      if (!player->dx && !player->isSquatting)
        player->frame = TALL_STILL;
      else if (player->isSquatting)
        player->frame = TALL_SQUATTING;
      else player->frame = walkFrame + TALL_WALK_1;
    }
  } else {
    if (player->holdingJump)
      player->frame = FIRE_JUMP;
    else if (player->onSurface) {
      if (!player->dx && !player->isSquatting)
        player->frame = FIRE_STILL;
      else if (player->isSquatting)
        player->frame = FIRE_SQUATTING;
      else player->frame = walkFrame + FIRE_WALK_1;
    }
  }
}

// If it is not free, then it will be static
u_short handleItemFrames(Item *item) {
  enum { FLOWER_FRAME = 2, STAR_FRAME = 7, COIN_FRAME = 11 };
  u_short itemFrame = item->isFree ? SDL_GetTicks() / 180 % 4 : 0;
  if (item->type == FIRE_FLOWER) return itemFrame + FLOWER_FRAME;
  else if (item->type == STAR) return itemFrame + STAR_FRAME;
  else return itemFrame + COIN_FRAME;
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

  SDL_Rect dstplayer = {player->x, player->y, player->w, player->h};

  SDL_Rect srcground = {0, 16, 32, 32};
  SDL_Rect dstground = {0, screen->h - tile * 2, tile * 2, tile * 2};

  SDL_RenderCopyEx(
    state->renderer, sheets->mario,
    &sheets->srcmario[player->frame], &dstplayer,
    0, NULL, player->facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL
  );
  // Rendering ground
  // NOTES: This method is very limitating, reform it later.
  //        This must be behind the block breaking bits
  for (uint i = 0; i < state->objsLength; i++) {
    state->objs[i] = dstground;
    SDL_RenderCopy(state->renderer, sheets->objs, &srcground, &dstground);
    dstground.x += dstground.w;
  }
  // Rendering blocks
  for (uint i = 0; i < state->blocksLenght; i++) {
    if (blocks[i].type != EMPTY) {
      SDL_Rect dstblock = {blocks[i].rect.x, blocks[i].y, tile, tile};
      blocks[i].rect = dstblock;
    }
    if (blocks[i].type != NOTHING && blocks[i].item.isVisible) {
      Item *item = &blocks[i].item;
      SDL_Rect dstitem = {item->x, item->y, item->w, item->h};
      u_short frame;

      if (item->type == MUSHROOM) frame = 0;
      else frame = handleItemFrames(item);
      SDL_RenderCopy(
        state->renderer,
        sheets->items,
        &sheets->srcitems[frame],
        &dstitem
      );
    }
    if (!blocks[i].gotDestroyed) {
      SDL_RenderCopy(
        state->renderer,
        sheets->objs,
        &sheets->srcsobjs[blocks[i].sprite],
        &blocks[i].rect
      );
    } else {
      u_short bitSize = screen->tile / 2;
      for (u_short j = 0; j < 4; j++) {
        float *bitDx = &blocks[i].bitDx, *bitDy = &blocks[i].bitDy,
              *bitX = &blocks[i].bitsX[j], *bitY = &blocks[i].bitsY[j];
        SDL_Rect bit = {*bitX, *bitY, bitSize, bitSize};

        if (blocks[i].bitsY[0] > (int) screen->h + 1) break;
        else if (bit.y > (int) screen->h + 1) continue;

        const float SPEED = 1.2f * screen->targetFps * screen->deltaTime;
        const u_short MAX_SPEED = 6,
                      LIMIT = blocks[i].initY - tile;

        if (*bitDy > -MAX_SPEED && !blocks[i].bitFall)
          *bitDy -= SPEED;
        else if (*bitY <= LIMIT) blocks[i].bitFall = true;
        if (*bitDx < MAX_SPEED && !blocks[i].bitFall)
          *bitDx += SPEED * screen->targetFps * screen->deltaTime;
        if (*bitDy < MAX_SPEED * 1.25f)
          *bitDy += GRAVITY * screen->targetFps * screen->deltaTime;
        *bitY += *bitDy;
        if (j < 2 && *bitDy < 0) *bitY += *bitDy;
        if (j == 1 || j == 3) *bitX += *bitDx * 0.5f;
        else *bitX -= *bitDx * 0.5f;
        SDL_RenderCopy(
          state->renderer,
          sheets->effects,
          &sheets->srceffects[j],
          &bit
        );
      }
    }
  }
  // TODO: Add a way of removing fireballs both from the screen and fom the array
  for (u_short i = 0; i < player->fireballLimit; i++) {
    Fireball *ball = &player->fireballs[i];
    if (!ball->visible) continue;
    const u_short fs = tile / 2;
    SDL_Rect fireballrect = {ball->x, ball->y, fs, fs};
    const u_short frame = SDL_GetTicks() / 180 % 4 + 4;
    SDL_RenderCopy(
      state->renderer,
      sheets->effects,
      &sheets->srceffects[frame],
      &fireballrect
    );
  }
  SDL_RenderPresent(state->renderer);
}

int main(void) {
  GameState state;
  initGame(&state);
  uint currentTime = SDL_GetTicks(), lastTime = 0;

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
// TODO: Have an delay on player events
// TODO: Have the bitsX[2] and bitsY[2] since there are only
//       two X and Y positions the 4 bits can be
