# Experimentos e resultados

## 1. Teste local com rede real e rede simulada (ja executado)

Executado em 18/11/2026 no Windows 11 com GStreamer 1.28.6 (MinGW x64). O receptor e o transmissor rodaram na mesma maquina via `127.0.0.1`, cada teste com uma porta propria. Receptor com `--duracao 20 --silencioso` e transmissor com fonte `seno`. A "rede simulada" usa `--simular-rede` (`netsim`): atraso de 0 a 120 ms em 30% dos pacotes e descarte de 1%. As saidas completas e os CSVs por segundo estao em [`../evidencias/loopback/`](../evidencias/loopback/).

| Rede | Latencia | num-pushed | num-lost | num-late | avg-jitter | Latencia da pipeline |
|---|---:|---:|---:|---:|---:|---:|
| real | 50 ms | 1070 | 0 (0 %) | 0 | 5,9 ms | 70 ms |
| real | 500 ms | 1029 | 0 (0 %) | 0 | 5,7 ms | 520 ms |
| simulada | 50 ms | 773 | **283 (26,8 %)** | **263** | 50,9 ms | 70 ms |
| simulada | 500 ms | 1019 | **14 (1,4 %)** | 0 | 62,7 ms | 520 ms |

- **Rede real (loopback):** sem jitter relevante, as duas latencias entregam tudo. A unica diferenca e o atraso (70 ms contra 520 ms). Isso confirma o que o exercicio 3 ja tinha observado: uma rede sem variacao nao mostra a troca entre as latencias.
- **Rede simulada:** com atrasos de ate 120 ms, o buffer de **50 ms** descarta como atrasados cerca de 1 em cada 4 pacotes, e o audio sai picotado. Com **500 ms**, nenhum pacote chega tarde e so sobram as perdas reais do `netsim` (cerca de 1%), que nenhum buffer recupera. O custo e quase meio segundo de atraso.
- **Diferenca do exercicio 3:** os pacotes agora levam 20 ms de audio (cerca de 53 por segundo) em vez de 128 ms (cerca de 8 por segundo). Por isso os contadores sao maiores e cada pacote perdido corta so 20 ms de som.

## 2. Transmissao real entre dois computadores

> **EXEMPLO ILUSTRATIVO: valores ficticios, NAO medidos.** Esta secao serve de modelo do formato e da ordem de grandeza esperados. Para entregar, substitua pelos numeros de `evidencias/latencia_50ms.txt` e `latencia_500ms.txt` da sua demonstracao.

| | Maquina A (transmissor) | Maquina B (receptor) |
|---|---|---|
| Aparelho / SO | Notebook, Windows 11 | Notebook, Windows 11 |
| IPv4 | 192.168.43.120 | 192.168.43.87 |
| Rede | Hotspot de celular (Wi-Fi 2,4 GHz) | mesma rede |

Receptor com `-Duracao 30`, transmissor com `-Fonte bipe` iniciado cerca de 2 s depois (cerca de 28 s de audio a 50 pacotes/s, ou seja, cerca de 1400 pacotes esperados).

| Latencia | num-pushed | num-lost (%) | num-late | avg-jitter | Percepcao (atraso do bipe, cortes) |
|---|---:|---:|---:|---:|---|
| 50 ms | 1377 | 23 (1,6 %) | 19 | 9,4 ms | Bipe praticamente imediato; 3 ou 4 estalos curtos em 30 s, nos picos de jitter do Wi-Fi |
| 500 ms | 1396 | 4 (0,3 %) | 0 | 8,9 ms | Bipe atrasado cerca de meio segundo; nenhum corte audivel |

Leitura: o jitter medio do Wi-Fi (cerca de 9 ms) fica abaixo de 50 ms, mas rajadas ocasionais passam dessa margem e viram `num-late`. Com 500 ms nenhum pacote chega tarde, e as 4 perdas restantes sao perdas reais da rede. A tendencia e a mesma do teste com `netsim` da secao 1, so que bem mais suave, porque a rede real tem menos jitter que a simulada.

### Modelo em branco para a demonstracao

| | Maquina A (transmissor) | Maquina B (receptor) |
|---|---|---|
| Aparelho / SO | | |
| IPv4 | | |
| Rede | Wi-Fi ___ / hotspot / Ethernet direta | |

Comandos:
- Maquina B: `executar.ps1 -Modo receptor -Latencia 50 -Duracao 30 -Registro`, depois o mesmo com `-Latencia 500`. Isso gera `evidencias/latencia_50ms.*` e `latencia_500ms.*`.
- Maquina A: `executar.ps1 -Modo transmissor -Destino IP_B -Fonte bipe`

| Latencia | num-pushed | num-lost (%) | num-late | avg-jitter | Percepcao (atraso do bipe, cortes) |
|---|---:|---:|---:|---:|---|
| 50 ms | | | | | |
| 500 ms | | | | | |

Para ter variacao mesmo numa rede boa, repita com `-SimularRede` no transmissor.

## 3. Parte 1: VLC

| Data | Rede | Receptor (PC ou celular) | Resultado |
|---|---|---|---|
| *exemplo ilustrativo* | Hotspot de celular | VLC no notebook B (`udp://@:5000`) | Bola visivel cerca de 2 s apos iniciar; imagem fluida a 15 fps |
| *exemplo ilustrativo* | Wi-Fi institucional | VLC no notebook B | Nada aparece: isolamento de clientes. Registrado e refeito no hotspot |
| | | | |

Teste local do autor: a pipeline do enunciado enviada para `127.0.0.1:5000` e lida com `udpsrc ! tsdemux ! h264parse` entregou 59 quadros H.264 de 640x360 em cerca de 4 s (15 fps).

## 4. Bloqueios encontrados

| Data | Rede | Erro | Acao |
|---|---|---|---|
| 18/11/2026 | troca de Wi-Fi durante o envio | `Error sending UDP packets ... rede inacessivel` | Reconectar, rodar `ipconfig` de novo e usar o IP atual do receptor |

## Conclusao

O rtpjitterbuffer troca **atraso por robustez**. Com 50 ms a resposta e rapida, mas qualquer jitter maior que a margem vira pacote descartado e som picotado. Com 500 ms o jitter e absorvido e so as perdas reais da rede continuam, ao custo de meio segundo de atraso. Em rede local cabeada os dois valores funcionam. Em Wi-Fi congestionado a latencia maior e a mais segura para streaming, e a menor so vale a pena em uso interativo, como uma chamada de voz, em que o atraso incomoda mais.
