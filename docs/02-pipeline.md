# Arquitetura e justificativa da pipeline

## 1. Problema que a solução resolve

A proposta é mostrar como um fluxo de vídeo pode ser adaptado para uma representação de menor volume de dados. A comparação deve conservar a origem do conteúdo; por isso, primeiro geramos um vídeo e depois o dividimos. Apenas um ramo recebe as transformações.

```text
videotestsrc: barras coloridas com deslocamento horizontal
  |
caps: RGB / 640x360 / 30 FPS / pixels quadrados
  |
tee (divisor)
  +-- queue --> capsfilter antes --> MEDICAO ORIGINAL
  |                                   |
  |                                   +--> videoconvert --> autovideosink
  |
  +-- queue --> videoscale --> videorate --> videoconvert
                                                |
                     capsfilter depois: GRAY8 / 320x180 / 10 FPS
                                                |
                                       MEDICAO PROCESSADO
                                                |
                                  videoconvert --> autovideosink
```

No modo sem janela, a saída de cada ponto de medição vai diretamente para `fakesink sync=false`. Os probes continuam medindo os buffers e podem salvar o primeiro quadro. O desenho representa uma única pipeline com dois ramos, não duas execuções independentes.

## 2. Responsabilidade de cada elemento

| Elemento | Responsabilidade | Por que foi escolhido |
|---|---|---|
| `videotestsrc` | Gera vídeo sintético | Funciona offline e torna o experimento reproduzível |
| Caps de entrada | Fixam RGB, 640×360 e 30/1 | Criam uma referência conhecida para a comparação |
| `tee` | Distribui o fluxo para dois ramos | Mantém uma origem comum de conteúdo e tempo |
| `queue` em cada ramo | Armazena buffers e separa o processamento em threads | Evita que o processamento imediato de um ramo dependa da mesma thread do outro |
| `videoscale` | Reamostra espacialmente o vídeo | Produz 320×180 a partir de 640×360 |
| `videorate` | Ajusta a cadência descartando ou repetindo quadros | Produz 10 FPS a partir de 30 FPS; não cria movimento interpolado |
| `videoconvert` do processamento | Converte a representação de pixels | Torna possível negociar GRAY8 a partir de RGB |
| `capsfilter` depois | Restringe o contrato da saída processada | Exige simultaneamente tamanho, FPS e formato desejados |
| `videoconvert` da exibição | Adapta o formato à capacidade do sink escolhido | Evita exigir que a placa/driver exiba diretamente RGB ou GRAY8 |
| `autovideosink` | Escolhe um sink de vídeo disponível | Facilita executar o mesmo código em sistemas diferentes |
| `fakesink` | Consome buffers sem desenhar | Permite testes automatizados sem tela |

As filas não tornam os ramos ilimitadamente independentes: se um sink ficar bloqueado e uma fila encher, a pressão pode chegar ao `tee`. Não usamos modo leaky, porque descartar quadros adicionalmente nas filas dificultaria a interpretação das contagens. A função de `tee` e a necessidade de filas por ramo seguem a [documentação oficial](https://gstreamer.freedesktop.org/documentation/coreelements/tee.html).

## 3. Caps não fazem a conversão

`video/x-raw,format=GRAY8,width=320,height=180,framerate=10/1` descreve uma condição a ser atendida. O `capsfilter` não reduz uma imagem nem remove quadros sozinho. Os transformadores anteriores precisam ser capazes de produzir essas caps. A negociação propaga restrições pelo fluxo e encontra formatos compatíveis entre os pads.

O formato é medido no pad `src` de cada filtro nomeado, antes da adaptação ao dispositivo. Na tela, o backend pode receber outro formato após o último `videoconvert`; isso não invalida a medição de GRAY8 no ponto de processamento. A imagem continua visualmente sem cores.

## 4. Construção em C++

`gst_parse_launch` recebe uma descrição textual montada pelo programa e cria os elementos e links. Isso é integração direta com a biblioteca GStreamer, como no Tutorial 1. Não se trata de executar um comando de terminal. A escolha reduz código mecânico de criação e ligação, deixando explícita a arquitetura.

O programa só insere na descrição a quantidade validada de buffers e uma saída escolhida internamente. Caminhos de arquivos não são interpolados na linguagem da pipeline; são tratados pelo C++ durante a exportação.

Os ramos usam pads solicitados de `tee`, representados por `divisor.`. O parser faz as ligações. Não há `decodebin` nem descoberta tardia de streams. Assim, não é necessário callback `pad-added`: o Tutorial 3 foi estudado para entender quando ligações dinâmicas seriam necessárias, mas a fonte sintética permite uma topologia conhecida desde a montagem.

## 5. Ciclo de execução e tratamento de falhas

1. Inicializar GStreamer e validar os argumentos.
2. Preparar a pasta de exportação sem substituir arquivos existentes.
3. Criar a pipeline e rejeitar erros de montagem, inclusive resultado parcial com `GError`.
4. Instalar probes nos dois pontos de medição.
5. Obter o bus e solicitar estado `PLAYING`.
6. Aguardar `ERROR` ou `EOS`, consultando também pedido de interrupção e limite de tempo.
7. Voltar a `NULL`, parando as threads antes de consultar os dados coletados.
8. Mostrar o resumo, gravar o JSON e liberar referências.

O número de buffers da fonte é `segundos × 30`. Ao terminar a geração, a fonte envia EOS. `Ctrl+C` pede EOS pela thread principal; o manipulador de sinal apenas altera uma flag. O bus transporta notificações, não os pixels do vídeo. Erros de plugin, negociação, exibição ou arquivo provocam código de saída diferente de zero.

## 6. Organização do código

`SalvarImagem` transforma o primeiro buffer de cada ramo em PPM/PGM, respeitando o stride. `Observar` coleta caps, quantidade de buffers e timestamps. `InstalarMedicao` posiciona esses probes. `EscreverMedicao` produz os campos do relatório. `main` coordena montagem e execução.

A exportação é uma amostra estática do primeiro quadro de cada ramo, não uma gravação de vídeo. Os formatos simples foram escolhidos porque preservam exatamente os pixels medidos e dispensam codecs adicionais. O script auxiliar apenas converte essas imagens para PNG embutido em HTML, facilitando abrir no navegador.
