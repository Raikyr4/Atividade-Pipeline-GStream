# Histórico

## Importação de vídeo existente

- Fonte sintética substituída por `uridecodebin` e ligação dinâmica da faixa de vídeo.
- Resolução e cadência da referência preservadas; saída em 320 × 180, 10 FPS e GRAY8.
- Opções `--video`/`-Video` e `--inicio`/`-Inicio`, com limites por seek temporal.
- Vídeo Sintel incluído com atribuição; arrastar arquivo sobre `Iniciar.cmd` seleciona outra entrada.
- Testes de importação, caminhos Unicode, mídia inválida e comparação de pixels.
- Documentação, roteiro e evidências atualizados para arquivos reais.

## Legibilidade e comentários explicativos

- Separação visual entre declarações, validações, processamento e limpeza.
- Comentários detalhados nas funções C++, com dez etapas de execução em `main`.
- Explicação de caps, pads, PTS, EOS, stride, threads e liberação de referências.
- Scripts PowerShell e Python comentados, com etapas identificadas nos testes.

## Padronização da formatação

- Indentação de quatro espaços, blocos C++ e PowerShell expandidos e linhas longas organizadas.
- Configurações de formatação para VS Code, C++, Python e CMake.

## 1.1.0 — Execução Windows nativa

- Código incluído na pasta aberta no VS Code.
- Scripts PowerShell para GCC x64 e GStreamer instalado localmente.
- Compilação, integração e saída gráfica verificadas no Windows.
- Documentação e evidências atualizadas para os comandos locais.

## 1.0.0 — Projeto acadêmico

- Fonte sintética em C++ e pipeline GStreamer com comparação antes/depois.
- Alterações de resolução, FPS e formato de cor.
- Exportação de imagens e relatório, testes e documentação de apresentação.
