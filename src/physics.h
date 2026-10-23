/*
 * physics.h – Ballphysik, Flipper und einfacher Spielablauf (Meilenstein 1)
 */
#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include "fixmath.h"

/* Werte aus dem Prototyp (pro Substep bei 8 Substeps) */
#define SUBSTEPS     8
#define GRAVITY      340L
#define FLIP_SPEED   360
#define FLIP_DOWN    216
#define WALL_BOUNCE  128
#define FLIP_BOUNCE  72
#define BUMPER_KICK  90000L
#define SLING_KICK   70000L
#define MAX_V        229376L     /* 3.5 px pro Substep */
#define LAUNCH_MIN   72090L
#define LAUNCH_MAX   190054L
#define REST_V       (GRAVITY * 5)
#define PULL_MAX     40
#define TEST_PULL    30
#define TEST_FRAMES  120

enum { ST_PLUNGER, ST_PLAY, ST_DRAIN };

typedef struct { fix16 x, y, vx, vy; } ball_t;
typedef struct {
  int16_t px, py;
  int8_t side;          /* +1 links, -1 rechts */
  int16_t ang;          /* Winkel (Q8) in "linker" Orientierung */
  int16_t w;            /* Winkelaenderung im letzten Substep */
  uint8_t pressed;
} flipper_t;

typedef struct {
  uint8_t left, right, plunger, test, reset;
} input_t;

extern ball_t ball;
extern flipper_t flip[2];
extern uint8_t state, pull;
extern uint16_t bumper_hits, sling_hits, collisions;
extern uint8_t test_active;
extern uint16_t test_count;
extern uint8_t snap_valid;
extern fix16 snap_x, snap_y;

void phys_init(void);
void game_frame(const input_t *in);
uint8_t flip_dir(const flipper_t *f);

#endif
