@echo off
chcp 65001 >nul
setlocal EnableExtensions

rem Makam Nota: local build (portable, its settings stay in build.install\Data).
rem   makam\derle.bat           build what changed and install it to build.install
rem   makam\derle.bat yeniden   run CMake configuration again first (after changing CMake options)
rem   makam\derle.bat temiz     delete build.release and build.install, then build everything
rem Parallel jobs: JOBS (default 6, for 16 GB RAM). If the compiler runs out of memory: set JOBS=4

pushd "%~dp0.."
set "SRC=%CD%"
set "BUILD_DIR=%SRC%\build.release"
set "INSTALL_DIR=%SRC%\build.install\App\MuseScore"
if not defined QT_DIR set "QT_DIR=C:\Qt\6.10.2\msvc2022_64"
if not defined JOBS set "JOBS=6"

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo Qt bulunamadı: %QT_DIR%
    echo Önce makam\kurulum.ps1 çalıştırılmalı.
    goto :fail
)

if /I "%~1"=="temiz" (
    echo == Eski derleme siliniyor
    if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
    if exist "%SRC%\build.install" rmdir /S /Q "%SRC%\build.install"
)

rem --- Visual Studio C++ environment (cl, link, and the CMake / Ninja bundled with Visual Studio)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_DIR="
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_DIR=%%i"
if not defined VS_DIR (
    echo Visual Studio C++ araçları bulunamadı. Önce makam\kurulum.ps1 çalıştırılmalı.
    goto :fail
)
call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :fail
set "PATH=%QT_DIR%\bin;%PATH%"

set "REVISION=local"
for /f %%h in ('git rev-parse --short^=7 HEAD 2^>nul') do set "REVISION=%%h"

rem --- configure (only the first time, or with "yeniden" / "temiz"; Ninja re-runs CMake itself
rem     when a CMakeLists.txt changes)
set "CONFIGURE="
if not exist "%BUILD_DIR%\build.ninja" set "CONFIGURE=1"
if /I "%~1"=="yeniden" set "CONFIGURE=1"
if /I "%~1"=="temiz" set "CONFIGURE=1"
if defined CONFIGURE (
    echo == CMake yapılandırması
    cmake -S "%SRC%" -B "%BUILD_DIR%" -G Ninja ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
        -DMUSESCORE_BUILD_CONFIGURATION=app-portable ^
        -DMUSE_APP_BUILD_MODE=dev ^
        -DMUSESCORE_REVISION=%REVISION% ^
        -DMUE_RUN_LRELEASE=ON ^
        -DMUE_RUN_WINDEPLOYQT=ON ^
        -DMUE_DOWNLOAD_SOUNDFONT=ON ^
        -DMUSE_ENABLE_UNIT_TESTS=OFF ^
        -DMUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT=OFF ^
        -DMUSE_MODULE_UPDATE=OFF ^
        -DMUSE_COMPILE_USE_UNITY=ON
    if errorlevel 1 goto :fail
)

echo == Derleniyor (%JOBS% iş parçacığı) - başlangıç %TIME%
cmake --build "%BUILD_DIR%" -j %JOBS%
if errorlevel 1 goto :fail

echo == Kuruluyor: %INSTALL_DIR%
cmake --install "%BUILD_DIR%" >nul
if errorlevel 1 goto :fail

echo.
echo Derleme bitti (%TIME%). Programı açmak için: makam\calistir.bat
popd
exit /b 0

:fail
echo.
echo DERLEME BAŞARISIZ. Hata satırı yukarıda ("error C" ya da "error:" ile başlayan ilk satır).
popd
exit /b 1
