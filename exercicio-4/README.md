# Exercicio 4 - streaming entre dispositivos (RTP/UDP)

Evolucao do [exercicio 3](../exercicio-3/README.md): o mesmo transmissor e receptor de audio RTP/UDP (PCMU), agora preparados para comunicacao **entre dois computadores** e nao apenas via `127.0.0.1`. Atende a Parte 2 da atividade "Streaming entre dispositivos". A Parte 1 (teste de rede com VLC) tambem tem script aqui.

## O que mudou em relacao ao exercicio 3

| Exercicio 3 | Exercicio 4 |
|---|---|
| `--destino` com valor fixo `127.0.0.1` no codigo | **Sem endereco fixo**: `--destino` e obrigatorio, por argumento ou em `rede.ini` (`--config`), e o IPv4 e validado |
| Porta e latencia so por argumento | Porta e latencia por argumento **ou** configuracao (`[transmissor]` e `[receptor]` no `.ini`) |
| Pacote RTP = buffer do `audiotestsrc` (1024 amostras = **128 ms**) | `rtppcmupay max-ptime=20 ms`: **20 ms por pacote** (padrao de VoIP), com menos atraso e perda menos audivel |
| Estatisticas so no fim; Ctrl+C perdia o resumo | Estatisticas do `rtpjitterbuffer` **a cada segundo**, resumo tambem no Ctrl+C e registro em CSV (`-Registro`) |
| Sem aviso quando nada chega | `udpsrc timeout`: avisa apos 3 s sem pacotes (IP, porta ou firewall errados) |
| So tom senoidal | Fontes `seno`, `bipe` (bom para perceber o atraso), `mic` e `arquivo` |
| Sem como provocar jitter | `--simular-rede` (`netsim`) atrasa e descarta pacotes de proposito para comparar as latencias |

## Requisitos e compilacao

O ambiente e o mesmo do exercicio 3: GStreamer MinGW x64 (Runtime e Development) e `g++` x64 com C++17. Os plugins usados sao `udp`, `rtp`, `rtpmanager`, `mulaw`, `audiotestsrc`, `autodetect` e `netsim` (este ultimo de `gst-plugins-bad`, so para `--simular-rede`). Na Parte 1 tambem sao usados `x264enc` e `mpegtsmux`.

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/compilar.ps1
```

## IP e porta

| | Porta | Protocolo | Quem escuta |
|---|---|---|---|
| Parte 1 | 5000 | MPEG-TS sobre UDP | VLC `udp://@:5000` |
| Parte 2 | 5004 | RTP PCMU (payload 0) sobre UDP | `exercicio_4 receptor` (ou VLC `rtp://@:5004`) |

Para descobrir o IPv4 do receptor, use `ipconfig` no Windows ou `ip addr` no Linux. Os exemplos usam A = `192.168.10.1` (transmissor) e B = `192.168.10.2` (receptor). Em Ethernet direta, configure IPs estaticos nessa faixa com mascara `255.255.255.0`.

No receptor, permita o `exercicio_4.exe` quando o firewall perguntar, ou libere a porta num PowerShell **como administrador**:

```powershell
New-NetFirewallRule -DisplayName "RTP 5004" -Direction Inbound -Protocol UDP -LocalPort 5004 -Action Allow
```

## Parte 1: teste de conectividade com VLC

1. No receptor, abra o VLC em Midia > Abrir Fluxo de Rede e digite `udp://@:5000`.
2. No transmissor:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/parte1-tutorial.ps1 -Destino 192.168.10.2
```

O script confere `x264enc`, `mpegtsmux` e `udpsink` e roda o `gst-launch-1.0` do enunciado. O fluxo MPEG-TS/UDP so valida a conectividade: ele **nao** e o mesmo formato RTP/UDP da Parte 2.

## Parte 2: audio entre duas maquinas

Inicie o receptor primeiro. Rode uma vez com cada latencia:

```powershell
# Maquina B (receptor): grava evidencias/latencia_50ms.txt e .csv
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Modo receptor -Latencia 50 -Duracao 30 -Registro
# depois
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Modo receptor -Latencia 500 -Duracao 30 -Registro
```

```powershell
# Maquina A (transmissor)
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Modo transmissor -Destino 192.168.10.2 -Fonte bipe
```

Opcoes do transmissor:

- `-Fonte seno|bipe|mic|arquivo`. `arquivo` usa por padrao o trailer Sintel de `exercicio-1/midia`; outro arquivo vai em `-Arquivo`.
- `-Porta N`
- `-Duracao S`
- `-SimularRede`
- `-Config ./rede.ini`, que le os valores de [`rede.ini`](rede.ini). Argumentos tem prioridade sobre o arquivo.

Uso direto, sem os scripts:

```text
build\exercicio_4.exe transmissor --destino 192.168.10.2 --porta 5004 --fonte bipe
build\exercicio_4.exe receptor --porta 5004 --latencia 50 --duracao 30 --csv evidencias\latencia_50ms.csv
build\exercicio_4.exe transmissor --config rede.ini
build\exercicio_4.exe --ajuda
```

O receptor imprime a cada segundo `num-pushed`, `num-lost`, `num-late` e `avg-jitter`. No fim mostra um resumo com a porcentagem de perda e a latencia da pipeline.

## Pipelines

[Diagrama Draw.io (dois dispositivos + RTP/UDP)](docs/diagrama-dispositivos.drawio) | [Diagramas em Mermaid](docs/diagramas.md) | [Experimentos e resultados](docs/experimentos.md) | [Passo a passo com celular](docs/POP-celular.md)

```text
A: fonte -> audioconvert -> audioresample -> audio/x-raw,8000 Hz,mono -> mulawenc -> rtppcmupay pt=0 max-ptime=20ms
     -> [netsim] -> udpsink host=IP_B port=5004
                         |  RTP sobre UDP (rede Wi-Fi/Ethernet)
B: udpsrc port=5004 caps=application/x-rtp,PCMU,8000,pt=0 -> rtpjitterbuffer latency=50|500 do-lost=true
     -> rtppcmudepay -> mulawdec -> audioconvert -> audioresample -> autoaudiosink
```

Como o UDP nao informa o formato do conteudo, o `udpsrc` do receptor declara as mesmas caps RTP do transmissor (PCMU, 8000 Hz, payload 0). O `rtpjitterbuffer` segura cada pacote por `latency` ms para reordenar e absorver o jitter; o que chega depois desse prazo e descartado (`num-late`). Por isso a escolha da latencia e uma troca: **50 ms** responde rapido mas falha com jitter alto, e **500 ms** absorve o jitter com meio segundo de atraso. Os numeros estao em [docs/experimentos.md](docs/experimentos.md).

## Problemas comuns

| Sintoma | Causa provavel | O que fazer |
|---|---|---|
| "3 s sem pacotes" no receptor | IP ou porta errados, ou firewall | Confira o `ipconfig` do receptor e libere a UDP 5004 |
| Nada passa no Wi-Fi da faculdade | Isolamento de clientes | Use o hotspot do celular ou cabo Ethernet. Registre o erro |
| `Error sending UDP packets ... rede inacessivel` | O PC trocou de rede ou perdeu o Wi-Fi | Reconecte, rode `ipconfig` de novo e confira o IP de destino |
| `GLib-GIO-WARNING ... AppxManifest.xml` | Aviso do GIO sobre apps da Microsoft Store | Pode ignorar |
| Audio picotado | Latencia baixa para o jitter da rede | Aumente `-Latencia` |
