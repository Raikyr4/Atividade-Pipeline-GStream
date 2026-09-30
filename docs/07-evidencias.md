# Evidências de execução Windows nativa

Validação realizada nesta máquina com GStreamer **1.28.6**, GCC **16.2.0 x64** portátil e bibliotecas MinGW x64 do SDK local. Nenhum container foi usado nessa validação.

## Compilação e testes

A compilação C++17 passou com `-Wall -Wextra -Wpedantic`, sem avisos. O teste de integração local passou: caps, FPS por PTS, contagens, pixels, exportação, proteção de arquivos existentes e argumentos inválidos.

[Log do teste Windows](../evidencias/windows-testes.txt).

## Reprodução

O executável foi iniciado em modo gráfico nativo por três segundos, usando `autovideosink` em ambos os ramos. Chegou a EOS e terminou com código 0. A validação confirma execução do caminho gráfico; a percepção da diferença de movimento deve ser conferida no ensaio da apresentação.

| Medição real | Original | Processado |
|---|---:|---:|
| Resolução | 640 × 360 | 320 × 180 |
| Formato | RGB | GRAY8 |
| FPS das caps | 30 | 10 |
| FPS por timestamps | 30 | 10 |
| Quadros observados | 90 | 31 |

O ramo processado produziu um quadro adicional na fronteira de encerramento. A cadência permanece 10 FPS: 31 amostras delimitam 30 intervalos. O teste aceita um quadro de diferença na contagem finita e verifica também os timestamps.

- [Log gráfico Windows](../evidencias/windows-execucao.txt)
- [Relatório real](../evidencias/windows-nativo/relatorio.json)
- [Comparação HTML gerada localmente](../evidencias/windows-nativo/comparacao.html)
- [Original PPM](../evidencias/windows-nativo/original.ppm)
- [Processado PGM](../evidencias/windows-nativo/processado.pgm)

As imagens são o primeiro quadro de cada ramo, exportadas pelo C++. Elas demonstram resolução e cor; FPS é demonstrado por reprodução e medição temporal. Arquivos anteriores na pasta de evidências são históricos; os links acima identificam a execução Windows atual.
