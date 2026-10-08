# Diagramas das pipelines - Exercicio 2

Diagramas em [Mermaid](https://mermaid.js.org/). O GitHub mostra os desenhos direto neste arquivo; para exportar PNG/SVG, cole cada bloco em <https://mermaid.live>.

## 1. Pipeline completa

Uma unica pipeline: o arquivo e separado em video e audio, cada ramo e processado e o `qtmux` junta os dois no `.mov`.

```mermaid
flowchart LR
    FS["filesrc<br/><i>exemplo.webm</i>"] --> DB["decodebin<br/><i>separa e decodifica</i>"]

    DB -->|video| VC["videoconvert"] --> VCAPS["caps<br/><i>I420</i>"] --> X264["x264enc<br/><i>H.264</i>"] --> HP["h264parse"] --> QV["queue"] --> MUX
    DB -->|audio| AC["audioconvert"] --> AR["audioresample"] --> ACAPS["caps PCM<br/><i>A ou B</i>"] --> QA["queue"] --> MUX

    MUX["qtmux<br/><i>MOV</i>"] --> SINK["filesink<br/><i>saida_pcm_A/B.mov</i>"]

    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000
    class VCAPS,ACAPS caps
```

## 2. Ramo de video (H.264)

```mermaid
flowchart LR
    IN["decodebin<br/><i>VP8 decodificado</i>"] --> VC["videoconvert<br/><i>converte pixels</i>"]
    VC --> CAPS["caps<br/><i>format = I420</i>"]
    CAPS --> ENC["x264enc<br/><i>comprime em H.264</i><br/><i>tune = zerolatency</i>"]
    ENC --> HP["h264parse<br/><i>prepara para o container</i>"]
    HP --> Q["queue"] --> MUX["qtmux"]

    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000
    class CAPS caps
```

## 3. Ramo de audio (PCM)

O `capsfilter` define a configuracao; `audioconvert` e `audioresample` se ajustam a ele.

```mermaid
flowchart LR
    IN["decodebin<br/><i>Vorbis decodificado</i><br/><i>float 32 bits / 48000 Hz / 2 canais</i>"] --> AC["audioconvert<br/><i>formato e canais</i>"]
    AC --> AR["audioresample<br/><i>taxa de amostragem</i>"]
    AR --> CAPS["capsfilter audio/x-raw<br/><b>A:</b> S16LE / 44100 Hz / 2 canais<br/><b>B:</b> S16LE / 8000 Hz / 1 canal"]
    CAPS --> Q["queue"] --> MUX["qtmux"]

    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000
    class CAPS caps
```

| | Config A | Config B | Propriedade do audio digital |
|---|---|---|---|
| Taxa (`rate`) | 44.100 Hz | 8.000 Hz | Nyquist: maior frequencia = taxa / 2. A vai ate 22 kHz (toda a audicao); B so ate 4 kHz, por isso soa abafado, como telefone |
| Profundidade (`format`) | 16 bits | 16 bits | 65.536 niveis, cerca de 96 dB de faixa dinamica nas duas |
| Canais (`channels`) | 2 (estereo) | 1 (mono) | B perde a separacao esquerda/direita |
| Taxa de bits | 1.411 kbit/s | 128 kbit/s | taxa x bits x canais: B gera 11x menos dados de audio |

## 4. Fluxo do programa e tratamento de erros

```mermaid
flowchart TD
    A["Le argumentos<br/>--video, --pcm, --saida"] --> B{"Entradas validas?"}
    B -- nao --> ERRO["Mostra erro<br/>e sai com codigo 1"]
    B -- sim --> C["gst_parse_launch<br/>monta a pipeline"]
    C --> D["set_state PLAYING"]
    D --> E["Bus: espera ERROR ou EOS"]
    E -- ERROR --> ERRO
    E -- EOS --> F["Arquivo .mov pronto"]
    F --> G["set_state NULL<br/>libera recursos"]

    classDef erro fill:#fee2e2,stroke:#dc2626,color:#000
    classDef ok fill:#dcfce7,stroke:#16a34a,color:#000
    class ERRO erro
    class F ok
```

"Entradas validas" verifica: argumentos reconhecidos, PCM igual a A ou B, video de entrada existente e arquivo de saida ainda inexistente. Falhas ao criar a pipeline ou ao mudar para PLAYING tambem levam ao erro.
