param(
    [switch]$Package,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDir = Join-Path $Root 'build/windows'
$DistDir = Join-Path $Root 'dist/windows'
$StageDir = Join-Path $DistDir 'stage'

if ($Clean) {
    Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item $DistDir -Recurse -Force -ErrorAction SilentlyContinue
}

$cmakeFile = Get-Content (Join-Path $Root 'CMakeLists.txt') -Raw
if ($env:SONRAPTUNE_VERSION) {
    $RawVersion = $env:SONRAPTUNE_VERSION -replace '^release/', ''
    $Version = $RawVersion.TrimStart('v')
} elseif ($cmakeFile -match 'project\(SonRapTune VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    $Version = $Matches[1]
} else {
    throw 'Could not determine SonRapTune version.'
}

$cmakeHelp = (& cmake --help | Out-String)
if ($cmakeHelp -match 'Visual Studio 18 2026') {
    $Generator = 'Visual Studio 18 2026'
} elseif ($cmakeHelp -match 'Visual Studio 17 2022') {
    $Generator = 'Visual Studio 17 2022'
} else {
    throw 'Visual Studio 2026 or 2022 CMake generator was not found. Install Desktop development with C++.'
}

Write-Host "Building SonRapTune $Version for Windows x64 using $Generator..." -ForegroundColor Cyan

cmake -S $Root -B $BuildDir `
    -G $Generator -A x64 `
    -DSONRAPTUNE_BUILD_PLUGIN=ON `
    -DSONRAPTUNE_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }

cmake --build $BuildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

ctest --test-dir $BuildDir -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }

$Artefacts = Join-Path $BuildDir 'SonRapTune_artefacts/Release'
$Vst3 = Join-Path $Artefacts 'VST3/SonRapTune.vst3'
$Standalone = Join-Path $Artefacts 'Standalone/SonRapTune.exe'
if (-not (Test-Path $Vst3)) { throw "VST3 not found: $Vst3" }
if (-not (Test-Path $Standalone)) { throw "Standalone not found: $Standalone" }

Remove-Item $StageDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item (Join-Path $StageDir 'VST3') -ItemType Directory -Force | Out-Null
New-Item (Join-Path $StageDir 'Standalone') -ItemType Directory -Force | Out-Null
Copy-Item $Vst3 (Join-Path $StageDir 'VST3') -Recurse -Force
Copy-Item $Standalone (Join-Path $StageDir 'Standalone/SonRapTune.exe') -Force

$ZipPath = Join-Path $DistDir "SonRapTune-$Version-Windows-x64.zip"
Remove-Item $ZipPath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $StageDir '*') -DestinationPath $ZipPath -CompressionLevel Optimal
Write-Host "Portable package: $ZipPath" -ForegroundColor Green

if ($Package) {
    $isccCandidates = @(
        (Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6/ISCC.exe')
    ) | Where-Object { $_ -and (Test-Path $_) }

    if ($isccCandidates.Count -eq 0) {
        Write-Warning 'Inno Setup 6 was not found. VST3 and Standalone are built; installer was skipped.'
        Write-Host 'Install Inno Setup 6, then run build-windows.bat again.'
    } else {
        $Iscc = $isccCandidates[0]
        & $Iscc `
            "/DSourceRoot=$StageDir" `
            "/DAppVersion=$Version" `
            "/DOutputDir=$DistDir" `
            (Join-Path $Root 'packaging/windows/SonRapTune.iss')
        if ($LASTEXITCODE -ne 0) { throw 'Inno Setup packaging failed.' }
        Write-Host "Windows installer created in $DistDir" -ForegroundColor Green
    }
}

Write-Host 'Done.' -ForegroundColor Cyan
