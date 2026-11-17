"""karma_pcost_watch.py [SECONDS] : the measuring build's stage times, read once a second without touching the sound (3.1.5).
Needs the measuring build (KARMA_PCOST=1 sh build.sh -> build/karma-pcost.fwsc) on the FM-1. Unlike karma_pcost.py
it sends no Program Change and no notes: play as you like. Each line: the note starts (events_block whole, each
synth track's note-ons, the drum hits, the note-offs, a Plaits model's set-up) as us that second / calls / longest
call. Needs mido + python-rtmidi (~/fm1-firmware/venv)."""
import sys
import time
import mido
from karma_pcost import STAGES, ask, port

SHOW = ["ev_all", "ev_n1", "ev_n2", "ev_n3", "ev_drum", "ev_off", "plx_init"]


def main():
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 30.0
    i = mido.open_input(port(mido.get_input_names()))
    o = mido.open_output(port(mido.get_output_names()))
    n = len(STAGES)
    ask(i, o, 91)                                       # (clear)
    t0 = time.time()
    while time.time() - t0 < secs:
        time.sleep(1.0)
        v = ask(i, o, 91)
        if not v or len(v) < 3 * n + 2:
            print(f"{time.time() - t0:6.1f}s no answer (is karma-pcost.fwsc installed?)", flush=True)
            continue
        us = {s: v[2 * k] for k, s in enumerate(STAGES)}
        calls = {s: v[2 * k + 1] for k, s in enumerate(STAGES)}
        mx = {s: v[2 * n + 2 + k] for k, s in enumerate(STAGES)}
        late = v[2 * n]
        parts = " ".join(f"{s}={us[s]}/{calls[s]}/{mx[s]}" for s in SHOW)
        print(f"{time.time() - t0:6.1f}s late={late} {parts}", flush=True)


if __name__ == "__main__":
    main()
