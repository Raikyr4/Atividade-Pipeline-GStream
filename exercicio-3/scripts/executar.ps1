param(
  [Parameter(Mandatory=$true)][ValidateSet('transmissor','receptor')][string]$Modo,
  [string]$Destino = '127.0.0.1', [int]$Porta = 5004,
  [int]$Latencia = 200, [int]$Duracao = 0, [switch]$Silencioso
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ambiente.ps1"
$env:PATH = "$(Join-Path $gstRuntime 'bin');$env:PATH"
$exe = Join-Path $raiz 'build/exercicio_3.exe'
if (-not (Test-Path $exe)) { throw 'Compile primeiro com scripts/compilar.ps1.' }
$argsApp = @($Modo, '--porta', "$Porta", '--duracao', "$Duracao")
if ($Modo -eq 'transmissor') { $argsApp += @('--destino', $Destino) }
else { $argsApp += @('--latencia', "$Latencia"); if ($Silencioso) { $argsApp += '--silencioso' } }
& $exe @argsApp
if ($LASTEXITCODE -ne 0) { throw 'A aplicacao terminou com erro.' }
