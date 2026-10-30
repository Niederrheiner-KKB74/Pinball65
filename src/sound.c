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

static const uint8_t FX_START[SND_COUNT] = { 0, 3, 6, 9, 13, 15, 18, 23, 26, 28, 31, 35, 37 };
static const uint8_t FX_VOICE[SND_COUNT] = { 0, 0, 1, 1, 2, 2, 1, 2, 1, 2, 1, 0, 2 };

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

void snd_soft(uint8_t n) {
  uint8_t v = FX_VOICE[n];
  for (uint8_t k = 0; k < 3; k++) {
    if (!cur_frames[v]) { step_start(v, FX_START[n]); return; }
    if (++v == 3) v = 0;
  }
}

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
