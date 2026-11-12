$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ambiente.ps1"
$portatil = Join-Path $repositorio '.ferramentas/w64devkit/bin/g++.exe'
$cc = if (Test-Path $portatil) { $portatil } else { (Get-Command g++ -ErrorAction Stop).Source }
if ((& $cc -dumpmachine) -notmatch 'x86_64') { throw 'Use um compilador MinGW-w64 x64.' }
New-Item -ItemType Directory -Force (Join-Path $raiz 'build') | Out-Null
$libs = @($gstBibliotecas | ForEach-Object { Join-Path $gstSdk "lib/$_" })
& $cc -std=c++17 -O2 -Wall -Wextra -Wpedantic -static-libgcc -static-libstdc++ `
  "-I$gstSdk/include/gstreamer-1.0" "-I$gstSdk/include/glib-2.0" "-I$gstSdk/lib/glib-2.0/include" `
  (Join-Path $raiz 'src/principal.cpp') -o (Join-Path $raiz 'build/exercicio_3.exe') @libs
if ($LASTEXITCODE -ne 0) { throw 'Compilacao falhou.' }
Write-Host 'Compilacao concluida.'
