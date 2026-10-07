# Guia de defesa — perguntas do professor e respostas comentadas

Este material corresponde ao código atual de `src/principal.cpp`: ele importa um arquivo, decodifica e compara dois ramos. Use as respostas como base para falar com suas palavras. Se não lembrar um detalhe, mostre a função responsável e explique o que consegue confirmar nela.

## 1. A resposta central: que tipo de pipeline foi feito?

**Resposta sugerida:**

> Fizemos uma pipeline de processamento e reprodução de vídeo a partir de arquivo, com ramificação em dois caminhos. Um mostra a referência e o outro transforma resolução, FPS e cor. A ligação do decodificador é dinâmica, porque os pads aparecem quando o arquivo é identificado; o restante do processamento tem uma estrutura definida previamente.

Se o professor pedir uma classificação mais específica:

| Critério | Classificação nesta implementação |
|---|---|
| Origem | Arquivo local, não uma fonte ao vivo |
| Função | Decodificação, transformação, comparação e reprodução |
| Topologia | Ramificada, usando `tee` |
| Construção | Parte estática descrita em texto e ligação dinâmica do vídeo |
| Saída | Duas janelas; opcionalmente imagens e relatório |
| Codificação de saída | Não há encoder nem muxer para criar outro vídeo |

Não responda apenas “dinâmica”, pois isso deixa de explicar o objetivo e a ramificação. Não diga “duas pipelines”: o código cria **uma pipeline com dois ramos**. Não diga “pipeline de streaming pela internet”: a entrada do programa é um arquivo local.

## 2. Explicação pronta em aproximadamente 40 segundos

> Nosso programa abre um vídeo existente usando `uridecodebin`, que identifica o conteúdo e seleciona os decodificadores disponíveis. Quando aparece a faixa de vídeo, conectamos seu pad à entrada do processamento. Convertemos os quadros para RGB e usamos `tee` para separar dois ramos. O primeiro preserva resolução e cadência. O segundo usa `videoscale`, `videorate` e `videoconvert` para produzir 320 por 180 pixels, 10 FPS e escala de cinza. Exibimos os dois resultados e medimos os formatos e timestamps para comprovar as alterações.

## 3. Ordem para mostrar o código sem se perder

Abra `src/principal.cpp` e use a busca do editor pelos nomes abaixo. Não é necessário ler todas as linhas.

| Ordem | Buscar | O que explicar |
|---|---|---|
| 1 | `int main` | O programa inicializa o framework e recebe arquivo, início e duração |
| 2 | `const std::string descricao` | A topologia: leitor, entrada, divisor e dois ramos |
| 3 | `gst_parse_launch` | A descrição é transformada em elementos reais pela biblioteca |
| 4 | `g_signal_connect` | Registro do callback para quando a faixa for descoberta |
| 5 | `void ConectarVideo` | Seleção do pad de vídeo e ligação à entrada |
| 6 | `GST_STATE_PAUSED` | Preparação do arquivo antes do posicionamento |
| 7 | `gst_element_seek` | Seleção do trecho por tempo de mídia |
| 8 | `GST_STATE_PLAYING` | Início da reprodução |
| 9 | `GstPadProbeReturn Observar` | Leitura de caps, contagem de quadros e PTS |
| 10 | `gst_bus_timed_pop_filtered` | Recebimento de erro ou fim do fluxo |
| 11 | `GST_STATE_NULL` | Parada antes da leitura dos resultados e limpeza |

Os nomes são referências mais duráveis que números de linha, que mudam quando o código recebe comentários.

## 4. Perguntas sobre a arquitetura

### “O que é uma pipeline?”

> É um conjunto de elementos conectados por onde a mídia passa. Neste projeto, os dados são lidos de um arquivo, decodificados, transformados e enviados às saídas. A pipeline também coordena aspectos como estados e tempo de reprodução.

Uma analogia útil é uma sequência de estações de trabalho: cada estação tem uma responsabilidade. Aqui existe uma bifurcação para comparar duas versões do mesmo conteúdo.

### “Qual é a fonte de mídia?”

> A fonte de conteúdo é o arquivo local. O elemento de alto nível usado para abri-lo é `uridecodebin`, que cria internamente a leitura, a identificação do contêiner e a decodificação necessárias.

Não diga `videotestsrc`: esse elemento pertencia à versão anterior e não aparece na pipeline atual.

### “Por que usou uridecodebin?”

> Para abrir arquivos com diferentes contêineres e codecs sem escrever manualmente uma cadeia diferente para cada um. Ele seleciona os elementos conforme o conteúdo e os plugins instalados.

Isso não significa que qualquer vídeo será suportado. Um codec ausente pode impedir a abertura.

### “MP4 é um codec?”

> MP4 é um contêiner. Ele organiza faixas e informações temporais; o vídeo dentro dele pode usar diferentes codecs. O decodificador depende do conteúdo, não apenas da extensão.

O exemplo usado aqui é WebM. Não invente o nome de seu codec se você ainda não o inspecionou: não é necessário adivinhar para explicar a aplicação.

### “O que é um pad?”

> É uma porta de entrada ou saída de um elemento. Um pad `src` produz dados; um pad `sink` recebe. Os elementos são conectados ligando portas compatíveis.

### “Por que a ligação do leitor é dinâmica?”

> Quando criamos o leitor, as faixas ainda não foram identificadas. O sinal `pad-added` avisa quando um pad aparece. A função `ConectarVideo` consulta suas caps, verifica se é vídeo bruto e liga esse pad à entrada.

Mostre `g_signal_connect`, depois `ConectarVideo`. O callback não recebe o arquivo inteiro como argumento: ele recebe o pad recém-criado e o ponteiro para a entrada que deverá ser ligada.

### “E se houver áudio ou várias faixas de vídeo?”

> O callback ignora áudio e legendas. Para vídeo, verifica se a entrada já está conectada; assim seleciona a primeira faixa de vídeo que conseguir ligar. Esta versão não oferece escolha de faixa nem reprodução de áudio.

### “Por que usar tee?”

> Para enviar o mesmo fluxo decodificado aos dois ramos. Dessa maneira, a comparação usa a mesma origem de conteúdo e tempo, e o arquivo não precisa ser decodificado duas vezes pela aplicação.

Não diga que cada saída sempre recebe uma cópia completa de todos os pixels em memória. O GStreamer pode compartilhar referências a buffers até que um processamento precise produzir novos dados.

### “Para que servem as queues?”

> Elas armazenam buffers e separam a execução dos ramos em threads de streaming. Isso permite que cada ramo processe seus dados sem depender imediatamente da mesma thread do outro.

Se o professor insistir sobre bloqueio:

> A independência não é ilimitada. Quando uma fila enche, ela ainda pode bloquear e propagar pressão para a origem. Não configuramos descarte automático nas filas, para evitar perdas adicionais que confundiriam as medições.

### “O que significa o ponto em divisor. e o sinal de exclamação?”

> `!` descreve uma conexão entre elementos. `divisor.` referencia o `tee` nomeado e permite ao parser solicitar uma saída dele para cada ramo.

### “Você executa um comando gst-launch por baixo?”

> Não. O C++ chama `gst_parse_launch`, uma função da biblioteca que constrói os elementos a partir da descrição. Depois o próprio C++ controla estados, callbacks, mensagens e medições.

## 5. Perguntas sobre as modificações

### “Quais características foram alteradas?”

> Resolução, taxa de quadros e representação de cor. No exemplo medido, o original tem 854 por 480 pixels e aproximadamente 24 FPS. O processado tem 320 por 180, 10 FPS e formato GRAY8.

Esses valores de entrada pertencem ao arquivo incluído, não a todos os vídeos possíveis. Em outro arquivo, o relatório pode mostrar dimensões e cadência diferentes.

### “Qual elemento modifica cada característica?”

| Característica | Elemento | Explicação |
|---|---|---|
| Dimensões | `videoscale` | Calcula uma nova grade de pixels |
| Cadência | `videorate` | Descarta ou repete quadros para ajustar a taxa |
| Representação dos pixels | `videoconvert` | Permite converter RGB para GRAY8 |
| Exigência de formato final | `capsfilter` | Restringe o formato negociado |

### “O capsfilter faz a conversão?”

> Não. Ele estabelece o formato exigido. Os elementos anteriores precisam conseguir produzi-lo. Só exigir 320 por 180 não redimensiona uma imagem se não houver um elemento capaz de fazer isso.

### “Por que converter para RGB antes de dividir?”

> O decoder pode produzir outro formato, como uma representação YUV. Padronizar em RGB facilita exportar os pixels e comparar os ramos. No ramo de referência, preservamos tamanho e cadência, mas não necessariamente o formato que o codec entregou originalmente.

### “Por que existe outro videoconvert perto da janela?”

> O dispositivo gráfico pode não aceitar diretamente o formato medido. O conversor final adapta os pixels ao sink escolhido. Os probes ficam antes dele, então a medição da saída processada continua sendo GRAY8.

### “Como diminuir o FPS? Isso deixa o vídeo lento?”

> O `videorate` ajusta os quadros de acordo com os timestamps. Para diminuir a taxa, descarta amostras temporais. A duração do trecho é mantida; o movimento fica menos suave, não em câmera lenta.

### “E se eu pedir 60 FPS?”

> O elemento pode repetir quadros para preencher os novos instantes. Ele não cria por interpolação as posições do movimento que não estavam na entrada.

### “Cinza é só tirar dois canais de RGB?”

> Não. Há uma conversão da informação de cor para uma intensidade. RGB usa três componentes de 8 bits nesta aplicação; GRAY8 usa um componente de 8 bits. Não basta escolher um canal e descartar os outros para afirmar que fizemos a mesma conversão.

Não apresente uma fórmula específica de luminância como se estivesse implementada no código. A aplicação delega essa conversão ao GStreamer.

### “Reduzir resolução é comprimir o vídeo?”

> Aqui reduzimos a quantidade de amostras espaciais em quadros brutos. Não usamos um encoder para criar um vídeo comprimido de saída. Reamostragem e codificação de vídeo são operações diferentes.

### “A saída sempre tem menos informação?”

> No exemplo escolhido, sim: há menos pixels, menos quadros por segundo e perda de cor. Mas isso depende da entrada. Um vídeo já cinza, 320 por 180 e 10 FPS não demonstraria as mesmas alterações.

## 6. Perguntas sobre medição e execução

### “Como prova que mudou realmente?”

> Além das janelas, instalamos probes nos pads após os filtros. Eles leem as caps negociadas, contam buffers e registram os timestamps. O relatório mostra os resultados observados, e os testes também conferem os arquivos de pixels exportados.

Mostre `InstalarMedicao`, `Observar` e `evidencias/video-importado/relatorio.json`.

### “O que é um probe? Ele modifica o vídeo?”

> É um callback que observa o que passa por um pad. Nosso probe mede os buffers e pode salvar uma imagem. Retorna `GST_PAD_PROBE_OK`, deixando o fluxo continuar, sem aplicar as transformações que pertencem aos elementos de vídeo.

### “O que é PTS?”

> É o timestamp de apresentação do buffer, isto é, sua posição temporal de apresentação na mídia. Usamos a diferença entre o primeiro e o último PTS para medir a cadência média do trecho.

### “Por que a fórmula usa N − 1?”

> Porque N quadros formam N − 1 intervalos. Três quadros têm somente dois espaços de tempo entre eles. Dividimos a quantidade de intervalos pela duração entre o primeiro e o último quadro.

Se os PTS estiverem em nanossegundos, multiplicamos a quantidade de intervalos por `GST_SECOND`, equivalente a um bilhão, para obter quadros por segundo.

### “Por que apareceu FPS 0/1 no original?”

> Nesse WebM, a taxa nominal não está declarada nas caps. Isso não significa que o vídeo tem zero quadros. Pelos timestamps medimos aproximadamente 24 FPS.

O resultado real de três segundos registrou 72 quadros na referência e 30 na saída. A média medida foi 24,0027 e 10 FPS. Não arredonde os dados do JSON para fingir exatidão; explique que o primeiro valor é aproximadamente 24.

### “Por que não calcular FPS pelo tempo que o programa levou?”

> Isso mediria velocidade de processamento, não cadência de mídia. Sem sincronização com o relógio, o computador pode processar três segundos de vídeo em menos de três segundos reais, mantendo os timestamps originais.

### “O que fazem PAUSED, PLAYING e NULL?”

> `PAUSED` prepara a mídia e permite o preroll. `PLAYING` permite a reprodução. `NULL` encerra os recursos ativos. Na nossa sequência, preparamos, escolhemos o trecho, reproduzimos e depois paramos antes de escrever os resultados finais.

### “Por que instalar os probes depois do preroll?”

> Para não contabilizar o quadro preparado inicialmente como parte do trecho medido. Depois de instalar os probes, fazemos um seek com flush para o trecho selecionado e medimos o novo fluxo.

### “Como escolheu o início e a duração?”

> Chamamos `gst_element_seek` com posição inicial e final em tempo de mídia. `FLUSH` descarta dados pendentes e `ACCURATE` solicita posicionamento preciso. Assim, o trecho também funciona quando o processamento não espera o relógio.

Um início além do fim do arquivo não produz um trecho válido. Nem todo tipo de mídia aceita esse seek; a aplicação informa falha quando não consegue solicitá-lo.

### “O que é o bus? Os quadros passam por ele?”

> O bus comunica mensagens de controle entre a pipeline e a aplicação, como erro e fim do fluxo. Os quadros viajam pelos pads e buffers, não pelo bus.

### “O que é EOS?”

> Significa fim do fluxo. Pode ocorrer quando o arquivo ou o trecho acaba. A aplicação aguarda essa mensagem e encerra os recursos. EOS não é a mesma coisa que erro.

### “Por que voltar para NULL antes de ler os contadores?”

> Os probes são executados por threads de streaming. Ao parar a pipeline antes da leitura final, evitamos que os contadores estejam sendo alterados enquanto o relatório é escrito.

### “Para que servem unref e unmap?”

> `unref` libera uma referência a um objeto; ele pode ser destruído quando não houver mais proprietários. `unmap` encerra o acesso mapeado à memória do quadro. Não são a mesma operação, e não devemos substituir essas chamadas por `delete`.

### “O que é stride?”

> É a distância, em bytes, entre o começo de uma linha da imagem e o começo da próxima na memória. Pode haver bytes de alinhamento. Por isso a exportação usa o stride para localizar a linha e grava somente os pixels ativos.

## 7. Perguntas sobre escolhas e limitações

### “Por que C++ se a API parece C?”

> O GStreamer oferece uma API C que pode ser chamada pelo C++. Nosso programa é compilado como C++17 e usa recursos como `std::string`, `std::filesystem`, streams e tratamento de exceções para organizar a aplicação.

### “Por que há Python e PowerShell se a atividade pede C++?”

> O processamento é realizado pelo executável C++. PowerShell prepara o ambiente e compila; Python executa testes e monta a comparação HTML. Nenhum deles substitui o processamento exigido em C++.

### “O vídeo original foi alterado? Você salva outro vídeo?”

> O arquivo original é apenas lido. A versão transformada é reproduzida em uma janela. Opcionalmente, salvamos um quadro por ramo e um JSON. Não há geração de um arquivo de vídeo codificado nesta versão.

### “Como poderia salvar um MP4?”

> Seria preciso acrescentar uma cadeia de codificação e gravação: converter para um formato aceito pelo encoder, codificar, organizar em um contêiner com muxer e enviar a um filesink. Também seria necessário esperar EOS para finalizar corretamente o arquivo. Essa cadeia ainda não está implementada.

Não prometa um encoder específico sem verificar os plugins disponíveis.

### “Como testou?”

> Executamos o binário com o vídeo incluído, em modo sem janela, e verificamos os formatos reais, a cadência por PTS, as contagens e os pixels dos arquivos exportados. Também testamos caminho com acentos, arquivo inválido, argumentos e proteção contra sobrescrita. A reprodução gráfica foi executada no Windows.

### “Quais são as principais limitações?”

> Trabalha com arquivo local que permite seek, seleciona a primeira faixa de vídeo e não reproduz áudio. O suporte a codecs depende da instalação. Não cria vídeo codificado de saída e não acompanha todas as possíveis renegociações de formato durante o arquivo.

### “Por que você escolheu essa solução?”

> Porque mostra de forma observável três conceitos diferentes: amostragem espacial, amostragem temporal e representação de cor. A ramificação permite comparar o mesmo conteúdo, e a medição complementa a observação visual.

## 8. Se o professor pedir uma alteração na hora

| Pedido | Onde olhar | O que explicar antes de editar |
|---|---|---|
| Mudar a resolução final | Caps do filtro `depois` | Alterar largura/altura; uma proporção diferente pode gerar bordas |
| Mudar FPS | `framerate=10/1` do filtro `depois` | O `videorate` continua fazendo o ajuste |
| Manter cor no resultado | `format=GRAY8` do filtro `depois` | Pode usar RGB, mas precisa ajustar extensão/formato da exportação e os testes; não é só uma troca isolada em todo o projeto |
| Abrir outro arquivo | `-Video` no script | Não é necessário alterar C++ |
| Pular a introdução | `-Inicio` | É uma posição temporal, não um número de quadros |
| Rodar por mais tempo | `-Segundos` | O arquivo pode terminar antes desse limite |
| Testar sem monitor | `-SemJanela` | Usa fakesink sem sincronização com o relógio |

Depois de alterar parâmetros de processamento no código, recompile e ajuste os valores esperados no teste, a documentação e as mensagens que descrevem os resultados. Evite apresentar somente uma string alterada enquanto o restante ainda anuncia os parâmetros antigos.

## 9. Expressões que vale evitar

- “O capsfilter converte o vídeo.” Ele restringe as caps; os transformadores convertem.
- “São duas pipelines.” É uma pipeline com dois ramos.
- “O original não sofreu nenhuma conversão.” Ele foi decodificado e convertido para RGB.
- “10 FPS deixa tudo em câmera lenta.” Reduz as amostras temporais sem mudar a duração.
- “O FPS 0/1 significa que não há vídeo.” Nesse arquivo, significa taxa nominal não declarada.
- “O programa salva um MP4.” Nesta versão, não salva vídeo codificado.
- “MP4 é o codec.” É um contêiner.
- “As filas impedem qualquer travamento.” Filas cheias ainda podem bloquear.
- “A imagem estática prova o FPS.” Ela prova pixels; o tempo exige reprodução ou timestamps.

## 10. Ensaio rápido sem olhar o código

Tente responder, em voz alta e em até 30 segundos cada:

1. Qual é o tipo de pipeline?
2. Por onde os dados passam, do arquivo à tela?
3. Por que `pad-added` é necessário?
4. Qual elemento muda resolução, FPS e cor?
5. Por que o filtro de caps não basta sozinho?
6. Como o relatório comprova as mudanças?
7. Por que aparece 0/1 nas caps do arquivo?
8. O que ocorre quando chega EOS?
9. O que o programa ainda não faz?

Se conseguir explicar essas nove respostas e localizar os trechos da seção 3, terá uma base sólida para conduzir a apresentação sem decorar o arquivo inteiro.
