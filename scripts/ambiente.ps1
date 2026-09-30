# Este arquivo e carregado por dot-sourcing: suas variaveis ficam disponiveis
# ao script chamador. A raiz e calculada a partir do arquivo, nao do terminal,
# permitindo iniciar o projeto mesmo a partir de outro diretorio.
$raizProjeto = Split-Path $PSScriptRoot -Parent

# Preferir a variavel definida pelo instalador; usar o caminho local como reserva.
$raizGStreamer = $env:GSTREAMER_1_0_ROOT_MINGW_X86_64

if (-not $raizGStreamer) {
    $raizGStreamer = 'C:/Program Files/gstreamer/1.0/mingw_x86_64'
}

if (-not (Test-Path (Join-Path $raizGStreamer 'include/gstreamer-1.0/gst/gst.h'))) {
    throw 'GStreamer MinGW x64 Development nao encontrado. Configure GSTREAMER_1_0_ROOT_MINGW_X86_64.'
}

# O Windows precisa localizar DLLs do runtime e o scanner de plugins.
# A alteracao vale apenas neste processo; o PATH global permanece intacto.
$env:PATH = "$(Join-Path $raizGStreamer 'bin');$env:PATH"
