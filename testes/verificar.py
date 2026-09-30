"""Teste de integracao: executa C++ e verifica midia e caps reais, sem monitor."""

import json
import pathlib
import subprocess
import sys
import tempfile
import shutil

VIDEO_EXEMPLO = pathlib.Path(__file__).resolve().parents[1] / "midia" / "exemplo.webm"


def executar(*argumentos):
    """Executa o programa C++ indicado no primeiro argumento deste script.

    Captura stdout e stderr para explicar falhas de integracao. O limite de
    20 segundos evita deixar o teste preso se a pipeline nao terminar.
    O retorno preserva o codigo de saida para testar sucesso e rejeicoes.
    """

    return subprocess.run(
        [sys.argv[1], "--video", str(VIDEO_EXEMPLO), "--inicio", "5", *argumentos],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=25,
    )


# 1. Uma pasta temporaria isola o teste das evidencias salvas pelo usuario.
# Ela e removida ao sair do bloco, inclusive quando alguma verificacao falha.
with tempfile.TemporaryDirectory(prefix="pipeline-teste-") as pasta:
    resultado = executar("--sem-janela", "--segundos", "2", "--exportar", pasta)
    assert resultado.returncode == 0, resultado.stdout + resultado.stderr

    # 2. Conferir caps e timestamps medidos, nao apenas valores configurados.
    relatorio = json.loads((pathlib.Path(pasta) / "relatorio.json").read_text())

    for nome, largura, altura, formato, fps in [
        ("original", 854, 480, "RGB", 24),
        ("processado", 320, 180, "GRAY8", 10),
    ]:
        dados = relatorio[nome]

        assert (dados["largura"], dados["altura"], dados["formato"]) == (largura, altura, formato)
        # Este WebM declara 0/1 na entrada; a cadencia real e obtida pelos PTS.
        fps_caps = dados["fps_numerador"] / dados["fps_denominador"]
        assert fps_caps == (0 if nome == "original" else 10), dados
        assert abs(dados["fps_por_pts"] - fps) < 0.1, dados
        # A fronteira de EOS pode acrescentar um quadro ao fechamento temporal.
        assert abs(dados["quadros"] - 2 * fps) <= 1, dados

    # 3. Validar os arquivos binarios: cabecalho e tamanho dos pixels ativos.
    original = (pathlib.Path(pasta) / "original.ppm").read_bytes()
    processado = (pathlib.Path(pasta) / "processado.pgm").read_bytes()

    cabecalho_rgb = b"P6\n854 480\n255\n"
    cabecalho_cinza = b"P5\n320 180\n255\n"

    assert original.startswith(cabecalho_rgb)
    assert processado.startswith(cabecalho_cinza)

    rgb = original[len(cabecalho_rgb) :]
    cinza = processado[len(cabecalho_cinza) :]

    assert len(rgb) == 854 * 480 * 3
    assert len(cinza) == 320 * 180
    # 4. Evitar um falso sucesso com imagens vazias ou sem variacao de conteudo.
    # R diferente de G em algum pixel demonstra que o original possui cor.
    assert any(rgb[i] != rgb[i + 1] for i in range(0, len(rgb), 3)), "Original sem cor"
    assert len(set(cinza)) > 4, "Imagem cinza vazia/uniforme"

    # 5. Uma segunda exportacao deve falhar sem sobrescrever a primeira.
    repetido = executar("--sem-janela", "--segundos", "1", "--exportar", pasta)
    assert repetido.returncode != 0 and "existe" in repetido.stderr
    assert (pathlib.Path(pasta) / "original.ppm").read_bytes() == original

    # Caminhos com espacos e acentos precisam atravessar Python, Windows e URI.
    caminho_unicode = pathlib.Path(pasta) / "vídeo de apresentação.webm"
    shutil.copyfile(VIDEO_EXEMPLO, caminho_unicode)
    unicode = executar("--video", str(caminho_unicode), "--sem-janela", "--segundos", "1")
    assert unicode.returncode == 0, unicode.stdout + unicode.stderr

    # Arquivo existente, mas sem midia valida: deve rejeitar, nao travar.
    invalido = pathlib.Path(pasta) / "invalido.webm"
    invalido.write_text("Este arquivo nao contem video.")
    assert executar("--video", str(invalido), "--sem-janela").returncode != 0

# 6. A ajuda deve funcionar; opcoes invalidas devem produzir codigo de erro.
assert executar("--ajuda").returncode == 0

for argumentos in [
    ("--segundos", "0"),
    ("--segundos", "301"),
    ("--segundos", "2x"),
    ("--segundos", "abc"),
    ("--segundos",),
    ("--exportar",),
    ("--invalido",),
    ("--video",),
    ("--video", "arquivo-inexistente.webm"),
    ("--inicio", "-1"),
    ("--inicio", "3601"),
    ("--inicio", "2x"),
]:
    assert executar(*argumentos).returncode != 0, argumentos

print(
    "PASSOU: importacao, caminhos Unicode, arquivo invalido, caps, FPS por PTS, contagens, pixels, exportacao, protecao de saida e argumentos."
)
