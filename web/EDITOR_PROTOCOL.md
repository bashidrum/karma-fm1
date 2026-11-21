# Backup protocol (SysEx over USB-MIDI)

The firmware side is `firmware/src/editor.c`; the page is `web/editor.html` (the backup page, served at
`webapp/editor/`). KARMA's web editor edited sounds, steps, tracks, presets, samples and kits over this
protocol; those commands (2..24, 26..33, 37..42, 68..76) are cut. Their numbers stay reserved and the
device does not answer them, so an old editor finds no editing and changes nothing.

What is left: the device's identity (`INFO`), `PING`, the backup and restore of its data (`BK_LIST`,
`BK_GET`, `BK_PUT`) and the read-only load figures used by `tools/karma_*.py`.

## Framing

A request is `F0 7D 46 4C <cmd> <args...> F7`:

- `7D` is the non-commercial SysEx ID.
- `46 4C` is "FL" (the frames keep Felucca's header).

Every request the device knows gets exactly one reply, with the same header and the same `<cmd>`.
Requests it does not know get no reply. Every data byte is 7 bit.

| Item | Encoding |
| --- | --- |
| string | ASCII bytes, ended by a 0 byte |
| u35 | a number as 5 × 7 bit, LSB first |
| pack7 | groups of up to 7 bytes, each preceded by one byte holding their top bits (bit j = bit 7 of byte j) |

## Commands

| cmd | Request args | Reply args |
| --- | --- | --- |
| 1 INFO | — | version string ("FELUCCA KARMA x.y.z"), NENGINES, P_COUNT, G_COUNT, NSTEP, P_E0, then NENGINES engine-name strings, NTRK (4), then the protocol version (11) |
| 25 PING | — | 0 |
| 34 BK_LIST | — | rc (0 ok, 4 no flash), count, then per object: id, length u35, CRC-32 u35 (zlib). Takes a snapshot of the working project and the settings for GET |
| 35 BK_GET | id, offset u35, count (2 × 7 bit, 1..256) | id, rc (0 ok, 1 arguments, 5 the snapshot is gone: LIST again), offset u35, count, pack7 data |
| 36 BK_PUT | op 0 begin: id 0..10, length u35, CRC-32 u35 · op 1 data: id, offset u35, pack7 (≤ 256 bytes, in order) · op 2 commit: id · op 3 abort: id | op, id, rc: 0 ok, 1 arguments, 2 not a valid object (CRC, magic, sizes, ranges), 3 stop the sequencer first, 4 flash, 5 no begin for this object (or more than 15 s ago) |
| 90 STAT | — | the load and the USB, u35 numbers (`tools/karma_stat.py`) |
| 91 PCOST | — | KARMA_PCOST builds only: per DSP stage its time (us) and calls, then cleared (`tools/karma_pcost.py`) |
| 92 PVAR | [variant] | KARMA_PCOST builds only: which variant of a stage runs |
| 93 CRASH | — | the last crash's record (kept over resets until power-off), every word as u35 |
| 94 PROF | — | where the mix block's time goes: the blocks, then each part's summed and worst time since the last read (`tools/karma_prof.py`) |

## Backup objects

**0** the working project (`project_t`, as the autosave), **1** the settings (`persist_t`), **2..5** the
projects of sections A..D (length 0 = empty), **6..7** the user preset banks (`up_bank_t`, 16 records each;
0 = empty), **8** the FM6 user bank, **9** the SYN drum kits (`dsu_bank_t`, 1464 bytes, "DSU1"), **10** the
MIDI part (40 bytes: the MIDI LEARN map and GLO > MIDI's channels and BEND; BK_LIST names it only while it is
not the default).

A restore stages one object in RAM, checks it at the commit as a load checks it (projects: magic, size and
sum, older formats converted; banks: magic, record size, slot count; settings: magic, palette, a permutation
of the buttons and knobs) and writes it through the usual A/B commit; the working project is loaded at once.
Projects, banks, settings, kits and the MIDI part answer rc 3 while the sequencer plays (a flash erase
silences the audio for about 50 ms).

The page's file is JSON: `{format: "sloop-backup", version: 1, firmware, date, objects: [{id, len, crc, data
(base64)}]}`; it is checked (lengths, CRCs) before anything is written. A KARMA backup's sample slots (ids
32..35) are read and skipped: this firmware has no user samples.

## Notes

- **One request at a time.** Wait for the reply, about 10–50 ms, before sending the next. The device holds
  only one incoming SysEx frame.
- **Port.** The device's MIDI port is named "Felucca" (USB 1209:0001). Updates use the same port with
  other SysEx (the `F0 22 24 35 …` keys, `00 59 …` frames); the backup page never sends those.
- **Safety.** Only `BK_PUT` commits write flash, and only in the firmware's own storage; never the app or
  the update area.
