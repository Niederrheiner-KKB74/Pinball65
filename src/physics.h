/*
 * physics.h – Ballphysik, Flipper und einfacher Spielablauf (Meilenstein 1)
 */
#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include "fixmath.h"

/*
 * Einstellbare Werte (pro Substep bei 8 Substeps). Sie lassen sich im
 * laufenden Spiel mit F1/F3/F5/F7 aendern, siehe tune_* unten.
 * Startwerte nach dem ersten Hardwaretest: Flipper doppelt so schnell
 * (etwa 40 ms pro Schlag), Waende etwas weniger federnd.
 */
#define SUBSTEPS     8
enum { T_GRAVITY, T_FLIP_SPEED, T_WALL_BOUNCE, T_FLIP_BOUNCE, T_BUMPER_KICK, T_SLING_KICK, T_COUNT };
extern int32_t tune[T_COUNT];
#define GRAVITY      tune[T_GRAVITY]
#define FLIP_SPEED   ((int16_t)tune[T_FLIP_SPEED])
#define FLIP_DOWN    ((int16_t)(tune[T_FLIP_SPEED] * 3 / 5))
#define WALL_BOUNCE  tune[T_WALL_BOUNCE]
#define FLIP_BOUNCE  tune[T_FLIP_BOUNCE]
#define BUMPER_KICK  tune[T_BUMPER_KICK]
#define SLING_KICK   tune[T_SLING_KICK]

typedef struct {
  const char *name;
  int32_t def, min, max, step;
} tune_info_t;
extern const tune_info_t TUNE_INFO[T_COUNT];
void tune_reset(void);

#ifndef MAX_V
#define MAX_V        229376L     /* 3.5 px pro Substep */
#endif
#ifndef LAUNCH_MIN
#define LAUNCH_MIN   72090L
#endif
#ifndef LAUNCH_MAX
#define LAUNCH_MAX   190054L
#endif
#ifndef REST_V
#define REST_V       (GRAVITY * 5)
#endif
#ifndef PULL_MAX
#define PULL_MAX     40
#endif
#ifndef TEST_PULL
#define TEST_PULL    30
#endif
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
  uint8_t tune_toggle, tune_next, tune_down, tune_up;   /* F1, F3, F5, F7 */
} input_t;

extern ball_t ball;
extern flipper_t flip[2];
extern uint8_t state, pull;
extern uint16_t bumper_hits, sling_hits, collisions;
extern uint8_t test_active;
extern uint16_t test_count;
extern uint8_t snap_valid;
extern uint8_t tune_on, tune_sel;
extern fix16 snap_x, snap_y;

void phys_init(void);
void game_frame(const input_t *in);
uint8_t flip_dir(const flipper_t *f);

#endif
