/*
 * Pinball65 – Meilenstein 2: ganzer Tisch mit Scrolling und Display
 *
 * - Hardware wird komplett uebernommen (kein KERNAL nach dem Start)
 * - Full-Colour-Charakter-Modus, zwei Bildpuffer (kein Flackern)
 * - Tisch 320 x 640 nach der Layout-Skizze, Grafik im Attic-RAM
 * - Pixelgenaues Scrolling, Dot-Matrix-Display oben
 * - Physik aus dem Browser-Prototyp, 8 Substeps pro Bild, 50 Hz
 *
 * Tasten: Shift links / A = linker Flipper, Shift rechts / L = rechter Flipper,
 * Leertaste halten = Plunger, T = Testabschuss, B = Ball zurueck.
 * Joystick Port 2: links/rechts = Flipper, Feuer oder unten = Plunger.
 * F1 = Einstellmodus an/aus, F3 = naechster Wert, F5 = weniger, F7 = mehr.
 */
#include <mega65.h>
#include "hw.h"
#include "table.h"
#include "physics.h"
#include "render.h"
#include "dmd.h"

/* Taste mit Wiederholung: sofort, dann nach 15 Bildern alle 3 Bilder */
static uint8_t repeat(uint8_t down, uint8_t *cnt) {
  if (!down) { *cnt = 0; return 0; }
  uint8_t c = ++*cnt;
  if (c == 1) return 1;
  if (c >= 15) { *cnt = 12; return 1; }
  return 0;
}

static void read_input(input_t *in) {
  static uint8_t prev_t, prev_b, prev_f1, prev_f3, rep_f5, rep_f7;
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
  prev_t = t; prev_b = b;
}

/* Display-Text je nach Spielzustand (Texte im C64-Stil, Debugwerte unten) */
static void update_display(uint16_t load, int16_t cam) {
  static char l1[DMD_COLS + 1], l2[DMD_COLS + 1];
  static uint8_t last_mode = 0xFF, tick;
  char *p;
  /* Zahlen nur alle 10 Bilder (5 Mal pro Sekunde), Wechsel des Zustands sofort */
  uint8_t mode = (uint8_t)(tune_on ? 10 + tune_sel : test_active ? 1 : (snap_valid && state == ST_PLUNGER) ? 2 : 3 + state);
  if (mode == last_mode && ++tick < 10) return;
  last_mode = mode;
  tick = 0;
  if (tune_on) {
    dmd_put(l1, TUNE_INFO[tune_sel].name);
    p = dmd_put(l2, "= ");
    dmd_num(p, (uint32_t)tune[tune_sel], 0);
  } else if (test_active) {
    p = dmd_put(l1, "TEST ");
    dmd_num(p, test_count, 3);
    dmd_put(l2, "SEARCHING FOR *");
  } else if (snap_valid && state == ST_PLUNGER) {
    p = dmd_put(l1, "TEST X ");
    dmd_num(p, (uint32_t)snap_x, 8);
    p = dmd_put(l2, "TEST Y ");
    dmd_num(p, (uint32_t)snap_y, 8);
  } else if (state == ST_PLUNGER) {
    dmd_put(l1, "LOAD\"*\",8,1");
    p = dmd_put(l2, "L");
    p = dmd_num(p, load, 3);
    p = dmd_put(p, "  PRESS SPACE");
  } else if (state == ST_DRAIN) {
    dmd_put(l1, "?BALL LOST  ERROR");
    dmd_put(l2, "READY.");
  } else {
    p = dmd_put(l1, "READY.  L");
    dmd_num(p, load, 3);
    p = dmd_put(l2, "X");
    p = dmd_num(p, (uint32_t)(ball.x >> 16) & 0x3FF, 3);
    p = dmd_put(p, " Y");
    p = dmd_num(p, (uint32_t)(ball.y >> 16) & 0x3FF, 3);
    p = dmd_put(p, " C");
    p = dmd_num(p, (uint32_t)cam, 3);
    p = dmd_put(p, " B");
    dmd_num(p, bumper_hits, 0);
  }
  dmd_text(l1, l2);
  dmd_render();
}

int main(void) {
  hw_init();
  table_build_palette();
  dma_fill(FB_A, C_BG, 40 * COL_BYTES); /* Tischbereich dunkel */
  dma_fill(FB_B, C_BG, 40 * COL_BYTES);
  dma_fill(DISP_ADDR, C_DMD_BG, DISP_ROWS * ROW_BYTES);
  show_buffer(0);

  if (!attic_ok()) {                    /* Hintergrund braucht das Attic-RAM */
    dmd_text("?ATTIC RAM  ERROR", "READY.");
    dmd_render();
    VICIV.bordercol = 2;
    for (;;) {}
  }
  dmd_text("SEARCHING FOR *", "LOADING");
  dmd_render();

  table_build_mask();
  table_build_art();
  render_init();
  phys_init();

  uint8_t front = 0;
  uint16_t load = 0;
  input_t in;
  for (;;) {
    wait_vblank();
    show_buffer(front);                 /* fertig gezeichneten Puffer zeigen */
    read_input(&in);
    game_frame(&in);
    int16_t cam = camera_update();
    uint8_t back = front ^ 1;
    render_frame(back, cam);
    update_display(load, cam);
    front = back;
    uint16_t l = raster_line();         /* Auslastung: Rasterzeilen seit Zeile 251 */
    load = l >= 251 ? l - 251 : l + 312 - 251;
  }
}
