# Pipeline Multimídia — C++ + GStreamer

Projeto acadêmico nesta pasta do VS Code. Execução nativa no Windows, com o GStreamer instalado e GCC/MinGW-w64 x64.

## Executar

No PowerShell da raiz aberta no VS Code:

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1
```

O script compila e abre duas janelas por 30 segundos. A colorida é o original; a cinza é o processado. Organize-as lado a lado. Também pode usar **Terminal → Executar Tarefa → GStreamer: executar**.

| Característica | Original | Processado |
|---|---|---|
| Resolução | 640 × 360 | 320 × 180 |
| Taxa de quadros | 30 FPS | 10 FPS |
| Representação | RGB, colorido | GRAY8, cinza |

Uma fonte `videotestsrc` alimenta dois ramos por `tee`. `videoscale`, `videorate` e `videoconvert` transformam a mídia; `capsfilter` exige o formato final. A imagem se desloca horizontalmente para tornar a diferença de FPS perceptível.

## Testes e exportação

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/testar.ps1
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -SemJanela -Segundos 2 -Exportar ./saida-local
python ./scripts/gerar_comparacao.py ./saida-local
```

Abra `saida-local/comparacao.html`. Escolha outra pasta ao repetir: a aplicação protege os arquivos existentes. Python auxilia testes e visualização; a aplicação multimídia é C++.

## Ambiente local

GStreamer 1.28.6 MinGW x64 está em `C:/Program Files/gstreamer/1.0/mingw_x86_64`. O GCC global é 32 bits, incompatível com essas bibliotecas. Foi preparado GCC x64 portátil em `.ferramentas/w64devkit`, preservando a instalação global. Essa pasta é ignorada pelo Git; outra máquina precisa de um compilador x64 próprio ou do mesmo pacote portátil. Os scripts ajustam o PATH apenas no processo atual e compilam diretamente, sem exigir CMake no Windows.

## Documentação para apresentar

O código usa quatro espaços por nível de indentação. `.editorconfig` e as configurações
do VS Code mantêm esse padrão. O C++ segue `.clang-format`, com blocos expandidos e
chaves em linhas próprias; Python segue a configuração Black em `pyproject.toml`.

1. [Instalação e comandos](docs/01-instalacao.md)
2. [Arquitetura e justificativas](docs/02-pipeline.md)
3. [Conceitos e cálculos](docs/03-conceitos.md)
4. [Roteiro de apresentação e perguntas](docs/04-apresentacao.md)
5. [Testes e diagnóstico](docs/05-validacao.md)
6. [Referências oficiais](docs/06-referencias.md)
7. [Evidências nativas](docs/07-evidencias.md)

**Código:** `src/principal.cpp`. **Scripts:** `scripts/`. **Executável:** `build/pipeline_multimidia.exe`. **Testes:** `testes/verificar.py`.

O projeto atende à integração C++, fonte de mídia, processamento, saída funcional, três mudanças e comparação antes/depois. É independente dos serviços do restaurante.
