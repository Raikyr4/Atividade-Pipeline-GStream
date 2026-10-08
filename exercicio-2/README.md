# Exercicio 2 - H.264 e PCM

Aplicacao C++ com GStreamer que le um video existente, reencoda o video em H.264, converte o audio para PCM e grava tudo em um arquivo MOV (QuickTime).

Por padrao usa o trailer **Sintel** (Blender Foundation, CC BY 3.0) ja incluido em `../exercicio-1/midia/exemplo.webm` (VP8 854x480 + Vorbis 48 kHz estereo, 52 s). A atribuicao esta em `../exercicio-1/midia/README.md`.

## Requisitos

- GStreamer **MinGW x64**, pacotes Runtime e Development (`C:\Program Files\gstreamer\1.0\mingw_x86_64`, ou variavel `GSTREAMER_1_0_ROOT_MINGW_X86_64`).
- `g++` **64 bits** com C++17. O script usa `../.ferramentas/w64devkit/bin/g++.exe` se existir: extraia nessa pasta o [w64devkit x64 2.10.0](https://github.com/skeeto/w64devkit/releases/tag/v2.10.0). Senao, usa o `g++` do PATH (precisa ser x64; o MinGW.org 6.3 de 32 bits nao funciona).

## Executar

No PowerShell, dentro desta pasta:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Pcm A
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Pcm B
```

Use `-Video "CAMINHO"` para outro video (precisa ter faixa de audio) e `-Saida nome.mov` para escolher outro arquivo. A aplicacao nao sobrescreve uma saida existente.

## Configuracoes PCM

| Opcao | Caps `audio/x-raw` | Efeito perceptivel |
|---|---|---|
| A | `S16LE`, 44100 Hz, 2 canais | Maior fidelidade e estereo. |
| B | `S16LE`, 8000 Hz, 1 canal | Menor faixa de frequencias e mono. |

## Pipeline implementada

Diagramas (Mermaid): [docs/diagramas.md](docs/diagramas.md).

```text
filesrc -> decodebin -+-> videoconvert -> video/x-raw,format=I420 -> x264enc -> h264parse -> queue -+-> qtmux -> arquivo .mov
                      |                                                                            |
                      +-> audioconvert -> audioresample -> audio/x-raw (A ou B) -> queue ----------+
```

`decodebin` identifica o container e os codecs do arquivo e cria um pad dinamico para cada fluxo; cada pad e ligado ao ramo compativel (video ou audio). `audioconvert` ajusta formato e canais; `audioresample` ajusta a taxa de amostragem. O `qtmux` junta os dois fluxos no arquivo final. O programa espera `EOS` e exibe erros recebidos pelo bus da pipeline.

## Por que uma unica pipeline

- A saida e **um arquivo** com video e audio **sincronizados**: o `qtmux` precisa receber os dois fluxos ao mesmo tempo para intercala-los pelos timestamps.
- Video e audio vem do **mesmo arquivo de entrada**, entao um unico `filesrc ! decodebin` le e separa os dois.
- Uma `queue` em cada ramo da a ele uma thread propria, e os ramos processam em paralelo sem um travar o outro.

## Por que MOV e nao MKV

O leitor de MKV do Windows (Media Foundation, usado pelo app Media Player) nao decodifica PCM **estereo** dentro de MKV: a configuracao A ficava sem som, enquanto a B (mono) tocava. Testado com 8000, 44100 e 48000 Hz: todo PCM estereo em MKV falhou e todo mono funcionou. Em MOV (`qtmux`) e AVI (`avimux`) o estereo funciona; `mp4mux` nao aceita PCM. Escolhemos MOV por ser o container moderno da familia MP4/QuickTime.

O resultado abre no Media Player do Windows, no VLC e no QuickTime.
