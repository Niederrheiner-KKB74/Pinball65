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
 * WALL BOUNCE 80. v7.9: GRAVITY 260, FLIPPER SPEED 900 (Spiel war zu schnell).
 */
#define SUBSTEPS     8
enum { T_GRAVITY, T_FLIP_SPEED, T_WALL_BOUNCE, T_FLIP_BOUNCE, T_BUMPER_KICK, T_SLING_KICK, T_COUNT };
/* v6.9: Die Einstellung im Spiel (F1/F3/F5/F7) gibt es nur noch mit -DTUNE_UI und
 * in den Tests; das Spiel rechnet mit festen Werten (spart Speicher fuer die Musik). */
#if defined(PB_BUILDER) || defined(TUNE_UI)
#define PHYS_TUNE 1
#endif
#ifndef PHYS_TUNE
#define p_gravity     260L        /* v7.9: 340 -> 260, Kugel langsamer */
#define p_rest_v      (260L * 5)
#define p_flip_up     900         /* v7.9: 1100 -> 900, Flipper beschleunigen weniger */
#define p_flip_down   540
#define p_wall_imp    (65536L + 80L * 256)
#define p_flip_imp    (65536L + 72L * 256)
#define p_bumper_kick 90000L
#define p_sling_kick  70000L
#define tune_on       0
static inline void tune_reset(void) {}
#else
extern int32_t tune[T_COUNT];
/* Abgeleitete Werte, nur bei Aenderung neu berechnet (tune_apply) */
extern int32_t p_gravity, p_rest_v, p_wall_imp, p_flip_imp, p_bumper_kick, p_sling_kick;
extern int16_t p_flip_up, p_flip_down;
#endif
#define GRAVITY      p_gravity
#define REST_V       p_rest_v
#define FLIP_SPEED   p_flip_up
#define FLIP_DOWN    p_flip_down
#define BUMPER_KICK  p_bumper_kick
#define SLING_KICK   p_sling_kick

#ifdef PHYS_TUNE
typedef struct {
  const char *name;
  int32_t def, min, max, step;
} tune_info_t;
extern const tune_info_t TUNE_INFO[T_COUNT];
void tune_reset(void);
void tune_apply(void);           /* nach Aenderung von tune[] */
#endif

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
  uint8_t left, right, plunger, test, reset, diag, mb, mute, help;   /* help: H (v7.7) */   /* mb: RUN/STOP startet Multiball (Test), mute: M */
  uint8_t tune_toggle, tune_next, tune_down, tune_up;   /* F1, F3, F5, F7 */
} input_t;

/* Zustand eines Balls; pb ist der aktuelle, ball2 der zweite im Multiball */
typedef struct { ball_t b; fix16 rs, rv; uint8_t onr, rseg, kt, kc, inl; } bctx_t;
extern bctx_t pb;
#define ball       (pb.b)
#define on_ramp    (pb.onr)            /* Ball faehrt auf der Rampe (zweite Ebene) */
#define kick_timer (pb.kt)             /* > 0: Ball liegt im Kick-Out-Hole */
#define ramp_s     (pb.rs)
#define ramp_v     (pb.rv)
#define N_FLIP 3                 /* links unten, rechts unten, rechts oben */
extern flipper_t flip[N_FLIP];
/* Ereignisse des letzten Bildes fuer die Spielregeln (game.c), Bit = Nummer */
extern uint8_t ev_bumper, ev_sling;
extern uint16_t ev_target;
/* Meilenstein 4: Rampe und Kick-Out-Hole */
#define EV_RAMP_DONE  1
#define EV_RAMP_BACK  2
#define EV_RAMP_ENTER 4           /* v7.7: Ball faehrt auf die Rampe */
#define RAMP_ASSIST_X 128         /* v8.2: Einfahrhilfe: Mitte, halbe Breite, Mindesttempo nach oben, Staerke */
#define RAMP_ASSIST_W 30
#define RAMP_ASSIST_V 65536L
#ifndef RAMP_ASSIST_K
#define RAMP_ASSIST_K 0x80
#endif
#define EV_KICK_IN    1
#define EV_KICK_OUT   2
#define KICK_HOLD     75           /* Bilder im Loch (1,5 s) */
extern uint8_t ev_ramp, ev_kick;
extern uint8_t state, pull;
extern uint16_t bumper_hits, sling_hits, collisions;
#ifdef PB_BUILDER
extern uint8_t test_active;
extern uint16_t test_count;
extern uint8_t snap_valid;
extern fix16 snap_x, snap_y;
#else
#define test_active 0
#define snap_valid  0
#define snap_x      0L
#define snap_y      0L
#endif
#ifdef PHYS_TUNE
extern uint8_t tune_on, tune_sel;
#endif

/* Multiball (v6.4): ein zweiter Ball. Sein Zustand liegt in ball2; phys_swap
 * tauscht ihn mit dem aktuellen Ball (ball, on_ramp, ...), damit derselbe Code
 * beide bewegt und auswertet. ball_idx sagt, welcher gerade "ball" ist. */
extern bctx_t ball2;
extern uint8_t multi, ball_idx;
extern uint8_t ev_saved;          /* Ball im Multiball zurueckgeholt (Ball-Save) */
#define MB_SAVE 500              /* 10 s nach dem Start */
void phys_swap(void);
void phys_multiball(void);       /* zweiten Ball aus der Abschussbahn starten */
uint8_t phys_gate_open(void);    /* v7.2: Tor oben in der Abschussbahn offen */

void phys_init(void);
void in_lane_reset(void);        /* nur fuer Tests: Ball nicht in der Abschussbahn */
void game_frame(const input_t *in);
uint8_t flip_dir(const flipper_t *f);

#endif
