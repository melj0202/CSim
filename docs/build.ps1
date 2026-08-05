[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$docsRoot = $PSScriptRoot
$latexRoot = Join-Path $docsRoot 'latex'
$outputRoot = Join-Path $docsRoot 'output'
$sourceFile = Join-Path $latexRoot 'illumo.tex'

if (-not (Test-Path -LiteralPath $sourceFile)) {
    throw "Illumo LaTeX source was not found: $sourceFile"
}

$latexmkCommand = Get-Command -Name 'latexmk' -ErrorAction SilentlyContinue
if ($null -eq $latexmkCommand) {
    throw 'latexmk was not found on PATH. Install TeX Live or add its bin directory to PATH.'
}

New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

Push-Location -LiteralPath $latexRoot
try {
    $outputArgument = '-outdir=' + $outputRoot
    if ($Clean) {
        & $latexmkCommand.Source '-C' $outputArgument 'illumo.tex'
    }
    else {
        & $latexmkCommand.Source '-pdf' '-interaction=nonstopmode' '-halt-on-error' $outputArgument 'illumo.tex'
    }

    if ($LASTEXITCODE -ne 0) {
        throw "latexmk failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
