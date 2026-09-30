# Pesquisa e referências oficiais

Fontes consultadas em 30/09/2026. Os links abaixo fundamentam a escolha dos elementos; a topologia comparativa, os parâmetros, a instrumentação e os testes foram organizados para esta atividade. Não é necessário afirmar que o grupo escreveu o framework ou inventou os elementos: o trabalho é pesquisar, compor e justificar seu uso.

## Tutoriais recomendados no enunciado

| Fonte | Relação com esta solução |
|---|---|
| [Tutorial 1 — Hello World](https://gstreamer.freedesktop.org/documentation/tutorials/basic/hello-world.html) | Inicialização, montagem por `gst_parse_launch`, reprodução e liberação de recursos |
| [Tutorial 2 — GStreamer Concepts](https://gstreamer.freedesktop.org/documentation/tutorials/basic/concepts.html) | Elementos conectados, estado e comunicação de erros pelo bus |
| [Tutorial 3 — Dynamic Pipelines](https://gstreamer.freedesktop.org/documentation/tutorials/basic/dynamic-pipelines.html) | Pads que surgem durante a execução; fundamenta explicar por que esta fonte dispensa `pad-added` |
| [Tutorial 6 — Media Formats and Pad Capabilities](https://gstreamer.freedesktop.org/documentation/tutorials/basic/media-formats-and-pad-capabilities.html) | Caps, compatibilidade entre pads e leitura dos formatos negociados |
| [Tutorial 10 — GStreamer Tools](https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html) | Pesquisa com `gst-inspect`, experimentação e diagnóstico |

## Elementos selecionados

| Documentação | Decisão baseada na pesquisa |
|---|---|
| [videotestsrc](https://gstreamer.freedesktop.org/documentation/videotestsrc/index.html) | Usar padrão SMPTE, deslocamento horizontal e quantidade finita de buffers |
| [tee](https://gstreamer.freedesktop.org/documentation/coreelements/tee.html) | Separar o mesmo fluxo em dois ramos com filas |
| [queue](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html) | Separar threads por ramo e compreender limites do armazenamento |
| [videoscale](https://gstreamer.freedesktop.org/documentation/videoconvertscale/videoscale.html) | Alterar resolução por negociação de caps |
| [videorate](https://gstreamer.freedesktop.org/documentation/videorate/index.html) | Ajustar cadência por descarte/repetição, sem interpolação de movimento |
| [videoconvert](https://gstreamer.freedesktop.org/documentation/videoconvertscale/videoconvert.html) | Converter formatos de pixels e adaptar saída para o dispositivo |

## Ambiente

- [Instalação Linux](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html): pacotes de desenvolvimento, plugins e descoberta de bibliotecas por pkg-config.
- [Instalação Windows](https://gstreamer.freedesktop.org/documentation/installing/on-windows.html): necessidade de Runtime e Development compatíveis com compilador e arquitetura. Há seções antigas na página oficial; este projeto fornece seu próprio CMake para MSVC, sem depender do antigo wizard de Visual Studio.
- [Downloads oficiais](https://gstreamer.freedesktop.org/download/): distribuição do SDK/runtime.

## Decisões que o grupo deve defender

Escolhemos fonte sintética para repetibilidade, `tee` para referência comum, três transformações para cobrir conceitos distintos e exportação de quadros para comprovar a saída mesmo sem monitor. Não escolhemos decodificar um vídeo externo porque isso introduziria codec, demuxer e pads dinâmicos antes de o objetivo central estar demonstrado. Essas são escolhas do escopo acadêmico, não limitações gerais do GStreamer.
