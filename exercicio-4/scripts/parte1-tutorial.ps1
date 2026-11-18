# Parte 1 - teste de rede: video de teste H.264 em MPEG-TS sobre UDP.
# No receptor, abra o VLC em Midia > Abrir Fluxo de Rede > udp://@:5000.
# Uso: ./scripts/parte1-tutorial.ps1 -Destino IP_DO_RECEPTOR
param([Parameter(Mandatory = $true)][string]$Destino, [int]$Porta = 5000)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ambiente.ps1"
$env:PATH = "$(Join-Path $gstRuntime 'bin');$env:PATH"

foreach ($plugin in 'x264enc', 'mpegtsmux', 'udpsink') {
  & gst-inspect-1.0 $plugin *> $null
  if ($LASTEXITCODE -ne 0) { throw "Plugin $plugin nao encontrado." }
  Write-Host "$plugin OK"
}
Write-Host "Enviando para udp://${Destino}:$Porta (Ctrl+C para parar)"
& gst-launch-1.0 -v videotestsrc is-live=true pattern=ball `
  '!' video/x-raw,width=640,height=360,framerate=15/1 '!' videoconvert `
  '!' x264enc tune=zerolatency bitrate=500 speed-preset=ultrafast `
  '!' h264parse '!' mpegtsmux '!' udpsink host=$Destino port=$Porta
