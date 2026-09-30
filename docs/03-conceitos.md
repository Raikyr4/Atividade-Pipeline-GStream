# Conceitos da disciplina aplicados

## Resolução e amostragem espacial

Resolução indica quantos pixels compõem cada quadro. O original tem 640 × 360 = 230.400 pixels. A saída tem 320 × 180 = 57.600 pixels. Dividir largura e altura por dois reduz a quantidade total para um quarto. A proporção 16:9 é mantida; não há intenção de esticar a imagem.

Redimensionar exige reamostragem: calcular uma nova grade de pixels a partir da anterior. `videoscale` realiza esse processamento. Detalhes menores podem desaparecer. Se o usuário ampliar a janela do vídeo reduzido, a janela cresce, mas os detalhes descartados não reaparecem. É por isso que a resolução negociada deve ser lida no relatório, não deduzida pelo tamanho da janela.

## FPS e amostragem temporal

FPS é a quantidade de quadros por segundo de mídia. A 30 FPS, o intervalo nominal é aproximadamente 33,33 ms. A 10 FPS, é 100 ms. A segunda versão tem menos atualizações de movimento, mesmo que a duração total seja a mesma.

`videorate` usa os tempos dos buffers para adequar a cadência. Na redução, descarta quadros; em aumentos de taxa, pode repetir quadros. Não é um algoritmo que inventa posições intermediárias dos objetos. A [documentação do elemento](https://gstreamer.freedesktop.org/documentation/videorate/index.html) descreve essa política.

Reduzir FPS não significa colocar o vídeo em câmera lenta. Câmera lenta alteraria a relação entre tempo do conteúdo e reprodução. Aqui mantemos a duração e reduzimos as amostras temporais. O deslocamento horizontal da imagem facilita observar a diferença.

## Representação RGB e GRAY8

RGB combina componentes vermelho, verde e azul. Nesta configuração, cada componente ocupa 8 bits, totalizando 24 bits ou 3 bytes por pixel. GRAY8 usa uma amostra de intensidade de 8 bits por pixel, totalizando 1 byte e até 256 valores possíveis.

A conversão perde informação cromática: dois pixels originalmente de cores diferentes podem passar a intensidades próximas ou iguais. O cinza não é necessariamente a média aritmética de R, G e B. O conversor considera as regras de conversão e informações de cor negociadas. A experiência não fixa uma matriz colorimétrica nem pretende medir fidelidade de cor; ela demonstra mudança de representação e perda de cor visível.

Não confunda canais de cor com canais de áudio. Este trabalho usa vídeo; os três requisitos modificados são espaciais, temporais e de representação de pixels. Também não há codec de compressão: `video/x-raw` identifica quadros não comprimidos.

## Estimativa do volume bruto

Considerando apenas os pixels ativos, sem padding, metadados ou custos da pipeline:

| Cálculo | Original RGB | Processado GRAY8 |
|---|---:|---:|
| Pixels por quadro | 230.400 | 57.600 |
| Bytes por pixel | 3 | 1 |
| Bytes por quadro | 691.200 | 57.600 |
| Quadros por segundo | 30 | 10 |
| Bytes por segundo | 20.736.000 | 576.000 |
| MB/s decimais | 20,736 | 0,576 |

Razão: `20.736.000 / 576.000 = 36`. A representação de saída tem aproximadamente 97,22% menos bytes ativos por segundo. Isso resulta do produto de três fatores: quatro vezes menos pixels, três vezes menos componentes por pixel e três vezes menos quadros por segundo.

Esse cálculo **não** prova uma redução de 36 vezes no consumo total de RAM, na CPU ou no tráfego de rede. A aplicação mantém os dois ramos, possui filas, pode ter alinhamento em memória e não transmite pela rede. É uma comparação matemática de carga bruta de pixels, não um benchmark nem uma taxa de compressão de arquivo.

## Buffer, pad, caps e bus

- **Buffer:** carrega dados e informações temporais. Neste experimento, cada buffer medido representa um quadro.
- **Pad:** ponto de entrada ou saída de um elemento; links conectam pads compatíveis.
- **Caps:** descrevem mídia e formatos possíveis ou negociados, como largura, altura e FPS.
- **Bus:** entrega mensagens da pipeline à aplicação, como erro e fim do fluxo.
- **PTS:** timestamp de apresentação de um buffer; permite relacioná-lo à linha do tempo.

O relatório apresenta tanto FPS declarado nas caps quanto FPS calculado com timestamps: `(N − 1) × 1.000.000.000 / (PTS_último − PTS_primeiro)`. O fator converte nanossegundos em segundos. A conta usa N − 1 porque N quadros definem N − 1 intervalos. Ela mede cadência temporal dos buffers; não mede a taxa de atualização física do monitor nem desempenho em quadros por segundo de CPU.

## Estados e sincronização

`NULL` corresponde ao estado inicial sem recursos ativos. `READY` prepara recursos; `PAUSED` permite preparar dados para reprodução; `PLAYING` faz a pipeline avançar. A chamada para `PLAYING` pode realizar transições intermediárias de forma assíncrona.

No modo visual, os sinks usam sincronização com o relógio. No modo de teste, `sync=false` permite consumir os quadros mais rapidamente, preservando seus timestamps. EOS significa que o fluxo acabou; não é uma falha. Um erro de renderização ou negociação é comunicado separadamente.
