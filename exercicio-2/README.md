# Exercicio 2 - H.264 e PCM

Aplicacao C++ com GStreamer que gera cinco segundos de video H.264 e audio PCM em um arquivo MKV. Nao usa arquivos de entrada: `videotestsrc` cria barras de cor e `audiotestsrc` cria um tom.

## Executar

No PowerShell, dentro desta pasta:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Pcm A
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Pcm B
```

Use `-Saida nome.mkv` para escolher outro arquivo. A aplicacao nao sobrescreve uma saida existente.

## Configuracoes PCM

| Opcao | Caps `audio/x-raw` | Efeito perceptivel |
|---|---|---|
| A | `S16LE`, 44100 Hz, 2 canais | Maior fidelidade e estereo. |
| B | `S16LE`, 8000 Hz, 1 canal | Menor faixa de frequencias e mono. |

## Pipeline implementada

```text
videotestsrc -> videoconvert -> x264enc -> h264parse -> queue -> matroskamux -> arquivo .mkv

audiotestsrc -> audioconvert -> audioresample -> audio/x-raw (A ou B) -> queue -> matroskamux
```

`audioconvert` ajusta formato e canais; `audioresample` ajusta a taxa de amostragem. O `matroskamux` junta os dois fluxos no arquivo final. O programa espera `EOS` e exibe erros recebidos pelo bus da pipeline.

Para abrir o resultado, use VLC ou outro player com suporte a MKV/H.264/PCM.
