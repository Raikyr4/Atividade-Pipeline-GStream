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

## 2. Transmissao real entre dois computadores (preencher na demonstracao)

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
| | | | |

Teste local do autor: a pipeline do enunciado enviada para `127.0.0.1:5000` e lida com `udpsrc ! tsdemux ! h264parse` entregou 59 quadros H.264 de 640x360 em cerca de 4 s (15 fps).

## 4. Bloqueios encontrados

| Data | Rede | Erro | Acao |
|---|---|---|---|
| 18/11/2026 | troca de Wi-Fi durante o envio | `Error sending UDP packets ... rede inacessivel` | Reconectar, rodar `ipconfig` de novo e usar o IP atual do receptor |

## Conclusao

O rtpjitterbuffer troca **atraso por robustez**. Com 50 ms a resposta e rapida, mas qualquer jitter maior que a margem vira pacote descartado e som picotado. Com 500 ms o jitter e absorvido e so as perdas reais da rede continuam, ao custo de meio segundo de atraso. Em rede local cabeada os dois valores funcionam. Em Wi-Fi congestionado a latencia maior e a mais segura para streaming, e a menor so vale a pena em uso interativo, como uma chamada de voz, em que o atraso incomoda mais.
