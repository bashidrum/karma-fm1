# PHEBUS direction review, 21 November 2026

**What was reviewed:** every feature, page, layer and gesture in PHEBUS against Ben's new direction (SPEC.md, 21 Nov 15:32): MiniFreak for the melodic voices, Analog Rytm MKII for drums, MPC for performance, and nothing else.
**Depth:** read-only. The code, the guide's gesture table, every screen rendered, and sourced research on the three devices. No long test runs.
**Overall state:** about half of PHEBUS already points the right way. The other half is either the old firmware's extras or the right job done the wrong way. The biggest gaps are not on the screen: they are in how drum steps and voice parameters are stored.
**Decisions waiting:** 24, put to Ben as pickers.

Nothing has been built for this direction. The previous build (branch `f-play`: step-record, HOME knobs, ACID bass, long stutters, LFO trigger, HOME band) is held unmerged.

Detail: `review/direction/INVENTORY.md` (every item, limits, routes), `VISUAL.md` (screens and mock-ups), `RESEARCH-devices.md` (how the three devices work, with sources). Yesterday's finished review is `review/REVIEW-2026-11-21-polish.md`.

---

## A. Keep, change, cut

### Already feels like its reference: KEEP
- **MiniFreak:** the OSC > FILT > ENV > FX > MOD strip in signal order; graphs that draw the wave, filter and envelope as you turn; presets by kind; plain INIT starts; the arpeggiator; key, chord and scale; chords on a step; step-record (held build), which is how the MiniFreak takes notes.
- **Rytm:** 16 keys as 16 steps; tap to place; hold a step for its detail; several steps at once; page slots; nudge; fill; undo; live record then refine.
- **MPC:** held effects on the keys; quarter, eighth, sixteenth and thirty-second loops; reverse, tape stop, half speed; filter sweeps and kills; note repeat (hold ARP); the mixer with mute and solo; echo and reverb throws.
- **Shared:** the colour rule, the bold knob row, autosave and the unsaved dot, MIDI and sync to the MPC, rescue and install, the simulator.

### Right job, wrong feel: CHANGE
| What | Today | What the reference does |
|---|---|---|
| Sound pages | Different knobs on every engine, 14 pages | MiniFreak: the same sections on every voice, four knobs each: Type + three macros; cutoff, resonance, envelope amount, type; A D S R; FX slot; matrix |
| Oscillator choice | Engines fixed to tracks | One Type list on every voice; the three macro knobs rename themselves per type |
| Filter | Plucked and alternate engines have a one-knob filter with no resonance | Cutoff, resonance, envelope amount and type on every voice |
| Filter envelope | One shared ADSR | The filter follows the amp envelope through one knob; "its own envelope" is a second envelope routed to cutoff in the matrix |
| Modulation | Fixed routes on two pages, one LFO | A visible grid of sources against destinations, last in the chain |
| FX | Sends, an insert, a chorus and a slicer on separate pages | Three slots, each Type, Time, Intensity, Amount |
| Parameter locks | Hold a step, press a button, then turn | Rytm: hold a step and turn whatever knob is on screen. No extra press |
| Locked values | Locked bright, the rest dim | Rytm: the locked value is drawn inverted |
| GRID knobs | Note, cursor, hit, velocity | Rytm: the knobs are always the sound page; step settings live on a TRIG page |
| Drum sound pages | "OSC FILT ENV FX MOD", and a placeholder on most kits | Rytm: SRC, FLTR, AMP, LFO for the selected sound, always shown |
| Drum steps | Level in four steps; condition and nudge shared by all sounds on a step | Rytm: velocity, condition, nudge and retrig per trig, per sound |
| Drum locks | A lock changes the whole drum mix | Rytm: a lock changes one sound |
| Retrig | Ratchet x1 to x4 | Rate, length and a velocity fade per trig |
| Drum mute | Hidden gesture, shown in red | A visible mute mode on the keys |
| Chromatic | None | Play the selected sound across the keys |
| Performance effects | 27 labels in a keyboard-shaped list, no latch shown | MPC: a pad map, latch, and knobs that tweak the held effect |
| SEQ and SAVE buttons | Cycle through several screens | One job each |

### Serves none of the three: CUT (each is a picker)
Song mode and the SONG screen; the SONGS bank; chains; the pattern generators (GEN, ACID GEN, HARMONY, DRUM GEN, PLAY); CLOUD and TEXTURE; the SLICER page; user sample kits; free take; the screen reader; most of the web editor; nine novelty punch effects; the hidden lock gesture.
Ben's own additions that are not one of the three, so his call: the characters, the snake view, the four screensavers, the HOME flow band.

### Unsure
- Whether sections A to D are "arranging" (cut) or the Rytm's pattern select (keep).
- Whether the stutter lane is un-MPC (stutters are played) or the thing the MPC cannot do. The research found the MPC Sample cannot record its pad effects into patterns.
- Whether ACID GEN is worth keeping as a MiniFreak-style "Dice".
- How much processor the real FM-1 has spare. Never measured.

### A bug found on the way
On the ACID engine, the envelope-to-filter and LFO-to-filter knobs do nothing. The engine never reads them. Verified in the code.

---

## B. Where the FM-1 cannot match, and the honest substitute

| Reference has | FM-1 has | Closest honest substitute |
|---|---|---|
| A knob per function (MiniFreak), eight per page (Rytm) | Four knobs | One section per page of four. For the Rytm's eight, draw all eight and light the live four |
| Velocity and pressure pads | Keys that are plain switches. Verified: no velocity in the hardware driver | OCT held for ghost and hard hits; velocity per trig by knob; play PHEBUS from the MPC's pads over MIDI, which works today |
| Pad pressure for effect depth | None | A knob while the key is held |
| Trig lights in colours | One colour per key, two brightness levels | Trig states by brightness on the keys, colour on the screen |
| Analogue filter | Digital models. ACID and JUNO are faithful ladder models; the other two engines have a plain filter | A resonant filter on every voice. On chords it filters the whole voice, not each note |
| Six-voice poly with a filter per note | Eight notes shared by three voices | Stated plainly as the limit |
| Full-quality stereo stutter | 0.74 s, mono, low resolution | Cutting CLOUD doubles the resolution for free; cutting the slicer lengthens it to about 1.1 s; long stutters replay the steps |
| Touch strips and macros pads | None | Macros on the knobs of a page |

Space: 23 KB of 581 KB is free today. The cuts on the table free roughly 90 to 100 KB (estimate, could be 30% either way).

---

## C. Edit or rebuild

**The finding that matters:** most of the distance to the Rytm and MiniFreak is not in the screens. It is in how things are stored: drum steps need per-sound velocity, conditions, retrig and locks; voices need a second envelope, a matrix, a filter and FX slots; and the saved project is full to the byte (verified: exactly 3,840 bytes, one flash object). That work is the same size whichever way the screens are done.

Sizes are in build waves, where one wave is roughly one of today's subagent builds.

| Route | Size | What it means | Lost |
|---|---|---|---|
| Keep editing | 20 to 28 waves | Every change fights the old firmware's hidden gestures. The result keeps its shape | Least up front |
| Rebuild the interface from blank | 28 to 36 waves | Clean, but rebuilds plumbing that works: lights, messages, menus, rescue screens | About 480 interface tests, the guide |
| **Hybrid (recommended)** | 22 to 28 waves | Keep the drawing helpers, header, knob row, strip, graphs, menus and rescue screens. Replace the page table with one uniform layout, replace the step editor with a Rytm-model one, replace screens one module at a time. Delete whole modules for the cuts | The tests and guide for replaced screens |

Why hybrid: the screens are not tangled into the sequencer or the engines (the sequencer calls no screen code), so they can be swapped one at a time with the simulator running at every step.

Lost on every route: projects saved by PHEBUS will no longer load on stock KARMA, because a new save format is unavoidable. Old projects can still be read.
Kept on every route: the engines, sequencer core, drivers, storage, MIDI and USB, install and rescue, the simulator and the build pipeline.

Not knowable from here: real processor headroom, real drawing speed, and whether one Type list on every voice fits the processor.

---

## D. Visual review

Sketches are in `review/direction/mock-*.png`.

**Reads as its reference already:** the signal-order strip; the wave, filter and envelope graphs; the full drum grid; the 4 by 4 kit tiles; the mixer; white and grey for shared effects.

**Does not, with the fix:**
1. **Knob values collide and change size** on several sound pages ("-34%-34%-34%33%"). Fixed columns, one size always, long values abbreviated.
2. **The oscillator Type is the smallest thing on its page.** Make it the largest, with the waveform beside it (`mock-synth-osc-A`).
3. **Four pages show empty boxes with a line.** Show the knob being touched, large, with a level meter, as the MiniFreak does (`mock-synth-osc-B`).
4. **FX is four pictures, not slots.** Three slot boxes (`mock-synth-fx-A`).
5. **No matrix.** A grid, dot size for amount (`mock-synth-mod-A`), or a list (`-B`).
6. **Locks are shown backwards.** Invert the locked value.
7. **Drum pages:** selected sound's name large, tabs SRC FLTR AMP LFO, a trig row with marks for locks and retrigs. Two layouts: eight dials in two rows (`mock-drum-A`) or four dials over the four knobs (`mock-drum-B`).
8. **Performance:** a 4 by 4 pad map with held and latched states (`mock-perf-A`), or a keyboard-true map (`-B`).
9. **Stutter lane:** show lengths as spans across the steps they cover, not numbers.
10. **HOME:** voice lanes plus the stutter lane with a playhead (`mock-home-A`), or three zones, one per pillar (`-B`).
11. **Shared list pages** still show the same four values twice.

**Identity:** the boot wordmark is the strongest piece and costs little. The characters work as heads in the strip and figures in the mixer; on sound pages they say which voice it is for a third time. That is Ben's call, not a finding.

---

## Clashes with earlier decisions

Each is asked in the pickers.
- Song insert and delete, built this morning, against "never needs to arrange".
- The 606 start kit against Rytm sound pages: the 606 cannot be edited today.
- The snake view, the four screensavers, the HOME flow band and the characters against "nothing else".
- JUNO on track 2 and fixed engines against "the same layout on every voice".
- The stutter lane and the HOME stutter knob against MPC-style played stutters.
- "EDIT walks the chain" for locks becomes unnecessary once hold-and-turn locks whatever is on screen.
- "Keep the one-knob pages, name them" against one uniform layout.
- CLOUD, moved to GLO this morning, against cutting it.

---

## Ben's decisions (21 Nov, evening, by picker)

| Decision | Choice |
|---|---|
| Route | Hybrid |
| Save format | New format accepted |
| Drum model | Per-sound trigs on one track |
| Drum sounds | 16 |
| Oscillator types | Same layout now, ACID and JUNO everywhere, the big list after a device check |
| Filter envelope | Its own ADSR page with a LINK switch |
| Mod matrix | Grid |
| Locks | Rytm way: hold a step and turn, shown inverted |
| Song and sections | A to D as patterns only; song, chains and songs bank cut |
| Generators | All cut; MiniFreak-style Spice and Dice added |
| Stutter lane | Kept; played effects record into it |
| CLOUD, TEXTURE, SLICER | CLOUD and TEXTURE cut; SLICER kept |
| Characters | Heads in the strip only |
| Snake view | Dropped |
| Screensavers and HOME band | Both kept |
| Sample kits, free take, screen reader, web editor editing | All cut (web editor keeps backup and install) |
| Drum pages | Four dials over the knobs |
| Drum kits | Every kit editable; 606 start kept |
| Performance | Pad map, trimmed set |
| HOME | Three zones, with cutoff, decay, stutter, level |
| OSC page | Type + three macros |
| Held build | Merged |
| Order | Cuts and save format, then drums, then melodic, then performance |

Taken as read, not asked: the visual fixes that follow from the new screens (one value size in fixed columns, the touched knob shown large on pages with no graph, stutter lengths drawn as spans, shared list pages showing values once).

Status: building. Wave 1 (the cuts) is done and merged. Wave 2 (drum hits and the new save format) is in progress. Ben asked on 22 Nov for all seven waves to be built.
