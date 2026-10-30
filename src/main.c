/*
 * Pinball65 – Meilenstein 3: Spielelemente, Punkte, Lampen, Klang
 *
 * - Hardware wird komplett uebernommen (kein KERNAL nach dem Start)
 * - Full-Colour-Charakter-Modus, zwei Bildpuffer (kein Flackern)
 * - Tisch 320 x 640 nach der Layout-Skizze, Grafik im Attic-RAM
 * - Pixelgenaues Scrolling, Dot-Matrix-Display oben
 * - Physik aus dem Browser-Prototyp, 8 Substeps pro Bild, 50 Hz
 *
 * Tasten: Shift links / A = linker Flipper, Shift rechts / L = rechter Flipper,
 * Leertaste halten = Plunger, T = Testabschuss, B = Ball zurueck,
 * D = Messwerte im Display an/aus.
 * Joystick Port 2: links/rechts = Flipper, Feuer oder unten = Plunger.
 * F1 = Einstellmodus an/aus, F3 = naechster Wert, F5 = weniger, F7 = mehr.
 */
#include <mega65.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"
#include "dmd.h"
#include "tables.h"
#include "game.h"
#include "sound.h"
#include "music.h"
#include "hiscore.h"

/* Taste mit Wiederholung: sofort, dann nach 15 Bildern alle 3 Bilder */
static uint8_t repeat(uint8_t down, uint8_t *cnt) {
  if (!down) { *cnt = 0; return 0; }
  uint8_t c = ++*cnt;
  if (c == 1) return 1;
  if (c >= 15) { *cnt = 12; return 1; }
  return 0;
}

static void read_input(input_t *in) {
  static uint8_t prev_t, prev_b, prev_d, prev_f1, prev_f3, rep_f5, rep_f7, prev_rs, prev_m;
  uint8_t c0 = key_col(0);
  uint8_t f1 = (c0 & 0x10) != 0, f3 = (c0 & 0x20) != 0;
  in->tune_toggle = f1 && !prev_f1;
  in->tune_next = f3 && !prev_f3;
  in->tune_down = repeat((c0 & 0x40) != 0, &rep_f5);
  in->tune_up = repeat((c0 & 0x08) != 0, &rep_f7);
  prev_f1 = f1; prev_f3 = f3;
  uint8_t c1 = key_col(1), c2 = key_col(2), c3 = key_col(3);
  uint8_t c5 = key_col(5), c6 = key_col(6), c7 = key_col(7);
  uint8_t j = joy2();
  in->left = (c1 & 0x80) || (c1 & 0x04) || (j & 0x04);      /* Shift links, A */
  in->right = (c6 & 0x10) || (c5 & 0x04) || (j & 0x08);     /* Shift rechts, L */
  in->plunger = (c7 & 0x10) || (j & 0x10) || (j & 0x02);    /* Leertaste */
  uint8_t t = (c2 & 0x40) != 0, b = (c3 & 0x10) != 0;       /* T, B */
  in->test = t && !prev_t;
  in->reset = b && !prev_b;
  uint8_t d = (c2 & 0x04) != 0;                             /* D */
  in->diag = d && !prev_d;
  uint8_t rs = (c7 & 0x80) != 0;                            /* RUN/STOP: Multiball zum Testen */
  in->mb = rs && !prev_rs;
  uint8_t mk = (key_col(4) & 0x10) != 0;                   /* M: Musik an/aus */
  in->mute = mk && !prev_m;
  prev_m = mk;
  static uint8_t prev_h;
  uint8_t h = (c3 & 0x20) != 0;                             /* v7.7: H = Anleitung */
  in->help = h && !prev_h;
  prev_h = h;
  prev_t = t; prev_b = b; prev_d = d; prev_rs = rs;
}

/* Display-Text je nach Spielzustand (Texte im C64-Stil, Debugwerte unten) */
/* Messwerte: Aufteilung des langsamsten Bildes der letzten Sekunde (Rasterzeilen) */
/* P Physik (Eingabe, game_frame, Kamera), R Regeln (game_update, Lampen, Klang),
 * S Hintergrund, Z Flipper/Ball, T Displaytext, D Display zeichnen, C Pruefsumme */
#define SPARE_FROM 200     /* bis hierher (Rasterzeilen) duerfen Display und Pruefsumme noch laufen */
#define SPARE_DMD  190     /* Display zeichnen (zwei Zeichenzeilen, je etwa 25-30 Rasterzeilen) */
enum { T_PHYS, T_RULES, T_SCROLL, T_DRAW, T_TEXT, T_DMD, T_CHK, T_N };
static uint16_t tmax[T_N], tmax_run[T_N];

static uint8_t diag_on;

/* Zeile 2 mit den Messwerten L und MAX */
static char *put_load(char *p, uint16_t load, uint16_t lmax) {
  p = dmd_put(p, "L");
  p = dmd_num(p, load, 3);
  p = dmd_put(p, " MAX");
  return dmd_num(p, lmax, 3);
}

/* Zeile der Highscore-Liste: "1 ABC  1.234.560" */
__attribute__((noinline)) static void hs_line(char *q, uint8_t i) {
  q[0] = (char)('1' + i); q[1] = ' ';
  for (uint8_t k = 0; k < 3; k++) q[2 + k] = hs[i].name[k];
  q[5] = ' '; q[6] = ' ';
  fmt_points(q + 7, hs[i].score);
}

/* v7.7: Anleitung im Wartebildschirm (Taste H). Flipper blaettern, sonst alle
 * 4 Sekunden weiter, nach der letzten Seite zurueck */
#define HELP_PAGES 12
static const char HELP[HELP_PAGES][2][27] = {
  { "FLIPPER LEFT: SHIFT OR A", "FLIPPER RIGHT: SHIFT OR L" },
  { "PLUNGER: HOLD SPACE", "OR JOYSTICK FIRE" },
  { "SKILL SHOT: FLIPPERS PICK", "A LANE, HIT IT AT LAUNCH" },
  { "C 6 4 LANES: ALL THREE", "RAISE BONUS UP TO 5X" },
  { "C 6 4 ALSO LIGHTS MISSION", "SHOOT 1541 TO START IT" },
  { "MISSIONS: FORMAT DISK", "SYS 64738  PEEK & POKE" },
  { "LOAD ERROR: LIGHT L O A D", "BEFORE TIME RUNS OUT" },
  { "L O A D LANES: 25.000", "PRESS PLAY ON TAPE" },
  { "RAMP: FAST LOADER", "5.000 UP TO 25.000" },
  { "VIC-II 6510 SID TARGETS", "THEN RAMP: MULTIBALL" },
  { "MULTIBALL: SHOOT 1541", "FOR JACKPOT 50.000" },
  { "CIA 1 + CIA 2: IRQ", "SAVES NEXT OUTLANE BALL" },
};

static uint8_t help_page, help_t;
static void help_input(const input_t *in) {
  static uint8_t pl, pr;
  if (!game_over || hs_entry) { help_page = 0; return; }
  if (in->help) { help_page = help_page ? 0 : 1; help_t = 0; }
  else if (help_page) {
    if (in->right && !pr) { help_page++; help_t = 0; }
    else if (in->left && !pl && help_page > 1) { help_page--; help_t = 0; }
    else if (++help_t >= 200) { help_page++; help_t = 0; }
    if (help_page > HELP_PAGES) help_page = 0;
  }
  pl = in->left; pr = in->right;
}

static uint8_t blink_t;
static uint8_t tick_blink(void) { return blink_t; }

static void update_display(uint16_t load, uint16_t lmax) {
  blink_t++;
  static char l1[DMD_COLS + 1], l2[DMD_COLS + 1];
  static uint8_t last_mode = 0xFF, tick;
  char *p;
  /* Zahlen nur alle 10 Bilder (5 Mal pro Sekunde), Wechsel des Zustands sofort */
  static uint8_t page;
  static uint16_t page_t;
  uint8_t type = 0;
  /* v6.6: nach Spielende wechseln vier Seiten alle 3 Sekunden */
  if (game_over && !msg_timer && !hs_entry && !help_page) { if (++page_t >= 150) { page_t = 0; if (++page == 6) page = 0; } }
  else { page = 0; page_t = 0; }
#ifdef PHYS_TUNE
  uint8_t mode = (uint8_t)(tune_on ? 10 + tune_sel : test_active ? 1 : (snap_valid && state == ST_PLUNGER) ? 2 :
                           msg_timer ? 7 : game_over ? 80 + page : 3 + state + (diag_on ? 30 : 0) + (mission_active ? 60 : 0));
#else
  uint8_t mode = (uint8_t)(test_active ? 1 : (snap_valid && state == ST_PLUNGER) ? 2 :
                           msg_timer ? 7 : game_over ? (help_page ? 100 + help_page : 80 + page) : 3 + state + (diag_on ? 30 : 0) + (mission_active ? 60 : 0));
#endif
  if (mode == last_mode && ++tick < 10 && !hs_entry && !msg_live) return;
  last_mode = mode;
  tick = 0;
#ifdef PHYS_TUNE
  if (tune_on) {
    dmd_put(l1, TUNE_INFO[tune_sel].name);
    p = dmd_put(l2, "= ");
    dmd_num(p, (uint32_t)tune[tune_sel], 0);
  } else
#endif
#ifdef PB_BUILDER
  if (test_active) {
    p = dmd_put(l1, "TEST ");
    dmd_num(p, test_count, 3);
    dmd_put(l2, "SEARCHING FOR *");
  } else if (snap_valid && state == ST_PLUNGER) {
    p = dmd_put(l1, "TEST X ");
    dmd_num(p, (uint32_t)snap_x, 8);
    p = dmd_put(l2, "TEST Y ");
    dmd_num(p, (uint32_t)snap_y, 8);
  } else
#endif
  if (hs_entry && !msg_timer) {               /* v7.4: Name eingeben, Buchstabe blinkt */
    dmd_put(l1, "ENTER YOUR NAME");
    for (uint8_t k = 0; k < 3; k++) l2[k] = hs_name[k];
    if (tick_blink() & 8) l2[hs_entry - 1] = '_';
    l2[3] = ' '; l2[4] = ' ';
    fmt_points(l2 + 5, score);
  } else if (msg_timer) {
    dmd_put(l1, msg1);
    dmd_put(l2, msg2);
    type = msg_type;
  } else if (game_over && help_page) {      /* v7.7: Seite der Anleitung */
    dmd_put(l1, HELP[help_page - 1][0]);
    dmd_put(l2, HELP[help_page - 1][1]);
    type = 1;
  } else if (game_over) {
    if (page == 0) {
      if (score) { dmd_put(l1, "?OUT OF DATA  ERROR"); fmt_points(l2, score); }
      else { dmd_put(l1, "**** PINBALL 65 ****"); dmd_put(l2, "PRESS FIRE"); type = 1; }   /* v7.5: nach dem Laden */
    }
    else if (page < 4) {                       /* v7.4: Highscore-Liste, zwei Eintraege je Seite */
      uint8_t i = (uint8_t)((page - 1) * 2);
      hs_line(l1, i);
      if (i + 1 < N_HS) hs_line(l2, (uint8_t)(i + 1)); else { dmd_put(l2, "PRESS FIRE"); type = 1; }
    }
    else if (page == 4) { dmd_put(l1, "LOAD\"*\",8,1"); dmd_put(l2, "READY."); type = 1; }
    else { dmd_put(l1, "PRESS H FOR HELP"); dmd_put(l2, "PRESS FIRE TO PLAY"); type = 1; }   /* v7.7 */
  } else if (state == ST_PLUNGER) {
    if (score) fmt_points(l1, score);
    else dmd_put(l1, "LOAD\"*\",8,1");
    p = dmd_put(l2, "BALL ");
    p = dmd_num(p, ball_no, 0);
    if (diag_on) {
      p = dmd_put(p, "  ");
      p = put_load(p, load, lmax);
      dmd_put(p, render_uses_dma() ? " DMA" : " CPU");
    } else { dmd_put(p, "  READY."); type = 1; }   /* v6.6: getippt, mit Cursor */
  } else if (state == ST_DRAIN) {
    dmd_put(l1, "?BALL LOST  ERROR");
    dmd_put(l2, "READY.");
  } else if (!diag_on && mission_active) {
    fmt_points(l1, score);
    mission_status(l2);
  } else if (!diag_on) {
    fmt_points(l1, score);
    p = dmd_put(l2, "BALL ");
    p = dmd_num(p, ball_no, 0);
    p = dmd_put(p, " ");
    p = dmd_num(p, mult, 0);
    dmd_put(p, "X");                       /* v6.9: L und MAX nur noch mit Taste D */
  } else {
    /* Zeile 1: L MAX P R, Zeile 2: S Z T D C (langsamstes Bild der letzten Sekunde) */
    static const char LET[T_N] = { 'P', 'R', 'S', 'Z', 'T', 'D', 'C' };
    p = put_load(l1, load, lmax);
    for (uint8_t k = 0; k < T_N; k++) {
      if (k == T_SCROLL) p = l2; else *p++ = ' ';
      *p++ = LET[k];
      p = dmd_num(p, tmax[k], 3);
    }
  }
  dmd_show(l1, l2, type);
}

/* v7.3: Pruefsumme nur noch mit -DCODE_CHECK (Speicher fuer CIA und Ball-Save) */
#ifdef CODE_CHECK
/* ---------- Diagnose: Pruefsumme ueber Programmcode und Konstanten ---------- */
extern char __data_start[];   /* Ende von Code und Konstanten; .data aendert sich im Spiel */
static uint16_t code_sum0;

static uint16_t code_sum(void) {
  uint16_t sum = 0;
  for (const uint8_t *p = (const uint8_t *)0x2001; p < (const uint8_t *)__data_start; p++)
    sum = (uint16_t)((sum << 1) | (sum >> 15)) ^ *p;
  return sum;
}

/* Im Spiel: 128 Byte pro Bild pruefen statt alles auf einmal (v5.0 mit 512 Byte: 24 Rasterzeilen) */
static void code_check_fail(const char *where);
static void code_check_step(void) {
  static const uint8_t *p;
  static uint16_t sum;
  if (!p) { p = (const uint8_t *)0x2001; sum = 0; }
  for (uint16_t n = 0; n < 128 && p < (const uint8_t *)__data_start; n++, p++)
    sum = (uint16_t)((sum << 1) | (sum >> 15)) ^ *p;
  if (p >= (const uint8_t *)__data_start) {
    if (sum != code_sum0) code_check_fail("IM SPIEL");
    p = 0;
  }
}

/* Haelt das Programm an, wenn der Code veraendert wurde */
static void code_check(const char *where) {
  if (code_sum() != code_sum0) code_check_fail(where);
}
static void code_check_fail(const char *where) {
  VICIV.bordercol = 2;
  dmd_text("?CODE CHANGED  ERROR", where);
  dmd_flush();
  for (;;) {}
}

#else
#define code_sum() 0
#define code_check(w) ((void)0)
#define code_check_step() ((void)0)
#endif

/* Hat der Lader die Tischdaten aufgebaut (Kennung im Attic-RAM)? */
static uint8_t data_ok(void) {
  dma_copy(ATTIC_SIG, CPUADDR(COLBUF), PB_SIG_LEN);
  for (uint8_t i = 0; i < PB_SIG_LEN; i++) if (COLBUF[i] != (uint8_t)PB_SIG[i]) return 0;
  return 1;
}

static void halt(const char *l1, const char *l2) {
  dmd_text(l1, l2);
  dmd_flush();
  VICIV.bordercol = 2;
  for (;;) {}
}

#if defined(__mos__) && !defined(PB_BUILDER)
/* v7.7: Speicher bei $E000-$FFF9 (src/game.ld): KERNAL ausblenden (MAP leer,
 * $01 = $35), Konstanten vom Lader holen (Attic-RAM), Variablen loeschen,
 * NMI und IRQ auf ein RTI. Erst in main(), weil der Startcode vorher noch das
 * KERNAL aufruft (CHROUT). Bis dahin keine Konstanten und Variablen benutzen. */
extern char __rodata_start[], __rodata_size[], __hibss_start[], __hibss_size[];
static const uint8_t RTI_OP = 0x40;
static uint8_t kmode = 0xFF;          /* mit Startwert: liegt in $2001-$CFFF, nicht bei $E000 */
static void himem_init(void) {
  dma_copy(ATTIC_KMODE, CPUADDR(&kmode), 1);   /* v7.7.1: vom Lader ermittelt */
  if (kmode) asm volatile("lda #0\n tax\n tay\n .byte 0x4b\n .byte 0x5c\n nop" ::: "a", "x", "y", "memory");   /* MAP leer */
  asm volatile("lda #$35\n sta $01" ::: "a", "memory");
  dma_copy(ATTIC_RODATA, CPUADDR(__rodata_start), (uint16_t)__rodata_size);
  dma_fill(CPUADDR(__hibss_start), 0, (uint16_t)__hibss_size);
  *(volatile uint16_t *)0xFFFA = (uint16_t)&RTI_OP;   /* NMI (RESTORE) */
  *(volatile uint16_t *)0xFFFE = (uint16_t)&RTI_OP;   /* IRQ, BRK */
  asm volatile("" ::: "memory");
}
#else
#define himem_init() ((void)0)
#endif

int main(void) {
  himem_init();
  hs_init();                            /* v7.4: zuerst, bevor $0F00 ueberschrieben wird */
#ifdef CODE_CHECK
  code_sum0 = code_sum();
#endif
  tables_init();
  hw_init();
  palette_load();                      /* v7.4: Palette vom Lader (Attic-RAM) */
  dmd_init();
  dma_fill(FB_A, C_BG, 40 * COL_BYTES); /* Tischbereich dunkel */
  dma_fill(FB_B, C_BG, 40 * COL_BYTES);
  dma_fill(DISP_ADDR, C_DMD_BG, DISP_ROWS * ROW_BYTES);
  show_buffer(0);

  if (!attic_ok()) halt("?ATTIC RAM  ERROR", "READY.");   /* Hintergrund braucht das Attic-RAM */
  /* Tischdaten baut der Lader PINBALL65; wurde das Spiel direkt gestartet: */
  if (!data_ok()) halt("?FILE NOT FOUND  ERROR", "RUN\"PINBALL65\"");
  code_check("NACH ATTIC-TEST");

  phys_init();
  render_init();
  if (!render_uses_dma()) halt("?DMA ERROR", "CORE 0.95 OR NEWER");
  snd_init();
  tick_init();
  game_new();
  game_over = 1;                        /* v7.5: Start im Attract-Modus mit Titelmusik */
  music_play(MUS_TITLE);
  VICIV.bordercol = 0;
  code_check("NACH AUFBAU");

  uint8_t front = 0;
  uint16_t load = 0, lmax = 0, lmax_run = 0;
  uint8_t max_tick = 0;
  input_t in;
  for (;;) {
    wait_vblank();
    dmd_tick();
    uint16_t t0 = raster_line();
    show_buffer(front);                 /* fertig gezeichneten Puffer zeigen */
    read_input(&in);
    if (in.diag) diag_on ^= 1;
    if (in.mute) music_mute(!music_muted());
    help_input(&in);
    game_input(&in);
    game_frame(&in);
    int16_t cam = camera_update();
    uint16_t ta = raster_line();
    game_update(&in);
    for (uint8_t n = ticks_elapsed(); n; n--) snd_update();   /* v7.6.2: Tempo unabhaengig von der Bildrate */
    uint16_t t1 = raster_line();
    uint8_t back = front ^ 1;
    render_frame(back, cam);
    uint16_t t2 = raster_line();
    /* Displaytext, Display und Pruefsumme duerfen warten: nur, wenn in diesem
     * Bild noch Zeit ist (v5.1). Sonst kommen sie im naechsten Bild dran. */
    uint8_t spare = raster_diff(t0, t2) < SPARE_FROM;
    if (spare) update_display(load, lmax);
    uint16_t tb = raster_line();
    /* v6.4: eine Textzeile kostet jetzt zwei Zeichenzeilen; nur, wenn noch Zeit ist */
    if (spare && raster_diff(t0, tb) < SPARE_DMD) dmd_render();
    uint16_t tc = raster_line();
    if (spare) code_check_step();
    uint16_t t3 = raster_line();
    uint16_t sect[T_N];
    sect[T_PHYS] = raster_diff(t0, ta);
    sect[T_RULES] = raster_diff(ta, t1);
    sect[T_TEXT] = raster_diff(t2, tb);
    sect[T_CHK] = raster_diff(tc, t3);
    sect[T_SCROLL] = render_bg_lines;
    sect[T_DRAW] = raster_diff(t1, t2) - render_bg_lines;
    sect[T_DMD] = raster_diff(tb, tc);
    front = back;
    /* Auslastung: Summe der Abschnitte, jeder innerhalb eines Bildes gemessen.
     * (v2.6/2.7 nutzten das Raster-Vergleichsflag; das loeste auf dem VIC-IV
     * mitten im Bild aus und meldete Ueberlaeufe, die es nicht gab.) */
    load = raster_diff(t0, t1) + raster_diff(t1, t2) + raster_diff(t2, t3) + raster_diff(t3, raster_line());
    /* Hoechstwerte der letzten Sekunde (50 Bilder) */
    if (load > lmax_run) {                /* neues langsamstes Bild: Aufteilung merken */
      lmax_run = load;
      for (uint8_t k = 0; k < T_N; k++) tmax_run[k] = sect[k];
    }
    if (++max_tick == 50) {
      max_tick = 0; lmax = lmax_run; lmax_run = 0;
      for (uint8_t k = 0; k < T_N; k++) { tmax[k] = tmax_run[k]; tmax_run[k] = 0; }
    }
  }
}
