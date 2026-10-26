/*
 * game.c – Spielregeln (Meilenstein 3)
 *
 * Punkte:  Bumper 100, Slingshot 10, Target 500 (schon an: 100),
 *          obere Lane 1000 (an: 200), L O A D 2000 (an: 500).
 * Gruppen: VIC-II 10.000, 6510 20.000, SID 10.000; danach gehen die Lampen aus.
 *          C 6 4 komplett: Bonus-Multiplikator +1 (bis 5X) und 5.000.
 *          L O A D komplett: 25.000.
 * Bonus:   jedes neu angeschaltete Target oder jede Lane 1.000, am Ballende
 *          mal Multiplikator.
 * Flipperknopf: Lampen der oberen Lanes wandern (Lane-Wechsel).
 * L O A D (v5.0): L und D sind Einzel-Targets, O und A die Inlanes.
 * Meilenstein 4:
 *          Rampe "FAST LOADER": 5.000, 10.000, ... bis 25.000 je Ball.
 *          Kick-Out-Hole "1541": haelt den Ball (Laufwerksgeraeusch), dann 3.000.
 *          Spinner in der Einfahrt: 100 je Umdrehung.
 */
#include "game.h"
#include "table.h"
#include "hw.h"
#include "dmd.h"
#include "sound.h"

uint32_t score;
uint8_t ball_no, mult, game_over;
uint16_t bonus;
uint8_t ramps;
static uint16_t spin_v, spin_ang;
static uint8_t spin_shown = 0xFF;
char msg1[DMD_COLS + 1], msg2[DMD_COLS + 1];
uint8_t msg_timer;

static uint16_t tgt_lit;
static uint8_t top_lit, load_lit;
static uint8_t tgt_cool[N_TARGETS];
static uint8_t flash[5];                      /* Bumper 0-2, Slingshots 3-4 */
static uint8_t prev_state, prev_l, prev_r, hold;
static int16_t prev_y;
static uint8_t lamp_shown[N_LAMPS];

/* Lampenfarben: aus, an */
static const uint8_t LAMP_RGB[N_LAMPS][2][3] = {
  {{34, 84, 108}, {80, 200, 255}}, {{34, 84, 108}, {80, 200, 255}}, {{34, 84, 108}, {80, 200, 255}},
  {{50, 100, 58}, {120, 240, 140}}, {{50, 100, 58}, {120, 240, 140}},
  {{50, 100, 58}, {120, 240, 140}}, {{50, 100, 58}, {120, 240, 140}},
  {{106, 50, 92}, {255, 120, 220}}, {{106, 50, 92}, {255, 120, 220}}, {{106, 50, 92}, {255, 120, 220}},
  {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}},
  {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}},
  {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}},
  {{200, 32, 46}, {255, 244, 210}}, {{200, 32, 46}, {255, 244, 210}}, {{200, 32, 46}, {255, 244, 210}},
  {{216, 48, 64}, {255, 236, 120}}, {{216, 48, 64}, {255, 236, 120}},
};
_Static_assert(N_LAMPS == 22, "Lampentabelle passt nicht zur Palette");

char *fmt_points(char *p, uint32_t v) {
  char tmp[12];
  char *e = dmd_num(tmp, v, 0);
  uint8_t n = (uint8_t)(e - tmp);
  for (uint8_t i = 0; i < n; i++) {
    *p++ = tmp[i];
    uint8_t rest = (uint8_t)(n - 1 - i);
    if (rest && rest % 3 == 0) *p++ = '.';
  }
  *p = 0;
  return p;
}

static void message(const char *a, uint32_t pts) {
  dmd_put(msg1, a);
  fmt_points(msg2, pts);
  msg_timer = 100;
}

static void add(uint16_t pts) { score += pts; }

static void lamps_apply(void) {
  for (uint8_t n = 0; n < N_LAMPS; n++) {
    uint8_t on;
    if (n < 10) on = (tgt_lit >> n) & 1;
    else if (n < 13) on = (top_lit >> (n - 10)) & 1;
    else if (n < 17) on = (load_lit >> (n - 13)) & 1;
    else on = flash[n - 17] != 0;
    if (on == lamp_shown[n]) continue;
    lamp_shown[n] = on;
    const uint8_t *c = LAMP_RGB[n][on];
    set_color((uint8_t)(C_LAMP0 + n), c[0], c[1], c[2]);
  }
}

/* Spinner: der helle Streifen wandert mit dem Drehwinkel */
static void spin_apply(void) {
  uint8_t ph = spin_v ? (uint8_t)(spin_ang >> 6) & 3 : 4;   /* 4 = steht, alle gleich */
  if (ph == spin_shown) return;
  spin_shown = ph;
  for (uint8_t k = 0; k < 4; k++) {
    uint8_t on = k == ph, mid = ph < 4 && ((k + 1) & 3) == ph;
    uint8_t v = on ? 255 : mid ? 160 : 70;
    set_color((uint8_t)(C_SPIN0 + k), v, v, (uint8_t)(v < 200 ? v + 30 : 255));
  }
}

void game_new(void) {
  score = 0;
  ball_no = 1;
  mult = 1;
  bonus = 0;
  game_over = 0;
  tgt_lit = 0;
  top_lit = load_lit = 0;
  ramps = 0;
  spin_v = 0;
  msg_timer = 0;
  prev_state = state;
  for (uint8_t n = 0; n < N_LAMPS; n++) lamp_shown[n] = 2;   /* alle Farben neu setzen */
  lamps_apply();
}

void game_input(input_t *in) {
  if (game_over && in->plunger && !hold) {
    game_new();
    snd_play(SND_START);
    hold = 1;
  }
  /* Nach Spielende und bis zum Loslassen nach dem Neustart kein Abschuss */
  if (hold && !in->plunger) hold = 0;
  if (game_over || hold) in->plunger = 0;
}

/* Ziel i getroffen */
static void low_lane(uint8_t i);
static void target_hit(uint8_t i) {
  if (i >= TGT_LD) { low_lane(i == TGT_LD ? 0 : 3); return; }   /* Targets L und D */
  static const uint16_t GROUP_MASK[3] = { 0x007, 0x078, 0x380 };
  static const char *GROUP_NAME[3] = { "VIC-II READY", "6510 READY", "SID READY" };
  static const uint16_t GROUP_PTS[3] = { 10000, 20000, 10000 };
  uint16_t bit = (uint16_t)1 << i;
  snd_play(SND_TARGET);
  if (tgt_lit & bit) { add(100); return; }
  tgt_lit |= bit;
  add(500);
  bonus++;
  for (uint8_t g = 0; g < 3; g++)
    if ((tgt_lit & GROUP_MASK[g]) == GROUP_MASK[g]) {
      tgt_lit &= (uint16_t)~GROUP_MASK[g];
      add(GROUP_PTS[g]);
      message(GROUP_NAME[g], GROUP_PTS[g]);
      snd_play(SND_COMPLETE);
    }
}

static void top_lane(uint8_t i) {
  uint8_t bit = (uint8_t)(1 << i);
  snd_play(SND_LANE);
  if (top_lit & bit) { add(200); return; }
  top_lit |= bit;
  add(1000);
  bonus++;
  if (top_lit == 7) {
    top_lit = 0;
    if (mult < 5) mult++;
    add(5000);
    char *p = dmd_put(msg1, "C64 BONUS ");
    p = dmd_num(p, mult, 0);
    dmd_put(p, "X");
    fmt_points(msg2, 5000);
    msg_timer = 100;
    snd_play(SND_COMPLETE);
  }
}

static void low_lane(uint8_t i) {
  uint8_t bit = (uint8_t)(1 << i);
  snd_play(SND_LANE);
  if (load_lit & bit) { add(500); return; }
  load_lit |= bit;
  add(2000);
  bonus++;
  if (load_lit == 15) {
    load_lit = 0;
    add(25000);
    message("PRESS PLAY ON TAPE", 25000);
    snd_play(SND_COMPLETE);
  }
}

void game_update(const input_t *in) {
  /* Lane-Wechsel und Flipperklang beim Druecken */
  if (in->left && !prev_l) {
    top_lit = (uint8_t)((top_lit >> 1) | ((top_lit & 1) << 2));
    snd_play(SND_FLIPPER);
  }
  if (in->right && !prev_r) {
    top_lit = (uint8_t)(((top_lit << 1) & 7) | (top_lit >> 2));
    snd_play(SND_FLIPPER);
  }
  prev_l = in->left;
  prev_r = in->right;

  int16_t x = (int16_t)(ball.x >> 16), y = (int16_t)(ball.y >> 16);
  if (state == ST_PLAY) {
    for (uint8_t i = 0; i < 3; i++)
      if (ev_bumper & (1 << i)) { add(100); flash[i] = 5; snd_play(SND_BUMPER); }
    for (uint8_t i = 0; i < 2; i++)
      if (ev_sling & (1 << i)) { add(10); flash[3 + i] = 5; snd_play(SND_SLING); }
    if (ev_target)
      for (uint8_t i = 0; i < N_TARGETS; i++)
        if ((ev_target & ((uint16_t)1 << i)) && !tgt_cool[i]) { tgt_cool[i] = 15; target_hit(i); }
    /* Spinner: Ball kreuzt ihn in der Einfahrt, Drehung nach Tempo */
    if ((prev_y < SPIN_Y) != (y < SPIN_Y) && x >= SPIN_X0 && x <= SPIN_X1) {
      int16_t d = y - prev_y;
      if (d < 0) d = -d;
      if (d > 40) d = 0;                     /* Sprung (neuer Ball, Rampe): kein Drehen */
      spin_v += (uint16_t)d * 8;
      if (spin_v > 700) spin_v = 700;
    }
    if (ev_ramp & EV_RAMP_DONE) {
      if (ramps < 5) ramps++;
      uint16_t pts = (uint16_t)ramps * 5000u;
      add(pts);
      bonus++;
      message("FAST LOADER", pts);
      snd_play(SND_RAMP);
    }
    if (ev_kick & EV_KICK_IN) {
      dmd_put(msg1, "SEARCHING FOR *");
      dmd_put(msg2, "LOADING");
      msg_timer = KICK_HOLD;
    }
    if (ev_kick & EV_KICK_OUT) {
      add(3000);
      bonus++;
      dmd_put(msg1, "READY.");
      fmt_points(msg2, 3000);
      msg_timer = 50;
      snd_play(SND_BUMPER);
    }
    if (kick_timer && (kick_timer & 7) == 0) snd_play(SND_DRIVE);
    /* Rollover: Ball ueberquert die Zaehllinie */
    if ((prev_y < TOPLANE_Y) != (y < TOPLANE_Y) && x > TOPLANE_X0 && x < TOPLANE_X0 + 72)
      top_lane(x < TOPLANE_X0 + 24 ? 0 : x < TOPLANE_X0 + 48 ? 1 : 2);
    if (prev_y < LOWLANE_Y && y >= LOWLANE_Y) {
      /* v5.0: O und A an den Inlanes; L und D sind Targets, die Out-Lanes zaehlen nicht */
      if (x >= 40 && x < 64) low_lane(1);
      else if (x > 236 && x <= 260) low_lane(2);
    }
  }
  prev_y = y;

  /* Ballende: Bonus; neuer Ball oder Spielende */
  if (state == ST_DRAIN && prev_state != ST_DRAIN) {
    uint32_t b = (uint32_t)bonus * 1000u * mult;
    score += b;
    char *p = dmd_put(msg1, "BONUS ");
    p = dmd_num(p, mult, 0);
    dmd_put(p, "X");
    fmt_points(msg2, b);
    msg_timer = 50;
    snd_play(SND_DRAIN);
  }
  if (state == ST_PLUNGER && prev_state == ST_DRAIN) {
    bonus = 0;
    ramps = 0;
    mult = 1;
    if (ball_no < BALLS_PER_GAME) ball_no++;
    else game_over = 1;
  }
  prev_state = state;

  for (uint8_t i = 0; i < N_TARGETS; i++) if (tgt_cool[i]) tgt_cool[i]--;
  for (uint8_t i = 0; i < 5; i++) if (flash[i]) flash[i]--;
  if (msg_timer) msg_timer--;
  /* Spinner dreht sich aus */
  if (spin_v) {
    uint16_t a = spin_ang + spin_v;
    uint8_t turns = (uint8_t)((a >> 8) - (spin_ang >> 8));
    if (turns) { add((uint16_t)turns * 100u); snd_play(SND_SPIN); }
    spin_ang = a;
    spin_v -= (spin_v >> 5) + 1;
  }
  spin_apply();
  lamps_apply();
  snd_update();
}
