# Pinball65

Eine Flippersimulation für den MEGA65 im Stil von Pinball Fantasies, mit einem Tisch rund um Retro-Computer. Privates Hobbyprojekt, geschrieben in C mit [llvm-mos](https://llvm-mos.org).

## Stand

| Meilenstein | Inhalt | Status |
|---|---|---|
| Prototyp | Physik im Browser, Festkomma 16.16, einstellbare Werte | fertig, siehe `prototype/` |
| 0 | Toolchain- und Hardware-Test: Build, D81, Hardware-Multiplizierer | bereit zum Test |
| 1 | Statischer Tisch, Ball, zwei Flipper, Kollisionsmaske | geplant |
| 2 | Scrollender Tisch, Raster-Split für das Display | geplant |
| 3 | Bumper, Slingshots, Targets, Punkte, Sound | geplant |

## Testumgebung

Jeder Meilenstein wird auf echter Hardware geprüft, nicht nur im Emulator, weil sich Xemu und der MEGA65 in Details unterscheiden.

- MEGA65 mit Core **0.97 (20250422.09)**, ROM **920413**
- Übertragung: D81-Image per **M65Connect**
- Emulator für schnelle Zwischentests: **Xemu** (mit demselben ROM 920413)

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

### Meilenstein 0: was zu erwarten ist

Das Programm schaltet auf 40 MHz, vergleicht 2000 Festkomma-Multiplikationen in Software mit dem Hardware-Multiplizierer und färbt den Rahmen grün (alles gleich) oder rot (Abweichungen). Zusätzlich wird `1.5 * -2.25` ausgegeben; erwartet ist `-221184`. Bitte das Ergebnis vom echten MEGA65 und aus Xemu notieren.

## Projektstruktur

```
prototype/   Physik-Prototyp im Browser (HTML-Datei direkt öffnen)
src/         C-Quellcode für den MEGA65
tools/       Hilfsprogramme, z. B. mkd81.py zum Erzeugen des Diskettenimages
build.bat    Build unter Windows
Makefile     Build mit make
```

## Technische Eckpunkte

- **Festkomma 16.16**: 65536 entspricht einem Pixel. Multiplikation über den Hardware-Multiplizierer (`$D770`).
- **Substeps**: 8 Physik-Schritte pro Frame bei 50 Hz (PAL), damit der Ball nicht durch Wände tunnelt.
- **Kollisionsmaske**: ein Byte pro Pixel mit Klassen für Wand, Slingshot, Tor und Targets. Normalen aus Randpunkten über eine 256er-Winkeltabelle.
- **Flipper**: sich drehende Kapseln mit Oberflächengeschwindigkeit am Treffpunkt.
- **Grafik (geplant)**: Full-Colour-Charakter-Modus, Tisch als Kachelkarte, vertikales Scrolling, Raster-Split für das Display.
