# Exercicio 2 - H.264 e PCM

Aplicacao C++ com GStreamer que le um video existente, reencoda o video em H.264, converte o audio para PCM e grava tudo em um arquivo MKV.

Por padrao usa o trailer **Sintel** (Blender Foundation, CC BY 3.0) ja incluido em `../exercicio-1/midia/exemplo.webm` (VP8 854x480 + Vorbis 48 kHz estereo, 52 s). A atribuicao esta em `../exercicio-1/midia/README.md`.

## Executar

No PowerShell, dentro desta pasta:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Pcm A
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Pcm B
```

Use `-Video "CAMINHO"` para outro video (precisa ter faixa de audio) e `-Saida nome.mkv` para escolher outro arquivo. A aplicacao nao sobrescreve uma saida existente.

## Configuracoes PCM

| Opcao | Caps `audio/x-raw` | Efeito perceptivel |
|---|---|---|
| A | `S16LE`, 44100 Hz, 2 canais | Maior fidelidade e estereo. |
| B | `S16LE`, 8000 Hz, 1 canal | Menor faixa de frequencias e mono. |

## Pipeline implementada

Diagramas detalhados (Mermaid), com os caps negociados em cada ligacao: [docs/diagramas.md](docs/diagramas.md).

```text
filesrc -> decodebin -+-> videoconvert -> video/x-raw,format=I420 -> x264enc -> h264parse -> queue -+-> matroskamux -> arquivo .mkv
                      |                                                                            |
                      +-> audioconvert -> audioresample -> audio/x-raw (A ou B) -> queue ----------+
```

`decodebin` identifica o container e os codecs do arquivo e cria um pad dinamico para cada fluxo; cada pad e ligado ao ramo compativel (video ou audio). `audioconvert` ajusta formato e canais; `audioresample` ajusta a taxa de amostragem. O `matroskamux` junta os dois fluxos no arquivo final. O programa espera `EOS` e exibe erros recebidos pelo bus da pipeline.

Para abrir o resultado, use VLC ou outro player com suporte a MKV/H.264/PCM.
