# Pinball65

Eine Flippersimulation für den MEGA65 im Stil von Pinball Fantasies, mit einem Tisch rund um Retro-Computer. Privates Hobbyprojekt, geschrieben in C mit [llvm-mos](https://llvm-mos.org).

## Stand

| Meilenstein | Inhalt | Status |
|---|---|---|
| Prototyp | Physik im Browser, Festkomma 16.16, einstellbare Werte | fertig, siehe `prototype/` |
| 0 | Toolchain- und Hardware-Test: Build, D81, Hardware-Multiplizierer | bestanden (MEGA65 und Xemu, 23.10.2026) |
| 1 | Statischer Tisch, Ball, zwei Flipper, Kollisionsmaske | bestanden (24.10.2026), aktuell v1.2 |
| 2 | Ganzer Tisch 320 × 640 mit Scrolling, Display oben, Platzhaltergrafik nach Entwurf | läuft; v2.6 mit schnellerem Display |
| 3 | Bumper, Slingshots, Targets, Lanes, dritter Flipper, Punkte, Lampen, erste Effekte | geplant |
| 4 | Rampe mit Brücke, Spinner, Kick-Out-Hole | geplant |
| 5 | Regeln, Missionen, Multiball, Display-Animationen, SID-Musik | geplant |
| 6 | Grafik-Werkzeug, endgültige Grafik, Highscore | geplant |

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

Erkenntnisse:
- Nicht nach `$00` schreiben, um 40 MHz einzuschalten. Im C65-Modus ist das bereits der Standard.
- llvm-mos belegt die Zeropage von BASIC, daher kehrt das Programm nicht ins BASIC zurück.
- Der Hardware-Multiplizierer liefert sofort korrekte Ergebnisse, ohne Warten.
- Die Physik rechnet auf MEGA65, Xemu und PC bitgleich. Damit lassen sich Physikänderungen im PC-Test vorab prüfen.
- Ein Bild braucht etwa 100 von 312 Rasterzeilen, also rund ein Drittel der verfügbaren Zeit.
- v1.1 brauchte 124 Zeilen, weil die einstellbaren Werte in jedem Substep neu verrechnet wurden, darunter eine 32-Bit-Division in Software. Seit v1.2 werden abgeleitete Werte nur bei einer Änderung berechnet.
- Im Einstellmodus ermittelt (24.10.2026): FLIPPER SPEED 1100 und WALL BOUNCE 80 fühlen sich gut an. Seit v1.2 sind das die Startwerte.
- **Übersetzt wird für den reinen 6502 (`-mcpu=mos6502`).** Mit den 45GS02-Erweiterungen erzeugte llvm-mos (Version cd91455) in `band_disc` einen `inw`-Befehl auf das falsche Register: Der Schleifenzähler blieb stehen, der Aufbau hing bei `GRAFIK 15/80` (Diagnoseversion 2.2). Der 6502-Simulator zeigte den Fehler nicht, weil er für den 6502 übersetzt. Mit `-mcpu=mos6502` prüft er jetzt denselben Befehlssatz, der auf dem MEGA65 läuft. Den 65C02-Befehlssatz meiden wir ebenfalls: Seine `(zp)`-Befehle bedeuten auf dem 45GS02 `(zp),Z` und funktionieren nur, solange Z = 0 ist.
- **Keine $A000-Grenze.** Die Vermutung, bei $A000–$BFFF liege beim Laden das BASIC-ROM, war falsch: `LOADTEST` lädt bis $C648 auf Xemu und MEGA65 fehlerfrei. Der Absturz von Version 2.0 kam vom `inw`-Fehler. Die eigentliche Grenze ist der Speicher bis $D000 (Code, Variablen, 1 KB Stack); mit `-Os` wäre das Programm 5,8 KB zu groß, deshalb bleibt es bei `-Oz`.
- **Spitzen im Ruhezustand (v2.5):** Eine Display-Zeichenzeile kostete rund 75 Rasterzeilen (640 Punkte einzeln), die Code-Prüfsumme alle 5 Sekunden prüfte 33 KB in einem Bild. Seit v2.6 kopiert DMA eine fertige leere Punktzeile, die CPU setzt nur die leuchtenden Punkte (im Simulator rund 29.000 statt 170.000 Zyklen pro Zeichenzeile, Ergebnis byte-identisch); die Prüfsumme läuft mit 512 Byte pro Bild. Bilder über 312 Zeilen erkennt das Programm am Raster-Vergleichsflag ($D019), vorher lief der Messwert dabei über und zeigte zu kleine Zahlen.
- **Display:** Seit v2.4 wird pro Bild höchstens eine Zeichenzeile neu gezeichnet. In v2.4 wurde das Display aber nur alle 10 Bilder aufgerufen, und die obere Zeile (mit dem L-Wert) kam immer zuerst dran, so dass die untere Zeile verhungerte. Seit v2.5 läuft es in jedem Bild und reihum.
- **Laufzeit (v2.4):** Im 6502-Simulator gemessen kosteten die Flipper rund 830.000 Zyklen pro Bild (Randprüfung über Funktionsaufrufe für jeden Pixel), das Display alle 10 Bilder auf einmal rund 600.000. Jetzt: Innenbereich und Rand der Flipper vorberechnet, ruhende Flipper werden nicht neu gezeichnet, das Display zeichnet höchstens eine Zeichenzeile pro Bild, die Kollisionsprüfung entfällt, wenn um den Ball keine Wand liegt. Ergebnis im Simulator: von etwa 490.000 auf 208.000 Zyklen pro Bild (ohne Multiplikation und DMA), Bild und Physik unverändert.
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

Ergebnis: `build\pinball65.d81` mit dem Programm `PINBALL65`.

## Auf dem MEGA65 starten

1. `build\pinball65.d81` mit M65Connect übertragen und als Laufwerk 8 einbinden.
2. Am MEGA65 eingeben: `RUN "PINBALL65"`

In Xemu: `xmega65 -8 build\pinball65.d81`, danach ebenfalls `RUN "PINBALL65"`.

### Meilenstein 2: Bedienung und Test

Nach dem Start zeigt das Display `SEARCHING FOR *` und den Fortschritt (`MASKE nn/14`, dann `GRAFIK nn/80`), während der MEGA65 Kollisionsmaske und Tischgrafik berechnet. Das dauert einige Sekunden. Eine Prüfsumme über den Programmcode wird nach jedem Schritt und im Spiel alle 5 Sekunden kontrolliert; ändert sich der Code, hält das Programm mit `?CODE CHANGED  ERROR` und rotem Rahmen. Danach erscheint der Tisch, das Bild folgt dem Ball pixelgenau.

Fehlt das Attic-RAM oder antwortet es falsch, zeigt das Display `?ATTIC RAM  ERROR` und der Rahmen wird rot.

Die Tasten sind dieselben wie in Meilenstein 1 (Tabelle unten). Das Display zeigt:

| Zustand | Zeile 1 | Zeile 2 |
|---|---|---|
| Ball in der Abschussbahn | `LOAD"*",8,1` | `L` Auslastung, `MAX` Höchstwert der letzten Sekunde |
| Ball im Spiel | `L` Auslastung, `MAX` Höchstwert der letzten Sekunde, `UEB` übergelaufene Bilder in der letzten Sekunde | Höchstwerte der letzten Sekunde in Rasterzeilen: `P` Physik, `S` Hintergrund kopieren (Scrollen), `Z` Flipper und Ball zeichnen, `D` Display und Prüfsumme |
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
             gen_font.py (5x7-Schrift fürs Display)
test/host/   PC-Test: derselbe C-Code mit simuliertem Speicher und DMA, schreibt Bilder
build.bat    Build unter Windows
Makefile     Build mit make
```

## PC-Test

Unter Linux oder in WSL: `make -C test/host run`. Das Programm führt das Spiel ohne MEGA65 aus, protokolliert die Ballbahn und schreibt Bilder (`.ppm`), die aus dem simulierten Zeichenspeicher so gelesen werden wie vom VIC-IV. Das Timing und die echten Register prüft nur die Hardware.

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
- **Speicher (Chip-RAM)**: Bildpuffer A `$40000`, B `$4D200`, Display `$5A400`, Bildschirmkarten `$5CC00` und `$5D400`. CPU-Puffer fest bei `$0800` (Zeichenzeile), `$1200` (Bildschirmspalte), `$1740` (Maskenfenster, 48 Zeilen).
- **Tischdesign-Regel aus dem PC-Test**: Die Schwerkraft wirkt senkrecht, deshalb ist jede waagerechte Fläche und jede Lücke in Ballbreite ein möglicher Ruheplatz. Targetreihen sind deshalb schräg und lückenlos. Ein Einwegtor oben im linken Orbit lenkt von oben kommende Bälle aufs Spielfeld, sonst liefen abgeschossene Bälle direkt in die linke Outlane.
- **Kollisionsmaske**: 25 KB im Farb-RAM ab `$FF81000`; die CPU arbeitet mit einem Fenster von 48 Zeilen, das per DMA nachgeladen wird.
