# Pinball65

Eine Flippersimulation für den MEGA65 im Stil von Pinball Fantasies, mit einem Tisch rund um Retro-Computer. Privates Hobbyprojekt, geschrieben in C mit [llvm-mos](https://llvm-mos.org).

## Stand

| Meilenstein | Inhalt | Status |
|---|---|---|
| Prototyp | Physik im Browser, Festkomma 16.16, einstellbare Werte | fertig, siehe `prototype/` |
| 0 | Toolchain- und Hardware-Test: Build, D81, Hardware-Multiplizierer | bestanden (MEGA65 und Xemu, 23.10.2026) |
| 1 | Statischer Tisch, Ball, zwei Flipper, Kollisionsmaske | bestanden (24.10.2026), aktuell v1.2 |
| 2 | Ganzer Tisch 320 × 640 mit Scrolling, Display oben, Platzhaltergrafik nach Entwurf | bestanden (25.10.2026), v2.10 |
| 3 | Bumper, Slingshots, Targets, Lanes, dritter Flipper, Punkte, Lampen, erste Effekte | bestanden (25.10.2026), v3.0 |
| 4 | Rampe mit Brücke, Spinner, Kick-Out-Hole | bestanden (26.10.2026), v4.3 |
| 5 | Regeln, Missionen, Multiball, Display-Animationen, SID-Musik | abgeschlossen (v7.0) |
| 6 | Grafik-Werkzeug, endgültige Grafik, Highscore | läuft; v7.3 zum Test (Bumper, Tasten, CIA und Ball-Save) |

## Testumgebung

Jeder Meilenstein wird auf echter Hardware geprüft, nicht nur im Emulator, weil sich Xemu und der MEGA65 in Details unterscheiden.

- MEGA65 mit Core **0.97 (20250422.09)**, ROM **920413**
- Übertragung: D81-Image per **M65Connect**
- Emulator für schnelle Zwischentests: **Xemu** (mit demselben ROM 920413)

## Testergebnisse

| Datum | Test | MEGA65 (Core 0.97) | Xemu |
|---|---|---|---|
| 23.10.2026 | Meilenstein 0 v1 | Absturz: `POKE 0,65` blendet KERNAL aus | Absturz |
| 23.10.2026 | Meilenstein 0 v2: 2000 × fmul, `1.5 * -2.25` | grün, 0 Abweichungen, -221184 | grün, 0 Abweichungen, -221184 |
| 24.10.2026 | Meilenstein 1: Testabschuss `T` | `X:07652669 Y:30380675`, L = 100 | identisch |
| 24.10.2026 | Meilenstein 1.1: Testabschuss `T` | `X:13317277 Y:19434387`, L = 124 | identisch |
| 24.10.2026 | Meilenstein 1.2: Testabschuss `T` | `X:17539107 Y:38809859`, L = 112 | identisch |
| 24.10.2026 | Meilenstein 2.0 | Absturz | Absturz bei PC=$956E |
| 24.10.2026 | Meilenstein 2.1 (`-Oz`) | – | hängt beim Aufbau, hellblauer Rahmen, PC um $8A4E (Software-Multiplikation) |
| 24.10.2026 | Meilenstein 2.2 (Diagnose) | bleibt bei `GRAFIK 15/80` stehen, Prüfsumme ok | identisch |
| 24.10.2026 | Meilenstein 2.3: Testabschuss `T` | `TEST X 07457637 Y 21572962`, L meist 160–200, Spitzen bis 307 | identisch |
| 25.10.2026 | LOADTEST ($2001–$C648) | OK, alle Bytes richtig | OK, alle Bytes richtig |
| 25.10.2026 | Meilenstein 2.4 | Testabschuss richtig; Display: untere Textzeile wird nicht erneuert, L gelegentlich über 300 | identisch |
| 25.10.2026 | Meilenstein 2.5 | Ball ruht: L 91, MAX 95/171/218; im Spiel L meist 90–150, kurze Spitzen bis 306 | – |
| 25.10.2026 | Meilenstein 2.6 | Abschussbahn L 111, MAX 130; Spiel MAX 622, SCROLL MAX 59 | Abschussbahn L 109, MAX 129; Spiel MAX 312, SCROLL MAX 71 |
| 25.10.2026 | Meilenstein 2.8 | MAX meist darunter, bis 431 | – |
| 25.10.2026 | Meilenstein 2.9 | Ball am Flipper: L 175, MAX 345; P 18, S 59, Z 243, D 25 – Zeichnen ist der Engpass | – |
| 25.10.2026 | Meilenstein 2.10 | Abschussbahn L 103, MAX 126, `DMA`; Spiel L 236, MAX 243; P 71, S 60, Z 56, D 56; Bild in Ordnung | Abschussbahn L 104, MAX 127, `DMA`; Ball am Flipper L 232, MAX 208; P 52, S 101, Z 30, D 25; Bild in Ordnung |
| 25.10.2026 | Meilenstein 3.1 (Lader) | – | hängt im KERNAL bei PC=$E65A beim Nachladen |
| 25.10.2026 | Meilenstein 3.2 (Lader) | – | Rahmen weiß (= Farbe 5 unserer Palette, also im Laden), hängt wieder bei $E65A |
| 25.10.2026 | Meilenstein 3.3 (Lader) | – | Rahmen grün, hängt weiter bei $E65A |
| 27.10.2026 | Meilenstein 5.6 | sehr viele Bälle direkt nach dem Abschuss durchs Outhole; Wunsch: 6510 zurück | – |
| 27.10.2026 | Meilenstein 5.5 | Missionen in über 30 Spielen kein einziges Mal ausgelöst; C 6 4 und Kick-Out-Hole schwer zu erreichen | – |
| 27.10.2026 | Meilenstein 5.4 | spielt sich sehr viel besser | – |
| 27.10.2026 | Meilenstein 5.3 | PLA ok; Rampe zu schwer, Out-Lane-Zugang zu groß | – |
| 27.10.2026 | Meilenstein 5.2 | Seiten ausgewogen; Ball konnte aus der linken Out-Lane zurück auf den Tisch fliegen; Wunsch: PLA-Targets | – |
| 27.10.2026 | Meilenstein 5.1 | MAX bis 234, Spielszene L098 MAX184 P080 R002 S061 Z009 T000 D026 C005; L/D gut, Display flüssig, Rampe erreichbar; linke Out-Lane kaum, rechte zu leicht erreichbar | – |
| 26.10.2026 | Meilenstein 5.0 | MAX bis 248; z. B. L126 MAX235 P080 R002 S060 Z031 T009 D029 C024; L und D gut zu treffen, aber L versperrt die Rampe | – |
| 26.10.2026 | Meilenstein 4.3 | Ball bleibt nirgends liegen; Rampe mit gutem Timing zu treffen; Kick-Out-Hole genau richtig; MAX bis 263 | – |
| 26.10.2026 | Meilenstein 4.2 | Testabschuss richtig; Ball bleibt auf dem Drehpunkt des dritten Flippers liegen; Wunsch: 1541 links, Rampenende weiter rechts über dem dritten Flipper | – |
| 25.10.2026 | Meilenstein 4.1 | Rampe nie getroffen; Wunsch: Trichter, VIC-II gegenüber SID, 6510 links vom geraden Rampenteil | – |
| 25.10.2026 | Meilenstein 4.0 | Rampe nicht zu erreichen (Einfahrt senkrecht), MAX bis 243 | – |
| 25.10.2026 | Meilenstein 3.4 (Lader) | Nachladen funktioniert | Nachladen funktioniert |
| 25.10.2026 | Meilenstein 3.0 | Effekte zu hören, Lampen richtig, dritter Flipper gut spielbar, MAX bis 258 | – |
| 25.10.2026 | Meilenstein 2.7 | L 499, MAX 452, UEB 49; P 9, S 59, Z 15, D 57 – Überlauf-Meldung falsch, echte Last höchstens etwa 190 Zeilen | – |

Erkenntnisse:
- Nicht nach `$00` schreiben, um 40 MHz einzuschalten. Im C65-Modus ist das bereits der Standard.
- llvm-mos belegt die Zeropage von BASIC, daher kehrt das Programm nicht ins BASIC zurück.
- **KERNAL aus eigenen Programmen aufrufen** (v3.1–3.4): Das KERNAL braucht seine ganze Zeropage, den Bereich $1000–$15FF, Z = 0 und den DMA-Baustein im F018A-Format. Erst als der Lader Zeropage und $0800–$1FFF sicherte und vor dem LOAD zurückschrieb, lief das Nachladen (Hinweis auf $1000–$15FF aus dem Vyper-Projekt).
- Der Hardware-Multiplizierer liefert sofort korrekte Ergebnisse, ohne Warten.
- Die Physik rechnet auf MEGA65, Xemu und PC bitgleich. Damit lassen sich Physikänderungen im PC-Test vorab prüfen.
- Ein Bild braucht etwa 100 von 312 Rasterzeilen, also rund ein Drittel der verfügbaren Zeit.
- v1.1 brauchte 124 Zeilen, weil die einstellbaren Werte in jedem Substep neu verrechnet wurden, darunter eine 32-Bit-Division in Software. Seit v1.2 werden abgeleitete Werte nur bei einer Änderung berechnet.
- Im Einstellmodus ermittelt (24.10.2026): FLIPPER SPEED 1100 und WALL BOUNCE 80 fühlen sich gut an. Seit v1.2 sind das die Startwerte.
- **Übersetzt wird für den reinen 6502 (`-mcpu=mos6502`).** Mit den 45GS02-Erweiterungen erzeugte llvm-mos (Version cd91455) in `band_disc` einen `inw`-Befehl auf das falsche Register: Der Schleifenzähler blieb stehen, der Aufbau hing bei `GRAFIK 15/80` (Diagnoseversion 2.2). Der 6502-Simulator zeigte den Fehler nicht, weil er für den 6502 übersetzt. Mit `-mcpu=mos6502` prüft er jetzt denselben Befehlssatz, der auf dem MEGA65 läuft. Den 65C02-Befehlssatz meiden wir ebenfalls: Seine `(zp)`-Befehle bedeuten auf dem 45GS02 `(zp),Z` und funktionieren nur, solange Z = 0 ist.
- **Keine $A000-Grenze.** Die Vermutung, bei $A000–$BFFF liege beim Laden das BASIC-ROM, war falsch: `LOADTEST` lädt bis $C648 auf Xemu und MEGA65 fehlerfrei. Der Absturz von Version 2.0 kam vom `inw`-Fehler. Die eigentliche Grenze ist der Speicher bis $D000 (Code, Variablen, 1 KB Stack); mit `-Os` wäre das Programm 5,8 KB zu groß, deshalb bleibt es bei `-Oz`.
- **Überlauf-Messung (v2.6/2.7) war falsch:** Das Raster-Vergleichsflag ($D019) löste auf dem VIC-IV nicht bei VIC-II-Zeile 251 aus, sondern offenbar mitten im Bild (vermutlich physische Zeile 251 von 576). Bilder, die länger als etwa 125 Zeilen dauerten, galten deshalb als übergelaufen (+312). Seit v2.8 ist L die Summe der einzeln gemessenen Abschnitte. Tatsächliche Höchstlast im Spiel: etwa 190 von 312 Zeilen.
- **Spitzen im Ruhezustand (v2.5):** Eine Display-Zeichenzeile kostete rund 75 Rasterzeilen (640 Punkte einzeln), die Code-Prüfsumme alle 5 Sekunden prüfte 33 KB in einem Bild. Seit v2.6 kopiert DMA eine fertige leere Punktzeile, die CPU setzt nur die leuchtenden Punkte (im Simulator rund 29.000 statt 170.000 Zyklen pro Zeichenzeile, Ergebnis byte-identisch); die Prüfsumme läuft mit 512 Byte pro Bild. Bilder über 312 Zeilen erkennt das Programm am Raster-Vergleichsflag ($D019), vorher lief der Messwert dabei über und zeigte zu kleine Zahlen.
- **Display:** Seit v2.4 wird pro Bild höchstens eine Zeichenzeile neu gezeichnet. In v2.4 wurde das Display aber nur alle 10 Bilder aufgerufen, und die obere Zeile (mit dem L-Wert) kam immer zuerst dran, so dass die untere Zeile verhungerte. Seit v2.5 läuft es in jedem Bild und reihum.
- **Laufzeit (v2.4):** Im 6502-Simulator gemessen kosteten die Flipper rund 830.000 Zyklen pro Bild (Randprüfung über Funktionsaufrufe für jeden Pixel), das Display alle 10 Bilder auf einmal rund 600.000. Jetzt: Innenbereich und Rand der Flipper vorberechnet, ruhende Flipper werden nicht neu gezeichnet, das Display zeichnet höchstens eine Zeichenzeile pro Bild, die Kollisionsprüfung entfällt, wenn um den Ball keine Wand liegt. Ergebnis im Simulator: von etwa 490.000 auf 208.000 Zyklen pro Bild (ohne Multiplikation und DMA), Bild und Physik unverändert.
- **Flipper und Ball per DMA (v2.10):** In v2.9 kostete das Zeichnen mit der CPU bis zu 243 Rasterzeilen, sobald der Ball an einem Flipper lag. Jetzt werden beim Start alle 25 Winkelstellungen beider Flipper und 8 seitlich verschobene Fassungen des Balls einmal gezeichnet und als Spaltenstreifen im Attic-RAM abgelegt. Im Spiel setzt ein DMA-Auftrag mit Transparenz (Wert 0 wird nicht geschrieben) einen Streifen in den Bildpuffer, ein Flipper braucht höchstens 8 Aufträge, der Ball 3. Ob die DMA-Transparenz funktioniert, prüft das Programm beim Start selbst; sonst zeichnet es wie bisher mit der CPU. Die Abschussbahn zeigt den aktiven Weg (`DMA` oder `CPU`). Die Flipperspannen liegen jetzt ebenfalls im Attic-RAM, das spart 4,4 KB im Hauptspeicher. Im PC-Test sind beide Wege pixelgleich mit v2.9. Auf dem MEGA65 sank Z am Flipper von 243 auf 56 Rasterzeilen, die Höchstlast im Spiel liegt bei 243 von 312.
- Rückmeldung zum Spielgefühl: Flipper zu langsam, Ball prallt viel ab und kommt selten herunter. Ursachen: Ein Flipperschlag dauerte etwa 85 ms (echte Flipper 30 bis 50 ms); der Ball ist rund zwei Drittel der Zeit oberhalb des sichtbaren Ausschnitts; ein zu schwacher Abschuss ließ den Ball auf dem Plunger liegen, ohne dass er neu abgeschossen werden konnte (behoben in v1.1).

## Einrichtung unter Windows 11

1. **llvm-mos SDK**: Von den [llvm-mos-sdk Releases](https://github.com/llvm-mos/llvm-mos-sdk/releases) die Datei `llvm-mos-windows.7z` laden, zum Beispiel nach `C:\llvm-mos` entpacken und `C:\llvm-mos\bin` zur Umgebungsvariable `PATH` hinzufügen. Test in einer neuen Eingabeaufforderung: `mos-mega65-clang --version`
2. **Python 3** (für das D81-Werkzeug): über [python.org](https://www.python.org/downloads/) oder `winget install Python.Python.3.12`. Zusätzliche Pakete sind nicht nötig.
3. **Git**: über [git-scm.com](https://git-scm.com/download/win) oder `winget install Git.Git`, dann `git clone https://github.com/Niederrheiner-KKB74/Pinball65`
4. **Xemu** (optional): Windows-Version von der [Xemu-Seite](https://github.lgb.hu/xemu/) laden.

## Bauen

Unter Windows im Projektordner:

```
build.bat
```

Unter Linux oder macOS (oder Windows mit `make`):

```
make
```

Ergebnis: `build\pinball65.d81` mit den Programmen `PINBALL65` (Lader), `PB65GAME` (Spiel) und `LOADTEST`.

**Lader und Spiel (seit v3.1):** `PINBALL65` baut Kollisionsmaske, Tischgrafik und die vorgezeichneten Flipper im Attic-RAM auf, setzt dort eine Kennung und lädt dann mit dem KERNAL das Spiel `PB65GAME` nach. Ein kleines Maschinenprogramm bei `$0E00` übernimmt das Laden, weil das Spiel denselben Speicher ab `$2001` belegt wie der Lader. So braucht das Spiel den Aufbau-Code (rund 11 KB) nicht mehr; es endet jetzt bei `$9863` statt `$C265`. Wird `PB65GAME` ohne den Lader gestartet, meldet es `?FILE NOT FOUND  ERROR` und `RUN"PINBALL65"`. Schlägt das Nachladen fehl, blinkt der Rahmen. Während des Ladens ist der Rahmen grün. Vor dem Laden stellt das Maschinenprogramm den DMA-Baustein auf das alte F018A-Format zurück, weil das Diskettensystem des C65 dieses benutzt (v3.1 hing mit F018B im KERNAL). Außerdem sichert der Lader beim Start, noch vor dem C-Startcode, die Zeropage und stellt sie vor dem Laden wieder her: Das MEGA65-KERNAL nutzt die ganze Zeropage, llvm-mos überschreibt aber $02–$8F (v3.2 hing deshalb weiter bei $E65A). Seit v3.4 sichert der Lader auch $0800–$1FFF (C65-Bildschirm und Arbeitsspeicher des KERNAL bei $1000–$15FF, Hinweis aus dem Vyper-Projekt), weil er dort seine Puffer hat, und setzt vor dem Aufruf Z = 0.

## Auf dem MEGA65 starten

1. `build\pinball65.d81` mit M65Connect übertragen und als Laufwerk 8 einbinden.
2. Am MEGA65 eingeben: `RUN "PINBALL65"`

In Xemu: `xmega65 -8 build\pinball65.d81`, danach ebenfalls `RUN "PINBALL65"`.

### Meilenstein 5: Bedienung und Test

Plan (abgestimmt am 26.10.2026): v5.0 Tischänderung L/D und Messung, v5.1 Missionen, v5.2 Multiball „RUN/STOP“, v5.3 Display-Animationen, SID-Musik, Highscore.

Missionen: Die oberen Lanes C 6 4 komplett machen `MISSION READY`, das Kick-Out-Hole startet die nächste Mission (45 Sekunden): „FORMAT DISK“ (dreimal Kick-Out-Hole), „SYS 64738“ (dreimal Rampe), „PEEK & POKE“ (Kick-Out-Hole und Spinner im Wechsel), „LOAD ERROR“ (L O A D komplett). Multiball, wenn VIC-II, 6510 und SID je einmal komplett waren.

Neu in v5.0 und v5.1:
- **L O A D**: L und D sind seit v5.1 Roll-Over-Kontakte auf dem Tisch (x 97 und 223, y 379): Der Ball rollt darüber und zählt beim Hineinrollen. In v5.0 waren es feste Targets, das linke versperrte den Weg zur Rampe. O und A bleiben die Inlanes. Die Out-Lanes zählen nicht mehr, L O A D ist ohne Ballverlust zu schaffen. Die L O A D-Lampen wandern nicht mehr mit den Flipperknöpfen (nur noch C 6 4).
- **Rampenende**: Der Ball fällt jetzt senkrecht auf den dritten Flipper und rollt über dessen Spitze nach links unten zum linken Flipper (mit dem schrägen Abwurf aus v4.3 hätte er das D-Target getroffen und wäre in die rechte Out-Lane gesprungen).
- **Messwerte (Taste D)**: feiner aufgeteilt. Zeile 1: `L`, `MAX`, `P` Physik (Eingabe, Bewegung, Kamera), `R` Regeln (Punkte, Lampen, Klang). Zeile 2: `S` Hintergrund, `Z` Flipper und Ball, `T` Displaytext, `D` Display zeichnen, `C` Prüfsumme. Damit lässt sich vor dem Multiball sehen, wo die Zeit hingeht.
- **v7.3**: Pop-Bumper plastisch (Schatten, leuchtender Kragen, Chromring, gewölbte Kappe mit Glanzpunkt, Aufschrift PEEK/POKE/SYS). Quadratische Tasten RUN/STOP und CLR/HOME in den Out-Lanes (Oberseite = Lampe C_SAVE). Roll-Over CIA 1 und CIA 2 im Keil hinter VIC-II und SID (beide Richtungen, 2.000 Punkte); beide an = IRQ (10.000) und Ball-Save: die nächste Kugel, die über eine Out-Lane verloren geht, kommt zurück (kein Bonus, Ball zählt nicht, Mission läuft weiter), die Tasten blinken solange. Geprüft und verworfen: VIC-II/SID an den Rand mit Dach dahinter (Spielroboter: Target-Gruppen halbiert, Out-Lanes +60 %, Querschüsse liefen unter den Targets in die Out-Lane). Heatmap des Spielroboters: `HEAT=1 BOT=n ./hosttest` schreibt heat.csv. Speicher: ohne 8-Bit-Divisionen, Prüfsumme nur noch mit -DCODE_CHECK; Heap ab $CE20.
- **v7.2**: Plunger sichtbar (rote Kappe, Stange, Feder; geht beim Ziehen mit nach unten, Bild aus dem Attic-RAM, eine Spalte). Tor oben quer über der Abschussbahn (290,140)–(301,132), Klasse M_GATE_UP: nur für einen Ball in der Bahn nach oben durchlässig; offen = unsichtbar, nach dem Abschuss zu (Chrom, Paletteneintrag C_GATE). Ein Ball aus dem Orbit rollt darauf nach links zum dritten Flipper; das bisherige seitliche Tor und das Ablenken in der Bahn entfallen, ein zu schwacher Abschuss rollt wieder auf den Plunger. 1541 als 5,25-Zoll-Diskette (Hülle, Etikett „1541“, Schreibschutzkerbe, Ring ums Mittelloch, Schlitz für den Lesekopf, Indexloch). Testabschuss (Taste T) nur noch in den Tests, Abschuss ohne 32-Bit-Division; Heap ab $CD3A.
- **v7.1, Tischgrafik (Meilenstein 6)**: Der Lader malt den Tisch neu: Holzleisten, C64-Einschaltbild als Bildschirm hinter Lanes und Bumpern (Rasterlinien, „**** PINBALL 65 BASIC V2 ****“, „38911 BASIC BYTES FREE“, „READY.“), Regenbogenstreifen, Platine mit Leiterbahnen, Chips VIC-II, 6510, SID und PLA neben ihren Targets, Laufwerk 1541 um das Kick-Out-Hole, Tasten RUN/STOP und RESTORE in den Out-Lanes. Chrom silbern, Bumper-Kappen cremefarben. Targets mit dunkler Fuge zwischen den einzelnen Targets (6510: vier, VIC-II, SID, PLA: drei). 46 Grafikfarben ab Palette 96 (C_ART); der Lader legt sie im Attic-RAM ab (ATTIC_PAL), das Spiel setzt sie von dort. Kollisionsmaske unverändert. Entwurf: tools/art_mockup.py.
- **v7.0**: Hardwaretest v6.9: alle vier SIDs hörbar, Musik gefällt, die Spielmusik wirkte aber zu langsam. Im Spiel läuft jetzt keine Hintergrundmusik mehr (`music_stop` bei Spielbeginn); Effekte, Jingles (Fanfare, Multiball, Spielende) und die Titelmusik im Wartebildschirm bleiben.
- **v6.9, SID-Musik und Effekte**: Musik auf dem zweiten SID-Paar ($D420/$D460, Lautstärke 10), Effekte weiter auf $D400/$D440 (15). Eigene Stücke (tools/gen_music.py, Akkordfolge Am–G–F–E): Spielmusik (Bass, Arpeggio-Akkorde, Schlagzeug), Titelmusik mit Melodie im Wartebildschirm, Jingles für Erfolge (Fanfare), Multiball und Spielende. Player music.c: drei Stimmen mit eigener Pattern-Folge, Instrumente mit Arpeggio, Rutschen (Schlagzeug), Vibrato, Pulsbreiten-Sweep. Effekte jetzt aus mehreren Schritten (Bumper Knall + Ton, Lanes und Gruppen als Tonfolgen, Rampe mit Klingeln). Taste M: Musik an/aus. Probehören am PC: `MUSIC=1 ./hosttest` schreibt die SID-Register je Bild, `tools/sid_wav.py` macht daraus eine WAV-Datei (grobe Nachbildung). Speicher: Einstellung F1–F7 nur noch mit -DTUNE_UI (feste Werte im Spiel, Physik unverändert), Ballzustand als Struktur (phys_swap kürzer); Heap ab $CDEC. L und MAX im Display nur noch mit Taste D.
- **v6.8, Pfeil-Lampen**: acht Pfeile vor den Schüssen (Rampe, 1541, Orbit links und rechts, 6510, VIC-II, SID, PLA), in der Tischgrafik als Dreieck mit dunklem Rand, Lampenfarben C_ARROW0 + ARW_*. Aus = gedimmt, an = lohnt sich, blinkt = lohnt sich jetzt besonders: Rampe bei RUN/STOP READY und für Missionen (SYS 64738, Spinner bei PEEK & POKE, D bei LOAD ERROR), 1541 bei MISSION READY, im Multiball (Jackpot) und für FORMAT DISK / PEEK & POKE, Orbits solange L fehlt (blinkt bei LOAD ERROR), Target-Gruppen solange sie für RUN/STOP fehlen oder angefangen sind (blinkt beim letzten Target), PLA angefangen / letztes Target. Nur Palette.
- **v6.7**: Hardwaretest v6.6: Display-Animationen gelungen, MAX ohne Multiball höchstens 232; die Rahmeneffekte störten. Rahmenfarben und Bandstreifen entfernt. Multiball-Start invertiert das Display schnell (alle 5 Bilder), Jackpot, MISSION COMPLETE, PRESS PLAY ON TAPE und NEW HIGH SCORE invertieren es viermal (alle 8 Bilder); nur Palette. Ballverlust flackert weiter im Display.
- **v6.6, Display-Animationen**: (1) Tipp-Effekt mit blinkendem Cursor-Block für „SEARCHING FOR * / LOADING“, Missionsnamen, „RUN/STOP READY“, „?TIME OUT ERROR / READY.“, „BALL SAVED“ und „BALL n READY.“ vor dem Abschuss (ein Zeichen je zwei Bilder, nur wenn Zeit ist; die Zeile bleibt dabei an ihrer Stelle). (2) Multiball-Start: „RUN/STOP / MULTIBALL“ blinkt, der Rahmen wechselt durch die 16 C64-Farben. (3) Jackpot, MISSION COMPLETE, PRESS PLAY ON TAPE und NEW HIGH SCORE: Display pulsiert, Rahmen gestreift wie beim Laden vom Band (in der Wartezeit bis zum Bildende, kostet keine Rechenzeit). (4) Ballverlust: „?BALL LOST ERROR“ flackert, darunter der Bonus. (5) Nach Spielende wechseln alle 3 s: Ergebnis, HIGH SCORE, PRESS FIRE, LOAD"*",8,1 / READY. Highscore bleibt bis zum Ausschalten (Speichern: Meilenstein 6). Messwerte vor dem Abschuss nur noch mit Taste D. C64-Farben auf Palette 240–255.
- **v6.5** (am 28.10.2026 auf dem MEGA65 bestätigt: „perfekt spielbar“): Hardwaretest v6.4: im Multiball MAX bis 376 (Foto: L190 MAX348), gut spielbar, aber die Punkteanzeige zeigte zerrissene Zeichen. Ursache: Das Display zeichnete je Bild nur eine Zeichenzeile, obere und untere Hälfte einer Textzeile kamen in verschiedenen Bildern dran; fehlte die Zeit, standen dort verschiedene Texte. Jetzt wird eine Textzeile immer ganz (beide Hälften) in einem Bild gezeichnet, und nur, wenn bis Rasterzeile 190 noch Zeit ist (SPARE_DMD); sonst bleibt der alte Text vollständig stehen.
- **v6.4, Multiball „RUN/STOP“**: bereit, wenn VIC-II, 6510 und SID je einmal komplett waren (Meldung, Rampenschienen blinken rot); die nächste Rampe startet einen zweiten Ball aus der Abschussbahn (Abschuss wie Zug 38, ganz um den Orbit). 10 s Ball-Save („RUN/STOP RESTORE“), 1541 im Multiball = JACKPOT 50.000, Multiball endet mit dem ersten verlorenen Ball. Taste RUN/STOP startet ihn zum Testen sofort. L zählt jetzt in beiden Richtungen, nur nicht direkt nach dem Abschuss.
  Technik: zweiter Ball als zweiter Physik-Zustand (`ball2`, `phys_swap`), Ball-Ball-Stoß, Kamera folgt dem tieferen Ball. Liegen die Bälle mehr als 24 Pixel auseinander, laufen erst alle Substeps des einen, dann des anderen (Maskenfenster sonst in jedem Substep neu geladen). Speicher: Sinus-Tabelle wird berechnet statt gespeichert (−1 KB), Rampenweg kompakt (−392 Byte), CPU-Zeichenweg ohne transparente DMA nur noch im Lader und in den Tests (−4 KB); Heap ab $BE3C. Spielroboter: Multiball etwa 0,2 je Spiel, im Mittel 15 s.
- **v6.3**: Rechte innere Orbitwand ganz entfernt (sie war in v6.2 grafisch durchgehend, obwohl der Ball durchging). Damit der Abschuss trotzdem oben herum läuft, ist der obere Bogen jetzt eine Führungsschiene (Klasse M_RAIL): der Ball gleitet daran entlang statt abzuprallen. PC-Test: dritter Flipper 5 von 40 Schlagzeitpunkten durch den Orbit, Abschuss Zug 33/34 in die Lanes, ab 36 ganz herum. Spielroboter je Spiel: Bumper 11,3, obere Lanes 4,9, Target-Gruppen 3,9, Rampe 1,9, 1541 1,6 (alle besser als v6.2).
- **v6.2**: Orbit auch von rechts nach links. Der untere Teil der rechten inneren Orbitwand (246,62)–(290,140) ist ein Einwegtor (Klasse M_GATE_UP): durchlässig, wenn der Ball nach oben und höchstens 0,375 Pixel pro Substep nach links läuft. So gehen Schüsse vom dritten Flipper in den Orbit, über C 6 4 und links hinunter; der Abschuss wird weiter geführt (ganz offene Wand: der Ball prallte vom Bogen ab, kein Skill-Shot mehr). PC-Test: dritter Flipper 3 von 40 Schlagzeitpunkten durch den Orbit (v6.1: 0), Abschüsse wie v6.1. Spielroboter je Spiel: Bumper 16,3 → 9,4, obere Lanes 5,2 → 3,4, Rampe 1,4, 1541 1,0; Ball verlässt den Bumperbereich jetzt auch nach rechts oben.
- **v6.1**: Orbit rundherum (Innenwände parallel zum oberen Bogen). Der Abschuss läuft die Bahn hoch und oben herum; mittlere Stärke fällt in C, 6 oder 4, voller Zug läuft links durch den Orbit zum rechten Flipper. Ein Orbit-Schuss vom rechten Flipper kommt rechts herunter und verlässt die Abschussbahn durch ein Tor (x 290, y 172–200) zum dritten Flipper; das Tor ist offen, solange der Ball dort nicht schnell nach oben läuft, physics.c lenkt ihn hinaus. Skill-Shot: vor dem Abschuss blinkt eine Lane (mit den Flippern verschiebbar), Treffer als erste Lane innerhalb von 5 s gibt 25.000. 1541 wirft senkrecht nach oben aus (VUK, 3,5 Pixel pro Substep) zu den Bumpern und Lanes. L im linken Orbit (21,200), D vor der Rampe (128,356), beide zählen nur aufwärts. Rampeneinfahrt hinten mit spitzem Dach geschlossen. Flipper, Slingshots, Inlanes, VIC-II und SID 28 Pixel tiefer (Flipper y 590). Inlane-Trennwände ab y 452. Spielroboter je Spiel (v6.0 → v6.1): Bumper 14,2 → 16,3, obere Lanes 3,8 → 5,2, Target-Gruppen 2,5 → 3,8, Rampe 2,6 → 1,4, 1541 3,1 → 1,1, Out-Lanes 17 % → 34 % der Ballverluste; nirgends liegen geblieben. Speicher: Heap ab $C3DE, rund 2 KB frei.
- **v6.0, Layout nach Schusslinien** (Grundlagen: Projektdokument „Flippertisch-Design-Grundlagen“): 1541 ist jetzt ein Scoop mit Trichter bei (179,266) am Ende des geraden Schusses vom rechten Flipper, Auswurf zum linken Flipper. Neuer linker Orbit (Einfahrt für Querschüsse des rechten Flippers), führt über C 6 4; schnelle Bälle laufen rechts zum dritten Flipper. Einwegtor oben in der Abschussbahn (Orbit-Bälle fallen nicht mehr in die Bahn). VIC-II und SID schräg, zum gegenüberliegenden Flipper gerichtet. Trennwände Inlane/Out-Lane 12 Pixel höher (ab y 416). Spielroboter, 4 Mio. Bilder (v5.7 → v6.0, je Spiel): Kick-Out 1,4 → 3,1, Rampe 1,5 → 2,6, Missionen geschafft 0,01 → 0,19, Target-Gruppen 1,6 → 2,5, Out-Lanes 1,0 → 0,5; nirgends liegen geblieben.
- **v5.7**: 6510-Targets wieder an der Stelle von v5.5. Kick-Out-Hole fängt im Umkreis von 6 Pixeln bis 4 Pixel pro Substep (v5.6: 8 und 5). Der Auswurf geht jetzt in die linke Inlane und von dort auf den linken Flipper; vorher traf der ausgeworfene Ball den linken Slingshot, der ihn mitten zwischen die Flipper warf.
- **v5.6, Missionen leichter erreichbar**: `MISSION READY` gibt es jetzt auch, wenn eine Target-Gruppe (VIC-II, 6510, SID, PLA) komplett ist, nicht nur über C 6 4. Das Kick-Out-Hole fängt im Umkreis von 8 statt 5 Pixeln und bis 5 statt 3 Pixel pro Substep. 6510-Targets höher und weiter rechts (x 226–250, y 128–166). Spielroboter: Missionsstarts 21 → 97 in rund 200 Spielen, Kick-Outs 1,3 → 2,1 pro Spiel. Messung zum dritten Flipper: Seine Schüsse gehen rechts senkrecht nach oben (x 260–300 bei y 200), die 6510-Targets lagen nicht im Weg.
- **Missionen (v5.5)**: Sind die oberen Lanes C 6 4 komplett, erscheint `MISSION READY` und der Ring des Kick-Out-Holes blinkt. Der nächste Ball im Kick-Out-Hole startet die nächste Mission (60 Sekunden, die Uhr steht, solange der Ball im Loch liegt): FORMAT DISK (dreimal Kick-Out-Hole), SYS 64738 (dreimal Rampe), PEEK & POKE (Spinner, Kick-Out-Hole, Spinner, Kick-Out-Hole), LOAD ERROR (L O A D komplett, beginnt von vorn). Im Display: Punkte und z. B. `FORMAT 1/3 42` (Fortschritt, Sekunden). Geschafft: `MISSION COMPLETE`, 50.000 Punkte plus 25.000 je bereits geschaffter Mission. Zeit abgelaufen: `?TIME OUT  ERROR`; Ballverlust beendet die Mission ebenfalls. Danach kommt die nächste Mission an die Reihe.
- **v5.4** (nach Mock-up vom 27.10.2026): Rampeneinfahrt senkrecht bei x 128, Mündung nach unten mit kleinem Trichter, Spinner quer darin. Dort fliegt der Standardschuss des linken Flippers hin (PC-Test: x 120–136); vom rechten Flipper traf die alte Einfahrt nur ein 0,1-Sekunden-Fenster. Der Weg biegt nach links ab und führt schräg auf den bisherigen Aufstieg; die Steigung bremst nur noch halb so stark. PLA-Targets höher (y 120–172). Kick-Out-Hole links unten auf der anderen Seite der Rampe (x 75, y 318), außerhalb der Bahn, auf der Bälle vom Orbit kommen; Auswurf nach unten zum linken Flipper. Trennwände Inlane/Out-Lane ab y 428 (vorher 444). Orbit-Abweiser ist eine feste Wand (ein Ball war durch das Einwegtor in den geschlossenen Kanal gesprungen). Spielroboter: 1,35 Rampen pro Spiel (v5.3: 0,52). Testabschuss `TEST X 08574834`, `TEST Y 20771080`.
- **v5.3**: Neue Target-Reihe P L A links oben, senkrecht direkt rechts neben dem aufsteigenden Rampenstück (x 60, y 150–202), Gruppe komplett 15.000 (`PLA READY`). Beide Out-Lanes oberhalb der Öffnung geschlossen (links schräge Wand unter dem Orbit-Kanal, rechts der Gang hinter den SID-Targets oben und unten zu); vorher konnte ein Ball aus der linken Out-Lane über den Orbit zurück auf den Tisch fliegen. SID-Targets etwas tiefer (y 348–400), sonst Mulde unter der Spitze des dritten Flippers. Testabschuss jetzt `TEST X 11818570`, `TEST Y 21549792`.
- **Out-Lanes symmetrisch (v5.2)**: Die Begrenzung hinter den VIC-II-Targets endet jetzt mit den Targets bei y 400 (vorher 420, der Durchgang zur linken Out-Lane war sehr schmal). Rechts gespiegelt: SID-Targets bei x 254 (vorher 280), dahinter eine Begrenzung bei x 264 bis y 400 über der Trennwand Inlane/Out-Lane; vorher fiel der Ball rechts sehr oft durch. Spielroboter: Ballverluste links 98, rechts 97, Mitte 188.
- **Display und Prüfsumme warten (v5.1)**: Displaytext, Display und Prüfsumme laufen nur, wenn das Bild nach dem Zeichnen noch unter 200 Rasterzeilen liegt; sonst im nächsten Bild. Die Prüfsumme prüft 128 statt 512 Byte pro Bild (v5.0 kostete sie 24 Zeilen).
- **Schnellere Physik**, Ergebnis bitgleich: ruhende Flipper werden übersprungen, Flipper und Bumper haben eine schnelle Vorprüfung in ganzen Pixeln. Im 6502-Simulator (Profil mit `make -C test/sim prof`) kostet die Physik im Mittel rund 90.000 Zyklen pro Bild ohne Multiplikation.

### Meilenstein 4: Bedienung und Test

Neu in v4.0, Einfahrt in v4.1 und Tischaufbau in v4.2 und v4.3 geändert:
- **Rampe „FAST LOADER“**: Die Einfahrt liegt links oberhalb der Mitte (Start bei y 275, v4.0: y 352) und ist um 21° schräg gestellt: Sie öffnet sich nach rechts unten und zielt auf den rechten Flipper (v4.0 zeigte sie senkrecht nach unten und war nicht zu treffen). Die Rampe steigt links hoch, führt als Brücke hinten quer über den Tisch (unterhalb der Lanes C 6 4) und rechts wieder herunter. Sie endet links unterhalb des dritten Flippers, der Ball rollt zum linken Flipper. Der Ball braucht in der Einfahrt mindestens etwa 1,6 Pixel pro Substep (13 Pixel pro Bild), sonst rollt er zurück. Der Deckel über der Einfahrt setzt das Einwegtor des linken Orbits nach rechts unten fort. Punkte: 5.000, 10.000, … bis 25.000 je Ball. Auf der Rampe folgt der Ball einem festen Weg (zweite Ebene); unter der Brücke liegt die Rampe sichtbar über dem Ball (halb durchsichtiger Boden).
- **Spinner** in der Einfahrt: dreht sich je nach Tempo des Balls, 100 Punkte pro Umdrehung, die Streifen wechseln die Helligkeit.
- **Kick-Out-Hole „1541“**: fängt einen nicht zu schnellen Ball, hält ihn 1,5 Sekunden (`SEARCHING FOR *` / `LOADING`, Laufwerksgeräusch), wirft ihn nach links unten aus und gibt 3.000 Punkte (`READY.`).
- **v4.2**: Einfahrt als Trichter (rechts weit, links etwas ausgestellt, Öffnung etwa 50 statt 22 Pixel). VIC-II-Targets links genau gegenüber den SID-Targets (x 46, y 328–400, mit Dach wie bei SID). 6510-Targets schräg links vom geraden Abstieg der Rampe zwischen Kick-Out-Hole und Rampenende, zum linken Flipper gerichtet. Die Inlane-Führungen enden jetzt knapp hinter den Flipper-Drehpunkten: Vorher blieb ein Ball aus der Inlane oben auf dem Drehpunkt liegen, und der Flipper traf ihn kaum.
- **v4.3** (nach Mock-up vom 26.10.2026): Kick-Out-Hole „1541“ links (x 112, y 222), Auswurf nach rechts unten. 6510-Targets schräg links vom geraden Abstieg der Rampe (y 196–236). Rampe: Abstieg weiter rechts (x 262), Ende über dem dritten Flipper, der Ball fällt sanft darauf. Schräge Führung über dem Drehpunkt des dritten Flippers (der Ball blieb dort oben liegen).
- Neue Wandstücke: rechte Schiene der Einfahrt, ein schräger Deckel darüber und ein schräges Dach über den SID-Targets (der Spielroboter fand dort unter dem dritten Flipper einen Ruheplatz).

Im PC-Test: Schüsse in die Rampe ab 1,8 Pixel pro Substep schaffen sie und kommen bei x 122–126 am linken Flipper an; der Spielroboter schaffte mit v4.1 in 122 Spielen 60 Rampen und 60 Kick-Outs; liegen blieb der Ball nur einmal auf dem Drehpunkt des rechten Flippers (ein Flipperdruck löst ihn). Der Testabschuss liefert unverändert `TEST X 07457637` und `TEST Y 21572962`.

**Was zu prüfen ist:** wie sich die Rampe spielt (zu leicht, zu schwer?), ob der Ball unter der Brücke richtig verdeckt wird, ob Kick-Out und Spinner gut wirken und wie hoch MAX steigt.

Bekannt: Funktioniert die DMA-Transparenz nicht (Anzeige `CPU`), liegt der Ball unter der Brücke über der Rampe.

### Meilenstein 3: Bedienung und Test

Ein Spiel hat 3 Bälle. Der Plunger (Leertaste oder Feuerknopf halten und loslassen) schießt ab. Nach dem dritten Ball zeigt das Display `?OUT OF DATA  ERROR` und die Punkte; Leertaste oder Feuer startet ein neues Spiel.

Neu:
- **Dritter Flipper** oben rechts bei (282, 300), er schlägt mit dem rechten Flipperknopf. Er nutzt dieselben vorgezeichneten Bilder wie der rechte untere Flipper.
- **Punkte**: Bumper 100, Slingshot 10, Target 500 (schon leuchtend: 100), obere Lane 1.000 (leuchtend: 200), Lanes L O A D 2.000 (leuchtend: 500).
- **Gruppen**: VIC-II komplett 10.000, 6510 komplett 20.000, SID komplett 10.000. Obere Lanes C 6 4 komplett: 5.000 und Bonus-Multiplikator +1 (bis 5X). L O A D komplett: 25.000 (`PRESS PLAY ON TAPE`).
- **Bonus**: Jedes neu angeschaltete Target und jede neu angeschaltete Lane zählt 1.000, am Ballende mal Multiplikator.
- **Lane-Wechsel**: Ein Druck auf den linken oder rechten Flipperknopf schiebt die leuchtenden Lampen der oberen Lanes und von L O A D eine Stelle weiter.
- **Lampen**: Jede Lampe hat einen eigenen Paletteneintrag. Ein- und Ausschalten ändert nur die Farbe, das kostet keine Zeichenzeit. Bumper und Slingshot-Gummis blitzen bei einem Treffer auf.
- **Klang**: einfache SID-Effekte für Bumper, Slingshot, Target, Lane, Flipper, Gruppe komplett, Ballverlust und Spielstart, links und rechts gleich.
- **Taste D**: schaltet im Spiel zwischen Punkteanzeige und der Messwert-Aufteilung (P, S, Z, D) um.

| Zustand | Zeile 1 | Zeile 2 |
|---|---|---|
| Ball in der Abschussbahn | Punkte (vor dem ersten Punkt `LOAD"*",8,1`) | `BALL n`, `L`, `MAX`, `DMA`/`CPU` |
| Ball im Spiel | Punkte | `BALL n`, Multiplikator, `L`, `MAX` |
| Ball im Spiel, Taste D | `L`, `MAX` | `P`, `S`, `Z`, `D` wie in Meilenstein 2 |
| Gruppe komplett, Ballende | Meldung, z. B. `VIC-II READY` oder `BONUS 2X` | Punkte dafür |
| Spielende | `?OUT OF DATA  ERROR` | Endstand |

**Gleichheitstest:** unverändert `TEST X 07457637` und `TEST Y 21572962` (der Testball erreicht den dritten Flipper in 120 Bildern nicht). Seit v4.2 (neue Targets): `TEST X 08018924` und `TEST Y 20543034`.

**Was zu prüfen ist:** ob die Lampen richtig an- und ausgehen, ob die Effekte zu hören sind (auf beiden Kanälen), wie sich der dritte Flipper spielt und wie hoch MAX im Spiel steigt.

### Meilenstein 2: Bedienung und Test

Nach dem Start zeigt das Display `SEARCHING FOR *` und den Fortschritt (`MASKE nn/14`, dann `GRAFIK nn/80`), während der MEGA65 Kollisionsmaske und Tischgrafik berechnet. Das dauert einige Sekunden. Eine Prüfsumme über den Programmcode wird nach jedem Schritt und im Spiel alle 5 Sekunden kontrolliert; ändert sich der Code, hält das Programm mit `?CODE CHANGED  ERROR` und rotem Rahmen. Danach erscheint der Tisch, das Bild folgt dem Ball pixelgenau.

Fehlt das Attic-RAM oder antwortet es falsch, zeigt das Display `?ATTIC RAM  ERROR` und der Rahmen wird rot.

Die Tasten sind dieselben wie in Meilenstein 1 (Tabelle unten). Das Display zeigt:

| Zustand | Zeile 1 | Zeile 2 |
|---|---|---|
| Ball in der Abschussbahn | `LOAD"*",8,1` | `L` Auslastung, `MAX` Höchstwert der letzten Sekunde, `DMA` oder `CPU`: Weg zum Zeichnen von Flipper und Ball |
| Ball im Spiel | `L` Auslastung, `MAX` Höchstwert der letzten Sekunde | Aufteilung des langsamsten Bildes der letzten Sekunde in Rasterzeilen: `P` Physik, `S` Hintergrund kopieren (Scrollen), `Z` Flipper und Ball zeichnen, `D` Display und Prüfsumme |
| Ball verloren | `?BALL LOST  ERROR` | `READY.` |
| Testabschuss läuft | `TEST` und Zähler bis 120 | `SEARCHING FOR *` |
| Testabschuss fertig | `TEST X` und Wert | `TEST Y` und Wert |
| Einstellmodus (F1) | Name des Werts | `=` Wert |

Die Zahlen im Display werden fünfmal pro Sekunde aktualisiert. L wird in Rasterzeilen gemessen, 312 entsprechen einem vollen Bild.

**Gleichheitstest:** Direkt nach dem Start `T` drücken und nichts weiter anfassen. Der PC-Test liefert für Meilenstein 2 `TEST X 07457637` und `TEST Y 21572962`. Der Wert hat sich geändert, weil der Tisch neu ist.

**Was zu prüfen ist:** ob das Scrolling ruckelt oder flackert, wie hoch L beim Scrollen steigt (der Hintergrund kommt dann aus dem Attic-RAM), und ob Display und Tisch sauber getrennt bleiben.

### Meilenstein 1: Bedienung und Test

Nach dem Start färbt sich der Rahmen kurz bunt, während der MEGA65 Kollisionsmaske und Hintergrund berechnet (ein bis zwei Sekunden). Danach erscheint der untere Teil des Tisches.

| Taste | Funktion |
|---|---|
| Shift links oder A | linker Flipper |
| Shift rechts oder L | rechter Flipper |
| Leertaste halten | Plunger spannen, Loslassen schießt |
| T | Testabschuss mit fester Stärke |
| B | Ball zurück in die Abschussbahn |
| F1 | Einstellmodus an/aus |
| F3 | nächster Wert |
| F5 / F7 | Wert verringern / erhöhen (gedrückt halten wiederholt) |
| Joystick Port 2 | links/rechts Flipper, Feuer oder unten Plunger |

Steigt der Ball über den sichtbaren Ausschnitt, zeigt ein gelbes Dreieck am oberen Rand seine x-Position. Gescrollt wird erst in Meilenstein 2.

Statuszeile:

- `F` Bildzähler, `L` Auslastung in Rasterzeilen (312 entspricht einem vollen Bild bei 50 Hz)
- `X`, `Y` Ballposition in Pixeln, `B` Bumper-Treffer, `S` Slingshot-Treffer
- `TEST`: nach `T` zählt der Wert bis 120, danach stehen dort die exakten Festkomma-Koordinaten des Balls

Im Einstellmodus zeigt die Statuszeile rechts den gewählten Wert. Die Änderung wirkt sofort und gilt bis zum Neustart. Gute Werte bitte notieren, sie werden dann als neue Startwerte eingetragen.

| Wert | Start | Bereich | Bedeutung |
|---|---|---|---|
| GRAVITY | 340 | 100–800 | Schwerkraft |
| FLIPPER SPEED | 1100 | 200–1400 | Schlaggeschwindigkeit; Rückweg 60 % davon. 1100 ≈ 25 ms pro Schlag |
| WALL BOUNCE | 80 | 0–230 | Abprall an Wänden, geteilt durch 256 (80 ≈ 0,31) |
| FLIPPER BOUNCE | 72 | 0–200 | Abprall am ruhenden Flipper, geteilt durch 256 |
| BUMPER KICK | 90000 | 20000–160000 | Stoß der Bumper (in Meilenstein 1 noch unsichtbar, oben im Tisch) |
| SLING KICK | 70000 | 0–140000 | Stoß der Slingshots |

**Gleichheitstest Xemu und MEGA65:** Direkt nach dem Start ohne andere Taste `T` drücken und nichts weiter anfassen. Die beiden Zahlen hinter `TEST` müssen auf beiden Systemen identisch sein. Mit den Startwerten von v1.2 liefert der PC-Test `X:17539107 Y:38809859` (v1.1: `X:13317277 Y:19434387`, v1.0: `X:07652669 Y:30380675`). Nach einer Änderung im Einstellmodus ergeben sich natürlich andere Zahlen.

### Meilenstein 0: was zu erwarten ist

Das Programm schaltet auf 40 MHz, vergleicht 2000 Festkomma-Multiplikationen in Software mit dem Hardware-Multiplizierer und färbt den Rahmen grün (alles gleich) oder rot (Abweichungen). Zusätzlich wird `1.5 * -2.25` ausgegeben; erwartet ist `-221184`. Bitte das Ergebnis vom echten MEGA65 und aus Xemu notieren.

## Projektstruktur

```
prototype/   Physik-Prototyp im Browser (HTML-Datei direkt öffnen)
src/         C-Quellcode für den MEGA65
tools/       Hilfsprogramme: mkd81.py (Diskettenimage), gen_tables.py (Sinus/atan),
             gen_ramp.py (Weg der Rampe),
             gen_font.py (5x7-Schrift fürs Display)
test/host/   PC-Test: derselbe C-Code mit simuliertem Speicher und DMA, schreibt Bilder,
             prüft die Spielregeln und lässt mit BOT=n einen Spielroboter n Bilder spielen
build.bat    Build unter Windows
Makefile     Build mit make
```

## PC-Test

Unter Linux oder in WSL: `make -C test/host run`. Mit `BOT=300000 ./hosttest` spielt zusätzlich ein einfacher Roboter und meldet Stellen, an denen der Ball liegen bleibt (v3.0: 169 Spiele, kein Hängenbleiben). Das Programm führt das Spiel ohne MEGA65 aus, protokolliert die Ballbahn und schreibt Bilder (`.ppm`), die aus dem simulierten Zeichenspeicher so gelesen werden wie vom VIC-IV. Das Timing und die echten Register prüft nur die Hardware.

## LOADTEST

Auf der Diskette liegt neben dem Spiel das Programm `LOADTEST` (`RUN "LOADTEST"`). Es reicht von $2001 bis $C648 und enthält ein 38.000 Byte großes Testmuster. Nach dem Laden prüft es jedes Byte und meldet, ob alles richtig angekommen ist, und wenn nicht, ab welcher Adresse und wie viele Bytes im Bereich $A000–$BFFF falsch sind.

## Simulator-Test

`make -C test/sim run` übersetzt Physik, Darstellung und Display mit dem echten llvm-mos-Compiler für den 6502-Simulator `mos-sim` aus dem SDK. Anders als im PC-Test ist `int` hier wie auf dem MEGA65 16 Bit breit. Jeder DMA-Auftrag, der in den CPU-Speicher schreibt, wird auf Ziel und Länge geprüft, ebenso jeder Pixel, der in eine Bildschirmspalte gezeichnet wird.

## Technische Eckpunkte

- **Festkomma 16.16**: 65536 entspricht einem Pixel. Multiplikation über den Hardware-Multiplizierer (`$D770`).
- **Substeps**: 8 Physik-Schritte pro Frame bei 50 Hz (PAL), damit der Ball nicht durch Wände tunnelt.
- **Kollisionsmaske**: ein Byte pro Pixel mit Klassen für Wand, Slingshot, Tor und Targets. Normalen aus Randpunkten über eine 256er-Winkeltabelle.
- **Flipper**: sich drehende Kapseln mit Oberflächengeschwindigkeit am Treffpunkt.
- **Grafik**: Full-Colour-Charakter-Modus (CHR16, 256 Farben pro Pixel), 40 × 25 Zeichen. Zeilen 0–3 sind das Display, Zeilen 4–24 der Tischausschnitt (168 Pixel).
- **Scrolling**: Die Zeichen des Tischausschnitts sind spaltenweise nummeriert, eine Bildschirmspalte ist ein zusammenhängender Block von 1344 Byte. Die Tischgrafik liegt als 40 Spaltenstreifen (640 × 8 Byte) im Attic-RAM ab `$8000000`. Bewegt sich die Kamera, füllt je ein DMA-Auftrag pro Spalte den Bildpuffer, auf jede Pixelzeile genau. Steht sie still, werden nur die Stellen erneuert, an denen Flipper und Ball waren.
- **Speicher (Chip-RAM)**: Bildpuffer A `$40000`, B `$4D200`, Display `$5A400`, Bildschirmkarten `$5CC00` und `$5D400`. CPU-Puffer fest bei `$0800` (Zeichenzeile), `$1200` (Bildschirmspalte), `$1740` (Maskenfenster, 48 Zeilen), `$1EC0` (Klassenfenster, 7 Zeilen). Das Klassenraster der Maske liegt seit v3.0 im Chip-RAM ab `$5F000`, im Attic-RAM ab `$8040000` die vorgezeichneten Flipper und der Ball.
- **Hauptspeicher ist knapp**: Programm und Variablen endeten bei v3.0 bei `$CD00`, darüber blieben nur 768 Byte für den Stack. Seit v3.1 baut der Lader die Tischdaten auf, das Spiel endet mit Variablen bei `$A198`.
- **Tischdesign-Regel aus dem PC-Test**: Die Schwerkraft wirkt senkrecht, deshalb ist jede waagerechte Fläche und jede Lücke in Ballbreite ein möglicher Ruheplatz. Targetreihen sind deshalb schräg und lückenlos. Ein Einwegtor oben im linken Orbit lenkt von oben kommende Bälle aufs Spielfeld, sonst liefen abgeschossene Bälle direkt in die linke Outlane.
- **Kollisionsmaske**: 25 KB im Attic-RAM ab `$8100000` (bis v3.0 im Farb-RAM), Klassenraster ab `$8110000`; die CPU arbeitet mit einem Fenster von 48 Zeilen, das per DMA nachgeladen wird.
