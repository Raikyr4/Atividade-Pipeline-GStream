param([string]$Compilador)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ambiente.ps1"
if (-not $Compilador) {
    $portatil = Join-Path $raizProjeto '.ferramentas/w64devkit/bin/g++.exe'
    if (Test-Path $portatil) { $Compilador = $portatil }
    else { $Compilador = (Get-Command g++ -ErrorAction Stop).Source }
}
$arquitetura = & $Compilador -dumpmachine
if ($LASTEXITCODE -ne 0 -or $arquitetura -notmatch 'x86_64') {
    throw "GStreamer e x64; compilador encontrado: $arquitetura. Use MinGW-w64 x64 com C++17."
}
$pastaBuild = Join-Path $raizProjeto 'build'
New-Item -ItemType Directory -Force $pastaBuild | Out-Null
$argumentos = @(
    '-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-static-libgcc', '-static-libstdc++',
    "-I$raizGStreamer/include/gstreamer-1.0", "-I$raizGStreamer/include/glib-2.0",
    "-I$raizGStreamer/lib/glib-2.0/include",
    (Join-Path $raizProjeto 'src/principal.cpp'), '-o', (Join-Path $pastaBuild 'pipeline_multimidia.exe'),
    "$raizGStreamer/lib/libgstvideo-1.0.dll.a", "$raizGStreamer/lib/libgstbase-1.0.dll.a",
    "$raizGStreamer/lib/libgstreamer-1.0.dll.a", "$raizGStreamer/lib/libgobject-2.0.dll.a",
    "$raizGStreamer/lib/libglib-2.0.dll.a"
)
& $Compilador @argumentos
if ($LASTEXITCODE -ne 0) { throw 'Compilacao falhou.' }
Write-Host "Executavel pronto: $pastaBuild/pipeline_multimidia.exe"
