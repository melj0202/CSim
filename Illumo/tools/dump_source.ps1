<#
.SYNOPSIS
  Concatenate first-party Illumo source into a single plain-text file.

.DESCRIPTION
  Walks Illumo project sources (Source/, Shader/, top-level CMakeLists.txt)
  and writes one file with clear per-file banners. Skips thirdparty/,
  build trees, binaries, and other non-source paths.

.PARAMETER OutFile
  Output path. Default: Illumo/Source/all.txt (relative to repo Illumo root).

.PARAMETER Root
  Project root containing Source/ and Shader/. Default: parent of this script's
  directory (i.e. Illumo/).

.EXAMPLE
  .\Illumo\tools\dump_source.ps1
  .\Illumo\tools\dump_source.ps1 -OutFile .\Illumo\illumo_first_party.txt
#>

[CmdletBinding()]
param(
    [string]$Root = "",
    [string]$OutFile = ""
)

$ErrorActionPreference = "Stop"

if (-not $Root) {
    $Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}
else {
    $Root = (Resolve-Path $Root).Path
}

if (-not $OutFile) {
    $OutFile = Join-Path $Root "Source\all.txt"
}
elseif (-not [System.IO.Path]::IsPathRooted($OutFile)) {
    $OutFile = Join-Path (Get-Location) $OutFile
}

# First-party roots to scan (relative to $Root)
$scanDirs = @(
    (Join-Path $Root "Source"),
    (Join-Path $Root "Shader")
)

# Also include top-level project CMake if present
$extraFiles = @(
    (Join-Path $Root "CMakeLists.txt")
)

# Directory name segments to skip anywhere under a scan root
$excludeDirNames = @(
    "thirdparty", "third_party", "ThirdParty",
    "build", "out", "CMakeFiles", ".git", ".vs",
    "x64", "x86", "Debug", "Release", "RelWithDebInfo", "MinSizeRel",
    ".agents", "node_modules", "__pycache__"
)

# File basenames to skip
$excludeFileNames = @(
    "all.txt",           # our own output
    "debug.cppcheck",
    "makefile"
)

# Pure source dump: code + shaders + CMake, not README/docs
$includeExtStrict = @(
    ".h", ".hpp", ".hh", ".hxx",
    ".c", ".cc", ".cpp", ".cxx",
    ".mm", ".m",
    ".glsl", ".hlsl", ".metal",
    ".cmake"
)

function Test-ExcludedPath {
    param([string]$FullPath)
    $parts = $FullPath -split '[\\/]'
    foreach ($p in $parts) {
        if ($excludeDirNames -contains $p) { return $true }
    }
    return $false
}

function Get-RelativePath {
    param([string]$Base, [string]$Full)
    $baseUri = [Uri]((Join-Path $Base ".") + [IO.Path]::DirectorySeparatorChar)
    $fullUri = [Uri]$Full
    return [Uri]::UnescapeDataString($baseUri.MakeRelativeUri($fullUri).ToString()) -replace '/', [IO.Path]::DirectorySeparatorChar
}

$files = New-Object System.Collections.Generic.List[string]

foreach ($dir in $scanDirs) {
    if (-not (Test-Path -LiteralPath $dir)) {
        Write-Warning "Skip missing directory: $dir"
        continue
    }
    Get-ChildItem -LiteralPath $dir -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object {
            $ext = $_.Extension.ToLowerInvariant()
            if ($includeExtStrict -notcontains $ext -and $_.Name -ne "CMakeLists.txt") {
                return $false
            }
            if (Test-ExcludedPath $_.FullName) { return $false }
            if ($excludeFileNames -contains $_.Name) { return $false }
            # Do not re-include the output file if it sits under Source/
            if ($OutFile -and ((Resolve-Path -LiteralPath $_.FullName -ErrorAction SilentlyContinue).Path -eq
                (Resolve-Path -LiteralPath $OutFile -ErrorAction SilentlyContinue).Path)) {
                return $false
            }
            return $true
        } |
        ForEach-Object { [void]$files.Add($_.FullName) }
}

foreach ($f in $extraFiles) {
    if ((Test-Path -LiteralPath $f) -and -not (Test-ExcludedPath $f)) {
        if (-not $files.Contains($f)) { [void]$files.Add($f) }
    }
}

$sorted = $files | Sort-Object

$outDir = Split-Path -Parent $OutFile
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

$utf8NoBom = New-Object System.Text.UTF8Encoding $false
$sw = New-Object System.IO.StreamWriter($OutFile, $false, $utf8NoBom)

try {
    $generated = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss K")
    $sw.WriteLine("================================================================================")
    $sw.WriteLine("Illumo first-party source dump (NOT thirdparty)")
    $sw.WriteLine("Generated: $generated")
    $sw.WriteLine("Root:      $Root")
    $sw.WriteLine("Files:     $($sorted.Count)")
    $sw.WriteLine("Excluded:  thirdparty/, build artifacts, docs, binaries")
    $sw.WriteLine("================================================================================")
    $sw.WriteLine("")

    $index = 0
    foreach ($path in $sorted) {
        $index++
        $rel = Get-RelativePath -Base $Root -Full $path
        $sw.WriteLine("")
        $sw.WriteLine("################################################################################")
        $sw.WriteLine("# FILE: $rel")
        $sw.WriteLine("# PATH: $path")
        $sw.WriteLine("################################################################################")
        $sw.WriteLine("")

        # Read as text; skip if clearly binary
        try {
            $bytes = [System.IO.File]::ReadAllBytes($path)
            if ($bytes.Length -ge 2 -and $bytes[0] -eq 0 -and $bytes[1] -eq 0) {
                $sw.WriteLine("/* [binary or empty - skipped] */")
                continue
            }
            # NUL in first 8K => binary
            $probeLen = [Math]::Min($bytes.Length, 8192)
            $hasNul = $false
            for ($i = 0; $i -lt $probeLen; $i++) {
                if ($bytes[$i] -eq 0) { $hasNul = $true; break }
            }
            if ($hasNul) {
                $sw.WriteLine("/* [binary - skipped] */")
                continue
            }

            $text = [System.Text.Encoding]::UTF8.GetString($bytes)
            # Normalize line endings in the dump for readability
            $text = $text -replace "`r`n", "`n" -replace "`r", "`n"
            $sw.Write($text)
            if (-not $text.EndsWith("`n")) {
                $sw.WriteLine("")
            }
        }
        catch {
            $sw.WriteLine("/* [error reading file: $($_.Exception.Message)] */")
        }
    }

    $sw.WriteLine("")
    $sw.WriteLine("================================================================================")
    $sw.WriteLine("END OF DUMP ($($sorted.Count) files)")
    $sw.WriteLine("================================================================================")
}
finally {
    $sw.Close()
}

Write-Host "Wrote $($sorted.Count) files -> $OutFile"
$size = (Get-Item -LiteralPath $OutFile).Length
Write-Host ("Size: {0:N0} bytes ({1:N1} KB)" -f $size, ($size / 1KB))
