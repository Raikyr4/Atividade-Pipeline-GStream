# Prepara somente o processo atual; nao altera o PATH global do Windows.
$raizProjeto = Split-Path $PSScriptRoot -Parent
$raizGStreamer = $env:GSTREAMER_1_0_ROOT_MINGW_X86_64

if (-not $raizGStreamer) {
    $raizGStreamer = 'C:/Program Files/gstreamer/1.0/mingw_x86_64'
}

if (-not (Test-Path (Join-Path $raizGStreamer 'include/gstreamer-1.0/gst/gst.h'))) {
    throw 'GStreamer MinGW x64 Development nao encontrado. Configure GSTREAMER_1_0_ROOT_MINGW_X86_64.'
}

$env:PATH = "$(Join-Path $raizGStreamer 'bin');$env:PATH"
