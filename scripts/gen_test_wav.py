#!/usr/bin/env python3
"""Generate small test WAVs with the standard library only (no numpy).

usage: scripts/gen_test_wav.py OUT_DIR
  kick120.wav   : 8 s, decaying 60 Hz sine "kick" every 0.5 s (120 BPM), silence after 6 s
  tone.wav      : 8 s, 220 Hz tone from 1 s to 5 s, silence elsewhere
  silence.wav   : 8 s of zeros (control)
All: 44100 Hz, 16-bit PCM, mono.
"""
import math, os, struct, sys, wave

SR = 44100

def write(path, samples):
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))

def kick(n_seconds=8.0, bpm=120, until=6.0):
    out = [0.0] * int(SR * n_seconds)
    period = 60.0 / bpm
    t = 0.0
    while t < until:
        start = int(t * SR)
        for i in range(int(0.25 * SR)):
            if start + i >= len(out): break
            tt = i / SR
            out[start + i] += 0.9 * math.exp(-tt * 14.0) * math.sin(2 * math.pi * 60.0 * tt)
        t += period
    return out

def tone(n_seconds=8.0, f=220.0, a=1.0, b=5.0):
    out = [0.0] * int(SR * n_seconds)
    for i in range(int(a * SR), int(b * SR)):
        out[i] = 0.6 * math.sin(2 * math.pi * f * i / SR)
    return out

if __name__ == "__main__":
    d = sys.argv[1] if len(sys.argv) > 1 else "experiments/audio"
    os.makedirs(d, exist_ok=True)
    write(os.path.join(d, "kick120.wav"), kick())
    write(os.path.join(d, "tone.wav"), tone())
    write(os.path.join(d, "silence.wav"), [0.0] * (SR * 8))
    print("wrote", d)
