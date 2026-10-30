/*
 * hiscore.c – Highscore-Tabelle (v7.4): fuenf Eintraege mit drei Buchstaben
 *
 * Die Tabelle steht im Spiel im RAM und im Attic-RAM (bleibt beim Neustart des
 * Laders erhalten) und als Datei PB65HS auf der Diskette (Ladeadresse $0F00,
 * Kennung "HS1"). Der Lader laedt die Datei vor dem Spiel nach $0F00; das Spiel
 * uebernimmt sie beim Start. Gespeichert wird mit dem KERNAL: Ein kleines
 * Programm bei $0E00 stellt die Zeropage und $0800-$1FFF so her, wie der Lader
 * sie beim Start vorfand, ruft SAVE auf und stellt danach den Zustand des
 * Spiels wieder her.
 */
#include "hiscore.h"
#include "hw.h"
#include "table.h"

hs_entry_t hs[N_HS];

static const hs_entry_t HS_DEFAULT[N_HS] = {
  { {'C','6','4'}, 500000UL }, { {'V','I','C'}, 400000UL }, { {'S','I','D'}, 300000UL }, { {'P','L','A'}, 200000UL }, { {'C','I','A'}, 100000UL },
};
#define HS_BYTES (3 + N_HS * sizeof(hs_entry_t))

static uint8_t hs_valid(const uint8_t *p) { return p[0] == 'H' && p[1] == 'S' && p[2] == '1'; }

static void hs_to(uint8_t *p) {
  p[0] = 'H'; p[1] = 'S'; p[2] = '1';
  const uint8_t *q = (const uint8_t *)hs;
  for (uint8_t i = 0; i < N_HS * sizeof(hs_entry_t); i++) p[3 + i] = q[i];
}

static void hs_from(const uint8_t *p) {
  uint8_t *q = (uint8_t *)hs;
  for (uint8_t i = 0; i < N_HS * sizeof(hs_entry_t); i++) q[i] = p[3 + i];
}

HICODE void hs_init(void) {
#ifdef __mos__
  const uint8_t *f = (const uint8_t *)HS_LOAD;            /* vom Lader geladen? */
  if (hs_valid(f)) hs_from(f);
  else
#endif
  {
    dma_copy(ATTIC_HS, CPUADDR(COLBUF), HS_BYTES);         /* sonst die Tabelle seit dem Einschalten */
    if (hs_valid(COLBUF)) hs_from(COLBUF);
    else for (uint8_t i = 0; i < N_HS; i++) hs[i] = HS_DEFAULT[i];
  }
  hs_to(COLBUF);
  dma_copy(CPUADDR(COLBUF), ATTIC_HS, HS_BYTES);
}

HICODE uint8_t hs_rank(uint32_t score) {
  uint8_t r = 0;
  while (r < N_HS && hs[r].score >= score) r++;
  return r;
}

HICODE void hs_insert(uint8_t rank, const char *name, uint32_t score) {
  for (uint8_t i = N_HS - 1; i > rank; i--) hs[i] = hs[i - 1];
  for (uint8_t k = 0; k < 3; k++) hs[rank].name[k] = name[k];
  hs[rank].score = score;
}


HICODE void hs_save(void) {
  hs_to(COLBUF);
  dma_copy(CPUADDR(COLBUF), ATTIC_HS, HS_BYTES);
#ifdef __mos__
  dma_copy(LOWRAM_SAVE, 0x0800u, 0x1800u);                 /* KERNAL-Arbeitsspeicher wie beim Start */
  dma_copy(ATTIC_KZP, 0x0D00u, 256);
  hs_to((uint8_t *)HS_LOAD);                               /* Dateiinhalt bei $0F00 */
  dma_copy(ATTIC_SAVESTUB, 0x0E00u, SAVE_STUB_LEN);        /* vom Lader abgelegt (boot.c) */
  /* v7.7: Das Speicher-Programm blendet das KERNAL bei $E000 ein und danach wieder aus;
   * Konstanten und Variablen des Spiels dort sind solange unsichtbar (nicht benutzen). */
  ((uint8_t *)0x0E00u)[SAVE_STUB_X] = (uint8_t)HS_BYTES;   /* Ende = $0F00 + HS_BYTES */
  asm volatile("jsr $0E00" ::: "a", "x", "y", "memory");
  tick_init();                                             /* falls das KERNAL CIA 2 umgestellt hat */
  mask_load(win_y0);                                       /* v7.5: Maskenfenster war ueberschrieben */
#endif
}
