# Experimentos e conclusao

## Teste das pipelines com gst-launch

Executado em 11/11/2026, Windows, GStreamer 1.26.11, na mesma maquina por `127.0.0.1:5004`. O receptor foi iniciado antes do transmissor. Foram enviados 24 buffers de audio gerado; uma `identity` adicional no receptor contou buffers de audio decodificado, e o `fakesink` consumiu os dados sem reproducao sonora. O receptor foi encerrado apos o transmissor. Os demais elementos e parametros correspondem as pipelines da aplicacao.

| Latencia do jitter buffer | Buffers decodificados | Saida do transmissor | Erros reportados |
|---|---:|---:|---|
| 50 ms | 24 | 0 | Nao |
| 200 ms | 24 | 0 | Nao |
| 500 ms | 24 | 0 | Nao |

Para repetir com Python 3 e o runtime do GStreamer no PATH: `python scripts/verificar_pipelines.py`. A contagem de buffers de audio nao e uma contagem de pacotes RTP nem uma medicao de qualidade audivel. Nao houve simulacao de perda ou jitter e nao foram coletadas as estatisticas da aplicacao C++ neste teste.

## Teste da aplicacao C++

Executado em 11/11/2026 com SDK e runtime GStreamer MSVC 1.26.11 e compilador portatil MinGW-w64. Receptor em `127.0.0.1:5004`, saida `fakesink`, duracao de 6 s; transmissor iniciado aproximadamente 1,5 s depois, por 3 s. Ambas as aplicacoes encerraram com codigo 0.

| Latencia | num-pushed | num-lost | num-late | num-duplicates | avg-jitter |
|---|---:|---:|---:|---:|---:|
| 50 ms | 23 | 0 | 0 | 0 | 3.903.878 ns |
| 200 ms | 23 | 0 | 0 | 0 | 3.295.829 ns |
| 500 ms | 23 | 0 | 0 | 0 | 2.408.048 ns |

O jitter e uma estimativa de cada execucao e pode variar; estes resultados nao provam que aumentar a latencia reduz o jitter da rede. A saida audivel ainda precisa ser conferida. Nao houve simulacao de perda ou atraso controlado.

## Conclusao

No teste local e silencioso das pipelines, as tres configuracoes encaminharam os 24 buffers enviados; no teste da aplicacao C++, cada configuracao encaminhou 23 pacotes sem perdas registradas. Isso confirma a passagem de audio pelas pipelines testadas, mas nao permite concluir qual latencia soa melhor ou como o receptor se comporta sob perda. Uma margem maior permite esperar mais por pacotes atrasados, ao custo de atraso adicional; o jitter buffer sozinho nao recupera dados que nunca chegaram.
