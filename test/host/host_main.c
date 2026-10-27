/*
 * host_main.c – Fuehrt den Spielablauf auf dem PC aus und schreibt Bilder.
 *
 * Bildschirmbilder werden wie vom VIC-IV gelesen: Bildschirmkarte -> Zeichen-
 * nummer -> 64 Byte Zeichendaten. Dazu ein Bild des ganzen Tisches aus den
 * Spaltenstreifen im (simulierten) Attic-RAM.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"
#include "dmd.h"
#include "tables.h"
#include "game.h"
#include "sound.h"

extern uint8_t chip[], attic[];
extern uint8_t pal[256][3];
extern unsigned long dma_jobs, dma_bytes;

static void dump_screen(const char *name, uint8_t b) {
  FILE *f = fopen(name, "wb");
  fprintf(f, "P6 320 200 255\n");
  const uint8_t *map = chip + (b ? MAP_B : MAP_A);
  for (int y = 0; y < 200; y++)
    for (int x = 0; x < 320; x++) {
      int cell = (y >> 3) * 40 + (x >> 3);
      uint32_t ch = map[cell * 2] | (map[cell * 2 + 1] << 8);
      uint8_t v = chip[ch * 64 + (y & 7) * 8 + (x & 7)];
      fwrite(pal[v], 1, 3, f);
    }
  fclose(f);
}

static void dump_table(const char *name) {
  FILE *f = fopen(name, "wb");
  fprintf(f, "P6 320 640 255\n");
  for (int y = 0; y < 640; y++)
    for (int x = 0; x < 320; x++)
      fwrite(pal[attic[(x >> 3) * STRIP_BYTES + y * 8 + (x & 7)]], 1, 3, f);
  fclose(f);
}

/* Regeltest: Ereignisse direkt einspeisen und Punkte pruefen */
static int fails;
static void check(const char *what, unsigned long got, unsigned long want) {
  printf("%-28s %8lu %s\n", what, got, got == want ? "ok" : "FALSCH");
  if (got != want) { printf("   erwartet %lu\n", want); fails++; }
}
static void step(input_t *in) {
  game_input(in);
  game_frame(in);
  ev_bumper = ev_sling = 0; ev_target = 0;
}
static void rules_test(void) {
  input_t in; memset(&in, 0, sizeof in);
  printf("--- Regeltest ---\n");
  game_new();
  phys_init();
  state = ST_PLAY;
  ball.x = 160L << 16; ball.y = 400L << 16; ball.vx = ball.vy = 0;
  uint32_t s0 = score;
  /* Ereignisse einspeisen (game_frame laeuft nicht, Physik bleibt stehen) */
  ev_bumper = 1; game_update(&in); ev_bumper = 0;
  check("Bumper", score - s0, 100);
  ev_sling = 2; game_update(&in); ev_sling = 0;
  check("Slingshot", score - s0, 110);
  ev_target = 1; game_update(&in);
  ev_target = 1; game_update(&in); ev_target = 0;   /* Sperrzeit: zaehlt nur einmal */
  check("Target V", score - s0, 610);
  for (int k = 0; k < 20; k++) game_update(&in);
  ev_target = 1; game_update(&in); ev_target = 0;
  check("Target V nochmal (an)", score - s0, 710);
  ev_target = 6; game_update(&in); ev_target = 0;    /* I und C: VIC-II komplett */
  check("VIC-II komplett", score - s0, 710 + 1000 + 10000);
  printf("   Meldung: %s / %s\n", msg1, msg2);
  check("Bonus-Zaehler", bonus, 3);
  /* obere Lanes: Ball ueberquert y 52 in jeder Lane */
  /* Ball ausserhalb der Lanes unter die Linie, dann in der Lane darueber */
  #define LANE_PASS(xx) do { ball.x = 100L << 16; ball.y = 60L << 16; game_update(&in); \
    ball.x = (long)(xx) << 16; ball.y = 45L << 16; game_update(&in); \
    ball.x = 100L << 16; ball.y = 60L << 16; game_update(&in); } while (0)
  for (int lane = 0; lane < 3; lane++) LANE_PASS(126 + 24 * lane);
  check("C 6 4 komplett: Multiplikator", mult, 2);
  printf("   Meldung: %s / %s\n", msg1, msg2);
  /* Lane-Wechsel: eine Lampe an, rechter Flipper schiebt sie */
  LANE_PASS(126);
  extern uint8_t pal[256][3];
  check("Lampe C an", pal[C_TOP0][0], 255);
  in.right = 1; game_update(&in); in.right = 0; game_update(&in);
  check("nach rechts: Lampe 6 an", pal[C_TOP0 + 1][0], 255);
  check("nach rechts: Lampe C aus", pal[C_TOP0][0], 100);
  /* L und D sind Targets (v5.0), die Out-Lanes zaehlen nicht mehr */
  ball.x = (long)ROLL_LX << 16; ball.y = (long)ROLL_LY << 16; ball.vy = -65536L; game_update(&in); ball.vy = 0;
  check("Roll-Over L: Lampe L an", pal[C_LOAD0][0], 255);
  ball.x = 20L << 16; ball.y = 488L << 16; game_update(&in);
  ball.y = 508L << 16; game_update(&in);
  check("Out-Lane zaehlt nicht: D bleibt aus", pal[C_LOAD0 + 3][0], 100);
  /* Missionen: C 6 4 komplett -> bereit; Kick-Out startet FORMAT DISK; 3 Kick-Outs */
  for (int lane = 0; lane < 3; lane++) LANE_PASS(126 + 24 * lane);
  check("C 6 4: Mission bereit", mission_ready, 1);
  ev_kick = EV_KICK_IN; game_update(&in); ev_kick = 0;
  check("Kick-Out startet Mission", mission_active, 1);
  check("Mission FORMAT DISK", mission_cur, 0);
  { char st[32]; mission_status(st); printf("   Anzeige: %s\n", st); }
  uint32_t sm = score;
  for (int k = 0; k < 3; k++) { ev_kick = EV_KICK_IN; game_update(&in); ev_kick = 0; }
  check("3 Kick-Outs: Mission geschafft", mission_active, 0);
  check("Missionspunkte (mit Kick-Out)", score - sm >= 50000, 1);
  check("naechste Mission SYS 64738", mission_cur, 1);
  /* SYS 64738 laeuft ab */
  for (int lane = 0; lane < 3; lane++) LANE_PASS(126 + 24 * lane);
  ev_kick = EV_KICK_IN; game_update(&in); ev_kick = 0;
  for (int k = 0; k < MISSION_TIME + 5; k++) game_update(&in);
  check("Zeit abgelaufen: Mission vorbei", mission_active, 0);
  printf("   Meldung: %s / %s\n", msg1, msg2);
  /* Ballende: Bonus = Zaehler * 1000 * Multiplikator */
  uint32_t s1 = score; uint16_t b = bonus; uint8_t m = mult;
  state = ST_DRAIN; game_update(&in);
  check("Bonus am Ballende", score - s1, (unsigned long)b * 1000 * m);
  printf("   Meldung: %s / %s\n", msg1, msg2);
  state = ST_PLUNGER; game_update(&in);
  check("Ball 2", ball_no, 2);
  state = ST_DRAIN; game_update(&in); state = ST_PLUNGER; game_update(&in);
  state = ST_DRAIN; game_update(&in); state = ST_PLUNGER; game_update(&in);
  check("nach Ball 3: Spielende", game_over, 1);
  in.plunger = 1; game_input(&in);
  check("Plunger startet neu, kein Abschuss", in.plunger, 0);
  check("neues Spiel: Punkte", score, 0);
  check("neues Spiel: Ball", ball_no, 1);
  in.plunger = 1; game_input(&in);
  check("Plunger noch gehalten: gesperrt", in.plunger, 0);
  in.plunger = 0; game_input(&in); in.plunger = 1; game_input(&in);
  check("neu gedrueckt: Abschuss frei", in.plunger, 1);
  char buf[20]; fmt_points(buf, 1234560); printf("   fmt_points: %s\n", buf);
  check("Tausenderpunkte", strcmp(buf, "1.234.560") == 0, 1);
  /* v6.1: Skill-Shot */
  memset(&in, 0, sizeof in);
  game_new(); state = ST_PLUNGER; game_update(&in);
  extern uint8_t skill_lane; uint8_t sl0 = skill_lane;
  in.right = 1; game_update(&in); in.right = 0; game_update(&in);   /* eine Lane weiter */
  check("Flipper verschiebt Skill-Lane", skill_lane, sl0 == 2 ? 0 : sl0 + 1);
  state = ST_PLAY; game_update(&in);
  { uint32_t s0 = score; LANE_PASS(126 + 24 * skill_lane); check("Skill-Shot in blinkender Lane", score - s0 >= 25000, 1); }
  game_new(); state = ST_PLUNGER; game_update(&in); state = ST_PLAY; game_update(&in);
  { uint32_t s0 = score; LANE_PASS(126 + 24 * ((skill_lane + 1) % 3)); check("falsche Lane: kein Skill-Shot", score - s0 < 25000, 1);
    s0 = score; LANE_PASS(126 + 24 * skill_lane); check("nur die erste Lane zaehlt", score - s0 < 25000, 1); }
  printf("Regeltest: %s\n", fails ? "FEHLER" : "alles ok");
  (void)step;
}

/* Spielroboter: schiesst ab und schlaegt, wenn der Ball nahe an einem Flipper ist.
 * Meldet Stellen, an denen der Ball lange liegen bleibt. */
static void bot_run(long frames) {
  input_t in;
  unsigned seed = 12345;
  long still = 0, games = 0, stuck = 0;
  int16_t lx = 0, ly = 0;
  uint8_t pulling = 0, target_pull = 0, hold_l = 0, hold_r = 0, hold_3 = 0, cool = 0;
  unsigned long best = 0, tgt_hits = 0, n_ramp = 0, n_back = 0, n_kick = 0;
  int kick_trace = 0;
  long d_kick = 0, d_left = 0, d_mid = 0, d_right = 0, mis_done = 0, mis_start = 0, n_lane = 0, n_grp = 0;
  game_new(); phys_init();
  for (long f = 0; f < frames; f++) {
    memset(&in, 0, sizeof in);
    seed = seed * 1103515245u + 12345u;
    int16_t x = (int16_t)(ball.x >> 16), y = (int16_t)(ball.y >> 16);
    if (game_over) {
      if (!(f & 1)) { if (score > best) best = score; games++; mis_done += missions_done; in.plunger = 1; }
      pulling = 0;
    }
    else if (state == ST_PLUNGER) {
      if (!pulling) { pulling = 1; target_pull = (uint8_t)(20 + (seed >> 16) % 21); }
      if (pull < target_pull && (pull || !(f & 1))) in.plunger = 1; else if (pull >= target_pull) pulling = 0;
    }
    if (cool) cool--;
    /* Zufaellige Verzoegerung wie bei einem Menschen, sonst laeuft der
     * Roboter nach dem Kick-Out immer dieselbe Bahn (Endlosschleife) */
    if (state == ST_PLAY && !cool && !hold_l && !hold_r && !hold_3 && ((seed >> 16) % 3) != 0) {
      if (y > 558 && y < 603 && x > 70 && x < 150 && ball.vy > 0) hold_l = 8;
      if (y > 558 && y < 603 && x > 150 && x < 230 && ball.vy > 0) hold_r = 8;
      if (y > 285 && y < 325 && x > 240 && x < 290) hold_3 = 8;
      if (hold_l || hold_r || hold_3) cool = 25;
    }
    if (hold_l) { in.left = 1; hold_l--; }
    if (hold_r || hold_3) { in.right = 1; if (hold_r) hold_r--; if (hold_3) hold_3--; }
    game_input(&in);
    game_frame(&in);
    if (ev_target) tgt_hits++;
    { extern uint8_t ev_bumper; static unsigned long nb; if (ev_bumper) nb++; if (f == frames - 1) printf("Roboter: Bumper-Treffer %lu, je Spiel %.1f\n", nb, games ? (double)nb / games : 0.0); }
    if (ev_ramp & EV_RAMP_DONE) n_ramp++;
    if (ev_ramp & EV_RAMP_BACK) n_back++;
    if (ev_kick & EV_KICK_IN) n_kick++;
    { static int16_t py; int16_t yy = (int16_t)(ball.y >> 16), xx = (int16_t)(ball.x >> 16);
      if ((py < 52) != (yy < 52) && xx > 114 && xx < 186 && state == ST_PLAY) n_lane++; py = yy; }
    { static uint16_t pt; if (tgt_groups_done != pt) n_grp += (uint16_t)(tgt_groups_done - pt); pt = tgt_groups_done; }
    { static uint8_t was_act; if (mission_active && !was_act) mis_start++; was_act = mission_active; }
    { static uint8_t was; static int16_t lastx;
      if (state == ST_PLAY) lastx = (int16_t)(ball.x >> 16);
      if (state == ST_DRAIN && was != ST_DRAIN) { if (lastx < 42) d_left++; else if (lastx > 258) d_right++; else d_mid++;
        if (kick_trace && kick_trace < 150) d_kick++; }
      was = state; }
    { static int16_t hx[32], hy[32]; static uint8_t hi; static int16_t oy;
      int16_t yy = (int16_t)(ball.y >> 16), xx = (int16_t)(ball.x >> 16);
      hx[hi & 31] = xx; hy[hi & 31] = yy; hi++;
      if (getenv("OTRACE") && state == ST_PLAY && oy <= 404 && yy > 404 && (xx < 40 || xx > 260)) {
        printf("   Out-Lane %s: vorher", xx < 40 ? "links" : "rechts");
        for (int k = 30; k >= 6; k -= 6) printf(" (%d,%d)", hx[(uint8_t)(hi - 1 - k) & 31], hy[(uint8_t)(hi - 1 - k) & 31]);
        printf("\n"); }
      oy = yy; }
    if (ev_kick & EV_KICK_OUT) kick_trace = 1;
    if (ev_kick & EV_KICK_IN) kick_trace = 0;
    if (kick_trace && kick_trace < 200 && getenv("KTRACE") && n_kick < 4 && (kick_trace % 10) == 1)
      printf("   nach Auswurf %d: x=%d y=%d\n", kick_trace, x, y);
    if (kick_trace) kick_trace++;
    game_update(&in);
    if (state == ST_PLAY && abs(x - lx) < 2 && abs(y - ly) < 2) {
      if (++still == 150) { stuck++; printf("   Ball liegt bei x=%d y=%d (Bild %ld)\n", x, y, f); ball.vx = 30000; ball.vy = -60000; }
    } else still = 0;
    lx = x; ly = y;
    if (getenv("TRACE") && f % 2000 == 0) printf("   f=%ld st=%u x=%d y=%d pull=%u ball=%u score=%lu over=%u\n", f, state, x, y, pull, ball_no, (unsigned long)score, game_over);
  }
  printf("Roboter: %ld Bilder, %ld Spiele, bestes Ergebnis %lu, Target-Bilder %lu, haengen %ld\n",
         frames, games, best, tgt_hits, stuck);
  printf("Roboter: obere Lanes %ld, Target-Gruppen komplett %ld\n", n_lane, n_grp);
  printf("Roboter: Missionen gestartet %ld, geschafft %ld\n", mis_start, mis_done);
  printf("Roboter: Ballverluste links %ld, Mitte %ld, rechts %ld (davon bis 3 s nach Kick-Out-Auswurf: %ld)\n", d_left, d_mid, d_right, d_kick);
  printf("Roboter: Rampe %lu geschafft, %lu zurueckgerollt, Kick-Out %lu\n", n_ramp, n_back, n_kick);
}

/* Rampe und Kick-Out-Hole: Ball gezielt hineinschicken */
static void ramp_shot(double v) {
  input_t in; memset(&in, 0, sizeof in);
  phys_init(); game_new();
  state = ST_PLAY; in_lane_reset();
  ball.x = 128L << 16; ball.y = 360L << 16;   /* unterhalb der Muendung */
  ball.vx = 0; ball.vy = (fix16)(-v * 65536);
  int done = 0, back = 0, entered = 0; long f;
  int16_t xs = -1;
  for (f = 0; f < 600; f++) {
    game_frame(&in);
    if (on_ramp) entered = 1;
    if (ev_ramp & EV_RAMP_DONE) done = (int)f;
    if (ev_ramp & EV_RAMP_BACK) back = (int)f;
    game_update(&in);
    if (done && getenv("RTRACE") && (f - done) % 4 == 0 && (f - done) < 120) printf("      +%ld: %ld %ld\n", f - done, (long)(ball.x >> 16), (long)(ball.y >> 16));
    if (done && xs < 0 && (ball.y >> 16) >= 540) { xs = (int16_t)(ball.x >> 16); break; }
    if (back && (ball.y >> 16) > 420) break;
    if (state != ST_PLAY) break;
  }
  printf("   Schuss %.1f px/Substep: %s%s%s", v, entered ? "auf der Rampe" : "nicht auf der Rampe",
         done ? ", geschafft nach " : back ? ", zurueckgerollt" : "", "");
  if (done) printf("%d Bildern, bei y 540 x = %d, Punkte %lu", done, xs, (unsigned long)score);
  printf("\n");
}
static void kick_test(void) {
  input_t in; memset(&in, 0, sizeof in);
  phys_init(); game_new();
  state = ST_PLAY;
  ball.x = (long)KICK_X << 16; ball.y = (KICK_Y - 30L) << 16; ball.vx = 3000; ball.vy = 0;
  int cap = -1, out = -1;
  for (long f = 0; f < 400; f++) {
    game_frame(&in);
    if ((ev_kick & EV_KICK_IN) && cap < 0) cap = (int)f;
    if ((ev_kick & EV_KICK_OUT) && out < 0) out = (int)f;
    game_update(&in);
    if (out >= 0 && f > out && (f - out) % 10 == 0 && f - out <= 100 && getenv("KPATH")) printf("      +%ld: x=%ld y=%ld\n", f - out, (long)(ball.x >> 16), (long)(ball.y >> 16));
    if (out >= 0 && f == out + 60) printf("   60 Bilder nach dem Auswurf: x=%ld y=%ld\n", (long)(ball.x >> 16), (long)(ball.y >> 16));
  }
  printf("   Kick-Out: gefangen in Bild %d, ausgeworfen in Bild %d, Punkte %lu\n", cap, out, (unsigned long)score);
}

/* Bild mit dem Ball an einer festen Stelle (unter oder auf der Rampe) */
static void pose(const char *name, int x, int y, int ramp) {
  phys_init(); state = ST_PLAY;
  ball.x = (fix16)x << 16; ball.y = (fix16)y << 16; on_ramp = (uint8_t)ramp;
  int16_t cam = y - 100; if (cam < 0) cam = 0; if (cam > TH - VIEW_H) cam = TH - VIEW_H;
  render_frame(0, cam); render_frame(0, cam);
  dump_screen(name, 0);
}

/* Vom rechten Flipper in die Rampe? Ball faellt an Stelle x auf den Flipper,
 * Flipperknopf wird bei Hoehe yk gedrueckt. Zaehlt Treffer im Raster. */
static int16_t cross_x[400]; static int cross_n;
static void flipper_sweep(void) {
  input_t in;
  int hits = 0, tries = 0;
  char line[64];
  for (int x = 160; x <= 230; x += 5) {
    int k = 0;
    for (int yk = 548; yk <= 584; yk += 3) {
      phys_init(); game_new(); state = ST_PLAY; in_lane_reset();
      ball.x = (fix16)x << 16; ball.y = 498L << 16; ball.vx = 0; ball.vy = 0;
      int got = 0, pressed = 0;
      for (int f = 0; f < 250 && state == ST_PLAY; f++) {
        memset(&in, 0, sizeof in);
        if ((ball.y >> 16) >= yk && !pressed) pressed = 1;
        if (pressed && pressed < 12) { in.right = 1; pressed++; }
        fix16 py0 = ball.y;
        game_frame(&in);
        if (on_ramp) { got = 1; break; }
        if (py0 >= (310L << 16) && ball.y < (310L << 16) && cross_n < 400) cross_x[cross_n++] = (int16_t)(ball.x >> 16);
      }
      tries++; hits += got;
      line[k++] = got ? '#' : '.';
    }
    line[k] = 0;
    printf("   x=%3d %s\n", x, line);
  }
  printf("   rechter Flipper -> Rampe: %d von %d\n", hits, tries);
  { int hist[16] = {0}; for (int i = 0; i < cross_n; i++) { int b = cross_x[i] / 20; if (b >= 0 && b < 16) hist[b]++; }
    printf("   Schuesse ueber y 310 nach x (je 20 px):"); for (int b = 0; b < 16; b++) printf(" %d", hist[b]); printf("\n"); }
  for (int x = 70; x <= 140; x += 5) {
    int k = 0;
    for (int yk = 548; yk <= 584; yk += 3) {
      phys_init(); game_new(); state = ST_PLAY; in_lane_reset();
      ball.x = (fix16)x << 16; ball.y = 498L << 16; ball.vx = 0; ball.vy = 0;
      int got = 0, pressed = 0;
      for (int f = 0; f < 250 && state == ST_PLAY; f++) {
        memset(&in, 0, sizeof in);
        if ((ball.y >> 16) >= yk && !pressed) pressed = 1;
        if (pressed && pressed < 12) { in.left = 1; pressed++; }
        game_frame(&in);
        if (on_ramp) { got = 1; break; }
      }
      line[k++] = got ? '#' : '.';
    }
    line[k] = 0;
    printf("   links x=%3d %s\n", x, line);
  }
}

/* Realistischer: Ball auf dem hochgehaltenen rechten Flipper fangen, loslassen,
 * nach T Bildern schlagen (der Ball rollt dabei Richtung Spitze) */
static void cradle_sweep(int left) {
  input_t in;
  char line[48]; int k = 0, hits = 0;
  int hist[16] = {0};
  for (int T = 0; T < 40; T++) {
    phys_init(); game_new(); state = ST_PLAY; in_lane_reset();
    if (getenv("FB")) { tune[T_FLIP_BOUNCE] = atol(getenv("FB")); tune_apply(); }
    if (getenv("FS")) { tune[T_FLIP_SPEED] = atol(getenv("FS")); tune_apply(); }
    ball.x = (left ? 120L : 180L) << 16; ball.y = 528L << 16; ball.vx = ball.vy = 0;
    int got = 0;
    for (int f = 0; f < 400 && state == ST_PLAY; f++) {
      memset(&in, 0, sizeof in);
      int hold = f < 80 || (f >= 80 + T && f < 92 + T);
      if (left) in.left = (uint8_t)hold; else in.right = (uint8_t)hold;
      fix16 py0 = ball.y;
      game_frame(&in);
      if (on_ramp) { got = 1; break; }
      if (getenv("CTRACE") && T == atoi(getenv("CTRACE")) && (f % 4) == 0 && f > 60 && f < 160)
        printf("      f=%d x=%ld y=%ld vx=%.2f vy=%.2f fl=%d\n", f, (long)(ball.x >> 16), (long)(ball.y >> 16), ball.vx / 65536.0, ball.vy / 65536.0, flip[left ? 0 : 1].ang >> 8);
      if (f > 80 && py0 >= (310L << 16) && ball.y < (310L << 16)) { int b = (int)(ball.x >> 16) / 20; if (b >= 0 && b < 16) hist[b]++;
        if (getenv("SHOTX")) printf("      T=%2d x=%ld vx=%.2f vy=%.2f\n", T, (long)(ball.x >> 16), ball.vx / 65536.0, ball.vy / 65536.0); }
    }
    hits += got; line[k++] = got ? '#' : '.';
  }
  line[k] = 0;
  printf("   %s Flipper gefangen, Schlag nach T=0..39: %s (%d Treffer)\n", left ? "linker " : "rechter", line, hits);
  printf("   ueber y 310 nach x:"); for (int b = 0; b < 16; b++) printf(" %d", hist[b]); printf("\n");
}

/* Ganzer Tisch mit Flippern (fuer Mock-ups): vier Ausschnitte uebereinander */
static void dump_mockup(const char *name) {
  phys_init(); state = ST_DRAIN;
  FILE *f = fopen(name, "wb");
  fprintf(f, "P6 320 640 255\n");
  static const int16_t CAMS[4] = { 0, 168, 336, 472 };
  static uint8_t img[640][320][3];
  for (int k = 0; k < 4; k++) {
    render_frame(0, CAMS[k]); render_frame(0, CAMS[k]);
    const uint8_t *map = chip + MAP_A;
    for (int y = 32; y < 200; y++)
      for (int x = 0; x < 320; x++) {
        int cell = (y >> 3) * 40 + (x >> 3);
        uint32_t ch = map[cell * 2] | (map[cell * 2 + 1] << 8);
        uint8_t v = chip[ch * 64 + (y & 7) * 8 + (x & 7)];
        memcpy(img[CAMS[k] + y - 32][x], pal[v], 3);
      }
  }
  fwrite(img, 1, sizeof img, f);
  fclose(f);
  state = ST_PLUNGER;
}

/* Dritter Flipper: Ball faellt wie vom Rampenende auf ihn, Schlag nach T Bildern.
 * Zaehlt Treffer der oberen Lanes C 6 4 und zeigt, wo der Ball y 200 kreuzt. */
static void flip3_sweep(void) {
  input_t in;
  char line[48]; int k = 0, hits = 0;
  for (int T = 0; T < 40; T++) {
    phys_init(); game_new(); state = ST_PLAY; in_lane_reset();
    ball.x = 262L << 16; ball.y = 252L << 16; ball.vx = 0; ball.vy = 16384;
    int got = 0, cx = -1;
    for (int f = 0; f < 300 && state == ST_PLAY; f++) {
      memset(&in, 0, sizeof in);
      if (f >= T && f < T + 12) in.right = 1;
      fix16 py0 = ball.y;
      game_frame(&in);
      if (py0 >= (200L << 16) && ball.y < (200L << 16) && cx < 0) cx = (int)(ball.x >> 16);
      if (py0 >= (52L << 16) && ball.y < (52L << 16) && (ball.x >> 16) > 114 && (ball.x >> 16) < 186) got = 1;
      if ((ball.x >> 16) < 30 && (ball.y >> 16) > 150) { got = 2; break; }
      if ((ball.y >> 16) > 500) break;
    }
    hits += got == 1; line[k++] = got == 2 ? 'O' : got ? '#' : (cx < 0 ? '.' : (char)('a' + cx / 20));
  }
  line[k] = 0;
  printf("   dritter Flipper -> C 6 4: %s (%d Treffer; Buchstabe = x/20 bei y 200)\n", line, hits);
}

/* Abschuss mit verschiedenen Plunger-Staerken, ohne Flipper: Wie schnell und
 * auf welchem Weg geht der Ball verloren? */
static void plunge_test(void) {
  input_t in;
  int fast = 0, kicked = 0;
  for (int p = 10; p <= PULL_MAX; p++) {
    phys_init(); game_new();
    int f, kick = 0;
    for (f = 0; f < 1500; f++) {
      memset(&in, 0, sizeof in);
      if (f < p) in.plunger = 1;
      game_frame(&in);
      if (ev_kick & EV_KICK_IN) kick = 1;
      game_update(&in);
      if (state == ST_DRAIN) break;
      if (getenv("PLTRACE") && f % 20 == 0 && f > 80) printf("      %d: %ld %ld\n", f, (long)(ball.x >> 16), (long)(ball.y >> 16));
    }
    if (f < 400) fast++;
    kicked += kick;
    if (getenv("PLTRACE")) printf("   Zug %2d: Ballverlust nach %4d Bildern%s\n", p, f, kick ? " (ueber Kick-Out)" : "");
  }
  printf("   Abschuesse ohne Flipper: %d von %d nach weniger als 8 s verloren, %d ueber das Kick-Out-Hole\n",
         fast, PULL_MAX - 9, kicked);
}

/* Orbit-Test: Ball tangential im Orbit starten (links unten bzw. rechts unten,
 * aufwaerts), Tempo v; wohin kommt er? */
static void orbit_test(void) {
  input_t in;
  for (int dir = 0; dir < 2; dir++) {
    printf("   Orbit %s:", dir ? "rechts->links" : "links->rechts");
    for (int v = 8; v <= 24; v += 2) {
      phys_init(); game_new(); state = ST_PLAY; in_lane_reset();
      ball.x = (dir ? 300L : 20L) << 16; ball.y = 150L << 16;
      ball.vx = 0; ball.vy = -(fix16)v * 8192L;          /* v/8 Pixel pro Substep */
      char r = '?';
      for (int f = 0; f < 300 && state == ST_PLAY; f++) {
        memset(&in, 0, sizeof in);
        game_frame(&in);
        int x = (int)(ball.x >> 16), y = (int)(ball.y >> 16);
        if (y < 75 && y > 40 && x > 114 && x < 186) { r = 'L'; break; }           /* in C 6 4 */
        if (dir && x < 30 && y > 170) { r = 'O'; break; }                        /* links herum */
        if (!dir && (x > 262 && y > 170)) { r = 'O'; break; }                     /* rechts herum */
        if (y > 260) { r = 'z'; break; }                                          /* zurueck */
      }
      printf(" %d:%c", v, r);
    }
    printf("\n");
  }
}

/* Shot-Map: Ball auf dem Flipper fangen (oder aus der Inlane kommen lassen),
 * zu verschiedenen Zeiten schlagen, Bahn in shots.csv schreiben */
static void shot_map(void) {
  FILE *out = fopen("shots.csv", "w");
  input_t in;
  for (int fl = 0; fl < 2; fl++)
    for (int mode = 0; mode < 2; mode++)          /* 0: gefangen, 1: aus der Inlane */
      for (int T = 0; T < 40; T++) {
        phys_init(); game_new(); state = ST_PLAY; in_lane_reset();
        if (mode == 0) { ball.x = (fl ? 180L : 120L) << 16; ball.y = 528L << 16; }
        else { ball.x = (fl ? 250L : 50L) << 16; ball.y = 488L << 16; }
        ball.vx = ball.vy = 0;
        int t0 = mode == 0 ? 80 : 0, hit = -1;
        for (int f = 0; f < 400 && state == ST_PLAY; f++) {
          memset(&in, 0, sizeof in);
          int hold = (mode == 0 && f < 80) || (f >= t0 + T && f < t0 + T + 12);
          if (fl) in.right = (uint8_t)hold; else in.left = (uint8_t)hold;
          game_frame(&in);
          if (f == t0 + T) hit = f;
          if (hit >= 0 && f > hit && f < hit + 70) fprintf(out, "%d,%d,%d,%ld,%ld\n", fl, mode, T, (long)(ball.x >> 16), (long)(ball.y >> 16));
          if (hit >= 0 && f > hit + 70) break;
        }
      }
  fclose(out);
}

extern int host_no_trans;
#include <stdlib.h>
#include <math.h>
int main(void) {
  host_no_trans = getenv("NO_TRANS") != 0;
  tables_init();
  hw_init();
  table_build_palette();
  dmd_init();
  dma_fill(FB_A, C_BG, 40 * COL_BYTES);
  dma_fill(FB_B, C_BG, 40 * COL_BYTES);
  dmd_text("SEARCHING FOR *", "LOADING");
  dmd_render();
  table_build_mask();
  table_build_art();
  table_build_overlay();
  phys_init();
  render_build();
  render_init();
  snd_init();
  game_new();
  dump_table("table_full.ppm");

  input_t in;
  uint8_t front = 0;
  for (uint16_t frame = 1; frame <= 900; frame++) {
    memset(&in, 0, sizeof in);
    if (frame == 2) in.test = 1;                         /* Testabschuss */
    if (frame > 400 && frame < 420) in.left = in.right = 1;
    if (frame > 600 && (frame % 40) < 8) in.left = 1;
    if (frame > 600 && (frame % 40) >= 20 && (frame % 40) < 28) in.right = 1;
    game_input(&in);
    game_frame(&in);
    game_update(&in);
    int16_t cam = camera_update();
    uint8_t back = front ^ 1;
    unsigned long j0 = dma_jobs, b0 = dma_bytes;
    render_frame(back, cam);
    char l1[27], l2[27], *p;
    p = dmd_put(l1, "TEST "); dmd_num(p, test_count, 3);
    p = dmd_put(l2, "C"); dmd_num(p, (uint32_t)cam, 3);
    dmd_text(l1, l2);
    dmd_render();
    front = back;
    if (frame % 25 == 0 || frame == 3)
      printf("F%4u st=%u x=%7.2f y=%7.2f cam=%3d bump=%u sling=%u score=%lu ball=%u%s dma=%lu/%lu\n",
             frame, state, ball.x / 65536.0, ball.y / 65536.0, cam,
             bumper_hits, sling_hits, (unsigned long)score, ball_no, game_over ? " OVER" : "", dma_jobs - j0, dma_bytes - b0);
    if (frame == TEST_FRAMES + 2 && snap_valid) printf("SNAPSHOT x=%ld y=%ld\n", (long)snap_x, (long)snap_y);
    if (frame == 3) dump_screen("frame_start.ppm", front);
    if (frame == 30) dump_screen("frame_top.ppm", front);
    if (frame == 410) dump_screen("frame_flippers_up.ppm", front);
    if (frame == 110) dump_screen("frame_play.ppm", front);
    if (frame == 125) dump_screen("frame_mid.ppm", front);
  }
  rules_test();
  printf("--- Rampe ---\n");
  for (double v = 0.6; v <= 3.6; v += 0.4) ramp_shot(v);
  kick_test();
  flipper_sweep();
  dump_mockup("mockup.ppm");
  cradle_sweep(0);
  cradle_sweep(1);
  flip3_sweep();
  plunge_test();
  if (getenv("SHOTMAP")) shot_map();
  if (getenv("ORBIT")) orbit_test();
  pose("pose_under_bridge.ppm", 150, 84, 0);
  pose("pose_on_bridge.ppm", 150, 82, 1);
  pose("pose_under_right.ppm", 240, 200, 0);
  if (getenv("BOT")) bot_run(atol(getenv("BOT")));
  return 0;
}
void build_progress(uint8_t phase, uint8_t step) { (void)phase; (void)step; }
