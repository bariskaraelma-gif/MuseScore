@echo off
chcp 65001 >nul
setlocal

rem Makam Nota: starts the locally built program (makam\derle.bat). Portable: its settings are in
rem build.install\Data\settings, the MuseScore installed on this computer is not touched.

set "BIN=%~dp0..\build.install\App\MuseScore\bin"
for %%f in ("%BIN%\MuseScore*.exe") do (
    start "" "%%~ff"
    exit /b 0
)
echo Program bulunamadı: %BIN%
echo Önce makam\derle.bat çalıştırılmalı.
exit /b 1
