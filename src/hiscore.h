/*
 * hiscore.h – Highscore-Tabelle mit Speichern auf Diskette (v7.4)
 */
#ifndef HISCORE_H
#define HISCORE_H

#include <stdint.h>

#define N_HS 5
typedef struct { char name[3]; uint32_t score; } hs_entry_t;
extern hs_entry_t hs[N_HS];

void hs_init(void);                 /* als Erstes in main: Datei vom Lader, Attic oder Vorgabe */
uint8_t hs_rank(uint32_t score);    /* Platz 0..4, N_HS = nicht in der Liste */
void hs_insert(uint8_t rank, const char *name, uint32_t score);
void hs_save(void);                 /* ins Attic-RAM und als PB65HS auf die Diskette */

#endif
