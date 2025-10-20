#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <stdbool.h>

#define GRAVITY 0.8f

typedef unsigned short ushort;

typedef enum { COINS, MUSHROOM, FIRE_FLOWER, STAR } ItemType;
typedef enum { NOTHING, FULL, EMPTY } BlockState;
typedef enum {
  SHINY_SPRITE,
  BRICK_SPRITE,
  EMPTY_SPRITE,
  INTERROGATION_SPRITE
} BlockSprite;

typedef enum {
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
  STAR_TALL,
  FIRE_STILL = 35 + 7 * 3,
  FIRE_WALK,
  FIRE_TURNING = 60,
  FIRE_JUMP,
  FIRE_SQUATTING,
  FIRE_FIRING,
  SMALL_TO_TALL = 75,
  SMALL_TO_FIRE = 78
} PlayerFrame;

typedef struct {
  float dx, dy;
  bool visible;
  SDL_FRect rect;
} Fireball;

typedef struct {
  float dx, dy;
  ushort fireballLimit;
  bool tall, fireForm, invincible, transforming, onSurface, holdingJump,
      jumping, gainingHeigth, facingRight, walking, squatting, firing;
  SDL_FRect rect, hitbox;
  Fireball fireballs[3];
  PlayerFrame frame;
} Player;

typedef struct {
  float dx, dy;
  bool free, visible, canJump;
  ItemType type;
  SDL_FRect rect;
} Item;

typedef struct {
  bool onAir, willFall;
  SDL_FRect rect;
} Coin;

typedef struct {
  SDL_FRect rect, bits[4];
  float initY, bitDx, bitDy, bitsX[4], bitsY[4];
  bool gotHit, broken, bitFall;
  BlockState type;
  BlockSprite sprite;
  Item item;
  Coin coins[10];
  ushort maxCoins, coinCount;
} Block;

typedef struct {
  SDL_Texture *mario, *objs, *items, *effects;
  SDL_Rect srcmario[85], srcsobjs[4], srcitems[20], srceffects[20];
} Sheets;

typedef struct {
  uint w, h, xformTimer, starTimer, firingTimer;
  ushort tile, targetFps;
  float deltaTime;
} Screen;

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

#endif
