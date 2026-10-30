/*
 * game.h – Spielregeln Meilenstein 3: Punkte, Baelle, Lampen, Bonus, Meldungen
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "physics.h"

#define BALLS_PER_GAME 3

extern uint32_t score;
extern uint8_t ball_no, mult, game_over;
extern uint16_t bonus;          /* Bonuspunkte in Tausend, vor Multiplikator */
extern uint8_t ramps;
extern uint16_t tgt_groups_done;           /* Rampen in diesem Ball */

/* Missionen (v5.5): C 6 4 komplett -> bereit, Kick-Out-Hole startet die naechste */
#define N_MISSIONS    4
#define MISSION_TIME  3000      /* 60 Sekunden (laeuft nicht, solange der Ball im Kick-Out-Hole liegt) */
extern uint8_t mission_ready, mission_active, mission_cur, mission_prog, missions_done;
extern uint8_t mb_ready;
extern uint8_t cia_lit, save_ready;          /* v7.3: CIA 1/2 und Ball-Save */                  /* Multiball RUN/STOP bereit (Rampe startet ihn) */
extern uint16_t mission_timer;
/* Schreibt Name, Fortschritt und Restzeit der laufenden Mission ("FORMAT 1/3 38") */
char *mission_status(char *p);

/* Meldung im Display (Zeile 1 und 2), solange msg_timer > 0 */
extern char msg1[], msg2[];
extern uint8_t msg_timer, msg_type;   /* msg_type 1: Meldung wird getippt, 2: faehrt herein (v7.7) */
extern uint8_t msg_live;              /* v7.7: Meldung aendert sich jedes Bild (Bonus zaehlt) */
extern uint8_t hs_entry, hs_rank_new;  /* v7.4: Namenseingabe (1..3 = Buchstabe), Platz */
extern char hs_name[4];

void game_new(void);
/* Vor game_frame: sperrt den Plunger nach Spielende, Plunger startet neu */
void game_input(input_t *in);
/* Nach game_frame: Ereignisse auswerten, Lampen und Klang */
void game_update(const input_t *in);

/* Zahl mit Tausenderpunkten (1.234.560) */
char *fmt_points(char *p, uint32_t v);

#endif
