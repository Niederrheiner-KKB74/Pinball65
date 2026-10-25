/*
 * physics.c – Portierung der Prototyp-Physik nach C (Festkomma 16.16)
 */
#include "physics.h"
#include "table.h"
#include "ramp_path.h"

ball_t ball;
flipper_t flip[N_FLIP];
uint8_t ev_bumper, ev_sling;
uint16_t ev_target;
uint8_t ev_ramp, ev_kick, on_ramp, kick_timer;
fix16 ramp_s, ramp_v;
static uint8_t ramp_seg, kick_cool;
uint8_t state, pull, in_lane;
uint16_t bumper_hits, sling_hits, collisions;
uint8_t test_active;
uint16_t test_count;
uint8_t snap_valid;
fix16 snap_x, snap_y;

uint8_t tune_on, tune_sel;
int32_t tune[T_COUNT];
const tune_info_t TUNE_INFO[T_COUNT] = {
  { "GRAVITY",      340,   100,    800,   10 },
  { "FLIPPER SPEED", 1100, 200,   1400,   20 },
  { "WALL BOUNCE",    80,    0,    230,    4 },
  { "FLIPPER BOUNCE", 72,    0,    200,    4 },
  { "BUMPER KICK", 90000, 20000, 160000, 2000 },
  { "SLING KICK",  70000,     0, 140000, 2000 },
};
int32_t p_gravity, p_rest_v, p_wall_imp, p_flip_imp, p_bumper_kick, p_sling_kick;
int16_t p_flip_up, p_flip_down;

static void tune_apply(void) {
  p_gravity = tune[T_GRAVITY];
  p_rest_v = p_gravity * 5;
  p_flip_up = (int16_t)tune[T_FLIP_SPEED];
  p_flip_down = (int16_t)(tune[T_FLIP_SPEED] * 3 / 5);
  p_wall_imp = FIX_ONE + tune[T_WALL_BOUNCE] * 256;      /* Faktor (1 + e) */
  p_flip_imp = FIX_ONE + tune[T_FLIP_BOUNCE] * 256;
  p_bumper_kick = tune[T_BUMPER_KICK];
  p_sling_kick = tune[T_SLING_KICK];
}

void tune_reset(void) {
  for (uint8_t i = 0; i < T_COUNT; i++) tune[i] = TUNE_INFO[i].def;
  tune_apply();
}

static void tune_input(const input_t *in) {
  if (in->tune_toggle) tune_on ^= 1;
  if (!tune_on) return;
  if (in->tune_next) tune_sel = (uint8_t)((tune_sel + 1) % T_COUNT);
  const tune_info_t *t = &TUNE_INFO[tune_sel];
  int32_t v = tune[tune_sel];
  if (in->tune_down) { v -= t->step; if (v < t->min) v = t->min; }
  if (in->tune_up)   { v += t->step; if (v > t->max) v = t->max; }
  if (v != tune[tune_sel]) { tune[tune_sel] = v; tune_apply(); }
}

static uint8_t sling_cool[2];
static int16_t plunger_top;      /* Pixel: Ballmittelpunkt darf nicht tiefer */
static uint8_t drain_timer;


/* Pruefpunkte auf dem Ballrand */
static int8_t pts_x[40], pts_y[40];
static uint8_t npts;

/* ---------- Winkel ---------- */
uint8_t angle256(int32_t dx, int32_t dy) {
  if (dx == 0 && dy == 0) return 0;
  uint32_t ax = dx < 0 ? (uint32_t)-dx : (uint32_t)dx;
  uint32_t ay = dy < 0 ? (uint32_t)-dy : (uint32_t)dy;
  while (ax > 1023 || ay > 1023) { ax >>= 1; ay >>= 1; }
  uint16_t x = (uint16_t)ax, y = (uint16_t)ay;
  uint8_t a;
  if (x >= y) a = ATAN_TAB[(uint16_t)(y * 64) / x];
  else a = (uint8_t)(64 - ATAN_TAB[(uint16_t)(x * 64) / y]);
  if (dx < 0) a = (uint8_t)(128 - a);
  if (dy < 0) a = (uint8_t)(0 - a);
  return a;
}

/* ---------- Maske ---------- */
static uint8_t sample(int16_t x, int16_t y) {
  if (y >= TH) return M_EMPTY;
  if (x < 0 || x >= TW || y < 0) return M_WALL;
  if (!mask_solid(x, y)) return M_EMPTY;
  uint8_t c = mask_class(x, y);
  if (c == M_GATE && in_lane) return M_EMPTY;
  if (c == M_GATE_UP && ball.vy < 0) return M_EMPTY;
  return c;
}

static uint8_t ball_hits_mask(void) {
  int16_t cx = (int16_t)(ball.x >> 16), cy = (int16_t)(ball.y >> 16);
  for (uint8_t i = 0; i < npts; i++)
    if (sample(cx + pts_x[i], cy + pts_y[i])) return 1;
  return 0;
}

/* Liegt in den 8x8-Feldern um den Ball irgendeine Wand? Wenn nicht, kann
 * die Pruefung der 36 Randpunkte entfallen (Ergebnis waere ohnehin leer). */
static uint8_t near_wall(int16_t cx, int16_t cy) {
  if (cx < 6 || cx > TW - 7 || cy < 6 || cy > TH - 7) return 1;
  uint8_t c0 = (uint8_t)((cx - 5) >> 3), c1 = (uint8_t)((cx + 5) >> 3);
  mask_ensure(cy - 5, cy + 5);
  uint8_t r0 = (uint8_t)(((cy - 5) >> 3) - cls_r0), r1 = (uint8_t)(((cy + 5) >> 3) - cls_r0);
  for (uint8_t r = r0; r <= r1; r++) {
    const uint8_t *g = &CLSWIN[((uint16_t)r << 5) + ((uint16_t)r << 3)];
    for (uint8_t c = c0; c <= c1; c++) if (g[c]) return 1;
  }
  return 0;
}

static void collide_mask(void) {
  int16_t cx = (int16_t)(ball.x >> 16), cy = (int16_t)(ball.y >> 16);
  if (!near_wall(cx, cy)) return;
  int16_t sx = 0, sy = 0;
  uint8_t n = 0, kind = 0;
  for (uint8_t i = 0; i < npts; i++) {
    uint8_t v = sample(cx + pts_x[i], cy + pts_y[i]);
    if (v) { sx -= pts_x[i]; sy -= pts_y[i]; n++; if (v > kind) kind = v; }
  }
  if (!n) return;
  collisions++;
  uint8_t a;
  if (sx == 0 && sy == 0)
    a = (ball.vx || ball.vy) ? (uint8_t)(angle256(ball.vx, ball.vy) + 128) : 192;
  else
    a = angle256(sx, sy);
  fix16 nx = cosA(a), ny = sinA(a);
  for (uint8_t k = 0; k < 16 && ball_hits_mask(); k++) { ball.x += nx >> 1; ball.y += ny >> 1; }
  fix16 vn = fmul(ball.vx, nx) + fmul(ball.vy, ny);
  if (vn >= 0) return;
  fix16 imp = (-vn < REST_V) ? vn : fmul(vn, p_wall_imp);
  ball.vx -= fmul(imp, nx);
  ball.vy -= fmul(imp, ny);
  if (kind >= M_TARGET) ev_target |= (uint16_t)1 << (kind - M_TARGET);
  if (kind == M_SLING_L || kind == M_SLING_R) {
    uint8_t s = kind - M_SLING_L;
    if (!sling_cool[s]) {
      ball.vx += fmul(SLING_KICK, nx);
      ball.vy += fmul(SLING_KICK, ny);
      sling_cool[s] = 8;
      sling_hits++;
      ev_sling |= (uint8_t)(1 << s);
    }
  }
}

/* ---------- Bumper ---------- */
static void collide_bumpers(void) {
  for (uint8_t i = 0; i < N_BUMPERS; i++) {
    fix16 bx = (fix16)BUMPERS[i][0] << 16, by = (fix16)BUMPERS[i][1] << 16;
    int16_t rr = BALL_R + BUMPERS[i][2];
    fix16 dx = ball.x - bx, dy = ball.y - by;
    fix16 lim = (fix16)(rr + 1) << 16;
    if (dx > lim || dx < -lim || dy > lim || dy < -lim) continue;
    int32_t qx = dx >> 8, qy = dy >> 8, qr = (int32_t)rr << 8;
    if (mul32(qx, qx) + mul32(qy, qy) >= mul32(qr, qr)) continue;
    collisions++;
    uint8_t a = angle256(dx, dy);
    fix16 nx = cosA(a), ny = sinA(a);
    ball.x = bx + nx * rr;
    ball.y = by + ny * rr;
    fix16 vn = fmul(ball.vx, nx) + fmul(ball.vy, ny);
    if (vn < 0) { ball.vx -= fmul(vn, nx); ball.vy -= fmul(vn, ny); }
    ball.vx += fmul(BUMPER_KICK, nx);
    ball.vy += fmul(BUMPER_KICK, ny);
    bumper_hits++;
    ev_bumper |= (uint8_t)(1 << i);
  }
}

/* ---------- Flipper ---------- */
uint8_t flip_dir(const flipper_t *f) {
  int16_t a = f->ang >> 8;
  return f->side > 0 ? (uint8_t)a : (uint8_t)(128 - a);
}

static void update_flipper(flipper_t *f) {
  int16_t prev = f->ang;
  if (f->pressed) { f->ang -= FLIP_SPEED; if (f->ang < F_UP * 256) f->ang = F_UP * 256; }
  else { f->ang += FLIP_DOWN; if (f->ang > F_REST * 256) f->ang = F_REST * 256; }
  f->w = f->ang - prev;
}

static void collide_flipper(const flipper_t *f) {
  fix16 pX = (fix16)f->px << 16, pY = (fix16)f->py << 16;
  fix16 ex = ball.x - pX, ey = ball.y - pY;
  const fix16 lim = (fix16)(F_LEN + F_R0 + BALL_R + 2) << 16;
  if (ex > lim || ex < -lim || ey > lim || ey < -lim) return;
  uint8_t d = flip_dir(f);
  fix16 ux = cosA(d), uy = sinA(d);
  fix16 t = fmul(ex, ux) + fmul(ey, uy);
  if (t < 0) t = 0; else if (t > ((fix16)F_LEN << 16)) t = (fix16)F_LEN << 16;
  fix16 qx = pX + fmul(ux, t), qy = pY + fmul(uy, t);
  fix16 dx = ball.x - qx, dy = ball.y - qy;
  /* Radius verjuengt sich von F_R0 auf F_R1: (F_R1-F_R0)*t/F_LEN = -t/12 */
  fix16 rr = ((fix16)(BALL_R + F_R0) << 16) + fmul(t, -5461L);
  int32_t a8 = dx >> 8, b8 = dy >> 8, r8 = rr >> 8;
  if (mul32(a8, a8) + mul32(b8, b8) >= mul32(r8, r8)) return;
  collisions++;
  uint8_t a = (dx == 0 && dy == 0) ? (uint8_t)(d + 192) : angle256(dx, dy);
  fix16 nx = cosA(a), ny = sinA(a);
  ball.x = qx + fmul(nx, rr);
  ball.y = qy + fmul(ny, rr);
  /* Oberflaechengeschwindigkeit am Treffpunkt */
  int16_t omega = f->side > 0 ? f->w : -f->w;
  fix16 rad = mul32(omega, 1608) >> 8;
  fix16 vsx = fmul(fmul(-uy, t), rad), vsy = fmul(fmul(ux, t), rad);
  fix16 rvx = ball.vx - vsx, rvy = ball.vy - vsy;
  fix16 vn = fmul(rvx, nx) + fmul(rvy, ny);
  if (vn >= 0) return;
  fix16 imp = (-vn < REST_V && omega == 0) ? vn : fmul(vn, p_flip_imp);
  ball.vx -= fmul(imp, nx);
  ball.vy -= fmul(imp, ny);
}

/* ---------- Rampe ----------
 * Auf der Rampe folgt der Ball einem festen Weg (zweite Ebene, keine Maske).
 * Am Anfang bremst die Steigung, auf der Bruecke rollt er fast ohne Verlust,
 * rechts abwaerts beschleunigt er. Reicht der Schwung nicht, rollt er zurueck. */
#define RAMP_V_MIN 32768L          /* 0,5 Pixel pro Substep auf der Bruecke */

static void ramp_place(void) {
  while (ramp_seg + 1 < RAMP_NSEG && ramp_s >= RAMP_SEGS[ramp_seg + 1].s0) ramp_seg++;
  while (ramp_seg > 0 && ramp_s < RAMP_SEGS[ramp_seg].s0) ramp_seg--;
  const ramp_seg_t *g = &RAMP_SEGS[ramp_seg];
  fix16 d = ramp_s - g->s0;
  ball.x = g->x0 + fmul(g->ux, d);
  ball.y = g->y0 + fmul(g->uy, d);
  /* Geschwindigkeit in Fahrtrichtung, damit Kamera und Anzeige stimmen */
  ball.vx = fmul(g->ux, ramp_v);
  ball.vy = fmul(g->uy, ramp_v);
}

static void ramp_step(void) {
  if (ramp_s < RAMP_CLIMB_END) ramp_v -= GRAVITY;
  else if (ramp_s >= RAMP_DESC_START) { ramp_v += GRAVITY >> 1; if (ramp_v > MAX_V) ramp_v = MAX_V; }
  else { ramp_v -= ramp_v >> 9; if (ramp_v < RAMP_V_MIN) ramp_v = RAMP_V_MIN; }
  ramp_s += ramp_v;
  if (ramp_s < 0) {                        /* zurueckgerollt: unten aus der Einfahrt */
    on_ramp = 0;
    ramp_seg = 0;
    ramp_place();                          /* Stelle knapp unterhalb des Starts, Tempo abwaerts */
    ev_ramp |= EV_RAMP_BACK;
  } else if (ramp_s >= RAMP_LEN) {         /* Ende unterhalb des dritten Flippers */
    on_ramp = 0;
    ball.x = RAMP_EXIT_X;
    ball.y = RAMP_EXIT_Y;
    ball.vx = -32768L;                     /* rollt zum linken Flipper */
    ball.vy = 32768L;
    ev_ramp |= EV_RAMP_DONE;
  } else ramp_place();
}

/* ---------- Kick-Out-Hole ---------- */
static void check_kick(void) {
  if (kick_cool) return;
  fix16 dx = ball.x - ((fix16)KICK_X << 16), dy = ball.y - ((fix16)KICK_Y << 16);
  const fix16 lim = 5L << 16;
  if (dx > lim || dx < -lim || dy > lim || dy < -lim) return;
  int32_t a = dx >> 8, b = dy >> 8;
  if (mul32(a, a) + mul32(b, b) >= 25L * 65536L) return;
  fix16 sp = (ball.vx < 0 ? -ball.vx : ball.vx) + (ball.vy < 0 ? -ball.vy : ball.vy);
  if (sp > 3L * 65536L) return;            /* zu schnell: rollt darueber */
  ball.x = (fix16)KICK_X << 16;
  ball.y = (fix16)KICK_Y << 16;
  ball.vx = ball.vy = 0;
  kick_timer = KICK_HOLD;
  ev_kick |= EV_KICK_IN;
}

/* ---------- Substep ---------- */
static void substep(void) {
  update_flipper(&flip[0]);
  update_flipper(&flip[1]);
  update_flipper(&flip[2]);
  if (state == ST_DRAIN || kick_timer) return;
  if (on_ramp) { ramp_step(); return; }
  fix16 prev_y = ball.y;
  ball.vy += GRAVITY;
  if (ball.vx > MAX_V) ball.vx = MAX_V; else if (ball.vx < -MAX_V) ball.vx = -MAX_V;
  if (ball.vy > MAX_V) ball.vy = MAX_V; else if (ball.vy < -MAX_V) ball.vy = -MAX_V;
  ball.x += ball.vx;
  ball.y += ball.vy;
  int16_t bx = (int16_t)(ball.x >> 16);
  /* Einfahrt der Rampe: Ball kreuzt die Linie RAMP_Y0 nach oben */
  if (prev_y >= ((fix16)RAMP_Y0 << 16) && ball.y < ((fix16)RAMP_Y0 << 16) &&
      bx >= RAMP_LANE_X0 && bx <= RAMP_LANE_X1) {
    on_ramp = 1;
    ramp_s = 0;
    ramp_seg = 0;
    ramp_v = fmul(ball.vx, RAMP_SEGS[0].ux) + fmul(ball.vy, RAMP_SEGS[0].uy);   /* Tempo bergauf */
    ramp_place();
    return;
  }
  if (bx > 293) in_lane = 1; else if (bx < 276) in_lane = 0;
  collide_flipper(&flip[0]);
  collide_flipper(&flip[1]);
  collide_flipper(&flip[2]);
  collide_bumpers();
  collide_mask();
  check_kick();
  if (in_lane && bx > 292 && ball.y > ((fix16)plunger_top << 16)) {
    ball.y = (fix16)plunger_top << 16;
    if (ball.vy > 0) ball.vy = 0;
    ball.vx = 0;
  }
}

void in_lane_reset(void) { in_lane = 0; }

static void new_ball(void) {
  ball.x = (fix16)LANE_X << 16;
  ball.y = (fix16)(PLUNGER_Y - BALL_R) << 16;
  ball.vx = ball.vy = 0;
  in_lane = 1;
  pull = 0;
  state = ST_PLUNGER;
  on_ramp = 0;
  kick_timer = 0;
}

static void launch(uint8_t p) {
  fix16 v = LAUNCH_MIN + ((LAUNCH_MAX - LAUNCH_MIN) * p) / PULL_MAX;
  ball.y = (fix16)(PLUNGER_Y - BALL_R) << 16;
  ball.vx = 0;
  ball.vy = -v;
  pull = 0;
  state = ST_PLAY;
}

void phys_init(void) {
  /* Pruefpunkte wie im Prototyp: 24 auf Radius 5, 12 auf Radius 3, ohne Doppelte */
  npts = 0;
  for (uint8_t ring = 0; ring < 2; ring++) {
    uint8_t n = ring ? 12 : 24;
    int32_t rad = ring ? BALL_R - 2 : BALL_R;
    for (uint8_t k = 0; k < n; k++) {
      uint8_t a = (uint8_t)(((uint16_t)k * 256 + n / 2) / n);
      int8_t ox = (int8_t)((mul32(cosA(a), rad) + 32768L) >> 16);
      int8_t oy = (int8_t)((mul32(sinA(a), rad) + 32768L) >> 16);
      uint8_t dup = 0;
      for (uint8_t j = 0; j < npts; j++) if (pts_x[j] == ox && pts_y[j] == oy) dup = 1;
      if (!dup) { pts_x[npts] = ox; pts_y[npts] = oy; npts++; }
    }
  }
  tune_reset();
  flip[0] = (flipper_t){ FLIP_L_X, FLIP_Y, 1, F_REST * 256, 0, 0 };
  flip[1] = (flipper_t){ FLIP_R_X, FLIP_Y, -1, F_REST * 256, 0, 0 };
  flip[2] = (flipper_t){ FLIP3_X, FLIP3_Y, -1, F_REST * 256, 0, 0 };
  new_ball();
}

void game_frame(const input_t *in) {
  flip[0].pressed = in->left;
  flip[1].pressed = in->right;
  flip[2].pressed = in->right;
  ev_bumper = ev_sling = 0;
  ev_target = 0;
  ev_ramp = ev_kick = 0;
  if (kick_cool) kick_cool--;
  if (kick_timer && --kick_timer == 0) {   /* auswerfen, nach links unten */
    ball.vx = -65536L;
    ball.vy = 19661L;
    kick_cool = 30;
    ev_kick |= EV_KICK_OUT;
  }

  tune_input(in);
  if (in->reset) { new_ball(); test_active = 0; }
  if (state == ST_PLUNGER) {
    if (in->test && !test_active) {
      launch(TEST_PULL);
      test_active = 1; test_count = 0; snap_valid = 0;
    } else if (in->plunger) {
      if (pull < PULL_MAX) pull++;
    } else if (pull > 0) {
      launch(pull);
    }
  }
  plunger_top = PLUNGER_Y - BALL_R + (int16_t)((pull * 7) / 20);

  collisions = 0;
  for (uint8_t i = 0; i < SUBSTEPS; i++) substep();
  ball.vx -= ball.vx >> 10;
  ball.vy -= ball.vy >> 10;

  /* Zu schwacher Abschuss: Ball liegt wieder auf dem Plunger -> neu abschiessen */
  if (state == ST_PLAY && in_lane && (ball.x >> 16) > 292 &&
      ball.y >= ((fix16)plunger_top << 16) && ball.vy == 0)
    state = ST_PLUNGER;

  if (test_active && ++test_count == TEST_FRAMES) {
    snap_x = ball.x; snap_y = ball.y; snap_valid = 1; test_active = 0;
  }
  if (state == ST_PLAY && ball.y > ((fix16)(TH + 14) << 16)) { state = ST_DRAIN; drain_timer = 50; }
  if (state == ST_DRAIN && --drain_timer == 0) new_ball();
  if (sling_cool[0]) sling_cool[0]--;
  if (sling_cool[1]) sling_cool[1]--;
}
