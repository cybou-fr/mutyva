# MUTYVA native acceptance on Windows + MSVC 2022 + Qt 6.12.x.
# Same script is executed locally and by GitHub Actions after installing Qt.
[CmdletBinding()]
param(
    [string] $QtRoot = $env:QT_ROOT_DIR,
    [string] $BuildDir = "build-ci",
    [string] $CaptureDir = "palette-captures",
    [string] $EvidenceDir = "qt612-evidence"
)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Require-Success([string] $Stage) {
    if ($LASTEXITCODE -ne 0) { throw "$Stage failed with exit code $LASTEXITCODE" }
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($QtRoot)) {
    throw "Pass -QtRoot C:\Qt\6.12.0\msvc2022_64 or set QT_ROOT_DIR."
}
$QtRoot = (Resolve-Path $QtRoot -ErrorAction Stop).Path
$qmake = Join-Path $QtRoot "bin/qmake.exe"
if (-not (Test-Path $qmake)) { throw "Qt qmake.exe is missing: $qmake" }
$version = (& $qmake -query QT_VERSION).Trim()
Require-Success "Qt version query"
if ($version -notmatch '^6[.]12[.][0-9]+$') {
    throw "Qt 6.12.x required: found $version. No older/newer fallback."
}
Write-Host "MUTYVA native acceptance: Qt $version, MSVC 2022, Windows"

function Resolve-Output([string] $Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) { return $Path }
    return (Join-Path $repo $Path)
}
$build = Resolve-Output $BuildDir
$gallery = Resolve-Output $CaptureDir
$evidence = Resolve-Output $EvidenceDir
# For reproducible CMake resolution, never reuse a cache created with an older Qt.
if (Test-Path (Join-Path $build "CMakeCache.txt")) {
    Remove-Item -LiteralPath (Join-Path $build "CMakeCache.txt") -Force
}
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
# The screenshot gate must not pass by counting stale files from a prior run.
if (Test-Path $gallery) { Remove-Item -LiteralPath $gallery -Recurse -Force }
New-Item -ItemType Directory -Path $gallery -Force | Out-Null

$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QUICK_CONTROLS_STYLE = "Fusion"
$env:QT_PLUGIN_PATH = Join-Path $QtRoot "plugins"
$env:QML_IMPORT_PATH = Join-Path $QtRoot "qml"
$env:QT_QPA_FONTDIR = Join-Path $env:WINDIR "Fonts"
$env:PATH = (Join-Path $QtRoot "bin") + [IO.Path]::PathSeparator + $env:PATH

& cmake -S $repo -B $build -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$QtRoot" "-DBUILD_TESTING=ON"
Require-Success "Configure"
& cmake --build $build --config Release --parallel 2
Require-Success "Build"

& ctest --test-dir $build -C Release --output-on-failure
$testCode = $LASTEXITCODE
$runtimeLog = Join-Path $build "runtime-tests.txt"
if (Test-Path $runtimeLog) {
    Get-Content $runtimeLog | Select-Object -Last 450
}
if ($testCode -ne 0) { throw "CTest failed with exit code $testCode" }

$env:MUTYVA_PALETTE_CAPTURE_DIR = $gallery
$exe = Join-Path $build "Release/runtime-tests.exe"
if (-not (Test-Path $exe)) { throw "Built Qt runtime-tests.exe is missing: $exe" }
& $exe paletteCaptureScreenshots -o -,txt
Require-Success "Actual Qt/QML palette render"

$images = @(Get-ChildItem -LiteralPath $gallery -Filter "*.png" -File)
if ($images.Count -ne 21) {
    throw "Expected 21 fresh Qt screenshots, got $($images.Count)"
}
$hashes = @()
foreach ($f in $images) {
    if ($f.Length -lt 1024) { throw "Screenshot empty or too small: $($f.Name)" }
    $hashes += (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
}
if (@($hashes | Select-Object -Unique).Count -ne 21) {
    throw "Palette screenshot collision: expected 21 distinct render hashes"
}
$head = (& git -C $repo rev-parse HEAD).Trim()
Require-Success "Git revision query"
$report = [ordered]@{
    status = "QT612_PASS"
    version = $version
    compiler = "MSVC 2022"
    gitCommit = $head
    timestampUtc = [DateTime]::UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ")
    runtimeCTestExitCode = 0
    screenshotCount = $images.Count
    screenshots = @($images | ForEach-Object { $_.Name })
}
$reportFile = Join-Path $evidence "qt612-acceptance.json"
$report | ConvertTo-Json -Depth 5 | Set-Content -Path $reportFile -Encoding utf8
Write-Host "PASS Qt ${version}: CTest and all 21 individual real Qt screenshot captures"
Write-Host "Evidence: $reportFile"
