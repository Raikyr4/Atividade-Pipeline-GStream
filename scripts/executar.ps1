param(
    [ValidateRange(1, 300)]
    [int]$Segundos = 30,
    [switch]$SemJanela,
    [string]$Exportar
)

$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/compilar.ps1"
. "$PSScriptRoot/ambiente.ps1"

$argumentos = @('--segundos', "$Segundos")

if ($SemJanela) {
    $argumentos += '--sem-janela'
}

if ($Exportar) {
    $argumentos += @('--exportar', $Exportar)
}

& (Join-Path $raizProjeto 'build/pipeline_multimidia.exe') @argumentos

if ($LASTEXITCODE -ne 0) {
    throw 'A pipeline terminou com erro.'
}
