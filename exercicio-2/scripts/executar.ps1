param(
  [ValidateSet('A', 'B')]
  [string]$Pcm = 'A',
  [string]$Saida,
  [string]$Video
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path $PSScriptRoot -Parent
$gstreamer = $env:GSTREAMER_1_0_ROOT_MINGW_X86_64
if (-not $gstreamer) { $gstreamer = 'C:/Program Files/gstreamer/1.0/mingw_x86_64' }
$env:PATH = "$gstreamer/bin;$env:PATH"

# Sem caminho informado, usar o trailer Sintel ja incluido no exercicio 1.
if (-not $Video) { $Video = Join-Path (Split-Path $raiz -Parent) 'exercicio-1/midia/exemplo.webm' }

& "$PSScriptRoot/compilar.ps1"
$argumentos = @('--video', $Video, '--pcm', $Pcm)
if ($Saida) { $argumentos += @('--saida', $Saida) }
& (Join-Path $raiz 'build/exercicio_2.exe') @argumentos
if ($LASTEXITCODE -ne 0) { throw 'A pipeline terminou com erro.' }
