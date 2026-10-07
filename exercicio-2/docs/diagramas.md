# Diagramas das pipelines - Exercicio 2

Diagramas em [Mermaid](https://mermaid.js.org/). O GitHub renderiza direto neste arquivo; para editar ou exportar PNG/SVG, cole cada bloco em <https://mermaid.live>.

Os caps mostrados nas ligacoes foram obtidos da execucao real com `gst-launch-1.0 -v` usando o trailer Sintel (`../exercicio-1/midia/exemplo.webm`).

1. [Visao geral da pipeline](#1-visao-geral-da-pipeline)
2. [Ramo de video (H.264)](#2-ramo-de-video-h264)
3. [Ramo de audio PCM - Configuracao A](#3-ramo-de-audio-pcm---configuracao-a)
4. [Ramo de audio PCM - Configuracao B](#4-ramo-de-audio-pcm---configuracao-b)
5. [Comparacao A x B](#5-comparacao-a-x-b)
6. [Fluxo do programa e tratamento de erros](#6-fluxo-do-programa-e-tratamento-de-erros)
7. [Estados da pipeline e mensagens do bus](#7-estados-da-pipeline-e-mensagens-do-bus)
8. [Ligacao dos pads dinamicos do decodebin](#8-ligacao-dos-pads-dinamicos-do-decodebin)

---

## 1. Visao geral da pipeline

Uma unica pipeline com dois ramos. O `decodebin` separa o arquivo em video e audio; cada ramo processa seu fluxo e o `matroskamux` junta os dois, sincronizados, no arquivo `.mkv`.

```mermaid
flowchart TB
    subgraph ENTRADA["Fonte"]
        direction LR
        FS["<b>filesrc</b><br/>location = exemplo.webm"]
        DB["<b>decodebin</b><br/>demux + decodificacao<br/>WebM: VP8 + Vorbis"]
        FS -->|"bytes WebM"| DB
    end

    subgraph VIDEO["Ramo de video - H.264"]
        direction LR
        VC["<b>videoconvert</b><br/>converte pixels"]
        VF["<b>capsfilter</b><br/>video/x-raw<br/>format = I420"]
        X264["<b>x264enc</b><br/>tune = zerolatency"]
        HP["<b>h264parse</b>"]
        QV["<b>queue</b>"]
        VC --> VF --> X264 -->|"video/x-h264"| HP --> QV
    end

    subgraph AUDIO["Ramo de audio - PCM"]
        direction LR
        AC["<b>audioconvert</b><br/>formato e canais"]
        AR["<b>audioresample</b><br/>taxa de amostragem"]
        AF["<b>capsfilter PCM</b><br/>audio/x-raw<br/>format / rate / channels<br/>(Config A ou B)"]
        QA["<b>queue</b>"]
        AC --> AR --> AF --> QA
    end

    subgraph SAIDA["Saida"]
        direction LR
        MUX["<b>matroskamux</b><br/>container MKV"]
        SINK["<b>filesink</b><br/>location = saida_pcm_A/B.mkv"]
        MUX -->|"video/x-matroska"| SINK
    end

    DB -->|"video/x-raw"| VC
    DB -->|"audio/x-raw"| AC
    QV -->|"video_0"| MUX
    QA -->|"audio_0"| MUX

    classDef fonte fill:#dbeafe,stroke:#2563eb,color:#000
    classDef video fill:#fef3c7,stroke:#d97706,color:#000
    classDef audio fill:#dcfce7,stroke:#16a34a,color:#000
    classDef saida fill:#f3e8ff,stroke:#9333ea,color:#000
    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000,stroke-width:2px
    class FS,DB fonte
    class VC,X264,HP,QV video
    class AC,AR,QA audio
    class MUX,SINK saida
    class VF,AF caps
```

Texto equivalente usado no codigo (`src/principal.cpp`, `gst_parse_launch`):

```text
matroskamux name=mux ! filesink name=arquivo
filesrc name=entrada ! decodebin name=dec
dec. ! videoconvert ! video/x-raw,format=I420 ! x264enc tune=zerolatency ! h264parse ! queue ! mux.
dec. ! audioconvert ! audioresample ! <caps PCM A ou B> ! queue ! mux.
```

---

## 2. Ramo de video (H.264)

O video original (VP8) e descomprimido e depois comprimido de novo em H.264 (MPEG-4 Part 10 / AVC).

```mermaid
flowchart TD
    subgraph DEC["decodebin (montado automaticamente)"]
        direction TB
        DMX["<b>matroskademux</b><br/>separa os fluxos do WebM"]
        VP8["<b>nvvp8dec</b> (GPU NVIDIA)<br/>ou <b>vp8dec</b> (CPU)<br/>descomprime VP8"]
        DMX -->|"video/x-vp8<br/>854x480"| VP8
    end

    VP8 -->|"video/x-raw<br/>format = NV12<br/>854x480"| VC["<b>videoconvert</b><br/>NV12 para I420"]
    VC --> CF["<b>capsfilter</b><br/>video/x-raw, format = I420<br/>(YUV 4:2:0)"]
    CF -->|"video/x-raw<br/>format = I420<br/>854x480"| ENC["<b>x264enc</b><br/>tune = zerolatency<br/>sem espera de quadros futuros"]
    ENC -->|"video/x-h264<br/>profile = high, level = 3<br/>stream-format = avc<br/>alignment = au"| PARSE["<b>h264parse</b><br/>organiza o fluxo H.264<br/>para o container"]
    PARSE --> Q["<b>queue</b><br/>thread propria / buffer"]
    Q -->|"pad video_0"| MUX["<b>matroskamux</b>"]

    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000,stroke-width:2px
    classDef video fill:#fef3c7,stroke:#d97706,color:#000
    class CF caps
    class VC,ENC,PARSE,Q video
```

| Elemento | Funcao | Propriedade / caps |
|---|---|---|
| `videoconvert` | Converte a representacao dos pixels | NV12 para I420 |
| `capsfilter` | Obriga YUV 4:2:0 (perfil High, compativel com players) | `format=I420` |
| `x264enc` | Codifica em H.264 | `tune=zerolatency` |
| `h264parse` | Ajusta formato do fluxo para o muxer | `stream-format=avc`, `alignment=au` |
| `queue` | Separa o ramo em outra thread | - |

---

## 3. Ramo de audio PCM - Configuracao A

**A = S16LE / 44.100 Hz / 2 canais (estereo).** Qualidade de CD.

```mermaid
flowchart TD
    subgraph DEC["decodebin"]
        direction TB
        DMX["<b>matroskademux</b>"]
        VD["<b>vorbisdec</b><br/>descomprime Vorbis"]
        DMX -->|"audio/x-vorbis<br/>48000 Hz, 2 canais"| VD
    end

    VD -->|"audio/x-raw<br/>format = F32LE (float 32 bits)<br/>rate = 48000<br/>channels = 2"| AC["<b>audioconvert</b><br/>F32LE para S16LE<br/>(canais mantidos: 2)"]
    AC -->|"audio/x-raw<br/>format = S16LE<br/>rate = 48000<br/>channels = 2"| AR["<b>audioresample</b><br/>48000 Hz para 44100 Hz"]
    AR -->|"audio/x-raw<br/>format = S16LE<br/>rate = 44100<br/>channels = 2"| CF["<b>capsfilter - Config A</b><br/>audio/x-raw<br/>format = S16LE<br/>rate = 44100<br/>channels = 2"]
    CF --> Q["<b>queue</b>"]
    Q -->|"pad audio_0"| MUX["<b>matroskamux</b><br/>grava PCM sem compressao"]

    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000,stroke-width:2px
    classDef audio fill:#dcfce7,stroke:#16a34a,color:#000
    class CF caps
    class AC,AR,Q audio
```

---

## 4. Ramo de audio PCM - Configuracao B

**B = S16LE / 8.000 Hz / 1 canal (mono).** Qualidade de telefone.

```mermaid
flowchart TD
    subgraph DEC["decodebin"]
        direction TB
        DMX["<b>matroskademux</b>"]
        VD["<b>vorbisdec</b><br/>descomprime Vorbis"]
        DMX -->|"audio/x-vorbis<br/>48000 Hz, 2 canais"| VD
    end

    VD -->|"audio/x-raw<br/>format = F32LE (float 32 bits)<br/>rate = 48000<br/>channels = 2"| AC["<b>audioconvert</b><br/>F32LE para S16LE<br/>downmix: 2 canais para 1"]
    AC -->|"audio/x-raw<br/>format = S16LE<br/>rate = 48000<br/>channels = 1"| AR["<b>audioresample</b><br/>48000 Hz para 8000 Hz"]
    AR -->|"audio/x-raw<br/>format = S16LE<br/>rate = 8000<br/>channels = 1"| CF["<b>capsfilter - Config B</b><br/>audio/x-raw<br/>format = S16LE<br/>rate = 8000<br/>channels = 1"]
    CF --> Q["<b>queue</b>"]
    Q -->|"pad audio_0"| MUX["<b>matroskamux</b><br/>grava PCM sem compressao"]

    classDef caps fill:#fee2e2,stroke:#dc2626,color:#000,stroke-width:2px
    classDef audio fill:#dcfce7,stroke:#16a34a,color:#000
    class CF caps
    class AC,AR,Q audio
```

**Negociacao de caps:** quem decide o formato final e o `capsfilter`. Ele anuncia o que aceita; `audioresample` so muda a taxa, entao repassa para tras a exigencia de formato e canais, e `audioconvert` faz essa parte. Cada conversor faz apenas o que lhe cabe.

---

## 5. Comparacao A x B

O que muda em cada caracteristica PCM e o efeito no audio.

```mermaid
flowchart LR
    ORIG["<b>Audio original</b><br/>Vorbis decodificado<br/>F32LE / 48000 Hz / 2 canais"]

    ORIG --> A
    ORIG --> B

    subgraph A["Configuracao A"]
        direction TB
        A1["<b>rate</b> = 44100 Hz<br/>Nyquist: ate 22050 Hz"]
        A2["<b>format</b> = S16LE<br/>16 bits: 65536 niveis, ~96 dB"]
        A3["<b>channels</b> = 2<br/>estereo"]
        A4["<b>Taxa de bits</b><br/>44100 x 16 x 2 = 1411 kbit/s<br/>52 s = ~9,2 MB de audio<br/>arquivo final: 19,6 MB"]
        A1 --- A2 --- A3 --- A4
    end

    subgraph B["Configuracao B"]
        direction TB
        B1["<b>rate</b> = 8000 Hz<br/>Nyquist: ate 4000 Hz"]
        B2["<b>format</b> = S16LE<br/>16 bits: 65536 niveis, ~96 dB"]
        B3["<b>channels</b> = 1<br/>mono"]
        B4["<b>Taxa de bits</b><br/>8000 x 16 x 1 = 128 kbit/s<br/>52 s = ~0,84 MB de audio<br/>arquivo final: 11,3 MB"]
        B1 --- B2 --- B3 --- B4
    end

    A --> RA["<b>Resultado A</b><br/>som cheio, com agudos<br/>espacialidade esquerda/direita"]
    B --> RB["<b>Resultado B</b><br/>som abafado de telefone<br/>sem agudos, tudo no centro"]

    classDef a fill:#dcfce7,stroke:#16a34a,color:#000
    classDef b fill:#fef3c7,stroke:#d97706,color:#000
    class A1,A2,A3,A4,RA a
    class B1,B2,B3,B4,RB b
```

| Caracteristica | A | B | Muda? |
|---|---|---|---|
| Taxa de amostragem (`rate`) | 44.100 Hz | 8.000 Hz | Sim |
| Formato / profundidade (`format`) | S16LE | S16LE | Nao |
| Canais (`channels`) | 2 | 1 | Sim |
| Taxa de bits do audio | 1.411 kbit/s | 128 kbit/s | 11x menor |

O video e identico nas duas saidas; a diferenca de tamanho dos arquivos (19,6 - 11,3 = 8,3 MB) corresponde a diferenca de audio calculada (9,2 - 0,84 = 8,4 MB).

---

## 6. Fluxo do programa e tratamento de erros

Sequencia executada pela funcao `main` em `src/principal.cpp`.

```mermaid
flowchart TD
    INICIO(["Inicio"]) --> ARGS["Le argumentos<br/>--video, --pcm A|B, --saida"]
    ARGS --> ARGOK{"Argumentos<br/>validos?"}
    ARGOK -- nao --> E1["Erro: argumento invalido<br/>ou PCM diferente de A/B"]
    ARGOK -- sim --> VID{"Video de entrada<br/>existe?"}
    VID -- nao --> E2["Erro: informe um video existente"]
    VID -- sim --> OUT{"Arquivo de saida<br/>ja existe?"}
    OUT -- sim --> E3["Erro: saida ja existe<br/>(nao sobrescreve)"]
    OUT -- nao --> INIT["gst_init<br/>carrega os plugins"]
    INIT --> PARSE["gst_parse_launch<br/>monta a pipeline com as caps A ou B"]
    PARSE --> POK{"Pipeline<br/>criada?"}
    POK -- nao --> E4["Erro ao criar pipeline<br/>(elemento ausente ou sintaxe)"]
    POK -- sim --> LOC["g_object_set location<br/>em filesrc e filesink"]
    LOC --> PLAY["gst_element_set_state<br/>PLAYING"]
    PLAY --> SOK{"Mudanca de estado<br/>falhou?"}
    SOK -- sim --> E5["Erro ao iniciar<br/>(plugins x264 / matroska)"]
    SOK -- nao --> BUS["gst_bus_timed_pop_filtered<br/>espera ERROR ou EOS"]
    BUS --> MSG{"Tipo da<br/>mensagem"}
    MSG -- ERROR --> E6["gst_message_parse_error<br/>mostra erro da pipeline"]
    MSG -- EOS --> OK["Concluido:<br/>arquivo .mkv fechado"]
    E5 --> NULL
    E6 --> NULL
    OK --> NULL["set_state NULL<br/>gst_object_unref<br/>libera recursos"]
    NULL --> FIM(["Fim<br/>retorna 0 ou 1"])
    E1 --> FIM
    E2 --> FIM
    E3 --> FIM
    E4 --> FIM

    classDef erro fill:#fee2e2,stroke:#dc2626,color:#000
    classDef ok fill:#dcfce7,stroke:#16a34a,color:#000
    class E1,E2,E3,E4,E5,E6 erro
    class OK ok
```

---

## 7. Estados da pipeline e mensagens do bus

```mermaid
stateDiagram-v2
    [*] --> NULL: gst_parse_launch
    NULL --> READY: set_state(PLAYING)
    READY --> PAUSED: abre arquivo, decodebin descobre os fluxos
    PAUSED --> PLAYING: caps negociadas, dados fluindo
    PLAYING --> NULL: EOS recebido no bus
    PLAYING --> NULL: ERROR recebido no bus
    NULL --> [*]: gst_object_unref

    note right of PLAYING
        Cada queue roda em sua thread.
        O programa fica bloqueado em
        gst_bus_timed_pop_filtered
        ate chegar ERROR ou EOS.
    end note

    note left of NULL
        No EOS o matroskamux grava
        indice e duracao do MKV
        antes de a pipeline parar.
    end note
```

Um unico `set_state(PLAYING)` faz o GStreamer passar automaticamente por READY e PAUSED.

---

## 8. Ligacao dos pads dinamicos do decodebin

O `decodebin` so sabe quais fluxos existem depois de ler o arquivo, entao seus pads de saida aparecem durante a execucao. O `gst_parse_launch` deixa as ligacoes `dec. ! ...` pendentes e as completa quando cada pad surge.

```mermaid
sequenceDiagram
    participant P as programa (main)
    participant DB as decodebin
    participant VC as videoconvert
    participant AC as audioconvert
    participant MUX as matroskamux

    P->>DB: set_state(PLAYING)
    DB->>DB: typefind detecta WebM
    DB->>DB: cria matroskademux, nvvp8dec, vorbisdec
    DB-->>VC: novo pad src_0 (video/x-raw)
    Note over DB,VC: so videoconvert aceita video: ligacao feita
    DB-->>AC: novo pad src_1 (audio/x-raw)
    Note over DB,AC: so audioconvert aceita audio: ligacao feita
    DB->>DB: no-more-pads
    loop ate o fim do arquivo
        VC->>MUX: quadros H.264 (via x264enc, h264parse, queue)
        AC->>MUX: amostras PCM (via audioresample, capsfilter, queue)
        MUX->>MUX: intercala por timestamp e grava no filesink
    end
    DB-->>MUX: EOS nos dois ramos
    MUX-->>P: mensagem EOS no bus
```
