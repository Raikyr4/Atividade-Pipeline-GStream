# Passo a passo: uso do celular na atividade

O celular serve para duas coisas:

- **Hotspot:** cria uma rede propria quando o Wi-Fi da faculdade bloqueia a comunicacao entre aparelhos.
- **Receptor extra:** o VLC do celular toca o fluxo da Parte 1 e o da Parte 2.

O receptor C++ so roda em computador. A demonstracao principal da Parte 2 e entre o **PC da dupla** e o seu.

## Preparacao

1. Instale o **VLC** (Play Store ou App Store).
2. Desligue os dados moveis e a economia de bateria, e deixe a tela ligada.

## Hotspot (se o Wi-Fi bloquear)

1. Ligue o roteador Wi-Fi do celular e conecte **os dois PCs** nele.
2. No Windows, marque a rede como **Privada**.
3. Rode `ipconfig` em cada PC e anote o IPv4. O IP do proprio celular e o **Gateway Padrao** que aparece no `ipconfig`.
4. Se algum aparelho trocar de rede, rode `ipconfig` de novo, porque o IP muda.

## Celular como receptor

| Parte | No VLC do celular (Fluxo de rede) | No PC transmissor |
|---|---|---|
| 1 (video) | `udp://@:5000` | `./scripts/parte1-tutorial.ps1 -Destino IP_DO_CELULAR` |
| 2 (audio) | `rtp://@:5004` | `./scripts/executar.ps1 -Modo transmissor -Destino IP_DO_CELULAR -Fonte arquivo` |

Para saber o IP do celular: Configuracoes > Wi-Fi > rede conectada > Endereco IP. Se o celular for o hotspot, use o Gateway Padrao do `ipconfig` do PC.

O PCMU com payload 0 e um tipo estatico do RTP, entao o VLC toca sem precisar de arquivo SDP.

## Evidencias

- [ ] Print do VLC mostrando a bola (Parte 1)
- [ ] Video curto do celular tocando o audio (Parte 2, extra)
- [ ] Prints do receptor C++ no PC com latencia 50 ms e com 500 ms
- [ ] Se algo falhar, anote o erro em `docs/experimentos.md`
