$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/compilar.ps1"
. "$PSScriptRoot/ambiente.ps1"

python (Join-Path $raizProjeto 'testes/verificar.py') (Join-Path $raizProjeto 'build/pipeline_multimidia.exe')

if ($LASTEXITCODE -ne 0) {
    throw 'Teste de integracao falhou.'
}
