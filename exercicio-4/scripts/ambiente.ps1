# Escolhe o SDK MinGW instalado ou o SDK portatil preparado neste repositorio.
$raiz = Split-Path $PSScriptRoot -Parent
$repositorio = Split-Path $raiz -Parent
$mingw = $env:GSTREAMER_1_0_ROOT_MINGW_X86_64
if (-not $mingw) { $mingw = 'C:/Program Files/gstreamer/1.0/mingw_x86_64' }
$msvcLocal = Join-Path $repositorio '.ferramentas/gstreamer-sdk/PFiles64/gstreamer/1.0/msvc_x86_64'
if (Test-Path (Join-Path $mingw 'include/gstreamer-1.0/gst/gst.h')) {
    $gstSdk = $mingw
    $gstBibliotecas = @('libgstreamer-1.0.dll.a', 'libgobject-2.0.dll.a', 'libglib-2.0.dll.a')
    $gstRuntime = $mingw
} elseif (Test-Path (Join-Path $msvcLocal 'include/gstreamer-1.0/gst/gst.h')) {
    $gstSdk = $msvcLocal
    $gstBibliotecas = @('gstreamer-1.0.lib', 'gobject-2.0.lib', 'glib-2.0.lib')
    $gstRuntime = $env:GSTREAMER_1_0_ROOT_MSVC_X86_64
    if (-not $gstRuntime) {
        $lancador = Get-Command gst-launch-1.0.exe -ErrorAction SilentlyContinue
        if ($lancador) { $gstRuntime = Split-Path (Split-Path $lancador.Source -Parent) -Parent }
    }
} else {
    throw 'SDK GStreamer nao encontrado. Instale Runtime e Development MinGW x64 e configure GSTREAMER_1_0_ROOT_MINGW_X86_64.'
}
if (-not $gstRuntime -or -not (Test-Path (Join-Path $gstRuntime 'bin/gst-launch-1.0.exe'))) {
    throw 'Runtime GStreamer nao encontrado. Configure a variavel do runtime correspondente ao SDK.'
}
