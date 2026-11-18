# Experimentos e resultados

Todos os testes abaixo foram executados em 18/11/2026.

## 1. Teste local: comparacao das latencias (mesma maquina)

Ambiente: Windows 11, GStreamer 1.28.6 (MinGW x64). Receptor e transmissor rodaram na mesma maquina via `127.0.0.1`, cada teste numa porta propria. Receptor com `--duracao 20 --silencioso`; transmissor com fonte `seno`. Nas linhas "simulada" o transmissor usou `--simular-rede` (`netsim`: atraso de 0 a 120 ms em 30% dos pacotes e descarte de 1%). As saidas completas e os CSVs por segundo estao em [`../evidencias/loopback/`](../evidencias/loopback/).

| Rede | Latencia | num-pushed | num-lost | num-late | avg-jitter | Latencia da pipeline |
|---|---:|---:|---:|---:|---:|---:|
| loopback | 50 ms | 1070 | 0 (0 %) | 0 | 5,9 ms | 70 ms |
| loopback | 500 ms | 1029 | 0 (0 %) | 0 | 5,7 ms | 520 ms |
| loopback + netsim | 50 ms | 773 | **283 (26,8 %)** | **263** | 50,9 ms | 70 ms |
| loopback + netsim | 500 ms | 1019 | **14 (1,4 %)** | 0 | 62,7 ms | 520 ms |

- **Sem simulacao:** sem jitter relevante, as duas latencias entregam tudo, e a unica diferenca e o atraso (70 ms contra 520 ms). Uma rede sem variacao nao mostra a troca entre as latencias, como o exercicio 3 ja tinha observado.
- **Com netsim:** com atrasos de ate 120 ms, o buffer de **50 ms** descarta como atrasados cerca de 1 em cada 4 pacotes, e o audio sai picotado. Com **500 ms** nenhum pacote chega tarde, e sobram so as perdas proprias do `netsim` (cerca de 1%), que nenhum buffer recupera. O custo e quase meio segundo a mais de atraso.
- **Diferenca do exercicio 3:** os pacotes agora levam 20 ms de audio (cerca de 53 por segundo) em vez de 128 ms (cerca de 8 por segundo). Por isso os contadores sao maiores e cada pacote perdido corta so 20 ms de som.

## 2. Transmissao real entre dispositivos: PC para celular

| | Dispositivo A (transmissor) | Dispositivo B (receptor) |
|---|---|---|
| Aparelho | PC Windows 11, `exercicio_4.exe` | Celular com VLC |
| IPv4 | 10.221.126.246 | 10.221.126.157 |
| Rede | Hotspot do celular (Wi-Fi), mascara 255.255.255.0 | o proprio celular e o roteador (gateway) |

Conectividade antes do teste (`ping 10.221.126.157`, executado no PC): 4 enviados, 4 recebidos, **0% de perda**, tempo minimo 6 ms, maximo 91 ms e media 44 ms. A variacao de 6 a 91 ms mostra o **jitter** tipico do Wi-Fi do hotspot.

Comandos:

```powershell
# PC (transmissor)
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Modo transmissor -Destino 10.221.126.157 -Fonte bipe
```

```text
pipeline: audiotestsrc is-live=true wave=ticks ! audioconvert ! audioresample ! audio/x-raw,rate=8000,channels=1 ! mulawenc ! rtppcmupay pt=0 max-ptime=20000000 ! udpsink name=rede sync=true
transmissor destino=10.221.126.157:5004 fonte=bipe rede=real
```

No celular: VLC > Fluxo de rede > `rtp://@:5004`.

**Resultado: funcionou.** O celular tocou o bipe (um clique por segundo) recebido por RTP/UDP do PC. Como o PCMU com payload 0 e um tipo estatico do RTP, o VLC reconheceu o fluxo sem arquivo SDP.

Limites deste teste:
- O receptor foi o VLC, nao o `exercicio_4 receptor`. Por isso nao ha estatisticas do `rtpjitterbuffer` (`num-lost`, `num-late`) nem a comparacao 50 ms x 500 ms entre dispositivos. O VLC usa o proprio buffer de rede (cache), que nao foi alterado.
- Nao foi medida a latencia fim a fim.

## 3. Transmissao entre dois computadores com o receptor C++ (pendente)

A comparacao das duas latencias entre maquinas diferentes ainda precisa ser feita com um segundo PC rodando o receptor. Pela variacao de 6 a 91 ms medida no ping, o hotspot deve mostrar diferenca entre 50 ms e 500 ms mesmo sem `-SimularRede`.

- Maquina B: `executar.ps1 -Modo receptor -Latencia 50 -Duracao 30 -Registro`, depois o mesmo com `-Latencia 500`. Isso gera `evidencias/latencia_50ms.*` e `latencia_500ms.*`.
- Maquina A: `executar.ps1 -Modo transmissor -Destino IP_B -Fonte bipe`

| | Maquina A (transmissor) | Maquina B (receptor) |
|---|---|---|
| Aparelho / SO | | |
| IPv4 | | |
| Rede | | |

| Latencia | num-pushed | num-lost (%) | num-late | avg-jitter | Percepcao (atraso do bipe, cortes) |
|---|---:|---:|---:|---:|---|
| 50 ms | | | | | |
| 500 ms | | | | | |

## 4. Parte 1: VLC (MPEG-TS sobre UDP)

Teste local: a pipeline do enunciado foi enviada para `127.0.0.1:5000` e lida com `udpsrc ! tsdemux ! h264parse`. Chegaram 59 quadros H.264 de 640x360 em cerca de 4 s (15 fps).

Envio para o celular pelo Wi-Fi da faculdade (`-Destino 10.42.39.22`): o `gst-launch-1.0` rodou cerca de 5 minutos sem erro de envio, mas a exibicao no VLC nao foi registrada. Logo depois o PC trocou de rede (veja a secao 5).

| Data | Rede | Receptor (PC ou celular) | Resultado |
|---|---|---|---|
| | | | |

## 5. Bloqueios e erros encontrados

| Rede | Erro observado | Causa provavel | Acao |
|---|---|---|---|
| Wi-Fi da faculdade (10.42.x.x) | `Error sending UDP packets ... Uma operacao de soquete foi tentada em uma rede inacessivel` durante o envio para 10.42.39.22 | O PC trocou de rede no meio da transmissao | Reconectar e conferir o IP com `ipconfig` |
| Wi-Fi da faculdade (PC em 10.42.20.251) | `ping 10.42.39.22`: tempo esgotado, 100% de perda | Isolamento de clientes, ou o celular mudou de IP | Usar o hotspot do celular (secao 2) |
| PC sozinho | Receptor rodando no proprio PC com `-Latencia 50 -Registro` recebeu **0 pacotes** em 30 s e avisou "3 s sem pacotes" | O transmissor estava enviando para outro IP (10.42.39.22), nao para o proprio PC | Receptor e destino precisam ser o mesmo aparelho |

## Conclusao

- **Entre dispositivos:** o transmissor C++ enviou audio RTP/UDP do PC para o celular pelo hotspot, sem endereco fixo no codigo (IP informado em `-Destino`). O Wi-Fi da faculdade nao permitiu a comunicacao entre os aparelhos, e o hotspot contornou o bloqueio.
- **Latencia do rtpjitterbuffer (teste local):** o buffer troca **atraso por robustez**. Com 50 ms a resposta e rapida, mas jitter acima da margem vira pacote descartado e som picotado (26,8% de perda com netsim). Com 500 ms o jitter e absorvido e sobram so as perdas reais (1,4%), com meio segundo a mais de atraso. Sem jitter, os dois valores funcionam igualmente.
- **Pendente:** repetir a comparacao 50 ms x 500 ms entre dois PCs (secao 3).
