# Pinball65

Eine Flippersimulation für den MEGA65 im Stil von Pinball Fantasies, mit einem Tisch rund um Retro-Computer. Privates Hobbyprojekt, geschrieben in C mit [llvm-mos](https://llvm-mos.org).

## Stand

| Meilenstein | Inhalt | Status |
|---|---|---|
| Prototyp | Physik im Browser, Festkomma 16.16, einstellbare Werte | fertig, siehe `prototype/` |
| 0 | Toolchain- und Hardware-Test: Build, D81, Hardware-Multiplizierer | bestanden (MEGA65 und Xemu, 23.10.2026) |
| 1 | Statischer Tisch, Ball, zwei Flipper, Kollisionsmaske | bestanden (24.10.2026); v1.1 mit Einstellmodus bereit zum Test |
| 2 | Scrollender Tisch, Raster-Split für das Display | geplant |
| 3 | Bumper, Slingshots, Targets, Punkte, Sound | geplant |

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

Erkenntnisse:
- Nicht nach `$00` schreiben, um 40 MHz einzuschalten. Im C65-Modus ist das bereits der Standard.
- llvm-mos belegt die Zeropage von BASIC, daher kehrt das Programm nicht ins BASIC zurück.
- Der Hardware-Multiplizierer liefert sofort korrekte Ergebnisse, ohne Warten.
- Die Physik rechnet auf MEGA65, Xemu und PC bitgleich. Damit lassen sich Physikänderungen im PC-Test vorab prüfen.
- Ein Bild braucht etwa 100 von 312 Rasterzeilen, also rund ein Drittel der verfügbaren Zeit.
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
| FLIPPER SPEED | 720 | 200–1400 | Schlaggeschwindigkeit; Rückweg 60 % davon. 720 ≈ 40 ms pro Schlag |
| WALL BOUNCE | 100 | 0–230 | Abprall an Wänden, geteilt durch 256 (100 ≈ 0,39) |
| FLIPPER BOUNCE | 72 | 0–200 | Abprall am ruhenden Flipper, geteilt durch 256 |
| BUMPER KICK | 90000 | 20000–160000 | Stoß der Bumper (in Meilenstein 1 noch unsichtbar, oben im Tisch) |
| SLING KICK | 70000 | 0–140000 | Stoß der Slingshots |

**Gleichheitstest Xemu und MEGA65:** Direkt nach dem Start ohne andere Taste `T` drücken und nichts weiter anfassen. Die beiden Zahlen hinter `TEST` müssen auf beiden Systemen identisch sein. Mit den Startwerten von v1.1 liefert der PC-Test `X:13317277 Y:19434387` (v1.0: `X:07652669 Y:30380675`). Nach einer Änderung im Einstellmodus ergeben sich natürlich andere Zahlen.

### Meilenstein 0: was zu erwarten ist

Das Programm schaltet auf 40 MHz, vergleicht 2000 Festkomma-Multiplikationen in Software mit dem Hardware-Multiplizierer und färbt den Rahmen grün (alles gleich) oder rot (Abweichungen). Zusätzlich wird `1.5 * -2.25` ausgegeben; erwartet ist `-221184`. Bitte das Ergebnis vom echten MEGA65 und aus Xemu notieren.

## Projektstruktur

```
prototype/   Physik-Prototyp im Browser (HTML-Datei direkt öffnen)
src/         C-Quellcode für den MEGA65
tools/       Hilfsprogramme: mkd81.py (Diskettenimage), gen_tables.py (Sinus/atan)
test/host/   PC-Test: derselbe C-Code mit simuliertem Speicher und DMA, schreibt Bilder
build.bat    Build unter Windows
Makefile     Build mit make
```

## PC-Test

Unter Linux oder in WSL: `make -C test/host run`. Das Programm führt Meilenstein 1 ohne MEGA65 aus, protokolliert die Ballbahn und schreibt Bilder (`.ppm`), die aus dem simulierten Zeichenspeicher so gelesen werden wie vom VIC-IV. Das Timing und die echten Register prüft nur die Hardware.

## Technische Eckpunkte

- **Festkomma 16.16**: 65536 entspricht einem Pixel. Multiplikation über den Hardware-Multiplizierer (`$D770`).
- **Substeps**: 8 Physik-Schritte pro Frame bei 50 Hz (PAL), damit der Ball nicht durch Wände tunnelt.
- **Kollisionsmaske**: ein Byte pro Pixel mit Klassen für Wand, Slingshot, Tor und Targets. Normalen aus Randpunkten über eine 256er-Winkeltabelle.
- **Flipper**: sich drehende Kapseln mit Oberflächengeschwindigkeit am Treffpunkt.
- **Grafik**: Full-Colour-Charakter-Modus (CHR16, 256 Farben pro Pixel), zwei Bildpuffer mit je 1000 Zeichen bei `$40000` und `$50000`, unveränderter Hintergrund bei `$10000`. Pro Bild werden nur die Zeichenzeilen mit Flippern und Ball neu gezeichnet.
- **Speicher**: Die Kollisionsmaske (23 KB) liegt im Farb-RAM ab `$FF81000`; die CPU arbeitet mit einem Fenster von 128 Zeilen, das per DMA nachgeladen wird.
- **Geplant**: Kachelkarte für den ganzen Tisch, vertikales Scrolling, Raster-Split für das Display.
