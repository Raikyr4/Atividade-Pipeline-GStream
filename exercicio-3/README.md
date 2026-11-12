# Exercicio 3 - streaming de audio RTP/UDP

Aplicacao C++ com dois modos: transmissor de um sinal senoidal e receptor com compensacao de jitter e diagnostico de pacotes RTP. O exemplo e reproduzivel sem microfone e pode ser executado na mesma maquina ou em duas maquinas da mesma rede.

## Requisitos e compilacao

No Windows, use o mesmo ambiente do exercicio 2: GStreamer **MinGW x64**, pacotes Runtime e Development, e compilador `g++` x64 com C++17. O script procura o SDK na variavel `GSTREAMER_1_0_ROOT_MINGW_X86_64` ou em `C:/Program Files/gstreamer/1.0/mingw_x86_64`, e o compilador em `../.ferramentas/w64devkit/bin/g++.exe` ou no PATH. Instale os plugins de audio, RTP, UDP e RTP Manager. Consulte o [exercicio 2](../exercicio-2/README.md) para o ambiente.

Nesta maquina, tambem foi preparado um SDK GStreamer 1.26.11 local em `../.ferramentas/gstreamer-sdk` e o compilador portatil em `../.ferramentas/w64devkit`. Os scripts reconhecem essa alternativa e usam o runtime MSVC encontrado no PATH (ou `GSTREAMER_1_0_ROOT_MSVC_X86_64`). Essas ferramentas locais estao no `.gitignore` e nao acompanham o repositorio.

No PowerShell, nesta pasta:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/compilar.ps1
```

Alternativamente, com CMake: `cmake -S . -B build` e `cmake --build build`. Para MSVC, configure `GSTREAMER_1_0_ROOT_MSVC_X86_64` apontando para um SDK MSVC compativel; para Linux, instale o pacote de desenvolvimento do GStreamer e `pkg-config`.

## Executar e demonstrar

Abra dois terminais. Inicie o receptor primeiro, depois o transmissor. Neste exemplo, o receptor encerra sozinho em 20 segundos e imprime suas estatisticas; o transmissor executa por 15 segundos.

```powershell
# Terminal 1
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Modo receptor -Porta 5004 -Latencia 200 -Duracao 20
# Terminal 2, logo em seguida
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Modo transmissor -Destino 127.0.0.1 -Porta 5004 -Duracao 15
```

O som esperado e um tom continuo de 440 Hz. Repita com `-Latencia 50`, `200` e `500`, e compare o inicio, as interrupcoes audiveis e as estatisticas. Use `-Silencioso` no receptor para testes sem dispositivo de audio: nesse caso os dados sao consumidos por `fakesink`, sem reproducao. A duracao zero deixa o processo em execucao continua; use duracao positiva para obter as estatisticas ao final (Ctrl+C pode encerrar o processo sem imprimi-las).

Em maquinas diferentes, informe em `-Destino` o IP da maquina receptora e use a mesma porta em ambos os terminais. A rede e o firewall devem permitir UDP de entrada nessa porta. Nao ha descoberta automatica nem criptografia.

A porta aceita 1 a 65535, a latencia 0 a 60000 ms e a duracao 0 a 86400 s. Erros de criacao, inicializacao e execucao da pipeline sao reportados no console.

## Funcionamento

[Diagramas das pipelines](docs/diagramas.md) | [Experimentos e conclusao](docs/experimentos.md)

O transmissor gera audio mono, converte e reamostra para 8 kHz, codifica em G.711 mu-law (PCMU) e encapsula em RTP payload 0. O receptor configura as caps RTP explicitamente no `udpsrc`, pois UDP nao informa sozinho o formato. O `rtpjitterbuffer` reordena pacotes e espera ate o limite configurado antes de encaminhar o fluxo para desempacotamento, decodificacao e reproducao. A escolha PCMU reduz a complexidade da demonstracao, mas limita a qualidade ao audio mono de 8 kHz.

A propriedade `stats` do jitter buffer e impressa ao fim do receptor. Observe `num-pushed`, `num-lost`, `num-late`, `num-duplicates` e `avg-jitter` (em nanossegundos). `do-lost=true` habilita eventos de perda a jusante; o diagnostico apresentado e a consulta das estatisticas, nao um algoritmo proprio de recuperacao. Esses contadores nao medem perda antes de o receptor comecar a receber e nao comprovam que o som foi ouvido.

A latencia configurada e a margem de espera do jitter buffer, nao uma medicao de latencia fim a fim. Um buffer maior pode absorver mais variacao de chegada, mas aumenta o atraso; nao recupera pacotes perdidos na rede por si so. Uma rede local sem variacao ou perda nao demonstra esse compromisso de forma conclusiva.

## Referencias oficiais

- [Conceitos e construcao de pipelines](https://gstreamer.freedesktop.org/documentation/tutorials/basic/concepts.html)
- [Caps e formatos](https://gstreamer.freedesktop.org/documentation/tutorials/basic/media-formats-and-pad-capabilities.html)
- [UDP source](https://gstreamer.freedesktop.org/documentation/udp/udpsrc.html) e [UDP sink](https://gstreamer.freedesktop.org/documentation/udp/udpsink.html)
- [RTP PCMU payloader](https://gstreamer.freedesktop.org/documentation/rtp/rtppcmupay.html) e [depayloader](https://gstreamer.freedesktop.org/documentation/rtp/rtppcmudepay.html)
- [RTP jitter buffer e estatisticas](https://gstreamer.freedesktop.org/documentation/rtpmanager/rtpjitterbuffer.html)
