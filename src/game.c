/*
 * game.c – Spielregeln (Meilenstein 3)
 *
 * Punkte:  Bumper 100, Slingshot 10, Target 500 (schon an: 100),
 *          obere Lane 1000 (an: 200), L O A D 2000 (an: 500).
 * Gruppen: VIC-II 10.000, 6510 20.000, SID 10.000, PLA 15.000 (v5.3); danach gehen die Lampen aus.
 *          C 6 4 komplett: Bonus-Multiplikator +1 (bis 5X) und 5.000.
 *          L O A D komplett: 25.000.
 * Bonus:   jedes neu angeschaltete Target oder jede Lane 1.000, am Ballende
 *          mal Multiplikator.
 * Flipperknopf: Lampen der oberen Lanes wandern (Lane-Wechsel).
 * L O A D: L und D sind Roll-Over-Kontakte auf dem Tisch (v5.1), O und A die Inlanes.
 * Meilenstein 4:
 *          Rampe "FAST LOADER": 5.000, 10.000, ... bis 25.000 je Ball.
 *          Kick-Out-Hole "1541": haelt den Ball (Laufwerksgeraeusch), dann 3.000.
 *          Spinner in der Einfahrt: 100 je Umdrehung.
 * Meilenstein 5, Missionen (v5.5): C 6 4 oder (seit v5.6) eine Target-Gruppe
 *          komplett macht MISSION READY (Ring des Kick-Out-Holes blinkt), der
 *          naechste Ball im Kick-Out-Hole startet die naechste Mission (60 s):
 *          FORMAT DISK (3x Kick-Out), SYS 64738 (3x Rampe), PEEK & POKE
 *          (Spinner, Kick-Out, Spinner, Kick-Out), LOAD ERROR (L O A D).
 *          Geschafft: 50.000 + 25.000 je bereits geschaffter Mission.
 * v6.4, Multiball "RUN/STOP": bereit, wenn VIC-II, 6510 und SID je einmal komplett
 *          waren (die Rampe blinkt); die naechste Rampe startet einen zweiten Ball aus
 *          der Abschussbahn. 10 s Ball-Save, 1541 = JACKPOT 50.000, Ende beim ersten
 *          verlorenen Ball. Taste RUN/STOP startet ihn zum Testen sofort.
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
uint16_t tgt_groups_done;       /* Zaehler fuer Tests */
uint8_t mission_ready, mission_active, mission_cur, mission_prog, missions_done;
uint16_t mission_timer;
static const char *const MIS_NAME[N_MISSIONS] = { "FORMAT DISK", "SYS 64738", "PEEK & POKE", "LOAD ERROR" };
static const char *const MIS_SHORT[N_MISSIONS] = { "FORMAT", "SYS", "PEEK", "LOAD" };
static const char *const MIS_GOAL[N_MISSIONS] = { "3X 1541", "3X RAMP", "SPIN 1541 SPIN 1541", "L O A D" };
static const uint8_t MIS_NEED[N_MISSIONS] = { 3, 3, 4, 4 };
enum { MS_KICK, MS_RAMP, MS_SPIN, MS_LOAD };
static uint16_t spin_v, spin_ang;
static uint8_t spin_shown = 0xFF;
char msg1[DMD_COLS + 1], msg2[DMD_COLS + 1];
uint8_t msg_timer, msg_type;           /* msg_type 1: Meldung wird getippt (v6.6) */
uint32_t hiscore;

static uint16_t tgt_lit;
static uint8_t top_lit, load_lit;
/* v6.1: Skill-Shot. Vor dem Abschuss blinkt eine der Lanes C 6 4 (mit den Flippern
 * verschiebbar); faellt der Ball nach dem Abschuss als erstes in diese Lane, gibt es
 * einen Bonus. skill_t: 255 = Ball in der Abschussbahn, sonst Restzeit in Bildern. */
uint8_t skill_lane;                     /* fuer den PC-Test sichtbar */
static uint8_t skill_t, blink_n;
static uint8_t tgt_cool[N_TARGETS];
static uint8_t flash[5];                      /* Bumper 0-2, Slingshots 3-4 */
static uint8_t prev_state, prev_l, prev_r, hold;
static int16_t prev_y[2];               /* je Ball (Multiball) */
static uint8_t roll_in[2];
/* Multiball "RUN/STOP" (v6.4): bereit, wenn VIC-II, 6510 und SID je einmal komplett
 * waren; die naechste Rampe startet ihn. Im Multiball gibt das 1541 den Jackpot. */
static uint8_t grp_mask;
uint8_t mb_ready;
static uint8_t lamp_shown[N_LAMPS];

/* Lampenfarben: aus, an */
static const uint8_t LAMP_RGB[N_LAMPS][2][3] = {
  {{34, 84, 108}, {80, 200, 255}}, {{34, 84, 108}, {80, 200, 255}}, {{34, 84, 108}, {80, 200, 255}},
  {{50, 100, 58}, {120, 240, 140}}, {{50, 100, 58}, {120, 240, 140}},
  {{50, 100, 58}, {120, 240, 140}}, {{50, 100, 58}, {120, 240, 140}},
  {{106, 50, 92}, {255, 120, 220}}, {{106, 50, 92}, {255, 120, 220}}, {{106, 50, 92}, {255, 120, 220}},
  {{100, 60, 20}, {255, 150, 40}}, {{100, 60, 20}, {255, 150, 40}}, {{100, 60, 20}, {255, 150, 40}},   /* PLA */
  {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}},
  {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}},
  {{100, 80, 24}, {255, 220, 80}}, {{100, 80, 24}, {255, 220, 80}},
  {{200, 32, 46}, {255, 244, 210}}, {{200, 32, 46}, {255, 244, 210}}, {{200, 32, 46}, {255, 244, 210}},
  {{216, 48, 64}, {255, 236, 120}}, {{216, 48, 64}, {255, 236, 120}},
};
_Static_assert(N_LAMPS == 25, "Lampentabelle passt nicht zur Palette");

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
  msg_type = 0;
}

static void add(uint16_t pts) { score += pts; }

static void lamps_apply(void) {
  for (uint8_t n = 0; n < N_LAMPS; n++) {
    uint8_t on;
    const uint8_t T = N_TARGETS;
    if (n < T) on = (tgt_lit >> n) & 1;
    else if (n < T + 3) on = (skill_t && n - T == skill_lane) ? (blink_n >> 3) & 1 : (top_lit >> (n - T)) & 1;
    else if (n < T + 7) on = (load_lit >> (n - T - 3)) & 1;
    else on = flash[n - T - 7] != 0;
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

char *mission_status(char *p) {
  p = dmd_put(p, MIS_SHORT[mission_cur]);
  *p++ = ' ';
  p = dmd_num(p, mission_prog, 0);
  *p++ = '/';
  p = dmd_num(p, MIS_NEED[mission_cur], 0);
  *p++ = ' ';
  return dmd_num(p, (mission_timer + 49) / 50, 0);
}

static void mission_end(uint8_t ok) {
  mission_active = 0;
  mission_ready = 0;
  mission_cur = (uint8_t)((mission_cur + 1) % N_MISSIONS);
  if (ok) {
    uint32_t pts = 50000UL + 25000UL * missions_done;
    missions_done++;
    score += pts;
    dmd_put(msg1, "MISSION COMPLETE");
    fmt_points(msg2, pts);
    msg_timer = 120;
    msg_type = 0;
    dmd_fx(FX_PULSE);
    snd_play(SND_COMPLETE);
  } else {
    dmd_put(msg1, "?TIME OUT  ERROR");
    dmd_put(msg2, "READY.");
    msg_timer = 100;
    msg_type = 1;
    snd_play(SND_DRAIN);
  }
}

/* Ereignis fuer die laufende Mission */
static void mission_event(uint8_t kind) {
  if (!mission_active) return;
  uint8_t m = mission_cur;
  if (m == 0 && kind == MS_KICK) mission_prog++;
  else if (m == 1 && kind == MS_RAMP) mission_prog++;
  else if (m == 2 && kind == ((mission_prog & 1) ? MS_KICK : MS_SPIN)) mission_prog++;
  else if (m == 3 && kind == MS_LOAD) {
    uint8_t n = 0;
    for (uint8_t b = load_lit; b; b >>= 1) n += b & 1;
    mission_prog = n;
  } else return;
  if (mission_prog >= MIS_NEED[m]) mission_end(1);
}

static void mission_start(void) {
  mission_active = 1;
  mission_prog = 0;
  mission_timer = MISSION_TIME;
  if (mission_cur == 3) load_lit = 0;      /* L O A D von vorn */
  dmd_put(msg1, MIS_NAME[mission_cur]);
  dmd_put(msg2, MIS_GOAL[mission_cur]);
  msg_timer = KICK_HOLD;
  msg_type = 1;
  snd_play(SND_START);
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
  mission_ready = mission_active = mission_cur = mission_prog = missions_done = 0;
  grp_mask = mb_ready = 0;
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
  static const uint16_t GROUP_MASK[4] = { 0x007, 0x078, 0x380, 0x1C00 };
  static const char *GROUP_NAME[4] = { "VIC-II READY", "6510 READY", "SID READY", "PLA READY" };
  static const uint16_t GROUP_PTS[4] = { 10000, 20000, 10000, 15000 };
  uint16_t bit = (uint16_t)1 << i;
  snd_play(SND_TARGET);
  if (tgt_lit & bit) { add(100); return; }
  tgt_lit |= bit;
  add(500);
  bonus++;
  for (uint8_t g = 0; g < 4; g++)
    if ((tgt_lit & GROUP_MASK[g]) == GROUP_MASK[g]) {
      tgt_lit &= (uint16_t)~GROUP_MASK[g];
      tgt_groups_done++;
      if (g < 3) grp_mask |= (uint8_t)(1 << g);
      add(GROUP_PTS[g]);
      message(GROUP_NAME[g], GROUP_PTS[g]);
      snd_play(SND_COMPLETE);
      if (!mission_active && !mission_ready) {   /* v5.6: auch Target-Gruppen machen die Mission bereit */
        mission_ready = 1;
        dmd_put(msg2, "MISSION READY");
      }
      if (grp_mask == 7 && !mb_ready && !multi) {
        mb_ready = 1;
        dmd_put(msg1, "RUN/STOP READY");
        dmd_put(msg2, "SHOOT RAMP");
        msg_type = 1;
      }
    }
}

static void top_lane(uint8_t i) {
  uint8_t bit = (uint8_t)(1 << i);
  snd_play(SND_LANE);
  if (skill_t && skill_t != 255) {
    skill_t = 0;
    if (i == skill_lane) { add(25000); message("SKILL SHOT", 25000); snd_play(SND_COMPLETE); }
  }
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
    msg_type = 0;
    snd_play(SND_COMPLETE);
    if (!mission_active && !mission_ready) {
      mission_ready = 1;
      dmd_put(msg2, "MISSION READY");
    }
  }
}

static void low_lane(uint8_t i) {
  uint8_t bit = (uint8_t)(1 << i);
  snd_play(SND_LANE);
  if (load_lit & bit) { add(500); return; }
  load_lit |= bit;
  add(2000);
  bonus++;
  mission_event(MS_LOAD);
  if (load_lit == 15) {
    load_lit = 0;
    add(25000);
    message("PRESS PLAY ON TAPE", 25000);
    dmd_fx(FX_PULSE);                      /* mit Bandstreifen im Rahmen */
    snd_play(SND_COMPLETE);
  }
}

/* Regeln, die von der Lage des Balls abhaengen (im Multiball fuer beide Baelle) */
static void ball_rules(void) {
  int16_t x = (int16_t)(ball.x >> 16), y = (int16_t)(ball.y >> 16), py = prev_y[ball_idx];
    /* Spinner: Ball kreuzt ihn in der Einfahrt, Drehung nach Tempo */
    if ((py < SPIN_Y) != (y < SPIN_Y) && x >= SPIN_X0 && x <= SPIN_X1) {
      int16_t d = y - py;
      if (d < 0) d = -d;
      if (d > 40) d = 0;                     /* Sprung (neuer Ball, Rampe): kein Drehen */
      spin_v += (uint16_t)d * 8;
      if (spin_v > 700) spin_v = 700;
      if (d) mission_event(MS_SPIN);
    }
    /* Roll-Over-Kontakte L und D: zaehlen beim Hineinrollen, erst wieder nach Verlassen */
    {
      uint8_t now = 0;
      if (!on_ramp) {
        /* v6.4: L in beiden Richtungen, nur nicht beim Abschuss (Skill-Shot-Zeit laeuft
         * noch, der Ball hat also nach dem Abschuss noch keine Lane erreicht) */
        if (!skill_t && y > ROLL_LY - ROLL_R && y < ROLL_LY + ROLL_R && x > ROLL_LX - ROLL_R && x < ROLL_LX + ROLL_R) now = 1;
        /* D nur aufwaerts (Schuss zur Rampe, nicht der zurueckrollende Ball) */
        else if (ball.vy < 0 && y > ROLL_DY - ROLL_R && y < ROLL_DY + ROLL_R && x > ROLL_DX - ROLL_R && x < ROLL_DX + ROLL_R) now = 2;
      }
      if (now && now != roll_in[ball_idx]) low_lane(now == 1 ? 0 : 3);
      roll_in[ball_idx] = now;
    }
    /* Rollover: Ball ueberquert die Zaehllinie */
    if ((py < TOPLANE_Y) != (y < TOPLANE_Y) && x > TOPLANE_X0 && x < TOPLANE_X0 + 72)
      top_lane(x < TOPLANE_X0 + 24 ? 0 : x < TOPLANE_X0 + 48 ? 1 : 2);
    if (py < LOWLANE_Y && y >= LOWLANE_Y) {
      /* v5.0: O und A an den Inlanes; L und D sind Targets, die Out-Lanes zaehlen nicht */
      if (x >= 40 && x < 64) low_lane(1);
      else if (x > 236 && x <= 260) low_lane(2);
    }
  prev_y[ball_idx] = y;
}

void game_update(const input_t *in) {
  /* Lane-Wechsel und Flipperklang beim Druecken */
  if (state == ST_PLUNGER) {
    if (skill_t != 255) { skill_t = 255; skill_lane = (uint8_t)(ball_no % 3); }
    if (in->left && !prev_l) skill_lane = skill_lane ? skill_lane - 1 : 2;
    if (in->right && !prev_r) skill_lane = skill_lane == 2 ? 0 : skill_lane + 1;
  } else if (skill_t == 255) skill_t = 250;           /* 5 s nach dem Abschuss */
  else if (skill_t) skill_t--;
  blink_n++;
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

  if (state == ST_PLAY) {
    for (uint8_t i = 0; i < 3; i++)
      if (ev_bumper & (1 << i)) { add(100); flash[i] = 5; snd_play(SND_BUMPER); }
    for (uint8_t i = 0; i < 2; i++)
      if (ev_sling & (1 << i)) { add(10); flash[3 + i] = 5; snd_play(SND_SLING); }
    if (ev_target)
      for (uint8_t i = 0; i < N_TARGETS; i++)
        if ((ev_target & ((uint16_t)1 << i)) && !tgt_cool[i]) { tgt_cool[i] = 15; target_hit(i); }
    if (ev_ramp & EV_RAMP_DONE) {
      if (ramps < 5) ramps++;
      uint16_t pts = (uint16_t)ramps * 5000u;
      add(pts);
      bonus++;
      message("FAST LOADER", pts);
      snd_play(SND_RAMP);
      mission_event(MS_RAMP);
      if (mb_ready) {
        mb_ready = 0;
        grp_mask = 0;
        phys_multiball();
        dmd_put(msg1, "RUN/STOP");
        dmd_put(msg2, "MULTIBALL");
        msg_timer = 100;
        msg_type = 0;
        dmd_fx(FX_BLINK);
        snd_play(SND_START);
      }
    }
    if (ev_kick & EV_KICK_IN) {
      dmd_put(msg1, "SEARCHING FOR *");
      dmd_put(msg2, "LOADING");
      msg_timer = KICK_HOLD;
      msg_type = 1;
      if (multi) { score += 50000UL; message("JACKPOT", 50000UL); dmd_fx(FX_PULSE); }
      if (mission_active) mission_event(MS_KICK);
      else if (mission_ready && !multi) mission_start();
    }
    if (ev_saved) { dmd_put(msg1, "RUN/STOP RESTORE"); dmd_put(msg2, "BALL SAVED"); msg_timer = 75; msg_type = 1; }
    /* Missionszeit */
    if (mission_active && !kick_timer && --mission_timer == 0) mission_end(0);
    if (ev_kick & EV_KICK_OUT) {
      add(3000);
      bonus++;
      dmd_put(msg1, "READY.");
      fmt_points(msg2, 3000);
      msg_timer = 50;
      msg_type = 0;
      snd_play(SND_BUMPER);
    }
    if (kick_timer && (kick_timer & 7) == 0) snd_play(SND_DRIVE);
    ball_rules();
    if (multi) { phys_swap(); ball_rules(); phys_swap(); }
  } else prev_y[ball_idx] = (int16_t)(ball.y >> 16);

  /* Ballende: Bonus; neuer Ball oder Spielende */
  if (state == ST_DRAIN && prev_state != ST_DRAIN && mission_active) {
    mission_active = 0;                      /* Ball verloren: Mission gescheitert */
    mission_cur = (uint8_t)((mission_cur + 1) % N_MISSIONS);
  }
  if (state == ST_DRAIN && prev_state != ST_DRAIN) {
    uint32_t b = (uint32_t)bonus * 1000u * mult;
    score += b;
    /* v6.6: Fehlermeldung flackert, darunter der Bonus */
    dmd_put(msg1, "?BALL LOST  ERROR");
    char *p = dmd_put(msg2, "BONUS ");
    p = dmd_num(p, mult, 0);
    p = dmd_put(p, "X ");
    fmt_points(p, b);
    msg_timer = 50;
    msg_type = 0;
    dmd_fx(FX_FLICKER);
    snd_play(SND_DRAIN);
  }
  if (state == ST_PLUNGER && prev_state == ST_DRAIN) {
    bonus = 0;
    ramps = 0;
    mult = 1;
    if (ball_no < BALLS_PER_GAME) ball_no++;
    else {
      game_over = 1;
      if (score > hiscore) {                 /* v6.6: Highscore (bis zum Ausschalten) */
        hiscore = score;
        message("NEW HIGH SCORE", score);
        dmd_fx(FX_PULSE);
      }
    }
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
  /* Ring des Kick-Out-Holes blinkt, solange eine Mission bereit ist */
  {
    static uint8_t blink, shown = 0xFF;
    uint8_t on = !(mission_ready && !mission_active) || ((++blink >> 3) & 1);
    if (on != shown) {
      shown = on;
      if (on) set_color(C_KICK_RING, 255, 210, 60); else set_color(C_KICK_RING, 70, 50, 12);
    }
    /* Rampe blinkt, solange RUN/STOP bereit ist */
    static uint8_t rshown = 0xFF;
    uint8_t ron = !mb_ready || ((blink >> 3) & 1);
    if (ron != rshown) {
      rshown = ron;
      if (ron) set_color(C_RAMP_RAIL, 220, 230, 255); else set_color(C_RAMP_RAIL, 255, 70, 60);
    }
  }
  snd_update();
}
