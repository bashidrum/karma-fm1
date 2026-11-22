# PHEBUS layout audit, 22 Nov 2026

Asked for by Ben after wave 7: is the way you interact with it consistent and obvious, measured against the MPC Sample's logic (everything is written on the knob or below it, reached by one shift press; the most hidden thing is a long press). Read-only. Nothing was changed. Sources: GUIDE.md sections 2 to 12 and the screen sheets in review/wave3 to wave7.

## Verdict

- Knobs: strong. Every screen names its four knobs above them with the value beside, in one size.
- Keys: weak. The keys have ten different jobs and several screens do not say which one is live.
- Buttons: mixed. Three buttons change scope or meaning depending on the voice.
- Uniqueness: mixed. The shared effects and the "repeat" family each live in several places.
- Depth: deeper than the MPC rule in about ten places (double presses, timed turns, three-control chords).

## 1. Knobs

Good:
- Four dials over four knobs on every screen, name above, value beside, touched knob inverted, locked value inverted.
- DECAY is KNOB 2 on synth ENV, drum AMP (synthesised kits) and HOME. LEVEL is KNOB 4 on HOME and drum AMP 2/2.
- TYPE is KNOB 1 on OSC and on every FX slot.

Not the same job in the same place:
1. Filter page order differs. Synth FILT: CUTOFF, RESO, ENV, TYPE. Drum FLTR: FREQ, RES, TYPE, DRIVE (HEX) or FREQ, RES, TYPE, ENV (synthesised). Same thing, different names, different order.
2. Tab names differ: FILT (synth) and FLTR (drum); ENV (synth) and AMP (drum); OSC and SRC. The SRC/FLTR/AMP/LFO names were Ben's own for the Rytm side, so this is a clash between two of his wishes, not an error.
3. Feedback is spelt FEEDBK on GLO DLY and FDBK on FX slots and PERFORM. Resonance is RESO and RES.
4. Pattern settings order: GUIDE and the wave 5 sheet say hold SEQ is GATE, DIV, SWING, STEPS. The wave 7 sheet "page-pattern" shows STEPS, DIV, SWING, GATE under a PATTERN title. To check: is that a second, leftover page?
5. STEPS is on hold SEQ, on hold EDIT (KNOB 2) and on the STUTTER lane (KNOB 4). SWING is on GLO GLOBAL and on hold SEQ.
6. Tempo is SELECT on HOME, the mixer and REC; KNOB 1 on GLO GLOBAL; tap on mixer key 16.

## 2. What the keys do

The 27 keys can be: notes, steps, drum hits, PERFORM pads, sound pick (hold SEL, drums), mutes (MUTE mode, and mixer keys 1 to 8), chromatic notes of one drum sound, erase targets (hold EDIT), roll or retrig (hold ARP), key and chord (hold SEL, synth), patterns (hold SAVE).

Screens that say what the keys are now: PERFORM (pad map), MUTE mode (grid), the mixer (key legend), hold SAVE (tiles), drum sound pick, CHROMATIC ("KEYS PLAY KICK").

Screens that do not: every synth sound page (keys are notes, nothing says so), the GRID (the tiles imply it), hold EDIT, hold ARP on a synth voice, hold SEL on a synth voice.

The black keys are the most hidden controls on the instrument: pages 1 to 4 on the GRID, retrig rates with ARP held, F#5 for FILL on the drum screen, F#3 for MIDI LEARN on the mixer, kills and triplets on PERFORM. Only PERFORM draws them.

## 3. Buttons that change meaning

1. FX. Tap: this voice's three FX slots and SLICER (voice colour). Hold: PERFORM, the whole mix (white). With a drum step held: lock the drum voice's bus pages. One button, three scopes.
2. SEL. Synth: tap for key and scale pages, hold for key and chord. Drums: tap for MUTE mode, hold to pick a sound.
3. ARP. Synth: arpeggiator pages, hold for roll. Drums: tap for CHROMATIC mode, hold for RETRIG.
4. EDIT, ENV, LFO map to OSC, ENV, MOD on synths and SRC, AMP, LFO on drums. Close, but ENV opening AMP is a rename.
5. SEQ. Tap: GRID. Tap again: TRIG tab. Tap again: back. Hold: pattern settings.

## 4. Deeper than "one long press"

1. Double press: F#5 twice latches FILL. HOME twice is a screensaver. SEQ twice is TRIG.
2. Timed double turn (two clicks inside 1.5 s): LOAD, SAVE, NEW, PADS clear, replace the edited kit.
3. Two ways to keep a layer open: hold the button and tap HOME (all layers), or hold 0.7 s untouched and let go (GLO and FX only).
4. Three timing thresholds: tap under 0.45 s, layer after 0.14 s, stay-open after 0.7 s.
5. Chords of three controls: hold a step + tap a page button + turn a knob (a lock on another page); hold a step + FX + knob (drum bus lock); hold OCT + drum key (ghost or hard hit); EDIT + OCT (undo, redo); EDIT + black key.
6. Two latch gestures: OCT+ with a pad (PERFORM), double press (FILL).
7. Menu: hold HOME, then SELECT, knobs, OCT+ to open, OCT- to close. Used nowhere else.

## 5. Things that live in more than one place

1. Delay time and feedback, reverb size and damp, chorus rate and depth: on GLO DLY and GLO REV/CHO; again as KNOB 2 and 3 of any voice's DELAY, REVERB or CHORUS slot (up to three voices); again on PERFORM's ECHO and VERB pads. One setting, up to five doors.
2. Six modulation routings: on the FILT page (ENV, KEY) and LFO pages, and as cells of the matrix.
3. Every sound page exists twice: as a sound page (keys play notes) and as a GRID tab (keys are steps).
4. Mute: mixer keys 1 to 4 (voices), drum MUTE mode (sounds), PERFORM kills (-DRM, -BAS).
5. Fill: mixer keys 9 and 10, and F#5 on the drum screen.
6. Level: HOME KNOB 4, the mixer, GLO DRUMS LVL, each drum sound's LEVEL.
7. Roll rate: GLO MASTER ROLL and hold ARP KNOB 1.
8. Filters: voice FILT; drum sound FLTR; the drum voice's own FILTER; GLO MASTER FILT (DJ filter); a hidden per-track DJ filter still on MIDI CC 74; PERFORM LP and HP. Six filters, four of them whole-mix or whole-voice sweeps.
9. The repeat family, five names for one idea: RATCH (synth step), RETRIG (drum hit and live), ROLL (hold ARP on synths), SLICER (a page on every voice's FX chain), STUTTER (PERFORM pads and the lane).
10. Dirt: FX slot drives and CRUSH; PERFORM CRUSH; drum sound DRIVE, DIST, CRUSH; GLO MASTER DUST.

## 6. Ben's FX thought, explored

His hunch: each voice's FX sends belong on that voice's own edit pages, and the FX button should mean master FX only.

Today: FX tap is per voice, FX hold is whole mix, and the master effects' settings are under GLO as well as inside the slots.

What his logic would give:
- Voice chain stays OSC > FILT > ENV > FX > MOD, reached by EDIT. The voice FX stage becomes: insert slots that are truly this voice's (phaser, flanger, drives, crush, EQ) plus one fixed SENDS page with three knobs: DELAY, REVERB, CHORUS. No shared knob ever appears on a voice page.
- The FX button is the whole mix, always white. Tap: the master pages DELAY, REVERB/CHORUS, MASTER (dust, duck, filter). Hold: PERFORM.
- GLO loses its effect pages and is settings and the mixer only.
- Result: delay time lives in exactly one place. FX never means "this voice". Nothing shared is drawn in a voice colour or needs the grey exception.

What it costs: the MiniFreak puts reverb and delay in the three slots, so this is one step away from the MiniFreak and one step towards the MPC. Sends stop taking up slots, so a voice gets three real inserts and all three sends at once (more, not less). Old projects convert cleanly, since sends are already separate values underneath.

## 7. Other changes the same logic suggests

A. A key line on every screen: one short white line saying what the keys are now (KEYS: NOTES, STEPS, PADS, SOUNDS, MUTES, ERASE) and, where black keys have jobs, what they are.
B. One latch gesture everywhere (OCT+ with the thing held), replacing the FILL double press.
C. One way to keep a layer open, for every layer.
D. Same filter page on drums and synths: CUTOFF, RESO, ENV, TYPE, with DRIVE on the second half. One spelling: CUTOFF, RESO, FDBK.
E. One word for repeats where they are the same thing (RATCH and RETRIG), and a decision on whether the per-voice SLICER still earns its page now that PERFORM exists.
F. Tempo in one place, shown on every screen as it is now.
G. Remove the hidden CC 74 filter and the GLO MASTER DJ filter in favour of PERFORM's LP and HP, or keep one master filter only.
H. Replace timed double turns with one confirm gesture used everywhere.

None of this is built. Each is a choice for Ben.
