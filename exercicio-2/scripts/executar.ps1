param(
  [ValidateSet('A', 'B')]
  [string]$Pcm = 'A',
  [string]$Saida
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path $PSScriptRoot -Parent
$gstreamer = $env:GSTREAMER_1_0_ROOT_MINGW_X86_64
if (-not $gstreamer) { $gstreamer = 'C:/Program Files/gstreamer/1.0/mingw_x86_64' }
$env:PATH = "$gstreamer/bin;$env:PATH"

& "$PSScriptRoot/compilar.ps1"
$argumentos = @('--pcm', $Pcm)
if ($Saida) { $argumentos += @('--saida', $Saida) }
& (Join-Path $raiz 'build/exercicio_2.exe') @argumentos
if ($LASTEXITCODE -ne 0) { throw 'A pipeline terminou com erro.' }
