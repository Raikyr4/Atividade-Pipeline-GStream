<#
.SYNOPSIS
    Compila e executa a demonstracao nativa, por padrao em duas janelas.
.DESCRIPTION
    Segundos controla a duracao da midia. SemJanela dispensa monitor e espera
    de relogio. Exportar recebe uma pasta nova para quadros e relatorio JSON.
#>
param(
    [ValidateRange(1, 300)]
    [int]$Segundos = 30,
    [switch]$SemJanela,
    [string]$Exportar,
    [string]$Video,
    [ValidateRange(0, 3600)]
    [int]$Inicio = 0
)

$ErrorActionPreference = 'Stop'

# Recompilar garante que alteracoes feitas no VS Code estejam no executavel.
& "$PSScriptRoot/compilar.ps1"
. "$PSScriptRoot/ambiente.ps1"

# Sem caminho informado, usar o trailer existente incluido na pasta midia.
if (-not $Video) {
    $Video = Join-Path $raizProjeto 'midia/exemplo.webm'

    # O trailer possui abertura preta. No exemplo, comecar em uma cena visivel.
    # Um arquivo do usuario continua comecando em zero, salvo -Inicio explicito.
    if (-not $PSBoundParameters.ContainsKey('Inicio')) {
        $Inicio = 5
    }
}

# Traduzir opcoes do PowerShell para a interface de linha de comando do C++.
$argumentos = @('--video', $Video, '--inicio', "$Inicio", '--segundos', "$Segundos")

if ($SemJanela) {
    $argumentos += '--sem-janela'
}

if ($Exportar) {
    $argumentos += @('--exportar', $Exportar)
}

# Aguardar o processo permite mostrar os resultados e detectar seu codigo de erro.
& (Join-Path $raizProjeto 'build/pipeline_multimidia.exe') @argumentos

if ($LASTEXITCODE -ne 0) {
    throw 'A pipeline terminou com erro.'
}
