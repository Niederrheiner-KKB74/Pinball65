/*
 * sound.c – Klangeffekte auf $D400 und $D440 (v6.9: Effekte aus mehreren Schritten)
 *
 * Jeder Effekt ist eine kurze Folge von Schritten auf einer festen Stimme:
 * Wellenform, Huellkurve, Startfrequenz, Aenderung pro Bild, Dauer. Die Musik
 * laeuft auf dem zweiten SID-Paar (music.c) und wird hier mit weitergefuehrt.
 */
#include "sound.h"
#include "hw.h"
#include "music.h"

typedef struct {
  uint8_t wave;       /* Steuerregister ohne Gate: $10 Dreieck, $20 Saege, $40 Puls, $80 Rauschen; 0 = Ende */
  uint8_t ad, sr;
  uint16_t freq;
  int16_t step;       /* Aenderung pro Bild */
  uint8_t frames;
} fxstep_t;

/* Alle Schritte hintereinander; jeder Effekt endet mit wave = 0 */
static const fxstep_t STEPS[] = {
  /* 0 Bumper: Knall und kurzer fallender Ton */
  { 0x80, 0x09, 0x00, 0x2800, -0x600, 2 }, { 0x40, 0x08, 0x00, 0x1400, -0x180, 5 }, { 0 },
  /* 3 Slingshot: kurzer Schnalzer */
  { 0x80, 0x06, 0x00, 0x4000, 0, 1 }, { 0x40, 0x06, 0x00, 0x0A00, 0x200, 3 }, { 0 },
  /* 6 Target: heller Doppelton */
  { 0x10, 0x09, 0x00, 0x2200, 0, 3 }, { 0x10, 0x09, 0x00, 0x2D00, 0, 4 }, { 0 },
  /* 9 Lane: drei Toene aufwaerts */
  { 0x20, 0x06, 0x00, 0x1100, 0, 2 }, { 0x20, 0x06, 0x00, 0x1560, 0, 2 }, { 0x20, 0x08, 0x00, 0x1980, 0, 4 }, { 0 },
  /* 13 Flipper: dumpfer Schlag */
  { 0x80, 0x03, 0x00, 0x0600, -0x80, 2 }, { 0 },
  /* 15 Ballverlust: langer fallender Ton */
  { 0x20, 0x0A, 0xA6, 0x1C00, -0x50, 40 }, { 0x80, 0x0A, 0x00, 0x0400, 0, 6 }, { 0 },
  /* 18 Gruppe komplett: Dur-Dreiklang aufwaerts */
  { 0x40, 0x06, 0x00, 0x1100, 0, 3 }, { 0x40, 0x06, 0x00, 0x1560, 0, 3 }, { 0x40, 0x06, 0x00, 0x1980, 0, 3 },
  { 0x40, 0x0A, 0x84, 0x2200, 0, 10 }, { 0 },
  /* 23 Spielstart: Einschalt-Piepen */
  { 0x10, 0x00, 0xF0, 0x2D00, 0, 8 }, { 0x40, 0x08, 0x84, 0x0E00, 0x100, 12 }, { 0 },
  /* 26 Spinner: kurzes Ticken */
  { 0x40, 0x02, 0x00, 0x3000, 0, 1 }, { 0 },
  /* 28 1541: Kopf rattert (wird wiederholt) */
  { 0x80, 0x04, 0x00, 0x0300, 0, 2 }, { 0x40, 0x02, 0x00, 0x0280, 0, 1 }, { 0 },
  /* 31 Rampe: lange steigend, dann Klingeln */
  { 0x20, 0x0A, 0x84, 0x0800, 0x0E0, 24 }, { 0x10, 0x09, 0x00, 0x3300, 0, 6 }, { 0x10, 0x09, 0x00, 0x4400, 0, 8 }, { 0 },
};
static const uint8_t FX_START[SND_COUNT] = { 0, 3, 6, 9, 13, 15, 18, 23, 26, 28, 31 };
static const uint8_t FX_VOICE[SND_COUNT] = { 0, 0, 1, 1, 2, 2, 1, 2, 1, 2, 1 };

static uint8_t cur_step[3], cur_frames[3];
static uint16_t cur_freq[3];

static void voice_freq(uint8_t v, uint16_t f) {
  uint8_t r = (uint8_t)(v * 7);
  sid_poke(r, (uint8_t)f);
  sid_poke((uint8_t)(r + 1), (uint8_t)(f >> 8));
}

void snd_init(void) {
  for (uint8_t r = 0; r < 0x18; r++) sid_poke(r, 0);
  for (uint8_t v = 0; v < 3; v++) {
    sid_poke((uint8_t)(v * 7 + 2), 0x00);       /* Pulsbreite 50 % */
    sid_poke((uint8_t)(v * 7 + 3), 0x08);
    cur_frames[v] = 0;
  }
  sid_poke(0x18, 0x0F);                         /* Lautstaerke voll, kein Filter */
  music_init();
}

static void step_start(uint8_t v, uint8_t i) {
  const fxstep_t *f = &STEPS[i];
  uint8_t r = (uint8_t)(v * 7);
  cur_step[v] = i;
  if (!f->wave) { cur_frames[v] = 0; return; }
  sid_poke((uint8_t)(r + 4), f->wave);          /* Gate aus: Huellkurve neu starten */
  sid_poke((uint8_t)(r + 5), f->ad);
  sid_poke((uint8_t)(r + 6), f->sr);
  voice_freq(v, f->freq);
  sid_poke((uint8_t)(r + 4), (uint8_t)(f->wave | 1));
  cur_freq[v] = f->freq;
  cur_frames[v] = f->frames;
}

void snd_play(uint8_t n) { step_start(FX_VOICE[n], FX_START[n]); }

void snd_update(void) {
  for (uint8_t v = 0; v < 3; v++) {
    if (!cur_frames[v]) continue;
    const fxstep_t *f = &STEPS[cur_step[v]];
    if (f->step) { cur_freq[v] = (uint16_t)(cur_freq[v] + f->step); voice_freq(v, cur_freq[v]); }
    if (--cur_frames[v] == 0) {
      sid_poke((uint8_t)(v * 7 + 4), f->wave);  /* Gate aus, naechster Schritt */
      step_start(v, (uint8_t)(cur_step[v] + 1));
    }
  }
  music_update();
}
