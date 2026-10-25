/*
 * sound.c – Klangeffekte (Meilenstein 3), SID-Musik folgt in Meilenstein 5
 */
#include "sound.h"
#include "hw.h"

typedef struct {
  uint8_t voice;      /* 0..2 */
  uint8_t wave;       /* Steuerregister ohne Gate: $10 Dreieck, $20 Saege, $40 Puls, $80 Rauschen */
  uint8_t ad, sr;     /* Huellkurve */
  uint16_t freq;      /* Startfrequenz (SID-Wert) */
  int16_t step;       /* Aenderung pro Bild */
  uint8_t frames;     /* Dauer bis Gate aus */
} fx_t;

static const fx_t FX[SND_COUNT] = {
  { 0, 0x80, 0x08, 0x00, 0x2400, -0x300,  5 },   /* Bumper: Rauschknall */
  { 0, 0x40, 0x06, 0x00, 0x0C00,  0x180,  4 },   /* Slingshot: kurzer Puls */
  { 1, 0x10, 0x09, 0x00, 0x2200,      0,  5 },   /* Target: heller Ton */
  { 1, 0x20, 0x08, 0x00, 0x1000,  0x280,  8 },   /* Lane: steigend */
  { 2, 0x80, 0x03, 0x00, 0x0500,      0,  2 },   /* Flipper: dumpfer Schlag */
  { 2, 0x20, 0x0A, 0xA6, 0x1800,  -0x40, 45 },   /* Ballverlust: fallend */
  { 1, 0x10, 0x0A, 0x84, 0x1200,  0x180, 20 },   /* Gruppe komplett: aufsteigend */
  { 2, 0x40, 0x08, 0x84, 0x0E00,  0x100, 12 },   /* Spielstart */
};

static uint16_t cur_freq[3];
static int16_t cur_step[3];
static uint8_t cur_frames[3], cur_wave[3];

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
}

void snd_play(uint8_t n) {
  const fx_t *f = &FX[n];
  uint8_t v = f->voice, r = (uint8_t)(v * 7);
  sid_poke((uint8_t)(r + 4), f->wave);          /* Gate aus: Huellkurve neu starten */
  sid_poke((uint8_t)(r + 5), f->ad);
  sid_poke((uint8_t)(r + 6), f->sr);
  voice_freq(v, f->freq);
  sid_poke((uint8_t)(r + 4), (uint8_t)(f->wave | 1));
  cur_freq[v] = f->freq;
  cur_step[v] = f->step;
  cur_frames[v] = f->frames;
  cur_wave[v] = f->wave;
}

void snd_update(void) {
  for (uint8_t v = 0; v < 3; v++) {
    if (!cur_frames[v]) continue;
    if (cur_step[v]) { cur_freq[v] += (uint16_t)cur_step[v]; voice_freq(v, cur_freq[v]); }
    if (--cur_frames[v] == 0) sid_poke((uint8_t)(v * 7 + 4), cur_wave[v]);   /* Gate aus */
  }
}
