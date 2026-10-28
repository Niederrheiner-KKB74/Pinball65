/*
 * music.c – Musik auf dem zweiten SID-Paar ($D420 und $D460), v6.9
 *
 * Drei Stimmen, jede mit eigener Pattern-Folge (music_data.c, erzeugt von
 * tools/gen_music.py). Ein Schritt dauert tempo Bilder. Kurze Stuecke
 * (Jingles) unterbrechen die Hintergrundmusik, danach beginnt sie neu.
 */
#include "music.h"
#include "music_data.h"
#include "hw.h"

#define poke2 sid2_poke

typedef struct {
  const uint8_t *ord, *p;     /* Folge und Leseposition im Pattern */
  uint8_t len, left, ins, note, gate, frames, fxc;
  uint16_t freq, pw;
} mvoice_t;

static mvoice_t mv[3];
static uint8_t song = 0xFF, bg = 0xFF, tick, tempo, active, muted;
static const uint8_t ARP[2][3] = { { 0, 3, 7 }, { 0, 4, 7 } };

static uint16_t note_freq(uint8_t n) {
  uint8_t oct = 0;
  while (n >= 12) { n -= 12; oct++; }
  return NOTE6[n] >> (6 - oct);
}

static void set_freq(uint8_t v, uint16_t f) {
  poke2((uint8_t)(v * 7), (uint8_t)f);
  poke2((uint8_t)(v * 7 + 1), (uint8_t)(f >> 8));
}

/* Naechstes Ereignis einer Stimme lesen (Note, Pause oder Halten) */
static void voice_step(uint8_t v) {
  mvoice_t *m = &mv[v];
  if (!m->ord) return;
  if (m->left && --m->left) return;
  for (;;) {
    uint8_t b = *m->p++;
    if (b == 0xFF) {                          /* Pattern zu Ende: naechstes aus der Folge */
      m->ord++;
      if (*m->ord == 0xFF) {
        if (!SONGS[song].loop) { m->ord = 0; poke2((uint8_t)(v * 7 + 4), INSTR[m->ins].wave); active--; return; }
        m->ord = SONGS[song].order[v];
      }
      m->p = MUS_PAT + MUS_OFS[*m->ord];
      continue;
    }
    if (b >= 0x80 && b < 0xFD) { m->ins = (uint8_t)(b - 0x80); continue; }
    if (b >= 0x60 && b < 0x80) { m->len = (uint8_t)(b - 0x5F); continue; }
    m->left = m->len;
    if (b == 0xFE) return;                    /* halten: Gate-Zeit wurde schon mitgezaehlt */
    /* Gate-Zeit: diese Note und folgende Halte-Schritte, ein Bild vor dem Ende aus */
    {
      const uint8_t *q = m->p;
      uint8_t l = m->len, tot = m->len;
      for (;;) {
        uint8_t c = *q;
        if (c >= 0x60 && c < 0x80) { l = (uint8_t)(c - 0x5F); q++; }
        else if (c == 0xFE) { tot = (uint8_t)(tot + l); q++; }
        else break;
      }
      m->frames = (uint8_t)(tot * tempo - 1);
    }
    const instr_t *in = &INSTR[m->ins];
    uint8_t r = (uint8_t)(v * 7);
    if (b == 0xFD) { poke2((uint8_t)(r + 4), in->wave); m->gate = 0; return; }
    m->note = b;
    m->freq = note_freq(b);
    m->pw = in->pw;
    m->fxc = 0;
    poke2((uint8_t)(r + 4), in->wave);         /* Huellkurve neu starten */
    poke2((uint8_t)(r + 5), in->ad);
    poke2((uint8_t)(r + 6), in->sr);
    poke2((uint8_t)(r + 2), (uint8_t)m->pw);
    poke2((uint8_t)(r + 3), (uint8_t)(m->pw >> 8));
    set_freq(v, m->freq);
    poke2((uint8_t)(r + 4), (uint8_t)(in->wave | 1));
    m->gate = 1;
    return;
  }
}

/* Pro Bild: Arpeggio, Rutschen, Vibrato, Pulsbreite, Gate */
static void voice_fx(uint8_t v) {
  mvoice_t *m = &mv[v];
  if (!m->ord || !m->gate) return;
  const instr_t *in = &INSTR[m->ins];
  uint8_t r = (uint8_t)(v * 7);
  if (m->frames && --m->frames == 0) { poke2((uint8_t)(r + 4), in->wave); m->gate = 0; return; }
  m->fxc++;
  if (in->fx == 1) {
    uint8_t k = (uint8_t)(m->fxc % 3);
    set_freq(v, note_freq((uint8_t)(m->note + ARP[in->arg][k])));
  } else if (in->fx == 2) {
    uint16_t d = (uint16_t)in->arg << 4;
    m->freq = m->freq > d ? (uint16_t)(m->freq - d) : 0;
    set_freq(v, m->freq);
  } else if (in->fx == 3 && m->fxc > in->arg) {
    uint8_t ph = (uint8_t)(m->fxc & 7);
    int16_t d = (int16_t)(m->freq >> 8);
    set_freq(v, (uint16_t)(m->freq + (ph < 4 ? d : -d)));
  }
  if (in->sweep) {
    m->pw = (uint16_t)((m->pw + in->sweep) & 0x0FFF);
    poke2((uint8_t)(r + 2), (uint8_t)m->pw);
    poke2((uint8_t)(r + 3), (uint8_t)(m->pw >> 8));
  }
}

static void start(uint8_t n) {
  song = n;
  tempo = SONGS[n].tempo;
  tick = 0;
  active = 3;
  for (uint8_t v = 0; v < 3; v++) {
    mvoice_t *m = &mv[v];
    m->ord = SONGS[n].order[v];
    m->p = MUS_PAT + MUS_OFS[*m->ord];
    m->left = 0; m->gate = 0; m->ins = 0; m->len = 1;
    poke2((uint8_t)(v * 7 + 4), 0);
    voice_step(v);
  }
}

void music_init(void) {
  for (uint8_t r = 0; r < 0x18; r++) poke2(r, 0);
  poke2(0x18, MUSIC_VOL);
}

void music_play(uint8_t n) {
  if (SONGS[n].loop) bg = n;
  if (n == song && SONGS[n].loop) return;    /* laeuft schon */
  start(n);
}

void music_mute(uint8_t on) { muted = on; poke2(0x18, on ? 0 : MUSIC_VOL); }
uint8_t music_muted(void) { return muted; }

void music_update(void) {
  if (song == 0xFF) return;
  if (!active) {                             /* Jingle zu Ende: Hintergrund von vorn */
    if (bg != 0xFF) start(bg); else song = 0xFF;
    return;
  }
  if (++tick >= tempo) {
    tick = 0;
    for (uint8_t v = 0; v < 3; v++) voice_step(v);
  }
  for (uint8_t v = 0; v < 3; v++) voice_fx(v);
}
