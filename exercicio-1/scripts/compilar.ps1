<#
.SYNOPSIS
    Compila a aplicacao C++ com GCC x64 e o SDK GStreamer instalado no Windows.
.DESCRIPTION
    Prefere o compilador portatil da pasta .ferramentas. Valida a arquitetura,
    configura os includes e vincula as import libraries do SDK explicitamente.
    O executavel fica em build; nenhum PATH global ou instalador e alterado.
.PARAMETER Compilador
    Caminho opcional para outro g++.exe x64 que suporte C++17.
#>
param(
    [string]$Compilador
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ambiente.ps1"

# 1. Resolver o compilador. GStreamer x64 nao pode usar o GCC global de 32 bits.
if (-not $Compilador) {
    $portatil = Join-Path $raizProjeto '.ferramentas/w64devkit/bin/g++.exe'

    if (Test-Path $portatil) {
        $Compilador = $portatil
    }
    else {
        $Compilador = (Get-Command g++ -ErrorAction Stop).Source
    }
}

# 2. Conferir o alvo antes de compilar, para dar um erro claro de incompatibilidade.
$arquitetura = & $Compilador -dumpmachine

if ($LASTEXITCODE -ne 0 -or $arquitetura -notmatch 'x86_64') {
    throw "GStreamer e x64; compilador encontrado: $arquitetura. Use MinGW-w64 x64 com C++17."
}

$pastaBuild = Join-Path $raizProjeto 'build'
New-Item -ItemType Directory -Force $pastaBuild | Out-Null

# 3. Montar argumentos separados preserva caminhos que contem espacos.
# -I informa os cabecalhos; as bibliotecas .dll.a resolvem chamadas ao runtime.
# Usar caminhos completos evita selecionar por engano a libstdc++ do SDK.
$argumentos = @(
    '-std=c++17',
    '-O2',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-static-libgcc',
    '-static-libstdc++',
    "-I$raizGStreamer/include/gstreamer-1.0",
    "-I$raizGStreamer/include/glib-2.0",
    "-I$raizGStreamer/lib/glib-2.0/include",
    (Join-Path $raizProjeto 'src/principal.cpp'),
    '-o',
    (Join-Path $pastaBuild 'pipeline_multimidia.exe'),
    "$raizGStreamer/lib/libgstvideo-1.0.dll.a",
    "$raizGStreamer/lib/libgstbase-1.0.dll.a",
    "$raizGStreamer/lib/libgstreamer-1.0.dll.a",
    "$raizGStreamer/lib/libgobject-2.0.dll.a",
    "$raizGStreamer/lib/libglib-2.0.dll.a"
)

# 4. Executar o compilador e propagar falhas para os scripts que o chamaram.
& $Compilador @argumentos

if ($LASTEXITCODE -ne 0) {
    throw 'Compilacao falhou.'
}

Write-Host "Executavel pronto: $pastaBuild/pipeline_multimidia.exe"
