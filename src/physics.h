/*
 * physics.h – Ballphysik, Flipper und einfacher Spielablauf
 */
#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include "fixmath.h"

/*
 * Einstellbare Werte (pro Substep bei 8 Substeps). Sie lassen sich im
 * laufenden Spiel mit F1/F3/F5/F7 aendern, siehe tune_* unten.
 * Startwerte aus dem Hardwaretest vom 24.10.2026: FLIPPER SPEED 1100,
 * WALL BOUNCE 80.
 */
#define SUBSTEPS     8
enum { T_GRAVITY, T_FLIP_SPEED, T_WALL_BOUNCE, T_FLIP_BOUNCE, T_BUMPER_KICK, T_SLING_KICK, T_COUNT };
extern int32_t tune[T_COUNT];
/* Abgeleitete Werte, nur bei Aenderung neu berechnet (tune_apply) */
extern int32_t p_gravity, p_rest_v, p_wall_imp, p_flip_imp, p_bumper_kick, p_sling_kick;
extern int16_t p_flip_up, p_flip_down;
#define GRAVITY      p_gravity
#define REST_V       p_rest_v
#define FLIP_SPEED   p_flip_up
#define FLIP_DOWN    p_flip_down
#define BUMPER_KICK  p_bumper_kick
#define SLING_KICK   p_sling_kick

typedef struct {
  const char *name;
  int32_t def, min, max, step;
} tune_info_t;
extern const tune_info_t TUNE_INFO[T_COUNT];
void tune_reset(void);
void tune_apply(void);           /* nach Aenderung von tune[] */

#ifndef MAX_V
#define MAX_V        229376L     /* 3.5 px pro Substep */
#endif
#ifndef LAUNCH_MIN
#define LAUNCH_MIN   72090L
#endif
#ifndef LAUNCH_MAX
#define LAUNCH_MAX   190054L
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
  uint8_t left, right, plunger, test, reset, diag;
  uint8_t tune_toggle, tune_next, tune_down, tune_up;   /* F1, F3, F5, F7 */
} input_t;

extern ball_t ball;
#define N_FLIP 3                 /* links unten, rechts unten, rechts oben */
extern flipper_t flip[N_FLIP];
/* Ereignisse des letzten Bildes fuer die Spielregeln (game.c), Bit = Nummer */
extern uint8_t ev_bumper, ev_sling;
extern uint16_t ev_target;
/* Meilenstein 4: Rampe und Kick-Out-Hole */
#define EV_RAMP_DONE  1
#define EV_RAMP_BACK  2
#define EV_KICK_IN    1
#define EV_KICK_OUT   2
#define KICK_HOLD     75           /* Bilder im Loch (1,5 s) */
extern uint8_t ev_ramp, ev_kick;
extern uint8_t on_ramp;            /* Ball faehrt auf der Rampe (zweite Ebene) */
extern uint8_t kick_timer;         /* > 0: Ball liegt im Kick-Out-Hole */
extern fix16 ramp_s, ramp_v;
extern uint8_t state, pull;
extern uint16_t bumper_hits, sling_hits, collisions;
extern uint8_t test_active;
extern uint16_t test_count;
extern uint8_t snap_valid;
extern uint8_t tune_on, tune_sel;
extern fix16 snap_x, snap_y;

void phys_init(void);
void in_lane_reset(void);        /* nur fuer Tests: Ball nicht in der Abschussbahn */
void game_frame(const input_t *in);
uint8_t flip_dir(const flipper_t *f);

#endif
