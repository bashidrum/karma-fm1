# PHEBUS against the new direction: inventory, limits, routes (subagent, 21 Nov 2026; saved by orchestrator, compacted)
Pillars: M MiniFreak melodic, D Rytm drums, P MPC performance, - none. Verdict KEEP / CHANGE / CUT. (?) = reviewer unsure.

## A. Inventory
### Global and navigation
KEEP: PLAY; REC tap (live record of trigs/notes); hold REC 2 s clear voice; HOME; hold HOME menu; tap ENV/LFO/FX/EDIT/ARP/SEL/GLO pages; layer + HOME lock; ALGORITHM voice select; PRESETS sound list; SELECT pages/tempo; OCT; OCT held + drum key ghost/hard (substitute for velocity); EDIT + OCT undo/redo; peek rule; message bar; header; chain strip; key LEDs.
CHANGE: tap SEQ (Rytm: trig keys always the sequencer, knobs always the sound page; SEQ only toggles pattern settings). Tap SAVE (with song cut: opens PRESETS/USER/TOOLS everywhere). Bottom strip (keep numbers and badges; heads depend on characters choice).
CUT: REC on empty project free take (~1.2 KB). HOME on HOME screensaver.
### Melodic sound pages
CHANGE: engine per track (T1 ACID/JUNO, T2 JUNO/ELEMENTS, T3 PLAITS; arenas T1 2.4 KB, T2 8.6 KB, T3 16.6 KB) > same Osc Type list on every voice. ACID OSC (WAVE ACC SLIDE DRV), JUNO OSC (WAVE PW SUB NOISE), ELEMENTS OSC/OSC 2 > Type + 3 macros + level. VOICE 2 (ALLOC DTUNE PAN MUTE) > pan/mute to mixer. FILTER pages > one FILTER page everywhere: CUT RES ENV(amt) TYPE; FILTER 2 merges. PLAITS/ELEMENTS have only a one-knob DJ LP/HP at fixed resonance: a resonant per-voice filter does not exist. ENV DEST (FLT PIT SHP ESYNC) > mod matrix; FLT amount on FILTER. FX (DIST CHORUS DELAY REVERB) > FX slots. CHORUS > a slot type (?). INSERT/INSERT 2 > FX slot 1. LFO DEST > mod matrix.
KEEP: PLAITS OSC layout (Type + HARM TIMB MRPH: closest already); VOICE; ENV (graph); ESYNC; LFO; LFO SYNC incl. f-play PATT; graphs; preset browser by kind; INIT presets, plain starts, effects off; f-play ACID INIT; SAVE > PRESETS/USER/TOOLS; Program Change.
CUT: SLICER per track (?: could be a pad FX; frees 32 KB RAM). Small dancing character on sound pages (?: Ben likes characters).
BUG FOUND: ACID ignores ENV and LFO to FLT (eng_acid.c never reads m->cutoff): those knobs do nothing on ACID.
REQUIRED, not built: filter envelope own ADSR + link.
### Melodic sequencing
KEEP: GRID 16 keys = steps, tiles, page slots; tap empty step sets (heard); tap set step clears; hold step detail; several steps together; pages/page add/follow; PRESETS/SELECT on GRID; ALGORITHM on GRID; nudge; f-play STEP REC and LIVE REC on GRID; live record quantise; SEQ layer (DIV SWING STEPS); undo; EDIT layer erase and SHIFT/STEPS/TRANSP utilities; ARP pages; SEL layer and pages (key, chord, scale); chords on a step.
CHANGE: hold step + knobs NOTE LEN VEL COND and SELECT right FLAG RATCH NUDGE > a TRIG page. Hold step + EDIT/ENV/LFO/FX then knob to lock > Rytm: whatever page is on the knobs, hold trig + turn = lock, no extra press. Nothing-held knobs NOTE STEP HIT VEL > knobs edit the current sound page. Conditions (-, FILL, NOFL, CHNC) > Rytm set (FILL, NOT FILL, PRE, NEI, 1ST, A:B, %). Ratchet x1-4 > retrig (rate, length, velocity curve). Locks 24 per track: tight. PATTERN page duplicates the SEQ layer: one place.
CUT: hold step + PRESETS/ALGORITHM lock gesture; PLAY page (DIR CHANCE JUMP STRIDE); GEN, GEN 2; ACID GEN, ACID GEN 2 (? could be a MiniFreak-style Dice); HARMONY.
### Drums: sound
CHANGE: one drum track with 16 lanes (nudge, condition, locks are per step for the whole drum track, not per lane; core.h) vs Rytm tracks each with own trigs/locks. Kit list (5 HEX + 32 synthesised + USR + MY HEX) > trim. 606 default: read-only synthesised kit with no sound pages (HOME DECAY shows "--"). HEX OSC/MOD/CLICK/MORE > SRC (two halves); HEX FILT > FLTR (no filter envelope today); HEX AEG > AMP (sends are on KIT; no per-sound pan). LFO on drums: none ("NO LFO ON DRUMS"; LXR's per-voice LFO exists upstream, not ported). KIT page > sends move to AMP, sound pick gets its own gesture. Drum black key (plays left white key) > chromatic or sound select. GLO > DRUMS > mixer/MIDI/AMP.
KEEP: copy built-in HEX kit to MY HEX on edit; closed hat chokes open; 6 drum voices.
CUT: user sample kits USR1-4, USR3+4 (frees 240 KB data flash + 1-2 KB code).
### Drums: sequencing
CUT: snake view (open). DRUM GEN, DRUM GEN 2.
KEEP: full grid up to 7 rows (? taste); fill (GLO + key 9 held, key 10 next bar); ARP roll on drums.
CHANGE: drum GRID knobs SOUND STEP HIT LEVEL > knobs = sound page, sound picked by key. Hold drum step knobs > TRIG page, per lane. Levels ghost/soft/norm/hard (2 bits per lane; dstep_t full at 10 bytes) > velocity. SEQ cycle GRID > KIT > DRUM GEN. Lane mute mode hidden in EDIT + black key > visible layer.
### Performance
CHANGE: FX held PUNCH (27 effects) > pad layout, latch, depth knob, straight 1/4 and 1/8 first. Key 5 STUTTER (beat/6). STUTTER lane (choice: keep / cut / keep and let played punch-ins record into it).
KEEP: LOOP 4/8/16/32; REVERSE STOP HALF; LOW/HIGH sweeps; CRUSH GATE ECHO; KILL LOW/MID/HIGH; FX held knobs master FILTER DUST DUCK TRK FILT; FX + OCT throws; ARP held ROLL (note repeat); GLO > MASTER; DLY, REV/CHO; mixer with mute/solo/fill/tap tempo (f-play pans); mixer latch; mute fades.
CUT: PHONE, ALIAS, WOBBLE (?); REWIND, OCT UP, FLANGER, AUTO-PAN, FREEZE, MONO, FADE OUT (most; keep REWIND/FREEZE at most) (?); LOOP 8T; CLOUD, CLOUD 2 (cutting it frees the ring's low byte so PUNCH can be 16-bit); TEXTURE, TEXTURE 2.
### Arranging and saving
CUT: chains; SAVE layer key 13 loop/song, 14 SONG REC, 16 SONG; SONG screen; SONGS bank (3 flash sectors); arranger.
CHANGE (choice): sections A-D play/store > keep as 4 Rytm-style patterns with queued switch, or cut. 4 project slots tied to that.
KEEP: working project autosave, restored at power-on; unsaved dot; writes only while stopped.
### Settings, MIDI, USB
KEEP: GLOBAL, SYSTEM (sync to the MPC matters), MIDI pages (OUT=SEQ lets the MPC record PHEBUS's parts; MPC pads can play PHEBUS with velocity), CC map, MIDI learn, menus SCREEN/LIGHTS/AUDIO, calibration, ABOUT, USB rescue, UBOOT, OTA, serial console, browser simulator.
CUT: screen reader (ui_speak + announce, ~4.6 KB).
CHANGE/CUT (?): web editor (editor.c ~16 KB device + 7.7k lines HTML; its contract is the P_/G_ ids, so new parameters need editor work): keep backup + install only.
### Visual extras
CUT (choice): characters (sprites 31 KB + scenes), character scenes, strip heads; screensavers (keep FLOW code for boot); f-play HOME FLOW strip.
KEEP: boot wordmark; colour rule; knob band.
CHANGE: HOME (drop section tiles if song is cut). f-play HOME knobs: choice.

## A12. Required and missing, with what could supply it
- Same page layout on every voice: a new uniform page table in params.c mapping each engine's E0-E7 to TYPE/MACRO 1-3 and FILTER slots; engines unchanged (PAGES and family ids are interface-only). Small-medium.
- One Osc Type list per voice: widen eng_allowed. JUNO and ACID fit every arena. PLAITS needs 16.6 KB per track (about +22 KB pool; pool 344 KB is full: delay 128, punch/cloud ring 64, canvas 59.5, slicer 32, arenas 27.6). Cutting SLICER pays. CPU risk with three PLAITS voices. Medium + device check.
- Resonant filter with TYPE on every voice: fx.c tsvf_coef(cut, res) gives a resonant LP/HP SVF; add per-track CUT/RES/TYPE/ENV; post-engine = paraphonic on poly voices. Medium.
- Filter envelope: second envelope in voice.c, +5 parameters. Medium.
- Mod matrix: today fixed ENV>FLT/PIT/SHP, LFO>PIT/FLT/SHP/AMP, one LFO; vmod_t carries pitch/cutoff/shape/amp only; ACID reads none. Matrix slots (8 x src/dst/amount about 24 B per track), LFO 2, per-engine plumbing for more destinations, fix ACID. Medium-large.
- FX slots per voice: INSERT (8 types) is a ready slot; a second instance costs CPU and about 4 KB RAM. Medium.
- Macro knobs: engine_t.macro[4] exists. Small.
- Rytm sound pages per drum sound: HEX has SRC/FLTR/AMP parameters (22 bytes per sound) edited via G_HX* mirrored to the selected sound; no filter envelope, pan or LFO. Synthesised kits have rich const dsnd_t fields but no pages: make editable by copy-on-write; port LXR's LFO. Medium.
- Hold trig + turn = lock a drum sound: locks are per track on P_* only (a drum lock changes the whole drum bus). New lock target (lane + sound parameter) in seq.c and the format; NLOCK 24 too small. Large.
- Per-trig velocity, conditions, nudge, retrig per lane: needs a bigger dstep or N drum tracks. Large.
- Drum retrig: new trig fields. Medium. FILL: gesture/naming only. Small. Chromatic drum mode: live small; recorded needs per-lane pitch (large). Per-drum mute layer: small. Pick a sound: hold a button + white key: small. Pad-like FX play: re-layout, latch per key, depth knob, 16-bit ring: small-medium.
- Parameters per track: P_COUNT 71; the project is exactly 3,840 B (one 4 KB flash object, full). Any new parameter needs a new project format over 2 sectors (freed song sectors and sample flash give room). Medium, once.

## B. Where the FM-1 cannot match
- Four knobs: pages of 4 in one fixed order; SELECT flips halves; draw all 8 Rytm parameters with the live 4 highlighted.
- No pads, no velocity, no pressure (key matrix is one switch per key). Substitutes: OCT held for ghost/hard; per-trig velocity by knob; MIDI velocity from the MPC's pads over MIDI IN (works today); hold FX + key + knob for depth. Key LEDs one colour with two dim levels: trig states by brightness, not colour.
- No analogue filter: ACID = Open303 diode-ladder model (close to a 303); JUNO = KR-106/IR3109 ladder model; PLAITS/ELEMENTS = only a 12 dB DJ SVF at fixed resonance; drums = LXR ZDF SVF on HEX. Substitute: per-voice resonant 12 dB SVF with drive; paraphonic on poly voices.
- Polyphony/CPU: NVOICE 8 shared by three synth parts; JUNO capped at 6; ACID mono; heavy PLAITS models mono; drums 6 voices. Host cost per mille vs idle+drums 1000: ACID ~1000, JUNO 1835-2083, PLAITS 937-2475, ELEMENTS 1102-1635; mixes 2476 to 3856. No device headroom ever measured.
- Stutter ring: 0.743 s, mono, 8-bit mu-law, shares words with CLOUD. Cut CLOUD = 16-bit at no RAM cost; cut SLICER = +32 KB (about 1.1 s); sequencer replay for long lengths. Stereo not affordable.
- Screen is bigger than either reference's: limit is legibility and redraw cost.
- Code flash 581,564 B; last device build 558,264 (23 KB left); f-play unmeasured on device. Data flash separate.
Approximate freed per cut (host x86-32 -Os; device may differ ±30%): characters 35-39 KB; web editor SysEx ~16 KB; snake/drum screen ~12 KB (partly replaced); screensavers ~8 KB (keeping ~3 KB FLOW); generators ~8 KB; song/sections/songs ~8-10 KB; screen reader ~4.6 KB; CLOUD + TEXTURE ~2-5 KB; SLICER ~1.6 KB + 32 KB RAM; free take ~1.2 KB; sample kits ~1-2 KB + 240 KB data flash. Total realistic ~90-100 KB.

## C. Edit or rebuild
Architecture: one translation unit. Engines/DSP, sequencer (seq.c 2,949 lines), drivers, storage (project.c formats FUN1-7), comms, and about 10,000 lines of interface (ui*.c, params.c page table, icons, sprites). seq.c calls no UI code; engines expose only page_title. PAGES and family ids are interface-only (not save format, MIDI map or editor contract: those use P_/G_ ids). The interface owns some model glue a rebuild must lift out (preset/engine selection, track defaults, HEX sound editing mirror, CLOUD/GEN/PLAY syncs, MIDI learn, Program Change). Tests: ui_pages_test (2,190 lines, 426 checks; 480 on f-play), speak, song_ui, studio_drums tied to the interface; seq2, project, punch, stutter, regress, engine tests reusable.
Dominant fact: most of the gap is in the data model, not the screens (per-lane drum locks/velocity/conditions/retrig; second envelope, mod matrix, filter on every voice, FX slots; the full project format). That work, about 12-16 agent-days, is the same on every route. (An agent-day = roughly one of today's build waves.)
- Route 1, keep editing: about 20-28 agent-days. Risk: hidden gestures, the 1,000-line input dispatcher, hold/layer regressions; keeps KARMA's shape.
- Route 2, rebuild the interface from blank: about 28-36. Risk: the long tail the old interface handles (LEDs, messages, autosave triggers, menu, calibration/rescue screens, MIDI learn). Loses about 480 interface checks, the guide.
- Route 3, hybrid (recommended): about 22-28. Keep drawing helpers, header, knob band, chain strip, graph functions, layer framework, menu, REC screen, recovery screens, LEDs. Replace the params.c page table with one uniform table; replace ui_grid with a Rytm-model trig editor; replace ui_op/ui_studio screens module by module. Delete ui_song, ui_speak, most of ui_vis, op_sprites, generators.
Lost on any route: save compatibility with stock KARMA (new format; old projects can be read through a converter); the web editor unless updated (recommend backup + install only); the current guide. Reusable unchanged: engines, DSP, drivers, storage layer, USB/MIDI, OTA/rescue, simulator, build pipeline, regress and engine tests.
Could not determine: device CPU headroom and draw speed; real device code sizes; whether three PLAITS voices run on the device.

## Reviewer's recommended answers
1 drum model: per-lane velocity/condition/nudge/retrig/locks on one drum track (half the cost of splitting into tracks; loses only per-sound pattern length). 2 16 sounds. 3 osc types: same layout now, JUNO and ACID allowed on every track, PLAITS everywhere after a device CPU check. 4 characters: cut all or heads only. 5 screensavers cut, keep boot. 6 snake cut. 7 sections: keep A-D as patterns, cut song and chains. 8 generators cut. 9 CLOUD cut (16-bit punch). 10 stutter lane: keep and let played punch-ins record into it. 11 HOME knobs keep for now. 12 make synthesised kits editable, keep 606. 13 web editor: backup + install only. 14 SLICER cut. 15 punch set 16-20 keys. 16 hybrid route. 17 accept new save format.

## Clashes with earlier decisions
1 SONG insert/delete (decision 10) vs no arranging. 2 sections/chains/SONG REC/songs. 3 save wording and HOME A-D tiles. 4 606 kit vs Rytm sound pages. 5 16 vs 12 sounds. 6 snake view (decision 5). 7 four screensavers (20) and f-play HOME FLOW strip. 8 characters (8, 13, 15, "I like them"). 9 ELEMENTS on track 2 / JUNO on track 2 vs same layout. 10 f-play HOME knobs vs macros/pads. 11 STUTTER lane vs played stutters. 12 CLOUD moved to GLO (9) vs cut. 13 GRID knobs NOTE/STEP/HIT/VEL vs "knobs are the sound page". 14 "EDIT walks the chain" for locks (3) redundant once hold trig + turn locks the shown page. 15 one-knob pages named (10:00) vs uniform layout. 16 generators. 17 filter envelope fits; ENV/LFO to FLT do nothing on ACID today.
