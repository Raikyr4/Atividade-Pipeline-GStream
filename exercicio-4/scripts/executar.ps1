# Executa o transmissor ou o receptor (compile antes com scripts/compilar.ps1).
# Exemplos:
#   ./scripts/executar.ps1 -Modo receptor -Latencia 50 -Duracao 30 -Registro
#   ./scripts/executar.ps1 -Modo transmissor -Destino 192.168.10.2 -Fonte bipe
#   ./scripts/executar.ps1 -Modo transmissor -Config ./rede.ini
param(
  [Parameter(Mandatory=$true)][ValidateSet('transmissor','receptor')][string]$Modo,
  [string]$Destino, [int]$Porta = 0, [int]$Latencia = -1, [int]$Duracao = 0,
  [ValidateSet('', 'seno', 'bipe', 'mic', 'arquivo')][string]$Fonte = '',
  [string]$Arquivo, [string]$Config,
  [switch]$Silencioso, [switch]$SimularRede, [switch]$Registro
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ambiente.ps1"
$env:PATH = "$(Join-Path $gstRuntime 'bin');$env:PATH"
$exe = Join-Path $raiz 'build/exercicio_4.exe'
if (-not (Test-Path $exe)) { throw 'Compile primeiro com scripts/compilar.ps1.' }

# Fonte arquivo sem caminho: musica do trailer Sintel ja usada nos exercicios anteriores.
if ($Fonte -eq 'arquivo' -and -not $Arquivo) { $Arquivo = Join-Path $repositorio 'exercicio-1/midia/exemplo.webm' }

$argsApp = @($Modo)
if ($Config) { $argsApp += @('--config', $Config) }
if ($Porta -gt 0) { $argsApp += @('--porta', "$Porta") }
if ($Duracao -gt 0) { $argsApp += @('--duracao', "$Duracao") }
if ($Modo -eq 'transmissor') {
  if ($Destino) { $argsApp += @('--destino', $Destino) }
  if ($Fonte) { $argsApp += @('--fonte', $Fonte) }
  if ($Arquivo) { $argsApp += @('--arquivo', $Arquivo) }
  if ($SimularRede) { $argsApp += '--simular-rede' }
} else {
  if ($Latencia -ge 0) { $argsApp += @('--latencia', "$Latencia") }
  if ($Silencioso) { $argsApp += '--silencioso' }
}

if ($Registro -and $Modo -eq 'receptor') {
  # Salva console (.txt) e estatisticas por segundo (.csv) em evidencias/.
  $nome = if ($Latencia -ge 0) { "latencia_${Latencia}ms" } else { 'latencia_config' }
  $pasta = Join-Path $raiz 'evidencias'
  New-Item -ItemType Directory -Force $pasta | Out-Null
  $argsApp += @('--csv', (Join-Path $pasta "$nome.csv"))
  # Tee-Object do PowerShell 5.1 grava UTF-16; Add-Content com UTF8 mantem o log legivel.
  $log = Join-Path $pasta "$nome.txt"
  Set-Content -Path $log -Value '' -Encoding UTF8
  & $exe @argsApp | ForEach-Object { $_; Add-Content -Path $log -Value $_ -Encoding UTF8 }
} else {
  & $exe @argsApp
}
if ($LASTEXITCODE -ne 0) { throw 'A aplicacao terminou com erro.' }
