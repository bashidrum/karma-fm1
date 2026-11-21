# PHEBUS UI research: what makes a groovebox fast, and what people hate

Research date: 20 Nov 2026. Sources are mainly Sound On Sound (SOS), MusicTech, MusicRadar, CDM, Synth Anatomy, Sonicstate and Elektronauts threads. I did not get usable Reddit, Gearspace or ModWiggler pages in this pass, so forum evidence leans on Elektronauts. Anything marked **(inference)** is my reading, not a source claim.

## 1. Devices praised for workflow, and why

**Elektron (Digitakt II, Syntakt, Digitone, Model:Cycles).** The core move is the parameter lock: hold a step and turn any knob, and that value belongs to that step only (SOS Digitakt II). Each step also gets separate slots for probability, fill and condition, which can be combined. Trigless trigs fire envelopes or modulation without a new note. Page Loop lets you loop chosen pages of a long pattern to park fills. Song mode now has its own top-level button. Song mode itself is a tracker-style list: each row names a pattern, a repeat count, a length, and which tracks are muted when that row starts; rows can be labelled (Intro, Fill) and you can turn the chain you are already playing into rows with one press (Perfect Circuit). Model:Cycles is praised because nearly every sound parameter has its own encoder, so the screen is only needed for presets and levels (SOS). Elektronauts users credit Elektron boxes with getting from idea to result quickly.

**Dirtywave M8.** Eight keys (four arrows plus Shift, Option, Edit, Play) do everything. Shift plus an arrow moves between screens, a mini map shows where you are, and Shift plus Right drills from Chain to Phrase to Instrument (SOS). The SOS reviewer finished about six full arrangements in two months and left the Cons box essentially empty. Song structure is a strict hierarchy: song rows point to chains, chains to 16-step phrases.

**Novation Circuit Tracks.** No screen. Sixteen scene pads sit on the Mixer page alongside levels and mutes; a scene stores the playing patterns, scenes chain into a song, and changes queue to the end of the loop. Pattern chains are made by pressing the first and last pattern together (SOS). Shift plus project recall jumps to a new project in time.

**Teenage Engineering EP-133 K.O. II.** Commit duplicates the current scene and switches you to the copy, which the SOS reviewer prefers for building variations. Punch-in FX are held with FX plus a key; while FX is held, the group buttons become momentary solos. Pattern and scene changes are instant and keep bar position. Scene numbers are slow to type, but +/- steps through neighbours fast.

**Teenage Engineering OP-XY and OP-Z.** OP-XY scenes store patterns, mutes and engine choices; song mode is a 32-slot grid where +/- queues the next scene for the end of the current one, and Shift plus a number recalls a scene (SOS). OP-Z works with no screen by colour-coding encoder pages on LEDs and using brightness as a value readout; its pattern switching is instant and keeps the beat (SOS). Both are criticised for memorised combos: the OP-Z reviewer kept looking up how to duplicate or clear patterns.

**Ableton Move.** Seven action buttons with one job each: Undo, Delete, Copy, Mute and so on, with no shift functions on them (CDM). A Capture button recovers what you just played even if you were not recording. Step buttons under the grid do TR-style entry and per-step locks.

**Polyend Tracker and Play.** Tracker has dedicated buttons that jump straight to pattern, song, sample and instrument screens, plus physical Copy, Paste and Delete that act on a selected block (SOS). Play lets you hold a pad and turn encoders to edit a step, and Shift selects a rectangle for copy or move; Perform mode turns the grid into held or latched FX that reset when you leave (SOS).

**Arturia MiniFreak** (detail in section 3). **1010music Blackbox:** song mode records up to 16 sections and captures mutes inside each, and you can still launch sequences over a playing song (SOS). **Synthstrom Deluge:** praised for its timeline arranger and lit grid, and one Elektronauts user calls it a groovebox without menu digging, but see the complaints below.

**Mixed or weaker:** Akai MPC One (song mode is a playlist that is hard to edit afterwards; duplicating a sequence is long-winded; cueing a sequence or muting means changing mode, per SOS). Korg Electribe 2 (velocity page about eight presses away, no song mode, accidental encoder grabs switch patterns, per SOS). Roland SP-404MKII (no step sequencing, still not straightforward, per SOS). Korg Volca Sample 2 (chain replaces song mode and cannot be saved, per SOS). Sonicware LIVEN Bass & Beats (see pitfalls). Roland T-8 offers dedicated per-instrument knobs, sub steps, probability, mute and fill (Roland), a TR-style "one control per job" layout.

## 2. Most common complaints, and who solved them

| Complaint | Where it shows up | Who solved it, and how |
|---|---|---|
| Menu diving | Very common on Elektronauts; Roland and Push named; Electribe 2 (SOS); Model:Cycles preset screen "fiddly" | Model:Cycles and MiniFreak: a knob per main function. Polyend Tracker: dedicated screen buttons. |
| Hidden shift combos you forget | Elektronauts menu thread (forgetting after a break; firmware adding unlabelled functions); OP-Z (SOS); Deluge (320-page manual, user cheat sheets) | Move: single-function buttons. M8: a tiny, consistent grammar (Shift plus arrow always means navigate). |
| No undo / lost work | Circuit Tracks has no undo (SOS); Elektronauts "No undo" thread (accidental reload loses a pattern); LIVEN discards unsaved locks on pattern change or power-off (SOS) | Move: dedicated Undo and Capture. Woovebox lists undo for sequencer and sampler. Groove OS autosaves. Elektron temp save and reload is liked by some users. |
| Song mode missing or clunky | Elektronauts song-mode threads (chains could not be saved; whole songs impossible); Volca chain unsaved; MPC song hard to edit; Electribe has none | Elektron rows with repeats, length and mutes; M8 chains; Blackbox sections with mutes; OP-XY queued scene grid. |
| Arrangement hard to see | Deluge arranger "hard to see a lot in one view" (Elektronauts); OP-XY does not keep song position on screen (SOS) | M8 song grid; Elektron row list with labels. |
| Unclear mode or state | Syntakt: buttons have two or more functions, unclear which mode you are in (MusicTech); LIVEN drum keys silently stop working (SOS); Deluge behaves differently per mode | Circuit and Electribe use pad colour as state. MiniFreak uses colour-coded LEDs to show strip mode. |
| Mutes not visible or slow | Deluge link groups invisible; Electribe red LEDs for Mute and Erase look alike, no solo; MPC mutes need a mode change | Circuit mutes and scenes on one page; EP-133 momentary solos while FX is held. |
| Parameter jumps | Classic pot problem | MiniFreak continuous rotaries, praised by MusicTech as an elegant fix. |
| Per-step editing fiddly | Electribe velocity; LIVEN steps cannot be toggled off; Deluge probability via hold and turn with no menu | Elektron and Play: hold step, turn knob, value shows on screen. |
| Copy/paste awkward | Electribe has no inter-pattern copy; MPC sequence duplicate is long | Tracker block select plus physical Copy/Paste; EP-133 Commit. |
| Accidental touches | Electribe encoder switches pattern; Play encoders switch modes (SOS) | **(inference)** Never let a bare knob turn change pattern or mode. |
| Sample/preset browsing | Digitakt II must load samples to a pool first; Model:Cycles preset list awkward (SOS) | MPC: touch an item and knobs jump to it. |

## 3. Why the MiniFreak flow feels good

- **Signal order on the panel.** Sections read Oscillators, Filter, Envelopes, LFOs, Mod matrix, FX, so the panel is the patch diagram (SOS).
- **Three knobs per oscillator engine.** Each of the 22 engines exposes three orange encoders plus pitch and level, so changing engine never changes where you reach (SOS).
- **Primary on the knob, secondary behind a press or Shift.** MusicTech singles out a layout that keeps every parameter within reach.
- **Mod matrix as a lit grid.** One encoder picks a cell and sets amount; active routings stay lit so you can see them at a glance (SOS; MusicRadar says it is easy to keep track of routings).
- **Macros.** Two touch strips each drive up to four targets, and their positions are saved with the preset (SOS, MusicRadar).
- **Screen as readout, not as the instrument.** The small display shows presets and the value you are turning and holds deeper settings (MusicTech).
- **Sequencer reuses the same 16 buttons** for steps, arp modes and live mutes; Spice and Dice vary a sequence without losing the original (SOS, MusicTech).
- **Weak spots:** FX sub-modes took the SOS reviewer weeks to find; custom LFO shapes are fiddly on hardware (MusicTech).

## 4. Small-screen and few-control lessons

- **M8:** Few keys, one grammar, a location map, strict hierarchy. Speed comes from muscle memory, not labels (SOS).
- **OP-Z:** Colour equals page, brightness equals value; but combos for duplicate and clear are forgettable (SOS).
- **Circuit:** No screen; a mixer page that doubles as the performance page; the "memory test" for deep settings is the cost (SOS).
- **Volca:** Hold step and turn knob for motion; but a global mode that needs a power cycle is a known annoyance (SOS).
- **Woovebox:** Undo in sequencer and sampler, chaining, song mode with per-fragment changes, stem export (Synthtopia).
- **FM-1 firmwares.** Groove OS uses white keys for drum hits and black keys for sound choice, a Perform page where white keys drop tracks in and out, black keys launch scenes and top keys fire punch-in FX, SHIFT+SEQ to roll variations, mute and solo on a mixer page, and autosave (Synth Anatomy). KARMA has 64-step tracks with ratchet, accent, slide, chance, fills and locks, song sections A to D with a bar count each, live section chaining, song recording, and a browser simulator and web editor (Sonicstate). I found no usable public notes on SLOOP.

## 5. Pattern and song management with few buttons

- **Elektron:** row list of pattern, repeats, length, mutes, labels; import the current chain in one press; loop or stop at end. Pitfall: normal pattern navigation drops you out of song mode (Perfect Circuit).
- **Circuit:** 16 scenes, queued at loop end, chained into a song (SOS).
- **OP-XY:** 32-slot scene grid, +/- queues the next scene, 10 songs per project (SOS).
- **M8:** song rows to chains to phrases; users call it usable and fertile because chains and phrases are reused and transposed (Elektronauts).
- **Blackbox:** sections with recorded mutes, improvise over the song (SOS).
- **MPC:** playlist easy to make, hard to edit (SOS).

**What users want from song mode (Elektronauts):** fast live arrangement, an editable overview of the whole song, reusable and varied sections with repeat counts, saved chains, and keeping probability and lock variation alive across the song rather than freezing it to audio. One user notes song mode is usually the last feature hardware makers add.

## 6. Ranked design rules for PHEBUS

1. **Every destructive action is undoable, with a dedicated undo gesture.** Evidence: Circuit has none (SOS); Elektronauts lose patterns to accidental reload. Best: Ableton Move.
2. **Never lose work silently: autosave, and no data dropped on pattern change or power-off.** Evidence: LIVEN discards locks (SOS). Best: Groove OS autosave.
3. **Add Capture: keep a rolling buffer of what was just played on the keys.** Best: Move.
4. **Parameter lock = hold step, turn knob, see the value on screen.** Best: Elektron; also Volca, Play, OP-Z.
5. **Screen layout mirrors the 4 knobs: four values in four fixed positions, same order on every page.** Evidence: Syntakt on-screen FX line up with its knobs (MusicTech). Best: Syntakt, MiniFreak three-knob engines.
6. **Sound pages follow signal flow: OSC, Filter, Env, FX, then Mod last.** Best: MiniFreak.
7. **Show the mod matrix as a visible grid with active routings lit.** Best: MiniFreak.
8. **Encoders are relative and pick up from the current value; no jumps.** Best: MiniFreak.
9. **One modifier grammar, used everywhere (Shift + X always means the same family).** Evidence: Deluge inconsistency; M8 consistency. Best: M8.
10. **Always show mode and location on screen (track, page, pattern, song row, rec state).** Evidence: Syntakt and LIVEN mode confusion. Best: M8 mini map.
11. **Show hints for held modifiers on screen** (what each key does while Shift is held). Evidence: Deluge essay proposes on-screen shortcut hints; forgotten combos thread. **(inference** on exact form).
12. **Mutes and solos visible at all times and one press away, including in song mode.** Best: Circuit mixer page; EP-133 momentary solos.
13. **Pattern changes queue to bar end by default, with a modifier for instant jump that keeps position.** Best: Circuit queue, OP-Z/EP-133 instant keep-position.
14. **Duplicate-and-switch in one press for variations.** Best: EP-133 Commit.
15. **Copy/paste of step, track and pattern with a select-range gesture.** Best: Polyend Tracker and Play.
16. **Song mode as a short row list: pattern, repeats, bars, mutes, label.** Best: Elektron; KARMA's A to D sections are a simpler start.
17. **Turn whatever chain is playing into a song with one action, and let song recording capture live moves.** Best: Elektron chain import; KARMA song record.
18. **Pattern navigation must not throw you out of song mode, and song position is always shown.** Evidence: Perfect Circuit (Elektron) and SOS (OP-XY) gaps. **(inference** as a fix).
19. **Keep variation alive in songs: probability and conditions still run per row.** Evidence: Elektronauts song thread.
20. **A Perform page on the 27 keys: track drops, scene launch, punch-in FX.** Best: Groove OS; EP-133.
21. **Variation tools never destroy the original.** Best: MiniFreak Spice and Dice.
22. **Bare knob turns never change pattern, mode or project.** Evidence: Electribe 2, Polyend Play accidents (SOS).
23. **No global modes that need a reboot or a computer to change.** Evidence: Volca Step Jump and chain order (SOS).

## 7. Top pitfalls to avoid

- Shipping without undo, or with an undo that clears more than the last action (LIVEN).
- Unsaved state that vanishes on pattern change or power-off (LIVEN, Elektron reload).
- Unlabelled double-function keys and shortcuts added in updates with no on-screen help (Syntakt, Deluge, Elektronauts).
- The same gesture doing different things in different modes (Deluge).
- Song mode as an afterthought: unsavable chains, uneditable playlists, or no overview (Volca, MPC, Elektronauts).
- Deep pages for common tasks such as velocity (Electribe 2) or long scroll lists for mod assignment (Digitakt II).
- Silent states: keys that stop responding with no indicator (LIVEN), indistinguishable LEDs (Electribe).
- Hidden features with no way to discover them (MiniFreak FX sub-modes).

## Sources

- SOS, Elektron Digitakt II: https://www.soundonsound.com/reviews/elektron-digitakt-ii?amp
- SOS, Elektron Model:Cycles: https://www.soundonsound.com/reviews/elektron-modelcycles
- MusicTech, Elektron Syntakt review: https://www.musictech.net/reviews/hardware-instruments/elektron-syntakt-review/
- Perfect Circuit, Elektron Song Mode update: https://www.perfectcircuit.com/signal/elektron-song-mode-update
- Gearnews, Elektron brings Song Mode: https://www.gearnews.com/?p=137041
- SOS, Dirtywave M8 Model:02: https://www.soundonsound.com/reviews/dirtywave-m8-model02
- SOS, Novation Circuit Tracks: https://www.soundonsound.com/reviews/novation-circuit-tracks?amp
- SOS, Teenage Engineering EP-133 K.O. II: https://www.soundonsound.com/reviews/teenage-engineering-ep-133-ko-ii?amp
- SOS, Teenage Engineering OP-XY: https://www.soundonsound.com/reviews/teenage-engineering-op-xy?amp
- SOS, Teenage Engineering OP-Z: https://www.soundonsound.com/reviews/teenage-engineering-op-z
- CDM, Ableton Move hands-on: https://cdm.link/ableton-move-hands-on/
- SOS, Polyend Tracker: https://www.soundonsound.com/reviews/polyend-tracker
- SOS, Polyend Play: https://www.soundonsound.com/reviews/polyend-play?amp
- SOS, Roland SP-404MKII: https://www.soundonsound.com/reviews/roland-sp-404-mkii
- Roland, T-8 features: https://www.roland.com/uk/products/t-8/features/
- SOS, Korg Electribe: https://www.soundonsound.com/reviews/korg-electribe
- SOS, Korg Volca Sample 2: https://www.soundonsound.com/reviews/korg-volca-sample-2
- SOS, Akai MPC One: https://www.soundonsound.com/reviews/akai-mpc-one
- SOS, 1010music Blackbox: https://www.soundonsound.com/reviews/1010music-blackbox
- SOS, Sonicware LIVEN Bass & Beats: https://www.soundonsound.com/reviews/sonicware-liven-bass-beats
- SOS, Arturia MiniFreak: https://www.soundonsound.com/reviews/arturia-minifreak
- MusicTech, The Big Review: Arturia MiniFreak: https://www.musictech.net/reviews/hardware-instruments/the-big-review-arturia-minifreak/
- MusicRadar, Arturia MiniFreak review: https://www.musicradar.com/reviews/arturia-minifreak
- Synthtopia, Woovebox: https://www.synthtopia.com/content/2023/09/04/the-woovebox-is-the-smallest-micro-music-workstation-on-the-planet/
- Synth Anatomy, Groove OS for the M-VAVE FM-1: https://synthanatomy.com/2026/10/groove-os-turns-the-m-vave-fm-1-into-an-8-track-groovebox.html
- Sonicstate, KARMA turns M-VAVE FM-1 into a groovebox: https://sonicstate.com/news/2026/11/02/karma-turns-m-vave-fm-1into-a-groovebox/
- Sean Luke, The Synthstrom Deluge Interface is a Hot Mess: https://people.cs.gmu.edu/~sean/projects/synth/deluge.html
- Elektronauts, Digging menu on a small screen: https://www.elektronauts.com/t/digging-menu-on-a-small-screen-is-a-workflow-like-using-computers/213150?page=2
- Elektronauts, Why do so few grooveboxes have thoughtful song workflows: https://www.elektronauts.com/t/why-do-so-few-grooveboxes-have-thoughtful-workflows-around-laying-out-a-whole-song/205599?page=3
- Elektronauts, What are the benefits of a song mode: https://www.elektronauts.com/t/what-are-the-benefits-of-a-song-mode/44461?page=3
- Elektronauts, No undo: https://elektronauts.com/t/no-undo/53681?page=3
