# Validação e diagnóstico

## Executar a integração nativa

```powershell
powershell -ExecutionPolicy Bypass -File ./scripts/testar.ps1
```

O script compila a aplicação e executa `testes/verificar.py`. O teste abre o arquivo Sintel incluído, mede o trecho de dois segundos iniciado em 5 s e confere:

- Entrada RGB 854 × 480 e cadência medida próxima de 24 FPS; caps nominais 0/1.
- Saída GRAY8 320 × 180, caps 10/1 e cadência medida próxima de 10 FPS.
- Aproximadamente 48 quadros originais e 20 processados, tolerando um quadro na fronteira.
- Cabeçalhos e tamanhos corretos dos pixels PPM/PGM, com cor e variação de intensidade.
- Rejeição de sobrescrita, de argumentos inválidos e de arquivo inexistente/corrompido.
- Importação por caminho com espaços e acentos.

A integração depende do arquivo incluído e dos codecs necessários para abri-lo. Não depende de internet ou monitor depois de preparado o ambiente. O vídeo é lido de disco, não gerado pelo teste.

## Ensaio manual

Execute `Iniciar.cmd`, compare as janelas e aguarde EOS. Depois teste arrastar seu próprio vídeo e confira o trecho escolhido. Verifique Ctrl+C no terminal. Para demonstrar duas ou mais mudanças, escolha entrada colorida com resolução maior que 320 × 180 e FPS maior que 10.

O teste sem janela verifica processamento e arquivos; não mede fluidez percebida ou compatibilidade de todos os drivers. Consulte [evidências atuais](07-evidencias.md).

## Problemas comuns

| Sintoma | Verificação |
|---|---|
| Não foi possível abrir o vídeo | Confira se o arquivo é válido, contém vídeo e se o decoder está instalado. |
| `no element` | Use gst-inspect para identificar o plugin ausente. |
| Arquivo não permite delimitar trecho | O formato precisa aceitar seek por tempo; use um arquivo local convencional. |
| Falha de medição após seek | O início pode estar além do fim do arquivo; experimente `-Inicio 0`. |
| Imagem exportada preta | O primeiro quadro do trecho pode ser preto; escolha `-Inicio` em uma cena visível. |
| FPS nominal 0/1 | Não significa quadro parado; confira `fps_por_pts`. |
| Saída já existe | Escolha outra pasta para preservar a exportação anterior. |
| Fechar janela gera erro | Alguns sinks tratam isso como falha; aguarde EOS ou use Ctrl+C. |
| DLL não encontrada | Abra pelo script, que prepara o PATH com o runtime GStreamer. |
| Vídeo próprio sem som | Esperado: esta aplicação seleciona somente a faixa de vídeo. |

## Logs

```powershell
$env:GST_DEBUG = '2'
powershell -ExecutionPolicy Bypass -File ./scripts/executar.ps1 -Video 'C:/Videos/exemplo.mp4'
Remove-Item Env:GST_DEBUG
```

## Limites

Seleciona a primeira faixa de vídeo descoberta. Não processa áudio, legendas, transmissão de rede ou alterações de formato durante o trecho. A reprodução exige o codec instalado e suporte a seek. A exportação captura o primeiro quadro de cada ramo e não garante pareamento adicional por PTS para todo arquivo possível.

O seek usa tempo de mídia; o relógio real tem um prazo de segurança separado. A preparação pode esperar até 15 segundos. Não há gravação de vídeo codificado nem coordenação entre processos exportando para a mesma pasta. Uma falha de disco pode deixar arquivos parciais e é sinalizada por erro.
