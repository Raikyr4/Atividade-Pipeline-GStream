"""Teste de integracao: executa C++ e verifica midia e caps reais, sem monitor."""

import json
import pathlib
import subprocess
import sys
import tempfile


def executar(*argumentos):
    """Executa o binario fornecido pelo CTest com limite de tempo."""
    return subprocess.run([sys.argv[1], *argumentos], capture_output=True, text=True, timeout=20)


with tempfile.TemporaryDirectory(prefix="pipeline-teste-") as pasta:
    resultado = executar("--sem-janela", "--segundos", "2", "--exportar", pasta)
    assert resultado.returncode == 0, resultado.stdout + resultado.stderr
    relatorio = json.loads((pathlib.Path(pasta) / "relatorio.json").read_text())
    for nome, largura, altura, formato, fps in [
        ("original", 640, 360, "RGB", 30),
        ("processado", 320, 180, "GRAY8", 10),
    ]:
        dados = relatorio[nome]
        assert (dados["largura"], dados["altura"], dados["formato"]) == (largura, altura, formato)
        assert dados["fps_numerador"] / dados["fps_denominador"] == fps
        assert abs(dados["fps_por_pts"] - fps) < 0.01, dados
        assert abs(dados["quadros"] - 2 * fps) <= 1, dados
    original = (pathlib.Path(pasta) / "original.ppm").read_bytes()
    processado = (pathlib.Path(pasta) / "processado.pgm").read_bytes()
    cabecalho_rgb = b"P6\n640 360\n255\n"
    cabecalho_cinza = b"P5\n320 180\n255\n"
    assert original.startswith(cabecalho_rgb)
    assert processado.startswith(cabecalho_cinza)
    rgb = original[len(cabecalho_rgb) :]
    cinza = processado[len(cabecalho_cinza) :]
    assert len(rgb) == 640 * 360 * 3
    assert len(cinza) == 320 * 180
    assert any(rgb[i] != rgb[i + 1] for i in range(0, len(rgb), 3)), "Original sem cor"
    assert len(set(cinza)) > 4, "Imagem cinza vazia/uniforme"
    repetido = executar("--sem-janela", "--segundos", "1", "--exportar", pasta)
    assert repetido.returncode != 0 and "existe" in repetido.stderr
    assert (pathlib.Path(pasta) / "original.ppm").read_bytes() == original

assert executar("--ajuda").returncode == 0
for argumentos in [
    ("--segundos", "0"),
    ("--segundos", "301"),
    ("--segundos", "2x"),
    ("--segundos", "abc"),
    ("--segundos",),
    ("--exportar",),
    ("--invalido",),
]:
    assert executar(*argumentos).returncode != 0, argumentos
print("PASSOU: caps, FPS por PTS, contagens, pixels, exportacao, protecao de saida e argumentos.")
