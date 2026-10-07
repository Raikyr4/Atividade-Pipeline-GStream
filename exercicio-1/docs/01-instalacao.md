# Instalação e execução nativa

## Preparação desta máquina

O GStreamer instalado é MinGW x64. O GCC global em `C:/MinGW/bin` é MinGW.org 6.3.0 de 32 bits. Bibliotecas e compilador precisam ter a mesma arquitetura. Por isso, foi extraído o GCC x64 portátil em `.ferramentas/w64devkit`; ele não substitui o GCC global.

Origem: [w64devkit 2.10.0 oficial](https://github.com/skeeto/w64devkit/releases/tag/v2.10.0). Para replicar, extraia o pacote x64 nesse diretório ou disponibilize um `g++` x64 com C++17 no PATH. Instale Runtime e Development do GStreamer MinGW x64, da mesma versão, incluindo plugins Base, Good e saída de vídeo. Python 3 é necessário para os testes e comparação HTML.

## Escolher um vídeo existente

Dê dois cliques em `Iniciar.cmd` para usar o trailer incluído. Arraste seu próprio arquivo sobre ele ou execute:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Video "C:/Videos/meu-video.mp4" -Inicio 0 -Segundos 30
```

Sem `-Video`, o script usa `midia/exemplo.webm` e pula os primeiros cinco segundos de abertura. Para um arquivo próprio, o início padrão é zero. Não há áudio na reprodução. O formato depende dos codecs instalados; a entrada precisa aceitar seek por tempo.

## Comandos no terminal do VS Code

Partindo da raiz atualmente aberta:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/compilar.ps1
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Segundos 30
powershell -ExecutionPolicy Bypass -File ./scripts/testar.ps1
```

`compilar.ps1` chama `g++`, integrante da família GCC, com C++17 e avisos habilitados. Usa os cabeçalhos do SDK e as import libraries `.dll.a` de GStreamer/GLib explicitamente. Isso evita que a biblioteca padrão C++ incluída no SDK seja confundida com a do compilador portátil. O código vincula `gstvideo`, `gstbase`, `gstreamer`, `gobject` e `glib`.

`executar.ps1` compila, prepara o PATH do processo e inicia o binário. `testar.ps1` compila e chama o teste Python contra o executável local. A opção `-ExecutionPolicy Bypass` vale apenas para essa chamada do PowerShell; não muda a política global.

## Caminhos alternativos

```powershell
$env:GSTREAMER_1_0_ROOT_MINGW_X86_64 = 'C:/caminho/do/gstreamer/mingw_x86_64'
./scripts/compilar.ps1 -Compilador 'C:/mingw64/bin/g++.exe'
```

O script rejeita compiladores cujo alvo não seja `x86_64`. Os scripts preferem o GCC portátil quando ele existe. O executável precisa das DLLs e plugins do GStreamer; distribuir somente o `.exe` não é suficiente em outra máquina.

## Opções de execução

| Opção do executável | Função |
|---|---|
| `--video ARQUIVO` | Arquivo local obrigatório no executável C++ |
| `--inicio N` | Posição inicial em segundos, entre 0 e 3600 |
| `--ajuda` | Mostra a sintaxe |
| `--segundos N` | Duração entre 1 e 300 segundos de mídia; padrão do binário: 15 |
| `--sem-janela` | Processa sem monitor, com `fakesink sync=false` |
| `--exportar PASTA` | Grava original.ppm, processado.pgm e relatorio.json |

O script PowerShell usa 30 segundos por padrão e oferece `-Video`, `-Inicio`, `-Segundos`, `-SemJanela` e `-Exportar`. Ao não aguardar o relógio, o modo sem janela pode processar dois segundos de mídia em menos de dois segundos reais. Os timestamps continuam representando o tempo de mídia.

## Inspeção de plugins

```powershell
& "$env:GSTREAMER_1_0_ROOT_MINGW_X86_64/bin/gst-inspect-1.0.exe" uridecodebin
& "$env:GSTREAMER_1_0_ROOT_MINGW_X86_64/bin/gst-inspect-1.0.exe" videorate
& "$env:GSTREAMER_1_0_ROOT_MINGW_X86_64/bin/gst-inspect-1.0.exe" videoconvert
```

Esses comandos mostram propriedades, pads e formatos. A aplicação chama a API GStreamer diretamente; ela não executa `gst-launch` externamente.

## Alternativa Linux

Na pasta do projeto multimídia:

```bash
sudo apt-get install g++ make cmake pkg-config python3 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-x
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/pipeline_multimidia --video midia/exemplo.webm --inicio 5 --segundos 30
```

A reprodução exige sessão gráfica. O caminho principal desta entrega é Windows nativo.
