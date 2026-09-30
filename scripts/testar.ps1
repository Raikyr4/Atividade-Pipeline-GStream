# Compila antes de testar para nao validar um executavel antigo por engano.
$ErrorActionPreference = 'Stop'

& "$PSScriptRoot/compilar.ps1"
. "$PSScriptRoot/ambiente.ps1"

# O Python executa o C++ real e inspeciona arquivos e medicoes da pipeline.
# Nao simula o processamento multimidia nem depende de uma janela grafica.
python (Join-Path $raizProjeto 'testes/verificar.py') (Join-Path $raizProjeto 'build/pipeline_multimidia.exe')

if ($LASTEXITCODE -ne 0) {
    throw 'Teste de integracao falhou.'
}
