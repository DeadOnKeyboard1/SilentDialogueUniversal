# Copyright (C) 2026 DeadOnKeyboard
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Ffmpeg,
    [Parameter(Mandatory = $true)]
    [string]$SkyrimRoot,
    [string]$XwmaEncode,
    [string]$LipGenerator
)

$ErrorActionPreference = "Stop"

if (-not $Ffmpeg) {
    $ffmpegCommand = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if (-not $ffmpegCommand) {
        throw "ffmpeg was not found. Add it to PATH or pass -Ffmpeg <path>."
    }
    $Ffmpeg = $ffmpegCommand.Source
}
if (-not $XwmaEncode) {
    $XwmaEncode = Join-Path $SkyrimRoot "Tools\Audio\xwmaencode.exe"
}
if (-not $LipGenerator) {
    $LipGenerator = Join-Path $SkyrimRoot "Tools\LipGen\LipGenerator\LipGenerator.exe"
}

foreach ($tool in $Ffmpeg, $XwmaEncode, $LipGenerator) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) {
        throw "Required tool was not found: $tool"
    }
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$output = Join-Path $projectRoot "package\Sound\Voice\SilentDialogueUniversal"
$work = Join-Path $projectRoot "asset-build"
New-Item -ItemType Directory -Force -Path $output, $work | Out-Null

foreach ($seconds in 1..10) {
    $wav = Join-Path $work "silence_$seconds.wav"
    $xwm = Join-Path $work "silence_$seconds.xwm"
    $lip = Join-Path $work "silence_$seconds.lip"
    $fuz = Join-Path $output "silence_$seconds.fuz"

    & $Ffmpeg -hide_banner -loglevel error -f lavfi -i "anullsrc=r=44100:cl=mono" -t $seconds -c:a pcm_s16le -y $wav
    if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed for $seconds seconds" }

    & $LipGenerator $wav "..." "-OutputFileName:$lip"
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $lip)) { throw "LipGenerator failed for $seconds seconds" }

    & $XwmaEncode $wav $xwm
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $xwm)) { throw "xwmaencode failed for $seconds seconds" }

    $lipBytes = [IO.File]::ReadAllBytes($lip)
    $xwmBytes = [IO.File]::ReadAllBytes($xwm)
    $stream = [IO.File]::Create($fuz)
    try {
        $writer = [IO.BinaryWriter]::new($stream)
        $writer.Write([Text.Encoding]::ASCII.GetBytes("FUZE"))
        $writer.Write([uint32]1)
        $writer.Write([uint32]$lipBytes.Length)
        $writer.Write($lipBytes)
        $writer.Write($xwmBytes)
        $writer.Flush()
    } finally {
        $stream.Dispose()
    }
}

Write-Output "Generated ten original silent FUZ assets in $output"
