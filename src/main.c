/*
 * Pinball65 – Meilenstein 1: Ball und Flipper auf einem statischen Tisch
 *
 * - Hardware wird komplett uebernommen (kein KERNAL nach dem Start)
 * - Full-Colour-Charakter-Modus, zwei Bildpuffer (kein Flackern)
 * - Physik aus dem Browser-Prototyp, 8 Substeps pro Bild, 50 Hz
 * - Fester Ausschnitt: untere 192 Zeilen des Tisches
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

int main(void) {
  hw_init();
  table_build_palette();
  dma_fill(FB_A, C_BG, 64000);          /* dunkler Bildschirm waehrend der Berechnung */
  show_buffer(0);

  table_build_mask();
  table_build_art(rowbuf);
  render_init();
  phys_init();

  uint8_t front = 0;
  uint16_t frame = 0, load = 0;
  input_t in;
  for (;;) {
    wait_vblank();
    show_buffer(front);                 /* fertig gezeichneten Puffer zeigen */
    frame++;
    read_input(&in);
    game_frame(&in);
    uint8_t back = front ^ 1;
    render_frame(back, frame, load);
    front = back;
    uint16_t l = raster_line();         /* Auslastung: Rasterzeilen seit Zeile 251 */
    load = l >= 251 ? l - 251 : l + 312 - 251;
  }
}
