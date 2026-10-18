param([string]$BuildDirectory = "build-msvc")
$ErrorActionPreference = "Stop"
$mutyvaRoot = Split-Path -Parent $PSScriptRoot
$mutyvaBuild = [System.IO.Path]::GetFullPath((Join-Path $mutyvaRoot $BuildDirectory))
& cmake --build $mutyvaBuild --target live-ai-tests --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$mutyvaTestExe = Join-Path $mutyvaBuild "Release/live-ai-tests.exe"
$mutyvaReport = Join-Path $mutyvaBuild "live-ai-report.json"
Write-Host "Complete ChatGPT sign-in in the browser. The three live checks then run automatically."
$mutyvaTestProcess = Start-Process -FilePath $mutyvaTestExe -WorkingDirectory (Split-Path $mutyvaTestExe) -ArgumentList @("--report", ('"' + $mutyvaReport + '"')) -PassThru -Wait
Write-Host "Report: $mutyvaReport"
exit $mutyvaTestProcess.ExitCode
