@echo off
rem Pinball65 bauen: erzeugt build\pinball65.prg und build\pinball65.d81
setlocal enabledelayedexpansion

where mos-mega65-clang >nul 2>nul || (
  echo mos-mega65-clang wurde nicht gefunden. Ist llvm-mos\bin im PATH?
  exit /b 1
)
set PY=python
where python >nul 2>nul || set PY=py -3

if not exist build mkdir build

rem Spiel: alle Quellen ausser dem Lader boot.c
set SRC=
for %%f in (src\*.c) do if /i not "%%~nf"=="boot" set SRC=!SRC! %%f
mos-mega65-clang -Oz -mcpu=mos6502 -Wall -Wextra -o build\pb65game.prg !SRC! || goto :err

rem Lader: baut die Tischdaten auf und springt danach zur Startadresse des Spiels
set ENTRY=
for /f "tokens=1,3" %%a in ('llvm-nm build\pb65game.prg.elf') do if "%%b"=="_start" set ENTRY=%%a
if "!ENTRY!"=="" goto :err
mos-mega65-clang -Oz -mcpu=mos6502 -Wall -Wextra -DPB_BUILDER -DGAME_ENTRY=0x!ENTRY! -o build\pinball65.prg src\boot.c src\table.c src\render.c src\physics.c src\hw.c src\dmd.c src\font5x7.c src\tables.c src\fixmath.c src\ramp_path.c || goto :err
mos-mega65-clang -Os -mcpu=mos6502 -o build\loadtest.prg test\loadtest\loadtest.c test\loadtest\pattern.c || goto :err
%PY% tools\mkd81.py build\pinball65.d81 PINBALL65 build\pinball65.prg build\pb65game.prg build\loadtest.prg || goto :err
echo.
echo Fertig: build\pinball65.d81
exit /b 0

:err
echo Build fehlgeschlagen.
exit /b 1
