# Roteiro de apresentação — aproximadamente 7 minutos

Este roteiro foi escrito para ser ensaiado, não decorado. Cada integrante deve saber explicar a origem dos dados, as transformações e a evidência de funcionamento. Antes da apresentação, preencha nomes do grupo, disciplina e professor no material exigido pela instituição.

## Antes de entrar na sala

1. Compile e execute no computador que será usado. Confirme que abre as duas janelas.
2. Rode o CTest e guarde a saída. Teste sem internet para confirmar autonomia.
3. Deixe o terminal aberto na pasta do projeto e o código em `src/principal.cpp`.
4. Gere uma pasta de evidências e abra `comparacao.html` como alternativa de demonstração.
5. Aumente a fonte do terminal e organize as duas janelas lado a lado.
6. Evite recompilar ou instalar dependências durante a fala.

## 0:00–0:45 — Objetivo

**Mostrar:** tabela antes/depois do README.

**Fala sugerida:** “Nossa aplicação demonstra três transformações em um vídeo: resolução, taxa de quadros e representação de cor. Implementamos em C++ usando a biblioteca GStreamer. O vídeo original tem 640 por 360 pixels, 30 quadros por segundo e formato RGB. A saída processada tem 320 por 180, 10 quadros por segundo e formato GRAY8.”

**Ideia que precisa ficar clara:** há uma transformação verificável da mídia, e não apenas uma imagem estilizada no navegador.

## 0:45–1:30 — Fonte e reprodução

Execute em Linux:

```bash
./build/pipeline_multimidia --segundos 60
```

Ou no Windows nativo:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Segundos 60
```

**Fala sugerida:** “Usamos uma fonte sintética, porque o objetivo é estudar o processamento. Isso elimina dependência de câmera, arquivo externo ou internet. A fonte tem barras coloridas e movimento horizontal: as cores ajudam a observar a transformação para cinza, e o movimento ajuda a perceber a redução de FPS.”

**Apontar:** original colorido e processado cinza. A versão processada tem menor resolução e atualização menos frequente. Se o tamanho das janelas for ajustado automaticamente pelo sistema, use o relatório para provar a resolução.

## 1:30–2:45 — Caminho da mídia

**Mostrar:** diagrama de [arquitetura](02-pipeline.md).

**Fala sugerida:** “Depois de gerar o vídeo, fixamos o formato de referência e usamos tee para dividir o fluxo. Cada ramo começa com queue. Um preserva a mídia original. No outro, videoscale altera a resolução, videorate ajusta a taxa de quadros e videoconvert muda a representação dos pixels. O capsfilter exige o formato final. Na saída visual, outro conversor permite compatibilidade com o dispositivo de vídeo.”

**Pergunta interna para ensaio:** se removermos os conversores e deixarmos só o capsfilter, ele transforma a mídia? Resposta: não, ele restringe a negociação; alguém precisa realizar a transformação.

## 2:45–3:45 — Conceitos e números

**Fala sugerida:** “Reduzir largura e altura pela metade reduz os pixels para um quarto. Passar de 30 para 10 FPS reduz as amostras temporais para um terço, sem mudar a duração. Trocar RGB por GRAY8 reduz de três bytes para um byte por pixel nesta configuração, perdendo a informação de cor. Combinando os três fatores, a carga bruta de pixels por segundo fica 36 vezes menor.”

**Complemento necessário:** “Esse valor é uma estimativa de dados brutos. Não significa que a aplicação inteira use 36 vezes menos memória ou CPU, porque ela ainda executa os dois ramos e mantém filas.”

## 3:45–4:45 — C++ e controle da execução

**Mostrar:** `gst_parse_launch`, `InstalarMedicao`, `gst_element_set_state`, laço de leitura do bus.

**Fala sugerida:** “A aplicação inicializa o GStreamer, constrói a pipeline pela API, instala pontos de medição e muda o estado para PLAYING. O bus avisa quando acontece um erro ou quando o fluxo termina. Ao receber EOS, voltamos para NULL e liberamos os recursos. O programa também limita a duração e permite interrupção pelo terminal.”

**Distinção importante:** a aplicação é C++ chamando GStreamer diretamente; a descrição textual é uma forma de construir a pipeline, não um script que substitui o programa exigido.

## 4:45–5:45 — Evidências

**Mostrar:** terminal com os dois resumos, `relatorio.json` e `comparacao.html`.

**Fala sugerida:** “Além da observação visual, contamos buffers reais e lemos as caps negociadas. Calculamos a cadência pelo intervalo entre timestamps. Em dois segundos, esperamos 60 quadros originais e aproximadamente 20 processados. Também salvamos o primeiro quadro de cada ramo. Isso permite conferir pixels e resolução independentemente da janela.”

As imagens exportadas não demonstram movimento sozinhas. Explique que a mudança de FPS é comprovada pelos timestamps e percebida na reprodução. O teste admite diferença de um quadro no término por causa do fechamento do fluxo, mas verifica FPS nominal e medido.

## 5:45–7:00 — Limites e conclusão do grupo

**Fala sugerida:** “Escolhemos um escopo pequeno e verificável. Não há áudio, captura de câmera ou transmissão pela rede. A pipeline é conhecida desde o início, então não precisamos tratar pads descobertos durante a decodificação. Como evolução, poderíamos usar uma câmera ou arquivo e acrescentar gravação de vídeo. O resultado atual atende à atividade com fonte, transformações, saída e comparação antes/depois.”

## Perguntas prováveis e respostas

| Pergunta | Resposta que o grupo deve compreender |
|---|---|
| Por que GStreamer? | Organiza processamento multimídia em elementos conectáveis, com negociação de formatos, tempo e mensagens de controle. |
| Por que uma única fonte? | Mantém conteúdo e referência temporal comuns; duas fontes poderiam produzir imagens ou tempos diferentes. |
| Por que `queue` depois do `tee`? | Cada ramo ganha uma thread de processamento e uma fila; isso reduz acoplamento imediato, mas uma fila cheia ainda pode bloquear a fonte. |
| O capsfilter muda o vídeo? | Não. Ele impõe o formato; os transformadores tornam esse formato possível. |
| A imagem menor está comprimida? | Não há codec. Reduzimos amostragem e representação em vídeo bruto. |
| 10 FPS significa câmera lenta? | Não. A duração é preservada; há menos quadros por segundo de mídia. |
| Aumentar FPS melhora os detalhes do movimento? | `videorate` pode repetir quadros; não recupera movimento que não foi capturado. |
| GRAY8 é RGB com três valores iguais? | No ponto medido, é um único componente de 8 bits. O dispositivo pode converter depois para exibição. |
| Como sabem a resolução real? | Lemos caps do pad após o processamento e verificamos as dimensões do arquivo exportado. |
| Por que não usaram `playbin`? | Ele esconde parte da montagem; queremos explicitar e controlar as transformações. |
| Onde está o Tutorial 3? | Na análise de pads e estados. Não usamos ligação dinâmica porque não há demuxer/decoder descobrindo streams. |
| O que é EOS? | Evento de fim do fluxo; a aplicação recebe uma mensagem no bus quando a pipeline termina. |
| Por que o teste termina tão rápido? | Sem sincronização do sink, o processamento não espera o relógio; os timestamps continuam válidos. |
| A saída sem janela vale como demonstração? | Ela gera quadros e relatório reais; para demonstrar movimento, use também as janelas nativas. |

## Sugestão de divisão do grupo

Uma pessoa apresenta objetivo e demonstração; outra explica topologia e elementos; outra explica os conceitos e cálculos; outra apresenta código, testes e limitações. Em grupos menores, combine os blocos. Todos devem conseguir responder o que muda, onde muda e como foi comprovado.
