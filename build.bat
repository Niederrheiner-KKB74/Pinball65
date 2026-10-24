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

set SRC=
for %%f in (src\*.c) do set SRC=!SRC! %%f

mos-mega65-clang -Oz -Wall -Wextra -o build\pinball65.prg !SRC! || goto :err
%PY% tools\mkd81.py build\pinball65.d81 PINBALL65 build\pinball65.prg || goto :err
echo.
echo Fertig: build\pinball65.d81
exit /b 0

:err
echo Build fehlgeschlagen.
exit /b 1
