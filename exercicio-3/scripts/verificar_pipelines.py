import subprocess, time, tempfile

falhou = False
for latency in (50, 200, 500):
    with tempfile.TemporaryFile(mode='w+t', encoding='utf-8', errors='replace') as output:
        rx = subprocess.Popen(['gst-launch-1.0.exe', '-v', 'udpsrc', 'port=5004', 'caps=application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)PCMU,payload=(int)0', '!', 'rtpjitterbuffer', f'latency={latency}', 'do-lost=true', '!', 'rtppcmudepay', '!', 'mulawdec', '!', 'audioconvert', '!', 'audioresample', '!', 'identity', 'silent=false', '!', 'fakesink', 'sync=true'], stdout=output, stderr=subprocess.STDOUT)
        time.sleep(1)
        tx = subprocess.run(['gst-launch-1.0.exe', '-q', 'audiotestsrc', 'is-live=true', 'num-buffers=24', 'wave=sine', 'freq=440', '!', 'audioconvert', '!', 'audioresample', '!', 'audio/x-raw,rate=8000,channels=1', '!', 'mulawenc', '!', 'rtppcmupay', 'pt=0', '!', 'udpsink', 'host=127.0.0.1', 'port=5004'], capture_output=True, text=True, timeout=15)
        time.sleep(1)
        rx.terminate(); rx.wait(timeout=5)
        output.seek(0); data = output.read()
        falhou |= tx.returncode != 0 or data.count("chain   ") != 24 or "PCMU" not in data or "ERROR" in data
        print(f'latencia={latency}ms transmissor_saida={tx.returncode} buffers_decodificados={data.count("chain   ")} caps_pcmu={"PCMU" in data} erro={"ERROR" in data}')

if falhou:
    raise SystemExit(1)
