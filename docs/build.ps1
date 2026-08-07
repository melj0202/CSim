[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$docsRoot = $PSScriptRoot
$latexRoot = Join-Path $docsRoot 'latex'
$outputRoot = Join-Path $docsRoot 'output'
$sources = @(
    (Join-Path $latexRoot 'illumo.tex'),
    (Join-Path $latexRoot 'architecture-map.tex')
)

foreach ($sourceFile in $sources) {
    if (-not (Test-Path -LiteralPath $sourceFile)) {
        throw "Illumo LaTeX source was not found: $sourceFile"
    }
}

$latexmkCommand = Get-Command -Name 'latexmk' -ErrorAction SilentlyContinue
if ($null -eq $latexmkCommand) {
    throw 'latexmk was not found on PATH. Install TeX Live or add its bin directory to PATH.'
}

New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

Push-Location -LiteralPath $latexRoot
try {
    $outputArgument = '-outdir=' + $outputRoot
    foreach ($sourceFile in $sources) {
        $leaf = Split-Path -Leaf $sourceFile
        if ($Clean) {
            & $latexmkCommand.Source '-C' $outputArgument $leaf
        }
        else {
            & $latexmkCommand.Source '-pdf' '-interaction=nonstopmode' '-halt-on-error' $outputArgument $leaf
        }

        if ($LASTEXITCODE -ne 0) {
            throw "latexmk failed on $leaf with exit code $LASTEXITCODE"
        }
    }
}
finally {
    Pop-Location
}
