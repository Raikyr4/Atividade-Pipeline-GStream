# Arquitetura — importar, decodificar e transformar

## Objetivo da solução

Comparar o mesmo trecho de um vídeo existente antes e depois de três alterações: resolução, cadência e cor. A entrada é um arquivo local. Não usamos uma fonte sintética. Uma única decodificação alimenta os dois ramos, mantendo uma origem comum de conteúdo e timestamps.

```text
arquivo local (URI file://)
    |
uridecodebin — identifica contêiner e decodifica
    |
    | pad-added: ConectarVideo seleciona o primeiro video/x-raw
    v
queue entrada → videoconvert → RGB → tee divisor
    |
    +→ queue → capsfilter antes (RGB) → medição → conversor → janela original
    |
    +→ queue → videoscale → videorate → videoconvert
                 → capsfilter depois (320×180 / 10 FPS / GRAY8)
                 → medição → conversor → janela processada
```

No modo sem janela, cada saída visual é substituída por `fakesink sync=false`. O processamento e as medições continuam. A exportação é realizada pelo C++ ao observar o primeiro quadro do trecho em cada ramo.

## Decisões e responsabilidades

| Componente | Função e justificativa |
|---|---|
| `uridecodebin` | Seleciona fonte, demultiplexador e decodificadores conforme o arquivo e os plugins disponíveis |
| `ConectarVideo` | Recebe `pad-added`, verifica `video/x-raw` e liga a primeira faixa de vídeo à entrada |
| `videoconvert` inicial | Padroniza a referência em RGB, permitindo comparar e exportar seus pixels |
| `tee` | Distribui o mesmo fluxo decodificado aos dois ramos |
| `queue` | Oferece fila e thread de streaming; uma fila cheia ainda pode propagar bloqueio |
| `videoscale` | Reamostra a imagem para as dimensões da saída |
| `videorate` | Ajusta os intervalos entre quadros por descarte ou repetição |
| `videoconvert` processado | Permite converter os pixels para GRAY8 |
| `capsfilter` | Exige o contrato da saída; não realiza a transformação sozinho |
| `autovideosink` | Escolhe uma saída gráfica disponível no ambiente |

O original é uma **referência decodificada em RGB**, e não uma cópia dos bytes comprimidos do arquivo. Dimensões e cadência são preservadas nesse ramo. A conversão inicial pode mudar a representação do codec, por exemplo de YUV para RGB; isso deve ser dito na apresentação.

## Por que a ligação é dinâmica

Ao criar o leitor, ainda não sabemos quais faixas o arquivo contém. Seu pad de vídeo aparece quando os dados são identificados. O callback consulta as caps do novo pad e conecta apenas vídeo bruto. Áudio e legendas são ignorados. Se houver várias faixas de vídeo, mantém a primeira conectada.

O parser monta a parte estática e cria o leitor separado. Depois, a aplicação atribui a URI por `g_object_set` e registra `pad-added`. Assim o nome do arquivo não precisa ser interpolado na linguagem textual da pipeline. `gst_filename_to_uri` trata caracteres especiais; no Windows a linha de comando é recebida em UTF-8 por meio da GLib.

A escolha foi fundamentada na [documentação de uridecodebin](https://gstreamer.freedesktop.org/documentation/playback/uridecodebin.html) e no [Tutorial 3](https://gstreamer.freedesktop.org/documentation/tutorials/basic/dynamic-pipelines.html).

## Preparação, trecho e reprodução

A aplicação solicita `PAUSED` e espera o preroll: preparação do primeiro quadro e descoberta dos fluxos. Só depois instala os probes. Em seguida faz um seek com `FLUSH` e `ACCURATE`, indicando início e fim do trecho em tempo de mídia. Esse procedimento evita contar o quadro de preparação como parte da execução medida.

Depois solicita `PLAYING`. Um vídeo curto pode terminar naturalmente antes do limite pedido. Arquivos sem suporte ao seek temporal são rejeitados. O modo sem janela processa mais rápido que o relógio, mas respeita os limites temporais do seek.

## Observação e encerramento

Os probes nos filtros `antes` e `depois` coletam caps, quantidade de buffers e PTS inicial/final. A exportação mapeia o quadro com `GstVideoFrame`, respeitando o stride. O primeiro quadro pode ser preto se o arquivo ou trecho começar com um fade; isso não é necessariamente erro.

O bus informa `ERROR` e `EOS`. Ctrl+C solicita EOS na thread principal. Há tempo limite para preparação e término. A pipeline volta a `NULL` antes de a thread principal ler as medições, evitando concorrência com os probes. Depois são escritos o resumo e o JSON e liberadas as referências.

As caps são registradas no primeiro quadro medido. O projeto foi pensado para arquivos com dimensões e formato estáveis durante o trecho; não implementa acompanhamento de todas as renegociações possíveis. Os arquivos exportados são imagens estáticas, não gravação de vídeo.
