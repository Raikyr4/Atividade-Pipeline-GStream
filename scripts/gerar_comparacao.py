"""Gera HTML independente, com PNGs embutidos a partir dos quadros C++ exportados."""

import base64
import html
import json
from pathlib import Path
import struct
import sys
import zlib


def bloco(tipo, dados):
    """Monta um bloco PNG: tamanho, identificador, dados e verificacao CRC.

    Inteiros usam big-endian, como exige o formato PNG. O CRC protege o tipo
    e os dados do bloco; o tamanho descreve apenas o comprimento dos dados.
    Esta funcao empacota bytes, sem alterar os pixels produzidos pelo C++.
    """

    return (
        struct.pack(">I", len(dados)) + tipo + dados + struct.pack(">I", zlib.crc32(tipo + dados))
    )


def imagem_png(caminho):
    """Converte um quadro exportado pelo C++ para um PNG embutivel em HTML.

    Aceita somente o cabecalho simples emitido por SalvarImagem: P6 para RGB
    ou P5 para cinza, seguido de dimensoes, valor maximo 255 e pixels brutos.
    Nao e um leitor generico de todas as variantes de arquivos PPM/PGM.
    """

    # As tres primeiras quebras separam o cabecalho do conteudo binario.
    assinatura, dimensoes, maximo, pixels = caminho.read_bytes().split(b"\n", 3)
    largura, altura = map(int, dimensoes.split())

    if assinatura not in (b"P6", b"P5") or maximo != b"255":
        raise ValueError("Formato de imagem inesperado")
    # RGB possui tres componentes; GRAY8 possui apenas uma intensidade.
    canais = 3 if assinatura == b"P6" else 1
    passo = largura * canais

    if len(pixels) != passo * altura:
        raise ValueError("Quantidade de pixels invalida")
    # Cada linha PNG comeca com o tipo de filtro. Zero significa "sem filtro";
    # os bytes de pixels seguem intactos, antes da compressao sem perdas zlib.
    linhas = b"".join(b"\x00" + pixels[i * passo : (i + 1) * passo] for i in range(altura))

    # IHDR descreve a imagem; IDAT guarda os dados; IEND marca o fim do arquivo.
    # Tipos de cor PNG: 2 = RGB, 0 = cinza. Ambos usam amostras de 8 bits.
    cabecalho = struct.pack(">IIBBBBB", largura, altura, 8, 2 if canais == 3 else 0, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + bloco(b"IHDR", cabecalho)
        + bloco(b"IDAT", zlib.compress(linhas))
        + bloco(b"IEND", b"")
    )

    # Base64 permite incluir o arquivo na propria pagina, sem links externos.
    return base64.b64encode(png).decode("ascii")


def gerar(pasta):
    """Cria uma pagina offline com os dois primeiros quadros e as medicoes.

    A resolucao, o formato e o FPS vem do relatorio real da aplicacao C++.
    As imagens mostram cor e dimensoes; a pagina nao reproduz movimento.
    HTML e CSS ficam embutidos para facilitar enviar um unico arquivo.
    """

    # Carregar os resultados antes de montar os paineis evita valores inventados.
    relatorio = json.loads((pasta / "relatorio.json").read_text(encoding="utf-8"))
    paineis = []

    for chave, titulo, arquivo in [
        ("original", "Antes · original", "original.ppm"),
        ("processado", "Depois · processado", "processado.pgm"),
    ]:
        dados = relatorio[chave]

        # 0/1 pode indicar cadencia nao declarada pelo arquivo, nao video parado.
        fps_nominal = "FPS nominal não declarado"

        if dados["fps_numerador"] > 0 and dados["fps_denominador"] > 0:
            fps_nominal = f'{dados["fps_numerador"] / dados["fps_denominador"]:g} FPS'

        legenda = f'{dados["largura"]} × {dados["altura"]} · {dados["formato"]} · {fps_nominal}'

        # Escapar o texto evita interpreta-lo como marcacao HTML.
        paineis.append(
            f"            <article>\n"
            f"                <h2>{titulo}</h2>\n"
            f"                <p>{html.escape(legenda)}</p>\n"
            f'                <div class="imagem">\n'
            f'                    <img alt="{titulo}"\n'
            f'                         src="data:image/png;base64,{imagem_png(pasta / arquivo)}">\n'
            f"                </div>\n"
            f'                <p>{dados["quadros"]} quadros medidos · '
            f'{dados["fps_por_pts"]:g} FPS por PTS</p>\n'
            f"            </article>\n"
        )

    # Estrutura visual comum aos dois ramos. As imagens ja estao em paineis.
    pagina = """<!doctype html>
<html lang="pt-BR">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>GStreamer — comparação antes e depois</title>
    <style>
        body {
            font: 17px system-ui, sans-serif;
            background: #101820;
            color: #f4f5f6;
            margin: 0;
            padding: 32px;
        }

        main {
            max-width: 1200px;
            margin: auto;
        }

        h1 {
            font-size: 32px;
        }

        p {
            line-height: 1.6;
            color: #d5dce2;
        }

        .grade {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 24px;
        }

        article {
            background: #1b2935;
            padding: 20px;
            border-radius: 12px;
        }

        .imagem {
            min-height: 220px;
            display: flex;
            align-items: center;
            justify-content: center;
            background: #080d12;
        }

        img {
            max-width: 100%;
            height: auto;
        }

        pre {
            white-space: pre-wrap;
            font-size: 14px;
            background: #1b2935;
            padding: 20px;
        }

        @media (max-width: 800px) {
            .grade {
                grid-template-columns: 1fr;
            }

            body {
                padding: 16px;
            }
        }
    </style>
</head>
<body>
    <main>
        <p>ATIVIDADE PRÁTICA · C++ + GSTREAMER</p>
        <h1>Uma fonte, duas representações</h1>
        <p>Resolução, cadência temporal e cor transformadas na mesma pipeline.</p>
        <section class="grade">
"""

    pagina += "".join(paineis)

    pagina += (
        "        </section>\n"
        "        <p>\n"
        "            Imagens estáticas: primeiros quadros exportados pela aplicação.\n"
        "            A diferença de movimento deve ser observada nas duas janelas de reprodução;\n"
        "            os FPS acima foram medidos pelos timestamps dos buffers.\n"
        "            O navegador pode reduzir a imagem original para caber na tela.\n"
        "        </p>\n"
        "        <details>\n"
        "            <summary>Relatório completo medido</summary>\n"
        "            <pre>"
        + html.escape(json.dumps(relatorio, ensure_ascii=False, indent=2))
        + "</pre>\n"
        "        </details>\n"
        "        <p>Vídeo de exemplo incluído no projeto: Sintel, Blender Foundation, "
        '<a href="https://durian.blender.org/about/">CC BY 3.0</a>. '
        "O processamento altera resolução, cadência e cor.</p>\n"
        "    </main>\n"
        "</body>\n"
        "</html>\n"
    )

    # UTF-8 conserva os acentos, e write_text permite abrir a pagina offline.
    destino = pasta / "comparacao.html"
    destino.write_text(pagina, encoding="utf-8")
    print(destino.resolve())


if __name__ == "__main__":
    # O usuario fornece a pasta que contem os tres arquivos exportados pelo C++.
    if len(sys.argv) != 2:
        raise SystemExit("Uso: python scripts/gerar_comparacao.py PASTA_EXPORTADA")

    gerar(Path(sys.argv[1]))
