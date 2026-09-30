# Pipeline Multimídia — processamento de vídeo existente

Aplicação C++ + GStreamer executada nativamente no Windows. Abre um arquivo de vídeo, divide os quadros decodificados em dois ramos e compara a referência com uma versão reduzida, em cinza e a 10 FPS. **Nenhum vídeo é gerado pela aplicação.**

## Executar agora

Dê dois cliques em **Iniciar.cmd**. Ele usa o trailer de exemplo já incluído em `midia/exemplo.webm`, começando no segundo 5 para pular a abertura preta.

Para usar seu próprio vídeo, **arraste o arquivo sobre Iniciar.cmd**. Também pode executar no terminal do VS Code:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Video "C:/Videos/meu-video.mp4"
```

Sem `-Video`, o comando usa o exemplo incluído. Para escolher um trecho:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Video "C:/Videos/meu-video.mp4" -Inicio 10 -Segundos 30
```

`-Inicio` é a posição inicial em segundos; `-Segundos` é a duração máxima do trecho. Arquivos menores terminam antes. Um arquivo próprio começa no segundo zero quando `-Inicio` não é informado. A reprodução apresenta **somente vídeo, sem áudio**, em duas janelas.

## Comparação

| Característica | Referência | Processado |
|---|---|---|
| Fonte | Arquivo local decodificado | Os mesmos quadros |
| Resolução | Preservada do vídeo | 320 × 180 |
| Cadência | Preservada do vídeo | 10 FPS |
| Representação medida | RGB, após conversão para comparação | GRAY8, cinza |

No exemplo incluído, a referência é 854 × 480 e aproximadamente 24 FPS. O WebM declara FPS nominal 0/1 nas caps; isso significa taxa não declarada, e não vídeo parado. O programa mede a cadência pelos timestamps.

Para demonstrar pelo menos duas mudanças, escolha um vídeo colorido, maior que 320 × 180 e com cadência superior a 10 FPS. Um arquivo já idêntico à saída desejada não evidenciará as mesmas diferenças. MP4, WebM, MKV e outros contêineres dependem dos decodificadores instalados no GStreamer.

## Testar e exportar

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/testar.ps1
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -SemJanela -Segundos 2 -Exportar ./saida-importada
python ./scripts/gerar_comparacao.py ./saida-importada
```

Abra `saida-importada/comparacao.html`. Use outra pasta ao repetir: a aplicação protege arquivos existentes. A exportação contém um quadro por ramo e um relatório, não um novo arquivo de vídeo. O arquivo de entrada nunca é modificado.

## Ambiente e código

Os scripts usam GStreamer MinGW x64 local e GCC x64 portátil em `.ferramentas/w64devkit`. Não usam containers nem alteram o GCC global de 32 bits. `src/principal.cpp` tem comentários detalhados e etapas separadas por linhas em branco. A função `ConectarVideo` explica a ligação dinâmica dos pads do decodificador.

## Material para apresentação

1. [Instalação e comandos](docs/01-instalacao.md)
2. [Arquitetura da pipeline](docs/02-pipeline.md)
3. [Conceitos e cálculos](docs/03-conceitos.md)
4. [Roteiro de apresentação](docs/04-apresentacao.md)
5. [Validação e diagnóstico](docs/05-validacao.md)
6. [Fontes pesquisadas](docs/06-referencias.md)
7. [Evidências atuais](docs/07-evidencias.md)
8. [Guia de defesa: perguntas do professor e respostas comentadas](docs/08-defesa-perguntas.md)

Vídeo de exemplo: **Sintel**, Blender Foundation, disponibilizado sob CC BY 3.0. Consulte [origem e atribuição](midia/README.md). O material visual de Sintel não foi criado pelo grupo; a contribuição do projeto é a aplicação de processamento.
