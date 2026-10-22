#!/usr/bin/env python3
"""Synthesize Tapocalypse's sound effects into sounds/*.wav.

Every cue is generated from oscillators, envelopes and a seeded noise source, so the sounds are reproducible,
licence-free and need no downloads. Standard library only. Run it from anywhere:

    python3 tools/generate_sounds.py

The style is retro neon synth: soft-square and triangle voices on a C major pentatonic scale, a short space echo,
and pops whose arpeggio climbs higher and longer the bigger the cleared group is.
"""

import math
import random
import struct
import wave
from pathlib import Path

RATE = 22050
OUT_DIR = Path(__file__).resolve().parent.parent / "sounds"

# C major pentatonic, so any cue can overlap any other without clashing.
NOTE = {
    "C5": 523.25, "E5": 659.26, "G5": 783.99, "A5": 880.00,
    "C6": 1046.50, "D6": 1174.66, "E6": 1318.51, "G6": 1567.98, "C7": 2093.00,
}


def silence(seconds):
    return [0.0] * int(seconds * RATE)


def voice(freq, seconds, shape="square", attack=0.004, decay=None, vibrato=(0.0, 0.0)):
    """One note. freq may be a number or a function of time; decay is the exponential time constant."""
    count = int(seconds * RATE)
    decay = decay if decay is not None else seconds / 3
    vib_rate, vib_depth = vibrato
    out = []
    phase = 0.0
    for i in range(count):
        t = i / RATE
        f = freq(t) if callable(freq) else freq
        f *= 1.0 + vib_depth * math.sin(2 * math.pi * vib_rate * t)
        phase += 2 * math.pi * f / RATE
        if shape == "sine":
            s = math.sin(phase)
        elif shape == "triangle":
            s = 2 / math.pi * math.asin(math.sin(phase))
        else:  # soft square: bright like a chip voice, without the harsh aliasing of a hard edge
            s = math.tanh(3.5 * math.sin(phase))
        env = min(1.0, t / attack) * math.exp(-t / decay)
        release = min(1.0, (count - i) / (0.004 * RATE))
        out.append(s * env * release)
    return out


def noise_burst(seconds, seed, decay=0.01):
    rng = random.Random(seed)
    count = int(seconds * RATE)
    return [rng.uniform(-1, 1) * math.exp(-(i / RATE) / decay) for i in range(count)]


def sparkle(seconds, seed, voices=10):
    """Glittering high pings at random moments, used for big groups and the highscore."""
    rng = random.Random(seed)
    out = silence(seconds)
    for _ in range(voices):
        start = rng.uniform(0, seconds * 0.6)
        ping = voice(rng.choice([NOTE["C7"], NOTE["G6"], NOTE["E6"] * 2]), 0.12, "sine", decay=0.03)
        mix_into(out, ping, start, 0.25)
    return out


def mix_into(target, source, at_seconds, gain=1.0):
    offset = int(at_seconds * RATE)
    needed = offset + len(source)
    if needed > len(target):
        target.extend([0.0] * (needed - len(target)))
    for i, s in enumerate(source):
        target[offset + i] += s * gain


def sequence(notes, step, seconds, shape, gain=1.0, decay=None):
    out = []
    for index, name in enumerate(notes):
        mix_into(out, voice(NOTE[name], seconds, shape, decay=decay), index * step, gain)
    return out


def echo(samples, delay=0.09, feedback=0.3, taps=3):
    out = list(samples) + silence(delay * taps)
    for tap in range(1, taps + 1):
        mix_into(out, samples, delay * tap, feedback ** tap)
    return out


def lowpass(samples, cutoff):
    alpha = 1 - math.exp(-2 * math.pi * cutoff / RATE)
    out, state = [], 0.0
    for s in samples:
        state += alpha * (s - state)
        out.append(state)
    return out


def bloop(freq):
    """Bubble pop: a sine that springs up to its pitch."""
    return voice(lambda t: freq * (0.55 + 0.45 * min(1.0, t / 0.035)), 0.14, "sine", attack=0.002, decay=0.05)


def pop(tier):
    root = {1: "E5", 2: "G5", 3: "G5", 4: "C6"}[tier]
    tail = {1: [], 2: ["C6"], 3: ["C6", "E6", "G6"], 4: ["E6", "G6", "C7", "G6", "C7"]}[tier]
    out = bloop(NOTE[root])
    mix_into(out, sequence(tail, 0.045, 0.09, "triangle", decay=0.04), 0.05, 0.6)
    if tier == 4:
        mix_into(out, sparkle(0.5, seed=4), 0.08)
    out = echo(out, feedback=0.22 + 0.04 * tier)
    # The click goes on after the echo, so it is heard once instead of stuttering through every tap.
    mix_into(out, noise_burst(0.01, seed=tier), 0, 0.15)
    return out


def reject():
    """Cartoon "nuh-uh": two dull, falling soft-square bonks."""
    out = []
    for index, freq in enumerate([233.08, 174.61]):
        note = voice(lambda t, f=freq: f * (1 - 0.25 * t / 0.08), 0.08, "square", decay=0.035, vibrato=(28, 0.03))
        mix_into(out, note, index * 0.09)
    return lowpass(out, 1400)


def land():
    """Soft thud when falling or sliding tiles settle."""
    out = voice(lambda t: 150 - 95 * min(1.0, t / 0.06), 0.09, "sine", attack=0.001, decay=0.03)
    mix_into(out, lowpass(noise_burst(0.03, seed=7, decay=0.006), 900), 0, 0.4)
    return out


def deal():
    """Fresh board: a quick rising arpeggio."""
    return echo(sequence(["C5", "E5", "G5", "C6", "E6"], 0.05, 0.12, "triangle", decay=0.05), feedback=0.25)


def game_over():
    """Power-down: a wobbling slide to the floor, then a low thump."""
    sweep = voice(lambda t: 520 * math.exp(-t * 2.2) + 50, 0.95, "square", decay=0.5, vibrato=(7, 0.04))
    out = lowpass(sweep, 2200)
    mix_into(out, voice(70, 0.2, "sine", decay=0.07), 0.85, 0.9)
    return out


def highscore():
    """Little fanfare: pickup notes into a shimmering held chord."""
    out = sequence(["G5", "C6", "E6"], 0.09, 0.11, "square", gain=0.6, decay=0.06)
    for name in ["C6", "E6", "G6"]:
        mix_into(out, voice(NOTE[name], 0.7, "triangle", attack=0.01, decay=0.35, vibrato=(6, 0.006)), 0.27, 0.45)
    mix_into(out, sparkle(0.6, seed=11, voices=14), 0.27, 0.8)
    return echo(lowpass(out, 5000), feedback=0.25)


# Peak level per cue: frequent cues stay quiet, rare celebratory ones may be louder.
CUES = {
    "pop1": (lambda: pop(1), 0.55),
    "pop2": (lambda: pop(2), 0.6),
    "pop3": (lambda: pop(3), 0.65),
    "pop4": (lambda: pop(4), 0.7),
    "reject": (reject, 0.5),
    "land": (land, 0.35),
    "deal": (deal, 0.5),
    "gameover": (game_over, 0.6),
    "highscore": (highscore, 0.7),
}


def write_wav(path, samples, peak):
    loudest = max(1e-9, max(abs(s) for s in samples))
    frames = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s / loudest * peak)) * 32767)) for s in samples)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(RATE)
        handle.writeframes(frames)


def main():
    OUT_DIR.mkdir(exist_ok=True)
    for name, (build, peak) in CUES.items():
        samples = build()
        write_wav(OUT_DIR / f"{name}.wav", samples, peak)
        print(f"{name}.wav  {len(samples) / RATE:.2f} s")


if __name__ == "__main__":
    main()
