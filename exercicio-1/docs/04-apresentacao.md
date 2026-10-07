# Roteiro de apresentação — aproximadamente sete minutos

Para ensaiar as perguntas após a demonstração, consulte o
[guia de defesa com respostas comentadas](08-defesa-perguntas.md).

## Preparação

Execute `Iniciar.cmd` e confira as duas janelas. Deixe aberto `src/principal.cpp` na montagem da pipeline e no callback `ConectarVideo`. Rode `scripts/testar.ps1` antes da apresentação. Se usar um vídeo próprio, ensaie no mesmo computador para confirmar os codecs. Leve também uma comparação HTML exportada como evidência estática.

## 0:00–1:00 — Objetivo e entrada

“Nosso programa importa um vídeo existente e compara dois ramos do mesmo conteúdo. Escolhemos um trailer de Sintel, da Blender Foundation, como material de exemplo. A contribuição do grupo é a aplicação C++ que decodifica, transforma, mede e apresenta o vídeo.”

Mostre `midia/exemplo.webm` e sua atribuição. Explique que um arquivo próprio também pode ser usado. Não afirme que o grupo criou o vídeo nem que ele é uma fonte sintética.

## 1:00–2:00 — Demonstração

Dê dois cliques em `Iniciar.cmd`. O exemplo começa no segundo 5 para evitar a abertura preta. A janela colorida é a referência decodificada; a cinza é o resultado. Explique que a entrada mantém 854 × 480 e aproximadamente 24 FPS, enquanto a saída usa 320 × 180, 10 FPS e GRAY8.

A janela pode redimensionar a imagem. Mostre os dados medidos ao final para confirmar a resolução real. O áudio não faz parte desta demonstração.

## 2:00–3:00 — Arquitetura

“uridecodebin identifica o contêiner e os codecs. Depois de decodificar, convertemos para RGB e dividimos o fluxo com tee. Cada ramo possui queue. Um conserva tamanho e cadência; o outro passa por videoscale, videorate e videoconvert. O capsfilter exige as características finais.”

Mostre o diagrama em [arquitetura](02-pipeline.md). Ressalte: capsfilter restringe formatos; são os transformadores que realizam as mudanças.

## 3:00–4:00 — Pads dinâmicos

Mostre `ConectarVideo` e o registro de `pad-added`. “As faixas do arquivo não estão disponíveis como pads na hora da criação do leitor. Quando um pad aparece, verificamos suas caps e ligamos o primeiro vídeo bruto à entrada. Áudio e legendas não entram na nossa pipeline de processamento.”

Relacione essa etapa ao Tutorial 3. O projeto agora usa ligação dinâmica de verdade, pois trabalha com mídia externa.

## 4:00–5:00 — Conceitos

Relacione resolução à amostragem espacial, FPS à amostragem temporal e RGB/GRAY8 à representação de pixels. Explique que reduzir FPS mantém a duração, não produz câmera lenta. Repetição de quadros não inventa movimento, ampliar não recupera detalhe e converter de volta para RGB não recupera cores.

Se apresentar volume de dados, use os cálculos do documento de conceitos. Não compare diretamente o tamanho do arquivo WebM comprimido com a memória dos quadros brutos.

## 5:00–6:00 — Medições e ciclo de vida

Mostre o relatório JSON. “Lemos caps e contamos buffers reais. A taxa nominal da entrada pode não estar declarada; nesse exemplo aparece 0/1. Por isso também calculamos FPS pelos timestamps, obtendo aproximadamente 24. A saída tem caps 10/1 e FPS medido próximo de 10.”

Explique `PAUSED`, seek do trecho, `PLAYING`, mensagens `ERROR`/`EOS` e retorno a `NULL`. Os probes são instalados depois da preparação inicial para não contar o preroll como parte do trecho medido.

## 6:00–7:00 — Testes e limites

Mostre o resultado dos testes: importação real, dimensões, cadência, pixels, exportação, caminhos com acentos e arquivo inválido. A exportação gera imagens e relatório, não um vídeo final codificado. O arquivo original é preservado.

“Escolhemos um escopo que demonstra claramente o processamento. Suporte a cada formato depende dos plugins instalados. Para evoluir, poderíamos acrescentar gravação do resultado em um codec e tratamento de áudio.”

## Perguntas prováveis

| Pergunta | Resposta |
|---|---|
| O original é uma cópia binária do arquivo? | Não. É vídeo decodificado em RGB, mantendo dimensões e cadência. |
| Por que o FPS da entrada aparece 0/1? | A taxa não está declarada nas caps; os timestamps permitem medir a cadência. |
| MP4 sempre funciona? | Depende do codec contido no arquivo e dos plugins instalados. |
| Por que não basta ligar o leitor imediatamente? | Seus pads aparecem quando os fluxos são descobertos; usamos pad-added. |
| Por que não ouvimos áudio? | O callback seleciona apenas vídeo; áudio está fora do escopo. |
| O programa altera o arquivo original? | Não. Lê o arquivo e processa quadros em memória. |
| O programa salva outro vídeo? | Não nesta versão; exibe vídeo e pode exportar quadros e JSON. |
| Por que duas filas? | Para separar o processamento dos ramos, sem tornar seu armazenamento ilimitado. |
| 10 FPS é sempre redução? | Não. Só é redução se a entrada tiver cadência maior que 10. |
| A imagem exportada prova movimento? | Não. Ela prova pixels e dimensões; reprodução e PTS demonstram a cadência. |
