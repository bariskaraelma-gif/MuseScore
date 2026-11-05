# Makam Nota: installs the build tools on a new Windows computer (run once).
#
#   powershell -ExecutionPolicy Bypass -File makam\kurulum.ps1
#
# Installs, when missing: Visual Studio 2022 Build Tools (C++, with CMake and Ninja), Python 3,
# GitHub CLI and Qt 6.10.2 (MSVC 2022, the version of the GitHub Actions build) in C:\Qt.
# winget asks you to accept each package's license; answer Y.

$ErrorActionPreference = 'Stop'

$QtVersion = '6.10.2'
$QtArch = 'win64_msvc2022_64'
$QtModules = @('qt5compat', 'qtnetworkauth', 'qtshadertools', 'qtwebsockets')
$QtRoot = 'C:\Qt'

function Update-PathFromRegistry {
    $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
                [Environment]::GetEnvironmentVariable('Path', 'User')
}

function Install-Package([string]$Id, [string[]]$Extra = @()) {
    winget list --id $Id -e *> $null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  $Id zaten kurulu" -ForegroundColor DarkGray
        return
    }
    Write-Host "== $Id kuruluyor" -ForegroundColor Cyan
    winget install --id $Id -e @Extra
    if ($LASTEXITCODE -ne 0) {
        throw "$Id kurulamadı (winget çıkış kodu $LASTEXITCODE)"
    }
    Update-PathFromRegistry
}

function Invoke-Python {
    if (Get-Command py -ErrorAction SilentlyContinue) { & py -3 @args } else { & python @args }
}

if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    throw 'winget bulunamadı. Microsoft Store''dan "Uygulama Yükleyici" (App Installer) güncellenmeli.'
}

# --- 1. Visual Studio 2022 Build Tools: MSVC, Windows SDK, CMake and Ninja
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsDir = $null
if (Test-Path $vswhere) {
    $vsDir = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
}
if ($vsDir) {
    Write-Host "  Visual Studio C++ araçları zaten kurulu: $vsDir" -ForegroundColor DarkGray
} else {
    Write-Host '== Visual Studio 2022 Build Tools kuruluyor (yaklaşık 6-8 GB, 15-30 dk)' -ForegroundColor Cyan
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override `
        '--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended'
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio Build Tools kurulamadı (winget çıkış kodu $LASTEXITCODE)"
    }
}

# --- 2. Git (already there if you cloned with it), Python, GitHub CLI
Install-Package 'Git.Git'
Install-Package 'Python.Python.3.12'
Install-Package 'GitHub.cli'

# --- 3. Qt with aqtinstall (no Qt account needed)
$qtDir = Join-Path $QtRoot "$QtVersion\msvc2022_64"
if (Test-Path (Join-Path $qtDir 'bin\qmake.exe')) {
    Write-Host "  Qt $QtVersion zaten kurulu: $qtDir" -ForegroundColor DarkGray
} else {
    Write-Host "== Qt $QtVersion kuruluyor ($QtRoot, yaklaşık 3 GB)" -ForegroundColor Cyan
    Invoke-Python -m pip install --user --upgrade aqtinstall
    if ($LASTEXITCODE -ne 0) { throw 'aqtinstall kurulamadı' }
    Invoke-Python -m aqt install-qt windows desktop $QtVersion $QtArch -m @QtModules -O $QtRoot
    if ($LASTEXITCODE -ne 0) { throw 'Qt kurulamadı' }
}

# --- 4. this clone: long paths for git
$repo = Split-Path -Parent $PSScriptRoot
if (Test-Path (Join-Path $repo '.git')) {
    git -C $repo config core.longpaths true
}

Write-Host ''
Write-Host 'Kurulum bitti.' -ForegroundColor Green
Write-Host "  Visual Studio C++ : $(if ($vsDir) { $vsDir } else { 'yeni kuruldu' })"
Write-Host "  Qt                : $qtDir"
Write-Host ''
Write-Host 'Sıradaki adımlar:'
Write-Host '  1. Yeni bir terminal açın (PATH yenilensin).'
Write-Host '  2. gh auth login           (GitHub hesabına giriş, bir kez)'
Write-Host '  3. makam\derle.bat         (ilk derleme uzun sürer)'
Write-Host '  4. makam\calistir.bat      (programı açar)'
