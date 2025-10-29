#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <stdbool.h>

// NOTE: All of these are resolution related
// TODO: Make these macros global variables initialized
// in init.c when resolution change is implemented
#define GRAVITY 0.8f
#define MAX_GRAVITY 20
#define SPEED 0.2f
#define MAX_SPEED 7
#define JUMP_FORCE 2.5f
#define MAX_JUMP -15
#define ITEM_SPEED (SPEED * 12)
#define ITEM_JUMP_FORCE (JUMP_FORCE * 6)
#define FRIC 0.85f

// The maximum ammount of pieces a block can break into
#define MAX_BLOCK_PARTICLES 4
#define MAX_FIREBALLS 3

typedef unsigned short ushort;

typedef enum { COINS, MUSHROOM, FIRE_FLOWER, STAR } ItemType;
typedef enum { NOTHING, FULL, EMPTY } BlockState;
// TODO: Nest this inside of Block if possible
typedef enum {
  SHINY_SPRITE,
  BRICK_SPRITE,
  EMPTY_SPRITE,
  INTERROGATION_SPRITE
} BlockSprite;

// TODO: Add a turning frame
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
  TALL_CROUNCHING,
  STAR_TALL,
  FIRE_STILL = 35 + 7 * 3,
  FIRE_WALK,
  FIRE_TURNING = 60,
  FIRE_JUMP,
  FIRE_CROUNCHING,
  FIRE_FIRING,
  SMALL_TO_TALL = 75,
  SMALL_TO_FIRE = 78
} PlayerFrame;

typedef struct {
  float x, y;
} Velocity;

typedef struct {
  SDL_FRect rect;
  Velocity velocity;
  bool visible;
} Fireball;

typedef struct {
  SDL_FRect rect, hitbox;
  Velocity velocity;
  // TODO: Remove a lot of these
  bool tall, fireForm, invincible, transforming, onSurface, jumping,
    facingRight, walking, crounching, firing;
  PlayerFrame frame;
  Fireball fireballs[MAX_FIREBALLS];
} Player;

typedef struct {
  SDL_FRect rect;
  Velocity velocity;
  bool free, visible, canJump;
  ItemType type;
} Item;

typedef struct {
  SDL_FRect rect;
  bool onAir, willFall;
} Coin;

typedef struct {
  SDL_FRect rect;
  struct Particle {
    SDL_FRect rect;
    Velocity velocity;
  } particles[MAX_BLOCK_PARTICLES];
  // TODO: Remove this
  float initY;
  bool gotHit, broken;
  BlockState type;
  BlockSprite sprite;
  Item item;
  // Maybe implement a linked list instead of a array
  Coin coins[10];
  // TODO: Merge these two
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
  // When reading levels, make it a dynamic array
  Block blocks[20];
  SDL_FRect objs[20];
  // When making multiple Levels, move this to Level
  uint objsLength, blocksLenght;
  Sheets sheets;
  Screen screen;
  Player player;
} GameState;

#endif
