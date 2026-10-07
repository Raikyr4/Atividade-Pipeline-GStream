# Evidências atuais — vídeo existente importado

Execução nativa no Windows com GStreamer 1.28.6 e GCC 16.2.0 x64. A entrada foi `midia/exemplo.webm`, trailer Sintel, lido do disco. O programa não gerou uma fonte sintética.

## Reprodução real

Executado o trecho de três segundos iniciado em 5 s, com duas saídas `autovideosink`. A aplicação chegou a EOS e retornou código 0.

| Medição | Referência | Processado |
|---|---:|---:|
| Resolução | 854 × 480 | 320 × 180 |
| Formato medido | RGB | GRAY8 |
| FPS nominal das caps | 0/1, não declarado | 10/1 |
| Cadência média por PTS | 24,0027 FPS | 10 FPS |
| Buffers observados | 72 | 30 |

- [Log da reprodução](../evidencias/video-importado-execucao.txt)
- [Relatório JSON](../evidencias/video-importado/relatorio.json)
- [Comparação HTML](../evidencias/video-importado/comparacao.html)
- [Original exportado](../evidencias/video-importado/original.ppm)
- [Processado exportado](../evidencias/video-importado/processado.pgm)

## Integração

[Log dos testes atuais](../evidencias/video-importado-testes.txt). Foram verificadas importação, resolução, formato, PTS, contagem, pixels, proteção da exportação, argumentos, arquivo inválido e caminho com espaços e acentos.

O teste de dois segundos observou aproximadamente 48 quadros na entrada e 20 na saída. Seu limite admite um quadro de diferença na fronteira temporal. O teste não presume que FPS 0/1 signifique ausência de quadros.

A execução gráfica comprova o caminho nativo neste ambiente; o grupo ainda deve ensaiar a comparação de movimento no monitor usado na apresentação. As imagens exportadas demonstram pixels e dimensões, não movimento.

## Histórico

Outras subpastas e logs antigos em `evidencias/` pertencem à versão anterior com fonte sintética. Para apresentar a versão atual, use apenas os arquivos `video-importado` listados acima.

Mídia de exemplo: Sintel, Blender Foundation, CC BY 3.0. [Créditos e fonte](../midia/README.md).
