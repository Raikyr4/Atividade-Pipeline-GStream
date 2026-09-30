# Validação, limites e diagnóstico

## O que o teste automatizado verifica

`testes/verificar.py` executa o binário C++ por dois segundos de mídia em modo sem janela. Em uma pasta temporária, verifica:

1. A pipeline termina com código 0 e exporta arquivos.
2. As caps medidas são 640×360/RGB/30 FPS e 320×180/GRAY8/10 FPS.
3. O FPS calculado pelos PTS coincide com o esperado.
4. As contagens são próximas de 60 e 20 quadros, com tolerância de um quadro.
5. PPM e PGM têm cabeçalhos corretos e a quantidade exata de bytes de pixels.
6. O original tem pixels cromáticos e o cinza contém diferentes intensidades.
7. Reexportar para a mesma pasta é rejeitado e preserva o arquivo original.
8. Ajuda funciona; duração inválida, argumentos incompletos e opções desconhecidas falham.

Para executar:

```text
powershell -ExecutionPolicy Bypass -File ./scripts/testar.ps1
```

O script nativo compila antes de executar essa verificação. Uma execução bem-sucedida do programa sem erros não substitui conferir os dados: o teste examina tanto o relatório quanto os arquivos de mídia.

## Checklist manual da apresentação

- Abrir as duas janelas no sistema gráfico que será usado na sala.
- Verificar cor no original e cinza no processado.
- Comparar movimento e identificar a cadência menor no processado.
- Conferir no terminal as dimensões e FPS; não inferir resolução pelo tamanho da janela.
- Aguardar EOS natural e confirmar saída 0.
- Repetir com `Ctrl+C` durante a reprodução e confirmar encerramento.
- Gerar imagens em uma pasta nova e abrir o HTML offline.

O teste sem janela não verifica driver de vídeo, fluidez percebida, posição das janelas ou taxa física do monitor. Esses itens exigem ensaio manual no computador da apresentação. Consulte [evidências de execução](07-evidencias.md) para o que foi efetivamente executado nesta entrega.

## Problemas comuns

| Sintoma | Causa possível e ação |
|---|---|
| `no element ...` | Falta plugin. Rode `gst-inspect-1.0 NOME`; instale Base/Good ou complete a instalação. |
| `not-negotiated` | Algum elemento não consegue atender às caps. Confira formatos e presença dos conversores. |
| Biblioteca ou DLL não encontrada | SDK/runtime ausente, PATH incorreto ou mistura de arquiteturas. No Windows use MSVC x64 nos dois pacotes. |
| Não abre janela em servidor sem desktop | Não há sessão gráfica disponível. Use `--sem-janela` e exportação, ou execute nativamente em desktop. |
| Fechar a janela produz erro | Alguns sinks reportam encerramento da superfície como erro. Use EOS natural ou `Ctrl+C` no terminal. |
| `Saida ja existe` | Foi escolhida uma pasta que já contém evidências. Escolha outro nome para preservar a execução anterior. |
| PPM/PGM não abre no visualizador padrão | Execute `python scripts/gerar_comparacao.py PASTA` e abra o HTML resultante. |
| CMake encontra compilador diferente no Windows | Use Developer PowerShell x64 e uma pasta de build nova ao trocar toolchain. |

## Logs de depuração

Em PowerShell:

```powershell
. ./scripts/ambiente.ps1
$env:GST_DEBUG = '2'
./build/pipeline_multimidia.exe --sem-janela --segundos 2
Remove-Item Env:GST_DEBUG
```

Em Linux:

```bash
GST_DEBUG=2 ./build/pipeline_multimidia --sem-janela --segundos 2
```

Aumente o nível apenas quando necessário; logs muito detalhados podem dificultar a apresentação e alterar o custo de processamento. `gst-inspect` ajuda a pesquisar elementos, e `gst-launch` ajuda em experimentos isolados; a entrega principal continua sendo a aplicação C++.

## Limites técnicos conhecidos

Não há áudio, rede, câmera, codificação de vídeo nem controle interativo de parâmetros. A exportação captura somente o primeiro quadro de cada ramo. Os arquivos exportados não são sincronizados por um pareamento adicional de PTS; neste fluxo determinístico, os primeiros buffers têm a mesma origem de conteúdo, e o teste valida propriedades da mídia, não igualdade pixel a pixel entre diferentes resoluções.

As transformações descartam informação. Converter cinza para RGB posteriormente não recupera cores, ampliar não recupera detalhes e repetir quadros não recupera movimento. O exemplo prioriza compreender essas perdas e a negociação de formatos.

O prazo máximo de execução é a duração pedida mais 20 segundos. Pastas de saída devem ser usadas por uma execução por vez. O programa não implementa coordenação entre processos nem gravação transacional de vários arquivos; se ocorrer falha de disco, arquivos parciais podem existir, mas a aplicação sinaliza erro. A recusa de sobrescrita protege execuções sequenciais comuns.
