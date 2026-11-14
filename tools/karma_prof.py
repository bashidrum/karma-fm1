#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""karma_prof.py [SECONDS] : where the FM-1's audio time goes while it plays (KARMA 3.1.3), once a second.
Editor command 94 (any build): mix_block's parts (fx.c PF_*): the average and the worst block, in us (a 32-frame block
has ~725 us). With the measuring build (KARMA_PCOST=1 sh build.sh -> build/karma-pcost.fwsc), command 91 too: the HEX
drum voices and the CLOUD by stage, as % of one core. Read only. Needs mido + python-rtmidi (~/fm1-firmware/venv)."""
import sys
import time
import mido

PARTS = ["events", "t1", "t2", "t3", "drums", "buses", "cloud", "out"]
STAGES = ["setup", "bank", "bowloop", "bownoise", "blow", "upsample", "out", "omin", "plaits", "plaits_out", "prism",
          "om_osc", "om_iir", "om_fir", "om_rest", "plx_init", "init_max",
          "hx_set", "hx_drum", "hx_snare", "hx_cym", "hx_dec", "cl_rec", "cl_play"]
SHOW = ["setup", "bank", "upsample", "plaits", "hx_drum", "hx_cym", "cl_play"]


def port(names):
    for n in names:
        if "KARMA" in n or "FM-1" in n or "Felucca" in n:
            return n
    raise SystemExit("no FM-1 MIDI port")


def ask(i, o, cmd):
    for _ in i.iter_pending():
        pass
    o.send(mido.Message("sysex", data=[0x7D, 0x46, 0x4C, cmd]))
    end = time.time() + 1.0
    while time.time() < end:
        for m in i.iter_pending():
            if m.type == "sysex" and len(m.data) > 4 and list(m.data[:4]) == [0x7D, 0x46, 0x4C, cmd]:
                b = list(m.data[4:])
                return [sum(b[k + j] << (7 * j) for j in range(5)) for k in range(0, len(b) - 4, 5)]
        time.sleep(0.005)
    return None


def main():
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 30.0
    i = mido.open_input(port(mido.get_input_names()))
    o = mido.open_output(port(mido.get_output_names()))
    ask(i, o, 94)
    pc = ask(i, o, 91)
    measuring = bool(pc) and len(pc) >= 2 * len(STAGES)
    t0 = time.time()
    last = t0
    while time.time() - t0 < secs:
        time.sleep(1.0)
        v = ask(i, o, 94)
        now = time.time()
        if not v or len(v) < 1 + 2 * len(PARTS):
            print(f"{now - t0:5.1f}s no answer to 94 (KARMA 3.1.3 or later?)", flush=True)
            continue
        blocks = max(1, v[0])
        avg = " ".join(f"{p}={v[1 + 2 * k] // blocks}/{v[2 + 2 * k]}" for k, p in enumerate(PARTS))
        line = f"{now - t0:5.1f}s blocks={v[0]} us avg/worst: {avg}"
        x = 1 + 2 * len(PARTS)
        if len(v) >= x + 5 + 1 + len(PARTS):          # (3.1.3) each one's longest block; the longest block's parts
            cm = v[x:x + 5]
            line += f" | longest: trk1={cm[0]} trk2={cm[1]} trk3={cm[2]} drums={cm[3]} cpu1own={cm[4]}"
            wb = v[x + 5]
            parts = " ".join(f"{p}={v[x + 6 + k]}" for k, p in enumerate(PARTS) if v[x + 6 + k])
            line += f" | worst block {wb}: {parts}"
        if measuring:
            c = ask(i, o, 91)
            if c and len(c) >= 2 * len(STAGES):
                dt = now - last
                line += " | % of a core: " + " ".join(
                    f"{s}={c[2 * STAGES.index(s)] / (dt * 10000.0):.1f}" for s in SHOW)
                mx = c[2 * len(STAGES) + 2:]
                if len(mx) >= len(STAGES):              # (3.1.3) each stage's longest call
                    line += " | longest us: " + " ".join(f"{s}={mx[k]}" for k, s in enumerate(STAGES) if mx[k])
        last = now
        print(line, flush=True)


if __name__ == "__main__":
    main()
