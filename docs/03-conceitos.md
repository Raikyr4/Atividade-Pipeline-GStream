# Conceitos aplicados ao vídeo importado

## Contêiner, codec e quadros brutos

MP4, WebM e MKV são contêineres: organizam faixas, timestamps e outros dados. O codec descreve como os dados foram comprimidos. A extensão não garante que o decodificador esteja instalado. `uridecodebin` identifica o conteúdo e seleciona plugins para obter quadros `video/x-raw`.

O vídeo de referência já foi decodificado e convertido para RGB. Portanto, o programa preserva seu conteúdo, tamanho e tempo, mas não os bytes nem necessariamente o formato de pixels originalmente usados pelo codec.

## Resolução e amostragem espacial

No exemplo Sintel, medimos 854 × 480 = 409.920 pixels por quadro. A saída usa 320 × 180 = 57.600 pixels. Reduzir essa grade exige reamostragem e pode eliminar detalhes. Aumentar a janela depois não recupera a informação perdida.

O destino tem proporção 16:9 e pixels quadrados. `videoscale` pode inserir bordas para preservar a proporção quando a entrada for diferente. A resolução deve ser conferida pelas caps, não pelo tamanho aparente da janela.

## FPS e amostragem temporal

O exemplo apresenta aproximadamente 24 quadros por segundo pelos PTS; a saída apresenta 10. São aproximadamente 41,67 ms entre quadros de entrada e 100 ms na saída. Há menos amostras do movimento, mantendo o intervalo temporal do trecho.

`videorate` descarta ou repete quadros; não cria movimento intermediário por interpolação. Reduzir FPS não é produzir câmera lenta. Se a entrada tiver menos de 10 FPS, o elemento poderá repetir quadros para atingir a saída; nesse caso não se deve falar em redução.

As caps do WebM incluído declaram FPS como 0/1. Esse valor indica cadência não especificada. A reprodução continua tendo timestamps. O programa calcula a média observada:

```text
FPS por PTS = (quantidade de quadros − 1) × 1.000.000.000
             / (PTS final − PTS inicial)
```

N quadros delimitam N − 1 intervalos. O fator converte nanossegundos em segundos. Essa média não mede FPS da CPU nem taxa física do monitor. Em arquivos com cadência variável, é uma média do trecho, não prova de intervalos uniformes.

## RGB e GRAY8

RGB usa três componentes de 8 bits nesta aplicação, totalizando três bytes por pixel. GRAY8 usa uma intensidade de 8 bits, um byte por pixel. A conversão perde informação cromática: não é possível recuperar as cores originais depois apenas voltando a RGB.

O cálculo do cinza não deve ser descrito como simples média aritmética de R, G e B: as regras de conversão e informações de cor negociadas influenciam o resultado. A comparação mede GRAY8 antes do conversor final exigido pela janela.

## Estimativa de dados brutos no exemplo

Usando 24 FPS como aproximação da entrada, sem alinhamento de memória ou metadados:

| Grandeza | Referência RGB | Processado GRAY8 |
|---|---:|---:|
| Pixels por quadro | 409.920 | 57.600 |
| Bytes por pixel | 3 | 1 |
| Bytes por quadro | 1.229.760 | 57.600 |
| Quadros por segundo | aproximadamente 24 | 10 |
| Bytes ativos por segundo | aproximadamente 29.514.240 | 576.000 |

A razão aproximada é 51,24. Isso corresponde a cerca de 98,05% menos bytes ativos por segundo na representação de saída. **Não é a taxa de compressão do WebM**, nem uma medição de economia total de CPU/RAM. A aplicação ainda executa decoder, dois ramos e filas. Para outro arquivo, refaça as contas com seus valores reais.

## Pad, caps, buffer, bus e estado

Pads são portas dos elementos. Caps descrevem formatos compatíveis ou negociados. Buffers carregam quadros e timestamps. O bus comunica mensagens de controle à aplicação. `PAUSED` prepara a mídia, `PLAYING` permite reprodução e `NULL` encerra recursos.

O seek escolhe um intervalo do vídeo existente. `FLUSH` descarta dados pendentes; `ACCURATE` solicita posicionamento preciso. O limite usa tempo de mídia, preservando seu significado mesmo quando o teste processa tudo rapidamente sem desenhar na tela.

## Critério para a apresentação

Use o exemplo incluído ou outro vídeo colorido, com resolução e FPS diferentes da saída. Assim é possível demonstrar pelo menos duas alterações reais. Um vídeo já cinza, 320 × 180 e 10 FPS não é uma boa escolha para essa atividade.
