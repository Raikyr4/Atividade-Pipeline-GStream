$ErrorActionPreference = 'Stop'

$raiz = Split-Path $PSScriptRoot -Parent
$gstreamer = $env:GSTREAMER_1_0_ROOT_MINGW_X86_64
if (-not $gstreamer) { $gstreamer = 'C:/Program Files/gstreamer/1.0/mingw_x86_64' }
if (-not (Test-Path (Join-Path $gstreamer 'include/gstreamer-1.0/gst/gst.h'))) {
  throw 'GStreamer MinGW x64 Development nao encontrado. Configure GSTREAMER_1_0_ROOT_MINGW_X86_64.'
}

$portatil = Join-Path (Split-Path $raiz -Parent) '.ferramentas/w64devkit/bin/g++.exe'
$compilador = if (Test-Path $portatil) { $portatil } else { (Get-Command g++ -ErrorAction Stop).Source }
$build = Join-Path $raiz 'build'
New-Item -ItemType Directory -Force $build | Out-Null

& $compilador -std=c++17 -Wall -Wextra -Wpedantic `
  "-I$gstreamer/include/gstreamer-1.0" `
  "-I$gstreamer/include/glib-2.0" `
  "-I$gstreamer/lib/glib-2.0/include" `
  (Join-Path $raiz 'src/principal.cpp') -o (Join-Path $build 'exercicio_2.exe') `
  "$gstreamer/lib/libgstreamer-1.0.dll.a" `
  "$gstreamer/lib/libgobject-2.0.dll.a" `
  "$gstreamer/lib/libglib-2.0.dll.a"

if ($LASTEXITCODE -ne 0) { throw 'Compilacao falhou.' }
