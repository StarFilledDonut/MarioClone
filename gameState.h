#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <stdbool.h>

#define GRAVITY 0.8f

typedef enum { COINS, MUSHROOM, FIRE_FLOWER, STAR } ItemType;
typedef enum { NOTHING, FULL, EMPTY } BlockState;
typedef enum {
  SHINY_SPRITE,
  BRICK_SPRITE,
  EMPTY_SPRITE,
  INTERROGATION_SPRITE
} BlockSprite;


typedef struct {
  float dx, dy;
  _Bool visible;
  SDL_FRect rect;
} Fireball;

typedef struct {
  float dx, dy;
  u_short frame, fireballLimit;
  _Bool tall, fireForm, invincible, transforming, onSurface, holdingJump,
      onJump, gainingHeigth, facingRight, isWalking, isSquatting, isFiring;
  SDL_FRect rect;
  SDL_FRect hitbox;
  Fireball fireballs[3];
} Player;

typedef struct {
  float dx, dy;
  _Bool isFree, isVisible, canJump;
  ItemType type;
  SDL_FRect rect;
} Item;

typedef struct {
  _Bool onAir, willFall;
  SDL_FRect rect;
} Coin;

typedef struct {
  SDL_FRect rect;
  float initY, bitDx, bitDy, bitsX[4], bitsY[4];
  _Bool gotHit, gotDestroyed, bitFall;
  BlockState type;
  Coin coins[10];
  Item item;
  u_short sprite, maxCoins, coinCount;
} Block;

typedef struct {
  SDL_Texture *mario, *objs, *items, *effects;
  SDL_Rect srcmario[85], srcsobjs[4], srcitems[20], srceffects[20];
} Sheets;

typedef struct {
  uint w, h, xformTimer, starTimer, firingTimer;
  u_short tile, targetFps;
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
