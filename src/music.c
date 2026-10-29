/*
 * music.c – Musik auf dem zweiten SID-Paar ($D420 und $D460), v6.9
 *
 * Drei Stimmen, jede mit eigener Pattern-Folge (music_data.c, erzeugt von
 * tools/gen_music.py). Ein Schritt dauert tempo Bilder. Kurze Stuecke
 * (Jingles) unterbrechen die Hintergrundmusik, danach beginnt sie neu.
 *
 * v7.4: Register erst in einer Kopie (sh) setzen und einmal pro Bild schreiben,
 * Stimmen als einzelne Felder statt Struktur: gleiche Musik, rund 1 KB kleiner.
 */
#include "music.h"
#include "music_data.h"
#include "hw.h"

static const uint8_t *m_ord[3], *m_p[3];   /* Folge und Leseposition im Pattern */
static uint8_t m_len[3], m_left[3], m_ins[3], m_note[3], m_gate[3], m_frames[3], m_fxc[3], m_arp[3];
static uint16_t m_freq[3], m_pw[3];
static uint8_t sh[21], retrig;             /* Registerkopie, Bit v: Huellkurve neu starten */
static uint8_t song = 0xFF, bg = 0xFF, tick, tempo, active, muted;
static const uint8_t ARP[2][3] = { { 0, 3, 7 }, { 0, 4, 7 } };

static uint16_t note_freq(uint8_t n) {
  uint8_t oct = 0;
  while (n >= 12) { n -= 12; oct++; }
  return NOTE6[n] >> (6 - oct);
}

static void set_reg16(uint8_t r, uint16_t f) { sh[r] = (uint8_t)f; sh[r + 1] = (uint8_t)(f >> 8); }

/* Naechstes Ereignis einer Stimme lesen (Note, Pause oder Halten) */
static void voice_step(uint8_t v) {
  if (!m_ord[v]) return;
  if (m_left[v] && --m_left[v]) return;
  const uint8_t *p = m_p[v];
  uint8_t r = (uint8_t)(v * 7);
  for (;;) {
    uint8_t b = *p++;
    if (b == 0xFF) {                          /* Pattern zu Ende: naechstes aus der Folge */
      const uint8_t *o = ++m_ord[v];
      if (*o == 0xFF) {
        if (!SONGS[song].loop) { m_ord[v] = 0; sh[r + 4] &= 0xFE; active--; return; }
        o = m_ord[v] = SONGS[song].order[v];
      }
      p = MUS_PAT + MUS_OFS[*o];
      continue;
    }
    if (b >= 0x80 && b < 0xFD) { m_ins[v] = (uint8_t)(b - 0x80); continue; }
    if (b >= 0x60 && b < 0x80) { m_len[v] = (uint8_t)(b - 0x5F); continue; }
    m_p[v] = p;
    m_left[v] = m_len[v];
    if (b == 0xFE) return;                    /* halten: Gate-Zeit wurde schon mitgezaehlt */
    /* Gate-Zeit: diese Note und folgende Halte-Schritte, ein Bild vor dem Ende aus */
    uint8_t l = m_len[v], tot = l;
    for (;;) {
      uint8_t c = *p;
      if (c >= 0x60 && c < 0x80) { l = (uint8_t)(c - 0x5F); p++; }
      else if (c == 0xFE) { tot = (uint8_t)(tot + l); p++; }
      else break;
    }
    m_frames[v] = (uint8_t)(tot * tempo - 1);
    const instr_t *in = &INSTR[m_ins[v]];
    if (b == 0xFD) { sh[r + 4] = in->wave; m_gate[v] = 0; return; }
    m_note[v] = b;
    m_freq[v] = note_freq(b);
    m_pw[v] = in->pw;
    m_fxc[v] = 0;
    m_arp[v] = 1;                            /* wie bisher: erstes Bild nach dem Anschlag = Stufe 1 */
    sh[r + 5] = in->ad;
    sh[r + 6] = in->sr;
    set_reg16((uint8_t)(r + 2), m_pw[v]);
    set_reg16(r, m_freq[v]);
    sh[r + 4] = (uint8_t)(in->wave | 1);
    retrig |= (uint8_t)(1 << v);
    m_gate[v] = 1;
    return;
  }
}

/* Pro Bild: Arpeggio, Rutschen, Vibrato, Pulsbreite, Gate */
static void voice_fx(uint8_t v) {
  if (!m_ord[v] || !m_gate[v]) return;
  const instr_t *in = &INSTR[m_ins[v]];
  uint8_t r = (uint8_t)(v * 7);
  if (m_frames[v] && --m_frames[v] == 0) { sh[r + 4] = in->wave; m_gate[v] = 0; return; }
  uint8_t c = ++m_fxc[v];
  uint16_t f = m_freq[v];
  if (in->fx == 1) {
    uint8_t k = m_arp[v];
    if (++m_arp[v] == 3) m_arp[v] = 0;
    f = note_freq((uint8_t)(m_note[v] + ARP[in->arg][k]));
  } else if (in->fx == 2) {
    uint16_t d = (uint16_t)in->arg << 4;
    f = m_freq[v] = f > d ? (uint16_t)(f - d) : 0;
  } else if (in->fx == 3 && c > in->arg) {
    uint16_t d = f >> 8;
    f = (c & 4) ? (uint16_t)(f - d) : (uint16_t)(f + d);
  }
  set_reg16(r, f);
  if (in->sweep) {
    m_pw[v] = (uint16_t)((m_pw[v] + in->sweep) & 0x0FFF);
    set_reg16((uint8_t)(r + 2), m_pw[v]);
  }
}

static void flush(void) {
  for (uint8_t v = 0; v < 3; v++)             /* neu angeschlagen: erst Gate aus */
    if (retrig & (1 << v)) sid2_poke((uint8_t)(v * 7 + 4), (uint8_t)(sh[v * 7 + 4] & 0xFE));
  retrig = 0;
  for (uint8_t r = 0; r < 21; r++) sid2_poke(r, sh[r]);
}

static void start(uint8_t n) {
  song = n;
  tempo = SONGS[n].tempo;
  tick = 0;
  active = 3;
  for (uint8_t v = 0; v < 3; v++) {
    const uint8_t *o = m_ord[v] = SONGS[n].order[v];
    m_p[v] = MUS_PAT + MUS_OFS[*o];
    m_left[v] = 0; m_gate[v] = 0; m_ins[v] = 0; m_len[v] = 1;
    sh[v * 7 + 4] = 0;
    voice_step(v);
  }
  flush();
}

void music_init(void) {
  for (uint8_t r = 0; r < 0x18; r++) sid2_poke(r, 0);
  sid2_poke(0x18, MUSIC_VOL);
}

void music_play(uint8_t n) {
  if (SONGS[n].loop) bg = n;
  if (n == song && SONGS[n].loop) return;    /* laeuft schon */
  start(n);
}

void music_stop(void) {
  song = bg = 0xFF;
  for (uint8_t v = 0; v < 3; v++) { m_ord[v] = 0; sh[v * 7 + 4] = 0; }
  flush();
}

void music_mute(uint8_t on) { muted = on; sid2_poke(0x18, on ? 0 : MUSIC_VOL); }
uint8_t music_muted(void) { return muted; }

void music_update(void) {
  if (song == 0xFF) return;
  if (!active) {                             /* Jingle zu Ende: Hintergrund von vorn */
    if (bg != 0xFF) start(bg); else { song = 0xFF; flush(); }
    return;
  }
  if (++tick >= tempo) {
    tick = 0;
    for (uint8_t v = 0; v < 3; v++) voice_step(v);
  }
  for (uint8_t v = 0; v < 3; v++) voice_fx(v);
  flush();
}
