# PHEBUS: the manual

PHEBUS is firmware for the M-VAVE FM-1 groovebox. It turns the FM-1 into three instruments in one box: melodic voices that work like the Arturia MiniFreak, drums that work like the Elektron Analog Rytm MKII, and effect pads that work like the Akai MPC. It is built to sit next to an MPC Sample, which does the sampling, arranging and recording.

> **Read this first.** PHEBUS is unreleased, personal firmware. **Nothing in it has been run on a real FM-1 yet.** Everything in this manual was written from the firmware's source code and checked against it and its host tests. **Do not flash PHEBUS until your FM-1 has arrived and its chip revision has been checked:** newer FM-1 units have a changed chip and can be bricked by custom firmware. See [Known limits](#13-known-limits) and [When the FM-1 arrives](#14-when-the-fm-1-arrives).

PHEBUS is Ben's fork of KARMA by Ariel Muchnik, itself a fork of SLOOP by isod89, based on Felucca by Leo Kuroshita (Hügelton Instruments). It is free software under GPL-3.0-only. The full credits are in [section 15](#15-credits-and-licences).

---

## Contents

1. [What PHEBUS is](#1-what-phebus-is)
2. [The controls and the screen](#2-the-controls-and-the-screen)
3. [A first session, ten minutes](#3-a-first-session-ten-minutes)
4. [SYNTH: the melodic voices](#4-synth-the-melodic-voices)
5. [DRUM: the drums](#5-drum-the-drums)
6. [PERFORM: the pads](#6-perform-the-pads)
7. [The STUTTER lane](#7-the-stutter-lane)
8. [Patterns A to D, recording and saving](#8-patterns-a-to-d-recording-and-saving)
9. [FX, GLO and the mixer](#9-fx-glo-and-the-mixer)
10. [Settings, MIDI and USB](#10-settings-midi-and-usb)
11. [Backup and install](#11-backup-and-install)
12. [Every gesture](#12-every-gesture)
13. [Known limits](#13-known-limits)
14. [When the FM-1 arrives](#14-when-the-fm-1-arrives)
15. [Credits and licences](#15-credits-and-licences)
16. [Glossary](#16-glossary)

---

## 1. What PHEBUS is

PHEBUS has four **voices**: three melodic voices and one drum voice. Each voice has its own sequencer part of up to 64 steps. On top of the four voices sit the **PERFORM** pads, effects you play on the keys like an MPC's pads, and the **STUTTER lane**, a sequencer part of the whole mix that plays stutters and the pads you recorded (FX's last category, STUT; it is not a voice).

| Voice | Head in the strip | Colour | Starts as |
| --- | --- | --- | --- |
| **1** | robot | purple | ACID INIT: a plain saw through the 303 filter, mono, an octave down. A bass |
| **2** | alien | blue | JUNO INIT: a plain saw, polyphonic. Chords |
| **3** | snake | coral | PLAITS VA: a plain saw, mono, about 6 dB quieter. A lead |
| **DR** | crocodile | teal | the 606's 16 sounds in the kit's 16 slots |

Every voice starts with no effects on and an empty mod matrix. **SAVE > PROJECT > NEW** puts the live project back to this start (the project kept in the bank stays as it is).

### The colour rule

Colour always means something:

- A voice's colour means "this acts on that voice": voice 1 purple, voice 2 blue, voice 3 coral, the drums teal (Ben's own colour code: teal is drums). On a voice's own pages everything is in its colour.
- White and grey mean shared: the screen acts on everything (the FX button's pages, PERFORM, the STUTTER lane, the GLO pages, the patterns).
- A yellow **M** means muted. An ice-blue **S** means soloed.
- Red is only for recording and erasing.

### Three things and nothing else

| Pillar | Works like | What you do |
| --- | --- | --- |
| **SYNTH** | MiniFreak | pick an oscillator Type, turn three macros, shape it with the filter and envelopes, add three inserts and set its sends, route the mod matrix. The same pages on every voice |
| **DRUM** | Analog Rytm MKII | pick a sound, the 16 keys are its steps, hold a step and turn a knob to lock a value on it. Every sound has the pages SRC, FLTR, AMP, LFO; the drum voice's own effects are its FX tab |
| **PERFORM** | MPC pad effects | hold FX and the keys are pads: stutters, half time, throws, the isolator, motion, crush, phaser, jet, vowel on the white keys; mutes, reverse, tape stop and two sweeps on the black keys; with latch |

---

## 2. The controls and the screen

### One idea per control

Ben's rule for wave 10 (23 Nov 2026): every control has one idea, and nothing sits on a button that has no business with it.

| Control | Its one idea | So it also does |
| --- | --- | --- |
| **EDIT** | the sound (oscillator, filter) | held: OCT- clears the voice (asks first); MASTER the voice's level |
| **ENV** | the envelopes | held: MASTER the envelope's length |
| **LFO** | the modulation | held: MASTER the modulation depth |
| **FX** | the effects | held: PERFORM, the effect pads; MASTER the whole mix's effect amount (FX MIX) |
| **SEQ** | rhythm: the steps and the patterns A to D | held: rolls and retrigs, play a pattern, FILL with PLAY, halve and double; MASTER the swing |
| **ARP** | notes: the arpeggiator, the key, scale and chords | held: OCT- / OCT+ transpose the pattern; MASTER the transpose |
| **GLO** | everything global with no more obvious home | held: the mixer; MASTER the tempo |
| **SAVE** | keep and confirm | held: store a pattern, undo, redo; the confirm everywhere |
| **REC** | record, only | nothing is ever taken away with REC |
| **SEL** | PAGE: the next page | held: nothing |
| **OCT- / OCT+** | minus and plus of whatever the screen shows | twice quickly: the octave. With something held, minus removes it |
| **MASTER** | the volume | with a button held: that button's broad parameter |
| **SELECT** | knob 5: the type (drums: the sound) | |
| **black keys** | only what the screen draws for them | otherwise a note, or nothing |

### How to move around: four ways, in this order

Ben's rule for the whole instrument (23 Nov 2026). Everything you reach, you reach the first way; the others are extras.

1. **Press a labelled button.** Its features show, and KNOB 1 to 4 set them. Pressing the **same button again** goes to its **next category** (EDIT: OSC, then FILT, then OSC again). A button never walks pages and never jumps to another button's things.
2. **SEL is the PAGE button.** A tap of **SEL** goes to the **next page** inside the category shown (OSC: OSC 2, OSC 3, OSC 4, then OSC again). A category with one page: SEL does nothing.
3. **The SELECT knob is knob 5: the TYPE.** On a synth page it turns that page's type (the oscillator Type, the filter's TYPE, an insert's TYPE, the LFO's WAVE, the SLICER's mode). On the drum screen it picks the sound (one of the 16). It never walks pages and never sets the tempo. What it turns is written in the header, in a dark chip with a little dial: no chip, SELECT does nothing there.
4. **Long presses are shortcuts or performance layers**, never the only way to something: hold FX for PERFORM, SEQ for the rhythm (rolls, playing a pattern, the fill), ARP to transpose the pattern, GLO for the mixer, SAVE to store, undo and redo. **Button combos are the last resort**, kept only where one button is always the same and the link is obvious: a held button with OCT- / OCT+ (its minus and plus), a held button with MASTER (its broad parameter).

The tab row above the dials always shows **the categories of the button you pressed** (the one shown filled), at the right **the page inside it** ("2/4") and, at the far right, **what OCT- and OCT+ do here** (the -/+ legend). So the next press is never a surprise.

**The steps show only on SEQ** (Ben, 23 Nov, 10:33). From the steps, with nothing held, EDIT, ENV, LFO, FX (and every other page button) open that button's real page, with its picture, and the keys play notes again. Only **while a step is held** do EDIT, ENV, LFO, FX and SEL bring that page's knobs under the steps, so a turn makes a lock: the tab row then says **LOCK:** and the page. Let go of the step and the plain steps (TRIG) are back.

### The button map

| Button | Idea | Tap (again: its next category, round) | SEL (the next page inside) | Hold | -/+ while held | MASTER while held | SELECT turns |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **EDIT** | sound | synth: **OSC**, FILT. Drums: **SRC**, FLTR, then the kit's **KIT FLT** | OSC: OSC 2, 3, 4. FILT: FILT 2. SRC: 2/3, 3/3. FLTR: 2/2 (CLASSIC sounds) | MASTER's pair | - CLEAR the voice (asks: hold SAVE) | LEVEL | synth: OSC the Type, FILT the filter TYPE. Drums: the sound (on KIT FLT: nothing) |
| **ENV** | envelopes | synth: **AMP**, FILT envelope. Drums: **AMP** | drums: AMP 2/2 | on a synth sound page: **MAP** the envelope last shown (SEL: the other); elsewhere MASTER's pair | - REMOVE its mappings on this page | ENV LENGTH (drums: the sound's decay) | nothing (synth); the sound (drums) |
| **LFO** | modulation | synth: **LFO 1**, LFO 2, MATRIX. Drums: **LFO** | LFO 1: its sync page. Drums: LFO 2/2 | on a synth sound page: **MAP** the LFO last shown (SEL: the other); elsewhere MASTER's pair | - REMOVE its mappings on this page | MOD DEPTH (drums: the sound's LFO DEPTH) | synth: the WAVE. Drums: the sound |
| **FX** | effects | synth: **INS**, SENDS, SLICER, then the whole mix: **MIX**, **STUT** (the STUTTER lane). Drums, the same order: **INS** (the kit's), **SENDS** (the selected sound's), **SLICER** (the kit's), **MIX**, **STUT** | INS: FX 1, 2, 3. Drums INS: one page (its MIX on KNOB 4). MIX: DELAY, REV/CHO, MASTER. STUT: one page (SEL nothing; OCT- / OCT+ its steps' pages) | PERFORM (the pads) | - every latch off, + LATCH | FX MIX | synth: FX 1 to 3 the insert TYPE, SLICER its mode. Drums: the sound on SENDS; nothing on the kit's INS and SLICER |
| **SEQ** | rhythm | the steps on **TRIG**, PATTERN, **A-D** (the patterns) | TRIG: 2/2. PATTERN: 2/2 (SHIFT) | white keys ROLL (drums RETRIG); F#3 G#3 A#3 C#4 play A B C D; D#4 fill bar; PLAY: FILL; K1 RATE, K2 SHIFT, K3 SWING, K4 STEPS | - HALVE (asks), + DOUBLE; with PLAY held: + latch the fill, - let it go | SWING | nothing (synth); the sound (drums) |
| **ARP** | notes | synth: **ARP**, KEY (key, scale, chord). Drums: **CHROMATIC** (again: off) | ARP: ARP 2. KEY: KEY 2 | -/+ and MASTER only | - / + the pattern a semitone (twice quickly: two) | TRANSP | nothing |
| **SEL** | page | **PAGE**: the next page of the category shown | | nothing | | | |
| **GLO** | global | **GLOBAL**, SYSTEM, MIDI, DEVICE (the menu) | MIDI: MIDI 2, MIDI 3. DEVICE: its sections; held: the mixer's pans | the mixer: white keys the 16 drum sounds, F#3 to C#4 the voices; a key mutes | - CLEAR every mute and solo, + with a key: SOLO | TEMPO | nothing |
| **SAVE** | keep, confirm | **PROJECT** (the live project and the bank: KEEP, SWAP, NEW), PRESETS, USER, KITS (your drum kits), PATTERNS (store A to D), TOOLS | | keys 1 to 4 store A to D; with a question showing: the ring | - UNDO, + REDO | | nothing |
| **HOME** | home | **HOME**, the screensaver | screensaver: the next one | nothing | | | nothing |
| **REC** | record | record / arm (synth steps, stopped: STEP REC); again: stop | | nothing more | | | |
| **PLAY** | play | start / stop (with SEQ held: the FILL) | | | | | |
| **OCT- / OCT+** | minus, plus | the screen's minus / plus (the legend says what); twice within 0.3 s: the octave | | nothing more (wave 12: ghost and hard hits are the TRIG page's velocity) | | | |

### The FM-1's controls

| Control | Name in this manual | Main job |
| --- | --- | --- |
| Buttons | **PLAY**, **REC** | transport and recording (REC only records) |
| Buttons | **HOME**, **SAVE** | the HOME screen (again: the screensaver); the SAVE pages (the project and the bank, presets, user presets, kits, patterns, tools); hold SAVE: the one confirm, store, undo, redo |
| Buttons | **ENV**, **LFO**, **FX**, **EDIT**, **ARP**, **SEQ**, **GLO** | tap: the button's categories, one a press. Hold: a layer (below) |
| Button | **SEL (PAGE)** | the next page of the category shown. Held: nothing |
| Buttons | **OCT-**, **OCT+** | minus and plus of the screen; twice quickly: the octave; OCT+ is the latch, OCT- removes |
| Knob | **ALGORITHM** | choose the voice: 1, 2, 3, DRUM, and nothing else, on every screen (the STUTTER lane is FX's **STUT**) |
| Knob | **PRESETS** | the voice's sounds. The drums: on the drum screen the selected slot's **sound**; on HOME the whole **kit** |
| Knob | **SELECT** | knob 5: the TYPE of a synth page; the SOUND on the drum screen. Never a page, never the tempo |
| Knobs | **KNOB 1** to **KNOB 4** | the four values on the screen, under the four dials |
| Knob | **MASTER** | the output volume; with a button held, that button's broad parameter (a pot with end stops, used relatively: nothing jumps) |
| Keys | **key 1** to **key 16** | the 16 white keys, F3 to G5 |
| Keys | the **black keys** | F#3, G#3, A#3, C#4, D#4, F#4, G#4, A#4, C#5, D#5, F#5. A job only where the screen draws it |

SEL is the button between FX and ENV (printed SEL; in this manual **SEL (PAGE)**). SELECT is the knob.

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Note | F3 | G3 | A3 | B3 | C4 | D4 | E4 | F4 | G4 | A4 | B4 | C5 | D5 | E5 | F5 | G5 |

**Tap** means press and let go in under about 0.45 seconds without touching anything else. **Hold** means keep it pressed. **FX + key 3** means hold FX and press key 3.

### One rule everywhere

| Gesture | The one way | Where |
| --- | --- | --- |
| **Confirm** | anything that erases or replaces asks first in a white bar, **HOLD SAVE: ...** (HOLD SAVE: NEW, BANK KEPT, HOLD SAVE: REPLACE THE BANK, HOLD SAVE: LOAD U07, HOLD SAVE: LOAD 808, HOLD SAVE: CLEAR PADS, HOLD SAVE: STORE B, HOLD SAVE: HALVE TO 8, HOLD SAVE: CLEAR VOICE 1). **Hold SAVE**: a ring fills (about 1.3 seconds); full, it is done. The ring is **red** when something is taken away or replaced, **white** when nothing is lost (SAVE into an empty user preset slot; SWAP WITH THE BANK). Let go early: nothing, the question stays. Tap SAVE, press another button or leave the screen: the question goes. If SAVE is already held when the question comes (storing a pattern), the ring starts at once | SAVE > PROJECT (KEEP over a kept project, SWAP, NEW), SAVE > USER (LOAD, ERASE, SAVE), SAVE > TOOLS (CLEAR, INIT), storing over a used pattern, halving a pattern that has steps in its second half, clearing a voice (EDIT + OCT-), loading a whole kit over one you changed, SAVE > KITS (ERASE, SAVE over a kit), clearing the lane's pads. The question stays when the button that asked is let go, so SAVE can be held after it |
| **Minus and plus** | a **tap** of OCT- or OCT+ is the screen's minus or plus, at once; the legend at the right of the tab row says what (**[-] STEPS [+]**, **[-] RESO [+]**). No legend: a tap does nothing there | every screen (table in [section 12](#12-every-gesture)) |
| **Octave** | **OCT- or OCT+ twice within 0.3 seconds**: the octave down or up (both at once: 0). The second tap takes back what the first did. The one double press, Ben's choice | everywhere nothing is held; on the drums' CHROMATIC its keys' octave |
| **Remove** | **OCT- with the thing held**: a step (its note or hit), a lock, a key's note or drum sound from the whole pattern, a pad's latch; with EDIT held the whole voice (asks) | the steps, the sound pages, PERFORM, EDIT |
| **Keep a layer open** | hold its button about 0.7 seconds without touching anything, then let go: **LOCK** shows. The same button again, HOME or any page button lets it go (PLAY, REC, OCT- and OCT+ do not) | every layer |
| **Latch** | **OCT+** with the thing held (or the thing pressed with OCT+ held). Press it again to let it go; STOP ends every latch | PERFORM pads, the FILL (SEQ + PLAY) |
| **MASTER pair** | hold a button and turn MASTER: its broad parameter, named in the header's chip, the value in the white bar while it turns ("SWING 24") | FX, SEQ, ARP, EDIT, ENV, LFO, GLO |

Nothing is deeper than a hold. Pressing a button again goes to its next category: there is no timing in it, so it is not a double press. The octave's double tap is the one exception.

### The times, all of them

| Time | Means |
| --- | --- |
| under 0.45 s | a tap (a button's categories) |
| 0.14 s | a held button's layer shows (a tap does not flash it) |
| 0.3 s | OCT- or OCT+ again within this of its first press: the octave (the first tap acts at once; the second takes it back) |
| 0.7 s | the long press: a layer let go after it stays open |
| 1.3 s | the confirm ring (SAVE held with a question showing, from its press) |

0.3 seconds is short enough that a single tap never waits (it acts on the press) and long enough for a quick double tap; the single tap's action is undone by the second press, so nothing is lost by acting at once.

### Tap and hold: the layers

Tap a button for its categories. Hold it and the keys, knobs, OCT- / OCT+ and MASTER do something else for as long as you hold it. That is a **layer**: a performance layer or a shortcut to something that also has a page. Every layer shows, in the same frame: the button in the header's tag, **MASTER's pair** in the header's chip, a line under it saying what it does with the **-/+ legend** at its right, the keys' jobs (tiles, and the **black-key strip** where black keys have jobs), and the knobs' dials.

| Hold | Layer | Keys | Knobs | -/+ | MASTER | Its normal home too |
| --- | --- | --- | --- | --- | --- | --- |
| **FX** | PERFORM | pads | the last pad's | - latches off, + latch | FX MIX | performance |
| **SEQ** | RHYTHM | white: ROLL (drums RETRIG); F#3 to C#4: play A to D; D#4: fill bar; PLAY: FILL | RATE, SHIFT, GATE (synths), STEPS | - halve, + double | SWING | SEQ's PATTERN and A-D pages |
| **ARP** | NOTES | nothing | nothing | the pattern a semitone | TRANSP | KEY 2's TRANS |
| **GLO** | MIX | white keys: the 16 drum sounds; F#3 G#3 A#3 C#4: voices 1, 2, 3, the drums. Tap: mute; OCT+ held + key: solo | levels (SEL: pans) | - clear every mute and solo; + held with a key: solo | TEMPO | performance |
| **SAVE** | SAVE | keys 1 to 4: store A to D | | undo, redo | | SAVE > PATTERNS; the confirm |
| **EDIT** | VOICE | nothing | nothing | - clear the voice (asks) | LEVEL | the mixer's levels |
| **ENV** | MAP (on a synth sound page; elsewhere ENVELOPE, MASTER only) | nothing | the envelope's amounts onto them | - remove; SEL the other envelope | ENV LENGTH | the MATRIX |
| **LFO** | MAP (on a synth sound page; elsewhere MODULATION, MASTER only) | nothing | the LFO's amounts onto them | - remove; SEL the other LFO | MOD DEPTH | the MATRIX |
| **SEL**, **HOME**, **REC** | none | | | | | |

While you hold a button (a layer, OCT- or OCT+), a step, a key or a pad, **every button and key that does something with it lights, and every other one goes dark** (the backlight too): the lights show the hidden controls. A button that is dark does nothing with what you hold. A question waiting for the confirm lights SAVE alone (it pulses). See "The lights" below.

A layer shows after about 0.14 seconds, so a tap does not flash it. To keep a layer open without holding it, hold its button about 0.7 seconds without touching anything, then let go: the line under the header says **LOCK**. Any other button lets it go, the same button too, except PLAY, REC, OCT- and OCT+.

### MASTER with a button held

MASTER is a pot with end stops, not an encoder, so it is used **relatively**: nothing ever jumps to where the pot is. A turn moves the value the same share of the way to the end it turns toward ("value scaling"): the pot and the value reach their ends together and meet again there. With a button held MASTER changes that button's pair and the volume stays where it was; let go, and the volume follows MASTER again, from where it was, as soon as MASTER is turned with nothing held.

| Hold | MASTER turns | Exactly |
| --- | --- | --- |
| **FX** | **FX MIX** | the whole mix's effect return, 0 to 100 %: how much of the shared delay, reverb and chorus comes back into the mix. The sends into them (and PERFORM's throws) are untouched, so tails run on. 100 % is as before; 0 % is the dry mix exactly |
| **SEQ** | **SWING** | the selected voice's pattern swing (as PATTERN's KNOB 3) |
| **ARP** | **TRANSP** | the selected synth voice's transpose (KEY 2's TRANS, -24 to +24) |
| **EDIT** | **LEVEL** | the selected voice's level (the drums: the drum voice's) |
| **ENV** | **ENV LENGTH** | every stage of the selected synth voice's amp envelope (attack, decay, release; sustain is a level) longer or shorter together, from a quarter to about four times; a scale, the stored values never change. The drums: the selected sound's decay |
| **LFO** | **MOD DEPTH** | the selected synth voice's modulation: every matrix amount from the LFOs and the envelopes (the six fixed cells and the free ones) scaled from none to about twice; the stored amounts never change. The drums: the selected sound's LFO DEPTH |
| **GLO** | **TEMPO** | the tempo (as GLOBAL's KNOB 1) |

FX MIX, ENV LENGTH and MOD DEPTH are performance settings: not saved with the project (power-on and NEW: as stored). On the simulator: **Page Up / Page Down** (or **\\** and **`**) turn MASTER, so a button can be held on the keyboard at the same time.

### The black keys

A black key does something **only where the screen draws it**, in the two-row strip under the header (F#3 to F#4 above, G#4 to F#5 below; the STUTTER lane's splits after D#4, one octave a row; PERFORM puts all eleven on one row, short names). Elsewhere it is a note on a synth voice, or nothing. The strip shows on: PERFORM (its mutes and four extras), the STUTTER lane (its eleven repeats), hold SEQ (A B C D BAR), the mixer (the four voices on F#3 to C#4), ARP's KEY pages with a chord mode on (3RD 7TH SUS4 9TH INV, held: lit).

### The screen

Every screen has the same frame, top to bottom:

1. **The header.** A boxed **tag** says what the screen acts on: **1**, **2**, **3** (a synth voice), **DRUM**, **HOME**, **MIX** (the whole mix: FX's MIX pages and its STUTTER lane, white, with the page or STUTTER as the title), **GLO** (settings), **REC**, or on a layer the button you hold (**FX**, **EDIT**, **ENV**, **LFO**, **ARP**, **SEQ**, **GLO**, **SAVE**). Then the title (the sound's name, the kit, the page or the layer) or, where SELECT turns something, **the SELECT chip** in its place (on a layer: **MASTER's chip**, its pair: FX MIX, SWING, TRANSP, LEVEL, LENGTH, DEPTH, TEMPO, filled for a moment as MASTER turns): a little dial and the value SELECT turns (LP 24, ACID, DIST, SINE, SNARE), on the dark shade of the voice's colour, filled for a moment as you turn it. Then the tempo, a play or stop symbol, four beat lights (beat 1 red only while recording), a red disc while recording or a small white **unsaved dot**, and the battery. While a fill is on, **FILL** shows inverted in white in the beat lights' place; while PERFORM pads are latched, **PF** and how many (both: F+P and how many). The title or the chip stays.
2. **The body.** What the screen is about: the steps, a graph, the pads. Where black keys have jobs, the **black-key strip** is its top two rows.
3. **The tabs** (on every page screen, the steps, the drum screen and the lane): the categories of the button you pressed, the one shown filled, "2/4": the page inside it (SEL's), and at the far right the **-/+ legend**: a boxed minus, the word, a boxed plus (**[-] STEPS [+]**), what a tap of OCT- and OCT+ does here; no legend, a tap does nothing. On FX the voice's categories come first in the voice's colour, then a divider and **MIX STUT** in white: the whole mix (on the whole-mix screens the voice's names are grey too). When the names do not fit beside the legend, the ones not shown are cut to their first letters, four then three (STUT stays whole); if they still do not fit, the legend keeps its boxed minus and plus and drops its word (the drums' FX rows: FIL INS SEN SLI | MIX STUT 1/3 [-] [+]; a synth MIX page after a knob was turned). With a step held on a synth voice's steps: **LOCK:** and the page. On a layer, the line under the header says what it does now, with the legend at its right; a message shows there as a white bar.
4. **The dials.** Four dials, one over each knob: the name above, the value beside it in one tall face, always in a fixed 40-pixel column, so a value never runs into the next one. The knob you are turning has its name inverted and its value underlined. A grey value does nothing in the current setting. A value **locked** on the step you hold is drawn **inverted**. A short message replaces the dials as a white bar.
5. **The strip.** The four voices' heads, numbered 1 2 3 DR. The selected voice is underlined and its number is in its colour. A yellow **M** means muted, an ice-blue **S** soloed, a red dot recording.

### HOME: three zones

Tap **HOME**. The header says **PATTERN A** (to D) while a pattern plays, otherwise PHEBUS. The body has a deck and three zones, one per pillar:

| Part | Shows |
| --- | --- |
| **deck** | BAR and the bar of the selected voice's loop; a strip of the **FLOW** backdrop, a band per voice in its colour that swells as that voice gets louder (it flows while something plays and stands still otherwise), with a **>** after it: HOME again opens it full screen; the patterns **A B C D**: the one playing filled white, the one asked for next outlined white, the stored ones outlined grey |
| **SYNTH** | the three melodic voices side by side: each sound's name in its voice's colour, with **M**, **S** or a red dot, and under it the 16 steps in view (a note in the voice's colour, a tie dim, the step playing white). **The selected voice's name is inverted.** A muted voice's name is grey |
| **DRUM** | DRUM (inverted when the drums are selected), M or S for the drum voice, the kit's name while the 16 slots are the kit you loaded, **CUSTOM** once one differs, how many sounds the mixer has muted (yellow M and a number) or soloed (ice-blue S), the selected sound's name, and the 16 steps in view: a step with the selected sound's hit in teal, with other sounds' hits dim teal, with only muted sounds grey, the step playing underlined white |
| **PERFORM** | **LANE ON** or **LANE OFF** for the STUTTER lane, at the right **FX>STUT** in grey (where the lane lives: FX, its STUT category) or, while pads are latched, their names in white (or "3 LATCHED" when the names do not fit), and the lane's 16 steps in view: lit steps grey, a run of lit steps of one length drawn as one bar, the step playing underlined |

The HOME knobs play the selected voice while it plays:

| Knob | Does |
| --- | --- |
| **KNOB 1 CUTOFF** | the voice filter's cutoff (on the drums: the kit's FILTER, EDIT's KIT FLT) |
| **KNOB 2 DECAY** | the amp envelope's DECAY, on every synth voice; on the drums the selected sound's decay, on every kit |
| **KNOB 3 STUT** | the STUTTER lane ON (turn right) or OFF (turn left). White-grey: it acts on the whole mix |
| **KNOB 4 LEVEL** | the voice's level. On a muted voice the first turn unmutes it |

All four values are in the same tall face as on every page. On HOME, **PRESETS** browses the selected voice's sounds (the drums: whole kits) and **HOME** again opens the screensaver, HOME's second category ([section 10](#10-settings-midi-and-usb)). SELECT does nothing on HOME. The tempo is set in one place: **GLO > GLOBAL, KNOB 1 BPM**; every header shows it.

---

## 3. A first session, ten minutes

This takes you from a new project to a saved groove. Start with **SAVE > PROJECT > NEW** (tap SAVE once, turn KNOB 4 one click: HOLD SAVE: NEW, BANK KEPT shows; hold SAVE until the ring fills) if the FM-1 has something on it already. If that something is worth keeping, turn **KNOB 1 (KEEP)** on the same page first: it goes into the bank, and SWAP brings it back later.

**1. A drum pattern (two minutes).**

1. Turn **ALGORITHM** to **DRUM** and tap **SEQ**. The drum screen opens: the 16 white keys are the steps of the selected sound.
2. Turn **SELECT** until the header's chip says **KICK** (you hear each sound as you pass it). Tap keys **1, 5, 9 and 13**: four kicks.
3. Turn **SELECT** to **SNARE**. Tap keys **5 and 13**.
4. Turn **SELECT** to **HAT**. Tap keys 3, 7, 11 and 15.
5. Press **PLAY**.

**2. Lock a hit (one minute).** Turn **SELECT** back to the snare. **Hold key 13** (the second snare), tap **ENV** to show the AMP page, and turn **KNOB 2** (DECAY) right. Let go. Only that snare is longer: its value was drawn inverted while you held it, and its step now has a small black square. To take the lock off, hold the step and press **OCT-** (minus removes: again, every lock; again, the hit itself). Let go of the step: the plain steps are back.

**3. A bass line (two minutes).** Turn **ALGORITHM** to **1**. You stay on the GRID, now voice 1's. Press **PLAY** to stop. Tap **REC**: the title reads STEP REC and a red frame marks the step the next note goes on. Play a note on the keys: it lands on that step and the frame moves on. Press **SEQ** for a rest (rhythm), **ARP** to tie the note before (a note matter). OCT- / OCT+ move the red frame a step; twice quickly, the octave. Tap **REC** to finish. Press **PLAY**.

**4. Open the filter (one minute).** Tap **HOME**. While it plays, turn **KNOB 1 CUTOFF** and **KNOB 2 DECAY**. For the full filter page tap **EDIT** twice (OSC, then FILT): KNOB 2 is RESO, and **SELECT** turns the filter's TYPE. After a turn of RESO, **OCT- / OCT+** step it one fine step at a time (the legend says [-] RESO [+]).

**5. A chord track (two minutes).** Turn **ALGORITHM** to **2** (JUNO, polyphonic). Tap **SEQ** for its GRID, stop the song and tap **REC**. Hold three keys together and let go: the chord goes onto the step when the last key comes up. Write four chords, using **SEQ** for rests. Tap **REC** to finish and press **PLAY**.

**6. PERFORM pads, played and recorded (one minute).** While it plays, tap **REC** (recording starts at once). **Hold FX**: the keys are now pads. Press **key 3** (the **1/4** pad) for a beat, then **key 9** (**ISO LO**: only the lows) and **C#4** (**KICKS**: the kicks out) together. Let go of FX and tap **REC** to stop. The pads you played are now in the STUTTER lane and play back every loop: tap **FX** until **STUT** is lit to see them (on voice 1: five taps). **KNOB 3 STUT** on HOME switches them off and on.

**7. Save (one minute).** Stop the song. The working project saves itself a moment later: the white unsaved dot in the header goes out. To keep this groove as pattern A, **hold SAVE** and press **key 1**. To play pattern A again later, **hold SEQ** and press **F#3** (the first black key, drawn as A), or tap SEQ until **A-D** shows and turn KNOB 1.

**8. Two live moves.** While it plays, **hold SEQ and PLAY** for a fill (OCT+ with them keeps it on). **Hold FX and turn MASTER** to pull the whole mix's effects back to dry and in again.

---

## 4. SYNTH: the melodic voices

### The sound chain

Every synth sound is one chain of five stages, in signal order, the same on every voice whatever its Type:

**OSC** > **FILT** > **ENV** > **FX** > **MOD**

The oscillator makes the sound, the filter shapes it (before the amp, so a ringing filter dies with the note), the amp envelope shapes its level, three insert slots colour it and its sends feed the whole mix's delay, reverb and chorus, and the mod matrix moves it.

| Button | Its categories (again: the next, round) | SEL: the pages inside |
| --- | --- | --- |
| **EDIT** | **OSC**, **FILT** | OSC 1 to 4; FILT 1, 2 |
| **ENV** | **AMP**, **FILT** (the filter's own envelope) | one page each |
| **LFO** | **LFO 1**, **LFO 2**, **MATRIX** | LFO 1: LFO, its sync page |
| **FX** | **INS**, **SENDS**, **SLICER** (this voice, its colour), then **MIX** (the whole mix, white) | INS: FX 1, 2, 3. MIX: DELAY, REV/CHO, MASTER |
| **SEQ** | the steps: **TRIG**, **PATTERN**, **A-D** | TRIG 1/2, 2/2; PATTERN 1/2, 2/2 (SHIFT) |
| **ARP** | **ARP**, **KEY** | ARP, ARP 2; KEY, KEY 2 |
| **SELECT** | the page's TYPE: OSC the Type, FILT the filter TYPE, FX 1 to 3 the insert TYPE, SLICER its mode, LFO 1 and LFO 2 their WAVE. ENV, SENDS, MATRIX, ARP, KEY have none: SELECT does nothing there | |

On a sound page the keys play notes. Each page shows its subject large in the voice's colour and draws what the knobs do: the wave, the filter's response and its envelope's sweep, the envelopes, the FX slots, the LFOs, the matrix grid.

### Pages

| Page | SELECT | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- | --- |
| **OSC 1/4** | **TYPE** (31) | macro 1 | macro 2 | macro 3 | the Type's 4th control |
| **OSC 2/4** | TYPE | **OCT** (-3 to +3) | **TUNE** (-99 to +99 cents) | **GLIDE** | **GLMODE** (RATE: GLIDE is the time an octave takes; TIME: the time any glide takes) |
| **OSC 3/4** | TYPE | the Type's 5th control | **VOICE** (POLY, MONO, LEG, UNI) | **DETUNE** (unison spread) | **PRIOR** (LAST, LOW, HIGH) |
| **OSC 4/4** | TYPE | **ALLOC** (ROT: notes take the voices in turn; REUSE: the nearest free one) | | | |
| **FILT 1/2** | **TYPE**: LP 24, LP 12, BP, HP | **CUTOFF** | **RESO** | **ENV** (how far the filter's envelope moves it, either way) | **DRIVE** |
| **FILT 2/2** | TYPE | **KEY** (the cutoff follows the note) | **LINK**: AMP or OWN | | |
| **ENV** AMP | | **ATTACK** | **DECAY** | **SUS** | **REL** (times in MS under a second, then S, as 1.2S; SUS in %; both envelopes) |
| **ENV** FILT | | **ATTACK** | **DECAY** | **SUS** | **REL** (the filter's own envelope) |
| **FX 1**, **FX 2**, **FX 3** | **TYPE** | TIME | DEPTH | AMOUNT (named by the type) | |
| **SENDS** | | **DELAY** | **REVERB** | **CHORUS** | |
| **SLICER** | **SLICE** (its mode) | **PAT** | **RATE** | **DEPTH** | |
| **LFO 1** | **WAVE** | **RATE** | **PHASE** | **FADE** | **POL** |
| **LFO 1** sync | WAVE | **SYNC** | **TRIG** (NOTE, FREE, PATT) | **E SYNC** (both envelopes in note values) | |
| **LFO 2** | **WAVE** | **RATE** | **SYNC** | **TRIG** (NOTE, FREE, PATT; FREE to start) | |
| **MATRIX** | | **SOURCE** (row) | **DEST** (column) | **AMOUNT** | **CLEAR** (on AS1, AS2: **TARGET**) |

### The oscillator

**SELECT** on any OSC page picks the **Type** from one list of 31, the same on every voice: ACID (the 303), JUNO (the Juno DCO), the Plaits models (VA, VA VCF, FOLD, PHASE D, 2-OP FM, 6-OP A, B, C, WAVETABLE, TERRAIN, ADDITIVE, CHORD, STR MACH, SWARM, FORMANT, CHIPTUNE, STRING, MODAL, NOISE, SPEECH, PARTICLE, KICK, SNARE, HI-HAT) and the Elements models (BOUNCE, PLUCKED, BOWED, BLOWN, OMINOUS). KNOB 1 to 3 are its three macros and KNOB 4 its fourth control, renamed for each Type (VA: DETUNE, SQUARE, SAW; ACID: WAVE, ACCENT, SLIDE; JUNO: WAVE, PW, SUB). Turning TYPE changes only the oscillator: the filter, envelopes, FX and voice stay. Another engine's Type starts its macros at plain defaults. ACID plays one note at a time, JUNO up to six, the heavy models (6-OP, STR MACH, STRING, SPEECH, PARTICLE, BOWED, BLOWN, OMINOUS) one.

### The filter

One resonant filter on every voice: a four-pole ladder whose TYPE gives 24 or 12 dB low-pass, band-pass or high-pass. On ACID and JUNO, LP 24 is their own ladder (the 303's, the Juno's IR3109); the line under the filter's name says which. RESO at the top self-oscillates. DRIVE pushes the sound into it. KEY at 100 % follows the note exactly.

**LINK** chooses the filter's envelope: **AMP** follows the amp ENV (one envelope for level and tone, the MiniFreak's quick way) or **OWN** uses its own ADSR on ENV 2/2. While it is linked, ENV 2/2 shows the amp values in grey; turning any of its knobs makes it OWN, starting from the amp values (FILTER ENV: OWN).

**LFO SYNC TRIG**: NOTE restarts the LFO at each new phrase, FREE never restarts it, PATT restarts it each time the pattern comes round to step 1.

### FX: inserts and sends

**FX** first opens this voice's own effects, in its colour: **INS** (SEL: FX 1, FX 2, FX 3), then **SENDS**, then **SLICER**; one more press is **MIX**, the whole mix's effects in white ([section 9](#9-fx-glo-and-the-mixer)). Three **insert** slots in series after the filter, all OFF to start. **SELECT** picks a slot's TYPE; KNOB 1 to 3 are named by the type:

| Type | KNOB 1 | KNOB 2 | KNOB 3 |
| --- | --- | --- | --- |
| **PHASER**, **FLANGER** | RATE | FDBK | MIX |
| **DIST**, **SOFT**, **HARD**, **FOLD**, **FUZZ** | TONE | DRIVE | MIX |
| **CRUSH** | RATE | BITS | MIX |
| **EQ** | LOW | MID | HIGH (-12 to +12 dB) |

A voice can have one FLANGER; SELECT skips it when another slot has it.

**SENDS** (after FX 3): how much of this voice goes to the whole mix's **DELAY**, **REVERB** and **CHORUS**, 0 to start. A voice has three inserts and all three sends at once. The delay's time, the reverb's size and the chorus's rate are not on any voice page: they are FX's MIX pages ([section 9](#9-fx-glo-and-the-mixer)).

### The mod matrix

LFO's last category is a grid of 7 sources (rows) by the destinations (columns), six columns in view at a time; the view slides with the cursor.

- **Sources**: LFO1, LFO2, AENV (amp envelope), FENV (the filter's envelope), PENV (a fast pitch fall at each note's start), VEL, KEY.
- **Destinations**: PIT, M1 M2 M3 (the macros), CUT, RES, LVL (only ever down), PAN, FX1 FX2 FX3 (each insert slot's AMOUNT), then one column for **each parameter mapped** (three letters: ATK, FAT for the filter envelope's attack, DRV, L1R and L2R the LFOs' rates, 1T for FX 1's first knob, E4 for the Type's fourth control ...) and a last **+** column to make another (KNOB 4 TARGET picks its parameter, any lockable sound value; one column a parameter).

A filled square is a positive amount, an empty one negative, bigger for more. Six cells are the routings shown on other pages too: LFO1 > PIT, LFO1 > CUT, LFO1 > LVL, FENV > CUT (FILT's ENV), PENV > PIT, KEY > CUT (FILT 2's KEY). Besides them a voice holds **19 cells**, on any columns; one more says **MATRIX FULL: 19 CELLS** and nothing changes. A voice can map up to 19 different parameters (every cell on its own). PIT, M1 to M3, CUT and RESO move each note with its own envelopes; every other column moves the whole voice, with the envelopes and velocity of the note started last.

### Mapping: hold LFO or ENV

On any of a synth voice's sound pages (OSC, FILT, ENV, FX, SENDS, SLICER, LFO, MATRIX), **hold LFO** (or **ENV**) and turn a knob: the page stays where it is, and the knob's dial now shows how much the LFO moves it.

- **Which LFO, which envelope**: the one you looked at last (LFO 1 if none yet; the amp envelope if none yet). The tab row says **MAP LFO 1** (outlined), and what **SEL** flips to: tap SEL while holding for the other one.
- **The dials**: each knob's amount from that source, right +, left -, back to 0 removes it. A set amount is **boxed** in the voice's colour (a lock is inverted; a map is boxed); 0 is grey; a knob that cannot be mapped is dark grey "--" and a turn says so (CUTOFF: NOT MAPPABLE, ATTACK: NOT FROM ITSELF).
- **OCT-** while holding removes every mapping from that source on this page (SAVE + OCT- brings them back). MASTER is still LFO's MOD DEPTH and ENV's ENV LENGTH.
- **One source of truth**: each amount is a cell of the MATRIX. CUTOFF and RESO are the CUT and RESO columns, the OSC page's three macros M1 to M3, TUNE the PIT column (vibrato), an insert's AMOUNT its FX column; every other knob gets a column of its own. LFO 1 on CUTOFF is the same value as LFO1 > CUT on the grid; the FILT envelope on CUTOFF is FILT's ENV knob.
- **Not mappable**: TYPE, OCT, GLMODE, VOICE, PRIOR, ALLOC, the filter's ENV, KEY and LINK (modulation amounts and switches), the LFO's POL, SYNC, TRIG, ESYNC, LFO 2's SYNC and TRIG, the SLICER's PAT and RATE, the MATRIX page's knobs. An envelope cannot move its own times and level. An LFO may move its own RATE, PHASE and FADE.
- **Outside the hold** a knob something moves has a small **wave** under its dial.
- Velocity and key: on the MATRIX page only. On HOME, the steps and every other page, holding LFO or ENV is MASTER's pair, as before. On the drums' sound pages holding LFO maps too (section 5, "Hold LFO on a drum sound"); holding ENV there stays MASTER's pair.

### The GRID: steps on the keys

**Tap SEQ** for the selected voice's GRID (its steps; they show only on SEQ). The 16 white keys are the 16 steps of the page shown. They play nothing. Under the steps: a line about the step held or under the cursor, the tab row (the categories of the last button pressed) and the dials.

| You do | Result |
| --- | --- |
| **tap an empty step** | it is set with the **pen** (the last note or chord you played, or TRIG's KNOB 1) at velocity 100; you hear it |
| **tap a set step** | it is cleared when the key comes up, unless you turned a knob while holding it |
| **hold a step** | the step is selected: the line shows STEP n HELD, its note and its locks |
| **hold a step and turn a knob on a sound tab** | that value is **locked** on the step (drawn inverted; the tile gets a small square) |
| **hold a step + OCT-** | minus removes: on TRIG the step itself (its note, locks, nudge, condition); on a lock view the lock of the knob last turned, again every lock, again the step |
| **EDIT**, **ENV**, **LFO**, **FX**, nothing held | that button's real page (the steps go, the keys play notes) |
| **EDIT**, **ENV**, **LFO**, **FX**, **SEL**, a step held | the lock view: that page's knobs under the steps (the tab row says LOCK: and the page); let go of the step: TRIG again |
| **SEQ** | TRIG; again PATTERN (GATE DIV SWING STEPS; SEL: SHIFT); again A-D (the patterns); again TRIG |
| **SELECT** | the TYPE of the page shown (a step held: locked on it, where it can be) |
| **black keys** | nothing (none drawn) |
| **OCT-**, **OCT+** tapped (nothing held) | the page of steps before or after; OCT+ on the last page adds a copy of it (up to 64 steps). On A-D: the pattern before or after. Twice quickly: the octave |

**TRIG** with a step held: KNOB 1 NOTE (by scale degrees), LEN (in steps; the steps after become ties), VEL, COND (-, FILL, NOFL, CHNC). TRIG 2/2 (SEL): FLAG (-, ACC, SLD, A+S), RETRIG (x1 to x4: the step repeated in its time), NUDGE. With nothing held: NOTE (the pen), STEP (the cursor), HIT (right sets the cursor's step, left clears it), VEL.

**Locks.** The Rytm's way: hold a step, turn whatever is on the knobs. The page buttons and SEL change the page while you hold it (the lock view), so you can lock the cutoff, tap ENV and lock the decay in one hold. SELECT with a step held locks the page's type where it can (the filter's TYPE, the LFO's WAVE, the SLICER's mode); the oscillator Type, an FX slot's TYPE, VOICE, PRIOR and the matrix's cursor cannot be locked (TYPE: NOT LOCKABLE). A voice holds **24 locks**.

**Spice and Dice.** On TRIG 2/2 with nothing held (SEL on TRIG): KNOB 1 **SPICE** (0 to 100 %: how much variation you hear; 0 is the pattern as written) and KNOB 2 **DICE** (each click right a new roll, left the roll before). A roll leaves notes out, moves some an octave, adds retrigs and changes velocities and lengths; the notes you wrote never change. Each pattern A to D keeps its own.

**STEP REC.** With the song stopped, on a synth voice's GRID, tap **REC**. A red frame marks the step the next note goes on. Play a note or a chord (keys held together: written when the last comes up); the frame moves on. OCT- and OCT+ tapped move the frame a step back or on (twice quickly: the octave), KNOB 1 the octave, KNOB 2 moves the frame, KNOB 3 the velocity, KNOB 4 the pattern length. SEQ writes a rest (rhythm; keys held: their notes first), ARP a tie (a note matter). SAVE + OCT- undoes the whole pass. Tap **REC** again to finish. With the song playing, REC on the GRID records live instead (LIVE REC).

**Undo.** **SAVE + OCT-** undoes, **SAVE + OCT+** redoes.

**Halve and double.** **Hold SEQ and press OCT-**: the pattern is halved. If its second half holds anything, it asks first (HOLD SAVE: HALVE TO 8): hold SAVE until the ring fills (SEQ may be let go); the second half is dropped. **Hold SEQ and press OCT+**: the pattern is doubled as a clone, its steps with their locks, nudges and conditions copied after themselves (up to 64 steps). One undo each.

### Presets

**PRESETS** browses the voice's sounds, grouped by kind (BASS, KEYS, ORGN, PAD, LEAD, PLCK, STAB, FX, INIT), then your user presets. Each engine has an INIT sound. **Tap SAVE** for its categories, one a press: **PROJECT** (the live project and the bank: KNOB 1 KEEP, KNOB 2 SWAP, KNOB 4 NEW; [section 8](#8-patterns-a-to-d-recording-and-saving)), **PRESETS** (KNOB 1 the sound, KNOB 2 the engine; -/+ the sound before / after), **USER** (KNOB 1 SLOT 1 to 32, KNOB 2 LOAD, KNOB 3 ERASE, KNOB 4 SAVE; -/+ the slot), **TOOLS** (KNOB 1 CLEAR clears the pattern, KNOB 2 INIT resets the sound; NEW moved to PROJECT). Each asks: turn it, then hold SAVE until the ring fills.

### Key, scale and chords (ARP, then KEY)

Ben chose to put them under ARP: tap **ARP** for the arpeggiator (SEL: ARP 2), tap it again for **KEY**:

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **KEY** | **ROOT**: the key of the song (all three synth voices) | **SCALE**: CHROM, MAJ, MIN, DOR, MIX, PEN, MPEN, HARM, PHRY, LYD, LOC, MEL, BLUES, WHOLE, DIMHW, DIMWH (all three voices) | **QNT**: OFF, SNAP (keys round to the scale), WHITE (white keys walk the scale, black keys silent) | **CHORD**: OFF, TRIAD, 7TH, 9TH, SUS4, POWER |
| **KEY 2** | **TRANS**: -24 to +24 semitones | **STRUM** | **VLEAD** (voice leading) | |

With a chord mode on, the white keys walk the scale from C4 and each plays the chord of its degree. **On the KEY pages** the black keys change it while held, drawn in the black-key strip: F# **3RD** flips the third, G# **7TH** adds the 7th, A# **SUS4**, C# **9TH**, D# **INV** inverts it. Elsewhere the black keys do nothing in chord mode (Ben: black keys only live when drawn).

**Hold ARP** (NOTES): **OCT- / OCT+** move every note of the selected voice's pattern a semitone down or up (one undo each; a double tap there is simply two semitones); **MASTER** turns TRANS.

---

## 5. DRUM: the drums

The drums work the Rytm's way. The drum voice has 16 sounds, laid out as **the drum grid**, like the Analog Rytm's pads (Ben: "Cymbals near the top. Toms on the second row. Kick in the bottom left with snares in the right hand corner and hi hats in the top left corner"; the grid as he typed it):

| | Column 1 | Column 2 | Column 3 | Column 4 |
| --- | --- | --- | --- | --- |
| **top row** | HAT | OP HAT | RIDE | CRASH |
| **row 2** | PEDAL | CONGA | C-BELL | SHAKER |
| **row 3** | KICK 2 | LO TOM | HI TOM | CLAP |
| **bottom row** | KICK | RIM | SNARE 2 | SNARE |

**Two key orders over the one grid** (Ben, 23 Nov 2026, 22:45 to 22:48). The picture above never changes; which white key is which sound depends on what the keys are doing:

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Playing, RETRIG, SELECT** (the Rytm's order: the bottom row first, left to right, then up a row) | **KICK** | RIM | SNARE 2 | SNARE | KICK 2 | LO TOM | HI TOM | CLAP | PEDAL | CONGA | C-BELL | SHAKER | HAT | OP HAT | RIDE | CRASH |
| **The mixer's mutes and solos** (Ben's 21:10 order: down each column, top to bottom) | HAT | PEDAL | KICK 2 | **KICK** | OP HAT | CONGA | LO TOM | RIM | RIDE | C-BELL | HI TOM | SNARE 2 | CRASH | SHAKER | CLAP | SNARE |

- **The Rytm order** (Ben: "Rytm order: bottom row first"; of SELECT, "It should follow the Rytm order"): the keys that play the sounds by hand (HOME, live recording, a sound's key held + OCT- to erase its hits), hold SEQ's RETRIG keys and their tiles, the key lights for hits, and SELECT's walk through the sounds on the drum screen (KICK, RIM, SNARE 2, SNARE, KICK 2 ... CRASH, then round to KICK). CHROMATIC's OCT- / OCT+ follow SELECT's order. A new project starts on KICK, key 1.
- **The mixer keeps the columns** (Ben, 22:46: "Are you undoing the very specific order that I've specified for the mixer for muting the drums?"): hold GLO, and key 4 mutes the KICK, key 16 the SNARE, key 1 the HAT. PERFORM's pads also count down columns (22:07), so the mixer and PERFORM agree with each other, and the hand-played keys count along the rows. That difference is Ben's choice.

It is only where each sound sits and which key reaches it: the slots, your projects, the kits, the MIDI notes, the trigs and locks and PERFORM's drum mutes are the same as before, so a project saved before plays the same. On the drum screen's steps the 16 keys are still the selected sound's 16 steps, in order.

The closed hat cuts the open hat. **Ghost and hard hits come from velocity** (Ben: "Move ghost hits elsewhere"): the keys play and record at the velocity TRIG's KNOB 1 sets with no step held (100 to start; turn it to about 40 for ghosts, 127 for hard hits), and a hit already on a step gets its own with the step held and TRIG's VEL. OCT- with a key is only ever erase: **a drum key held, then OCT-**: that sound's hits are erased from the pattern (playing: from each step the playhead passes while you hold the key; stopped: all at once). The black keys play no drum sound.

### The drum screen

Tap **SEQ** or **EDIT** with DRUM selected. The header shows DRUM and the **SELECT chip: the selected slot** (SELECT picks it). The body shows the **slot's name** large (SNARE), the kit (or CUSTOM) and how many steps it plays on (or STEP n HELD), the **slot map** top right (16 tiles in the drum grid: HAT top left, KICK bottom left, SNARE bottom right; a slot with hits dim teal, lit as it plays, the selected one framed white, a yellow bar muted, ice blue soloed).

**The keys are the steps, and only the keys** (Ben, 23 Nov 13:35: "I don't need to see the sequence on screen, it's shown on my keys"): the 16 white keys are the selected slot's 16 steps of the page shown, their lights its hits; the screen does not draw them again. Under the name:

- on a sound page (SRC, FLTR, AMP, LFO): **SOUND 606 SNARE** (what PRESETS browses) and its place in the list (8/41), then a **picture of the sound**: its level over the first half second (teal; a noise of its own in grey) and its pitch sweep (a white line from the top), a mark every eighth of a second. With a step held: that step's locks (LOCK DECAY TUNE);
- on SEQ (TRIG, PATTERN, A-D): which steps the keys are on (KEYS 1-16 OF 32 STEPS) and, with a step held, its values (VEL, RETRIG, NUDGE, the condition and chance, NOTE, how many locks); nothing held, the velocity new hits and the keys play at;
- on FX: THE WHOLE DRUM VOICE and the page's name.

Then the tab row (the categories of the button pressed: SRC FLTR for EDIT, AMP for ENV, LFO, the FX group, TRIG PATTERN A-D for SEQ) and the dials in teal.

| You do | Result |
| --- | --- |
| **SELECT** | the selected sound, one of the 16 a detent (heard while stopped), on every tab, in the Rytm order: KICK, RIM, SNARE 2, SNARE, KICK 2 ... RIDE, CRASH, then round to KICK (left: the other way). A new project starts on KICK |
| **tap an empty step** | a hit of the selected sound (velocity 100, or TRIG's KNOB 1 with nothing held) |
| **tap a hit** | it goes when the key comes up, unless a knob was turned |
| **hold a step and turn a knob** | lock that value on that sound's hit only (drawn inverted) |
| **hold a step + OCT-** | the lock of the knob last turned goes; again (or none turned): every lock; again: the hit itself. On TRIG: the hit itself, with all it holds |
| **EDIT** | SRC; again FLTR, SRC. **SEL**: the page inside (SRC 1/3 .. 3/3, FLTR 2/2) |
| **ENV**, **LFO** | AMP, LFO (SEL: their 2/2) |
| **EDIT** (third press) | **KIT FLT**: the kit's FILTER (LP / HP), after a divider: the header says KIT, SELECT does nothing |
| **FX** | INS (the kit's INSERT: TYPE on SELECT, three values, MIX on KNOB 4), **SENDS** (the selected sound's DELAY, REVERB, KIT FX), SLICER (the kit's), then MIX (DELAY, REV/CHO, MASTER: the whole mix, white): a synth voice's order |
| **SEQ** | TRIG (SEL: TRIG 2/2); again PATTERN (DIV SWING STEPS; SEL: SHIFT); again A-D |
| **OCT-**, **OCT+** tapped | pages of steps, as on the synth steps (A-D: the pattern); twice quickly: the octave |
| **PRESETS** | the selected slot's sound (below: Kits and sounds) |

The page buttons and SEL also work with a step held, as the Rytm's page keys do; SELECT does not change the sound while a step is held (LET GO OF THE STEP). With nothing held, a knob changes the selected sound.

### The sound pages

| Page | a HEX sound (from HEX BOOM .. HEX ROOM, MY HEX, INIT HEX) | a CLASSIC sound (from the 606, 808, 909 and the rest, INIT CLASSIC) |
| --- | --- | --- |
| **SRC 1/3** | TUNE, P.AMT, P.DEC, WAVE | TUNE, FINE, WAVE, TONE |
| **SRC 2/3** | MODEL, MOD, M.PIT, FM | SWEEP, SWTIME, PART2, P2 LVL |
| **SRC 3/3** | TRANS, T.LVL, T.PIT, PSLOPE (FINE on CYM, HAT) | NOISE, N.LVL, CLICK, CHIP |
| **FLTR 1/2** | CUTOFF, RESO, (none: no filter envelope), TYPE | CUTOFF, RESO, ENV, TYPE |
| **FLTR 2/2** | DRIVE | DRIVE, HPF |
| **AMP 1/2** | ATK, DEC, SLOPE, DIST | HOLD, DECAY, NS HLD, NS DEC |
| **AMP 2/2** | CRUSH, (none), (none), LEVEL | (none), (none), (none), LEVEL |
| **FX > SENDS** | DELAY, REVERB, KIT FX | the same |
| **LFO 1/2** | SPEED, DEPTH, DEST, WAVE | the same |
| **LFO 2/2** | SYNC, FADE, PHASE, MODE | the same |

FLTR starts as a synth voice's FILT: CUTOFF, RESO, ENV. Its TYPE stays on KNOB 4 and DRIVE leads 2/2, because the drum screen's SELECT picks the sound (on a synth voice TYPE is SELECT's and DRIVE takes KNOB 4). The sound's own sends are FX > SENDS (DELAY, REVERB), not on AMP any more. Every sound has its own LFO: DEST is PITCH (up to 12 semitones), CUT, RES or LEVEL; MODE is TRIG (restarts at each hit), FREE or ONE (one cycle).

**Every slot is the project's own.** Turning a knob changes that slot's sound in this project at once; the kit it came from never changes, and nothing asks. Each slot shows its machine's own parameters (the table above), so a kit with HEX and CLASSIC sounds side by side shows each slot's own. Locks: as before; MODEL cannot be locked.

### TRIG: the hit's own values

| TRIG | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **1/2** | **VEL** 1 to 127 | **RETRIG**: OFF, 1/1 .. 1/80 | **NUDGE** | **COND**: -, FILL, NOFL, CHNC |
| **2/2** | **CHANCE** 1 to 99 % | **LENGTH** of the retrig, 1/8 step to 16 steps or INF | **CURVE** of the retrig's velocity, -128 to 127 | **NOTE** (a pitch lock) |

With nothing held, TRIG's KNOB 1 is the velocity new hits get (100 to start); the other knobs say HOLD A STEP. A pattern holds 64 hits with a condition, chance, nudge or retrig (TRIG EXTRAS FULL: 64) and 128 sound locks (SOUND LOCKS FULL: 128).

### Hold LFO on a drum sound

Ben: "Hold LFO maps on drums too". On a drum sound page (SRC, FLTR, AMP, LFO, and FX > SENDS), **hold LFO and wiggle a knob**: that knob becomes the slot's LFO destination, and the turn is its amount (DEPTH), right +, left -, back to 0: off. The page stays; the tab row says **MAP LFO** (outlined, as on the synths); the dials show the amount on the knob the LFO moves and 0 on the others; a knob that cannot be mapped (a named value: WAVE, TYPE, MODEL, NOISE, TRANS; the LFO's own knobs) is dark grey "--" and a turn says so. A sound has one LFO and so one destination: wiggling another knob moves it there, and the screen says so (**LFO > CUT (WAS DECAY)**). **OCT-** while holding takes it off (SAVE + OCT- brings it back). Outside the hold the knob it moves has the small wave under its dial. SEL does nothing here (one LFO). **Hold ENV** on the drums stays MASTER's pair (a drum sound has no envelope to map).

The LFO's DEST knob (LFO 1/2) lists the same: PITCH, CUT, RESO, LEVEL, then every knob of the slot's machine that can be wiggled (HEX: FINE ATK DEC SLOPE P.AMT P.DEC M.PIT FM T.LVL T.PIT DRIVE DIST CRUSH REV DEL; CLASSIC: FINE SWEEP SWTIM HOLD DECAY TONE PART2 P2LVL CLICK N.LVL N.HLD N.DEC ENV HPF CHIP DRIVE REV DEL). PITCH, CUT, RESO and LEVEL move the hit as it rings; on a HEX sound every other knob does too; on a CLASSIC sound the others (and the sends on both) take the LFO's value as the hit starts, so the LFO changes them hit by hit.

### Modes and live play

- **Mutes and solos**: on the mixer. **Hold GLO** (whichever voice is selected): the 16 white keys mute and unmute the 16 sounds in the mixer's order, down the grid's columns (key 4 the KICK, key 16 the SNARE; yellow; playing by hand counts the other way, the Rytm's: see the drum grid above); hold **OCT+** and tap a key to solo it (ice blue); OCT- clears them all. Mutes and solos are not saved. For quick drops, PERFORM has four drum mutes of its own (KICKS, TOPS, SH+CL, HT+SN, held or latched).
- **CHROMATIC mode**: tap **ARP** (the drum voice's ARP group has this one category). The 27 keys play the selected sound (SELECT changes it) at pitches, C4 (key 5) its own; OCT- and OCT+ tapped step the sound (in key order, stopping at HAT and SNARE), twice quickly move the keys an octave (-2 to +2). Recorded live, each hit keeps its pitch as a NOTE lock. ARP again, or any other page button, leaves it.
- **RETRIG**: hold **SEQ** and press a key: that key's sound (the Rytm order: key 1 the KICK, key 4 the SNARE) repeats while held, at the rate KNOB 1 sets (all 17, 1/1 to 1/80). (Until wave 10 this was ARP's, with the rates on the black keys.)
- **FILL**: hold **SEQ** and **PLAY**: a fill while PLAY is held (the transport is not touched); **OCT+** with them latches it, SEQ + PLAY again (or STOP) ends it. **SEQ + D#4** (BAR) fills the next bar. FILL shows in every header.
- **Effects, as on a synth voice** (Ben, 23 Nov 2026: "ENV should be for dry sound design / modulation. Edit should be for the same. FX should be for FX"; "FILTER should live under Edit. Filter envelope should live under ENV. Sends should live under FX"; "Just mirror the synths as closely as possible"). EDIT and ENV are the sound only: SRC, FLTR, AMP, LFO, no sends. **Tap FX**: **INS** (the kit's **INSERT**: TYPE on SELECT, three values, its MIX on KNOB 4; one page), **SENDS** (the selected sound's own: **DELAY**, **REVERB**, **KIT FX**; SELECT picks the sound, the header names it), **SLICER** (the kit's), then **MIX** (the whole mix's pages, white) and **STUT**. The kit's FILTER (LP / HP, HOME's KNOB 1 on the drums) is EDIT's third category, **KIT FLT**, after the sound's own SRC and FLTR and a divider.
- **The kit's pages** (INS, SLICER, KIT FLT) act on all 16 sounds at once: the header says **KIT**, the big name says KIT and the kit's name, no sound is framed in the map, SELECT turns the INSERT's TYPE and the SLICER's mode (as on a synth voice; nothing on KIT FLT), and the line says how many sounds go through them: **THE KIT: 13 OF 16 SOUNDS** (an empty INSERT: **EMPTY: SELECT PICKS A TYPE**, no values, MIX included). On the drums' **MIX** pages (the one white MIX screen every voice shows) the white keys stay the drum steps.
- **KIT FX** (Ben: "a KIT FX switch per sound"), on each sound's SENDS page, KNOB 3: **ON** (to start, as before) the sound goes through the kit's FILTER, INSERT and SLICER; **OFF** it goes straight to the mix, untouched by them, its own DELAY and REVERB still working. Example, **stutter the tops, leave the kick alone**: select KICK (SELECT), FX until SENDS, turn KNOB 3 to OFF; then FX to SLICER and switch it on: the hats and snares stutter, the kick plays straight. The mixer's mutes and solos, PERFORM's drum mutes, chokes and the sends work the same for both.
- **Locks**: hold a step and turn a knob on any of these pages to lock it on that step: on SENDS, DELAY and REVERB lock that sound's send on that hit (as AMP 2/2 did); KIT FX cannot be locked (it is the sound's switch). **Hold LFO** on SENDS maps the sound's LFO onto DELAY or REVERB, as on its other pages (KIT FX: not mappable).
- **In the simulator**: select the drums (**Right**, ALGORITHM, to DRUM), press **1** (FX) twice for **SENDS**: **Up / Down** (SELECT) picks the sound, the knobs are DELAY, REVERB, KIT FX (KNOB 3: turn it left for OFF). Press **5** (EDIT) three times for **KIT FLT**.
- **There is no kit-wide send any more** (Ben: "Remove them"). An old project that used FX > SENDS on the drums has those amounts added to every sound's own sends when it loads (clamped to the most a send can be), so it sounds the same; with the drum SLICER on it is close but not exact (the kit's share used to go in after the SLICER).

### Kits and sounds

**The kit is 16 slots, one per white key** (which key: the drum grid above). Each slot holds one whole sound: its machine, every parameter on its pages, its sends, its LFO and its name ("606 SNARE"). The slots are this project's own: what you do to them is saved with the project and never changes a built-in kit. A new project starts with the 606's 16 sounds.

**Any slot can hold any sound from any kit.** There are two drum machines, and each slot plays with its own:

- **HEX**: the LXR-02's drum voices (the HEX kits: HEX BOOM, HEX ACID, HEX METAL, HEX GLITCH, HEX ROOM, and the old MY HEX kits)
- **CLASSIC**: the synthesised model of the classic drum machines (the 606, 808, 909 and the 29 other kits)

So an 808 snare can sit in a 606 kit, or a HEX kick beside a CLASSIC hat.

**PRESETS does two jobs, and the screen always says which**, in the band where the dials are, while you turn it and a moment after:

| Where | PRESETS changes | The screen says |
| --- | --- | --- |
| the drum screen (any tab: SRC, FLTR, AMP, LFO, FX, TRIG) | the **selected slot's sound** (SELECT picks the slot) | **SOUND 808 SNARE**, then the group and the place in it: SNARE 6/41 |
| HOME (the drums selected), SAVE > PRESETS, the mixer page | the **whole kit**: all 16 slots | **KIT 808**, then CLASSIC KITS 2/32 |

**Changing one sound.** On the drum screen each click of PRESETS puts the next sound of the list into the selected slot at once, and you hear it (stopped: once a click; playing: in the pattern). Turn back to where you started and you have the sound you had, exactly, with any changes you had made to it. **SAVE + OCT-** takes the whole browse back in one go (SAVE + OCT+ brings it again).

**The list, for the selected slot** (Ben: "Same kind first, then all"):

1. **INIT HEX** and **INIT CLASSIC**: a plain starting sound of each machine, for sound design from scratch. They suit the slot: on a kick, tom, conga or cowbell slot a clean sine thump at the slot's pitch; on a snare, clap or rim slot a sine and a noise burst; on a hat, cymbal or shaker slot a plain high-passed noise burst. No transient, no FM, no drive.
2. **The slot's own kind from every kit**, in kit order: on the snare slot HEX BOOM SNARE, HEX ACID SNARE ... 808 SNARE, 909 SNARE, 606 SNARE ... JAZZ SNARE, then the MY HEX and MY KIT snares.
3. **Every other kind**, KICK to COWBELL, each from every kit.

The kinds are the 16 slots every kit shares: KICK, KICK 2, SNARE, CLAP, HAT, OPEN HAT, PEDAL, RIM, SNARE 2, LOW TOM, HI TOM, CRASH, RIDE, SHAKER, CONGA, COWBELL. The HEX kits and MY HEX kits have the same 16; a sound in one of your kits counts as the kind of the slot it sits in there. The list goes round from the last to the first.

**Walkthrough: an 808 snare in the 606 kit.**

1. Start a new project (SAVE > PROJECT > NEW): the kit is the 606.
2. Turn **ALGORITHM** to **DRUM** and tap **EDIT**: the drum screen, SRC. The line under the big name says **606 SNARE** when SNARE is selected; turn **SELECT** until the header's chip says **SNARE**.
3. Turn **PRESETS** two clicks to the **left**. The band at the bottom says **SOUND 909 SNARE**, then **SOUND 808 SNARE** (SNARE 6/41): stop there. You heard each one. (The snares go HEX BOOM .. HEX ROOM, 808, 909, 606, 80S ...: right from the 606 are the 80S, VINTAGE and the rest.)
4. That's it: the snare slot is the 808's snare, the other 15 are still the 606's. HOME's DRUM zone now says **CUSTOM**. Tap **EDIT** to shape it: its pages are the CLASSIC machine's.
5. Changed your mind? Turn PRESETS back to 606 SNARE, or SAVE + OCT-.

**Starting from a machine instead**: turn PRESETS left from any sound until **INIT HEX** or **INIT CLASSIC** (they are the first two in the list), then tap EDIT and build the sound.

**Locks when a slot changes machine.** A lock on a send, the LFO or the pitch (NOTE) belongs to every machine and stays. A lock on one machine's own knob goes over to the other machine's knob of the same name (TUNE, CUTOFF, RESO, TYPE, DRIVE, DECAY, LEVEL, WAVE) at the same place in its range (a TYPE or WAVE only where the other has the same name); the rest are taken off, and the band says how many (2 LOCKS OFF, or 3 LOCKS MOVED). Turning back to the first sound brings every lock back as it was.

**The chokes belong to the slots**: whatever sounds sit in them, the HAT and PEDAL slots cut the OPEN HAT slot.

**Loading a whole kit.** On HOME (drums selected) PRESETS steps through the kits: five HEX kits (HEX BOOM, HEX ACID, HEX METAL, HEX GLITCH, HEX ROOM), 32 CLASSIC kits (808, 909, **606**, 80S, VINTAGE, TRAP, DRILL, BOOMBAP, LO-FI, PHONK, HOUSE, D.HOUSE, TECHNO, MINIMAL, ELECTRO, DISCO, GARAGE, JUNGLE, DUBSTEP, DEMBOW, AMAPIANO, AFRO, LATIN, TRIBAL, SYNTHWV, CHIP, ARCADE, GLITCH, INDUSTR, HYPER, AMBIENT, JAZZ), then your kits. A kit replaces all 16 slots. If the kit is still exactly the kit you loaded, it loads at once (nothing to lose). If you changed any slot (HOME says CUSTOM), the first click asks **HOLD SAVE: LOAD 808**: keep turning to choose another, hold SAVE until the ring fills to load it; the kits after that load at once. This is the only drum question. SAVE + OCT- brings your changed kit back.

**Your kits: SAVE > KITS** (tap SAVE four times). Six places, **MY KIT 1 to 6**, each a whole kit of 16 slots of any machines. KNOB 1 picks one; turn **KNOB 2 LOAD**, **KNOB 3 ERASE** or **KNOB 4 SAVE** to the right. SAVE into an empty place stores at once; over a kit, and ERASE, ask first (hold SAVE). LOAD asks only over a changed kit, as PRESETS does. Your kits are in the PRESETS lists too (whole on HOME, sound by sound on the drum screen). Until you save over them, places 1 to 4 hold your old **MY HEX 1 to 4** (marked OLD), which load as they always played.

**Patterns A to D share the project's one kit** (since wave 12). A project of waves 3 to 11 whose patterns played different kits now plays every pattern on the kit of the project as it was saved (the kit playing then, with its edits): switching to a pattern that had another kit keeps these 16 slots, so that pattern's hits play on them (its trigs, locks of the sounds and velocities as they were; a lock of the kit number does nothing). Load that other kit by hand (PRESETS on HOME) or keep it as MY KIT n to switch to it.

**Old projects** load with the same sound: the kit they played (with your copy of it, from waves 3 to 11) becomes the 16 slots, and their locks stay. If a project had a copy of a kit it was not playing, that copy is kept as one of your kits in a free place (MY KIT 5 or 6), and the screen says which.

---

## 6. PERFORM: the pads

PERFORM works the way the MPC's pad effects do: each key is one effect on the whole mix, on while you hold it; several play at once; a latch keeps one on; the four knobs set the last pad you pressed. Since wave 13 it is Ben's set (23 Nov, 14:13: "If you could play all those mutes and fx in one place that would be a power house for keeping beats varied in daft punk style drops"): the effects on the white keys, the mutes and four extras on the black keys.

**In and out.** Hold **FX**: PERFORM shows while you hold it and the keys are pads. Let go and the keys play notes again. Hold FX for about 0.7 seconds untouched and let go, and it stays open (LOCK on the line under the header); tap FX again or any page button to leave. Tapped, FX opens this voice's own effects first (INS), then SENDS, SLICER and the whole mix's MIX pages. PLAY and REC work while it is open. Pads never play or record notes.

**The pad map**, as the screen draws it (Ben, 23 Nov 2026, 22:07: "Group them by type into columns as much as possible. With the exception of the black keys which make sense being a row"). Under the line with the last pad's name, the eleven black keys sit on **one row**, in key order, as the black keys sit above the white ones; their names are short so they all fit. Under them, the sixteen white keys are **four columns of four**, one kind of effect in each, with its name over the column. The keys run **top to bottom** in a column, as on the drum grid: column 1 is keys 1 to 4, column 2 keys 5 to 8, and so on. Every pad is on the same key as before; only the drawing changed.

```
 black keys   F#3  G#3  A#3 | C#4    D#4    F#4    G#4   | A#4      C#5    D#5     F#5
 on screen    1    2    3   | KIK    TOP    S+C    H+S   | REV      STP    DN      UP
 the pad      V1   V2   V3  | KICKS  TOPS   SH+CL  HT+SN | REVERSE  STOP   SWP DN  SWP UP
 colour       purple, blue, | teal: the drum mutes        | white and grey: the whole mix
              coral         |                             |

 white keys   REPEAT         TIME           FILTER          COLOUR
              key 1  1/1     key 5  1/16    key 9  ISO LO   key 13 CRUSH
              key 2  1/2     key 6  HALF    key 10 ISO MID  key 14 PHASER
              key 3  1/4     key 7  ECHO    key 11 ISO HI   key 15 JET
              key 4  1/8     key 8  VERB    key 12 MOTION   key 16 VOWEL
```

REPEAT is the beat repeats, TIME the sixteenth, half time and the two throws, FILTER the isolator and the motion filter, COLOUR the crusher, phaser, flanger and vowel filter. On the black row, 1 2 3 are the three voices' mutes in their colours (as the voices are numbered in the bar at the foot of the screen), KIK TOP S+C H+S the drum mutes, REV STP DN UP reverse, tape stop and the two sweeps. A black key not pressed is drawn black, with a line under its name; pressed, it fills like a pad (latched: a white dot at its foot). The caption names the pad in full (MUTE VOICE 1, SWEEP DOWN).

| Key | Pad | Does |
| --- | --- | --- |
| key 1 (F3) | **1/1** | replays the bar before (the sequencer: every voice plays the last bar again; needs the transport running) |
| key 2 (G3) | **1/2** | replays the half bar before |
| key 3 (A3) | **1/4** | repeats the last quarter note, on the grid |
| key 4 (B3) | **1/8** | repeats the last eighth |
| key 5 (C4) | **1/16** | repeats the last sixteenth |
| key 6 (D4) | **HALF** | half time: the mix at half speed |
| key 7 (E4) | **ECHO** | throws the mix into the delay; it rings out after you let go |
| key 8 (F4) | **VERB** | throws the mix into the reverb; it rings out |
| key 9 (G4) | **ISO LO** | isolate the lows: only the mix below 220 Hz |
| key 10 (A4) | **ISO MID** | isolate the mids: only 220 Hz to 1.1 kHz |
| key 11 (B4) | **ISO HI** | isolate the highs: only above 1.1 kHz |
| key 12 (C5) | **MOTION** | a band-pass that sweeps up and down in time with the beat |
| key 13 (D5) | **CRUSH** | a subtle bit crusher: a little thin and papery, gritty |
| key 14 (E5) | **PHASER** | a phaser on the whole mix |
| key 15 (F5) | **JET** | a jet flanger on the whole mix |
| key 16 (G5) | **VOWEL** | a vowel (formant) filter on the whole mix |
| F#3 | **V1** | mute voice 1 |
| G#3 | **V2** | mute voice 2 |
| A#3 | **V3** | mute voice 3 |
| C#4 | **KICKS** | mute the kicks and the low tom |
| D#4 | **TOPS** | mute every drum but the tops: snare, clap, the hats, shaker |
| F#4 | **SH+CL** | mute every drum but the shaker and the clap |
| G#4 | **HT+SN** | mute every drum but the hats and the snare |
| A#4 | **REVERSE** | the mix backwards |
| C#5 | **STOP** | tape stop |
| D#5 | **SWP DN** | a low-pass sweeping down, then held |
| F#5 | **SWP UP** | a high-pass rising, then held (the riser) |

**The isolator** (Ben: "isolate, as in - I want to hear this band"; "Daft punk values of 220hz bass 1.1khz mids, high 1.1khz +"). Hold one ISO pad and you hear only that band; hold two and you hear both; hold all three and you hear the whole mix again. The bands are Linkwitz-Riley crossovers (24 dB an octave), so they add back up to the mix: all three held measure within 0.1 dB of the dry level at every frequency, and two held are exactly the mix without the third. Pressing and letting go crossfades (no click). The three pads share their knobs: **LOW X** (the low crossover, 220 Hz to start; 80 to 500 Hz), **HIGH X** (1.1 kHz to start; 600 Hz to 4 kHz) and **AMOUNT** (100 %: the other bands gone; 50 %: down 6 dB).

**The mutes** (Ben: "Hold to mute, latch with OCT+"). Each mutes its part while you hold it, and OCT+ latches it like any pad. A muted part keeps playing unheard, so it comes back exactly in place, with a short fade (about 6 ms) so nothing clicks. The drum mutes go by the 16 slots:

| Mute | Silences | Leaves |
| --- | --- | --- |
| **KICKS** | KICK, KICK 2, LO TOM | everything else |
| **TOPS** | KICK, KICK 2, RIM, LO TOM, HI TOM, CRASH, RIDE, CONGA, COWBELL | SNARE, SNARE 2, CLAP, HAT, OP HAT, PEDAL, SHAKER |
| **SH+CL** | every slot but two | SHAKER, CLAP |
| **HT+SN** | every slot but five | HAT, OP HAT, PEDAL, SNARE, SNARE 2 |

Snare means SNARE and SNARE 2; RIM is not a snare here. Hold several drum mutes and what each mutes adds up: you hear only what **all** of them leave (KICKS with HT+SN: the hats and the snares). A muted kick no longer pumps the voices through DUCK. These mutes are PERFORM's own: they never change the mixer's mutes and solos (hold GLO), and a part is silent if either mutes it.

**The extras.** **REVERSE** and **STOP** are as before (KNOB 1 LENGTH / TIME, KNOB 4 AMOUNT). **SWP DN** sweeps a low-pass from open down to its RANGE, **SWP UP** a high-pass from the bottom up to its RANGE (a high-pass rising is the classic riser: the mix thins out towards the drop). Each takes its LENGTH to get there, in time with the tempo, and holds there until you let go; let go and the mix comes back smoothly (about 50 ms).

- The repeats lock to the grid. **1/1** and **1/2** are too long for the 0.74-second buffer, so the sequencer replays the bar or half bar before; they need the transport running.
- **ECHO** and **VERB** throw the mix into the whole mix's delay and reverb; they ring out after you let go. The delay's time and the reverb's size and damp are the FX button's pages, not the pads'.
- One time effect plays at a time (a repeat, HALF, REVERSE, STOP or JET: the last pressed; let it go and an earlier one still held comes back), with every colour effect (the ISO pads, MOTION, CRUSH, PHASER, VOWEL, the sweeps), the throws and the mutes on top.

**Latch.** Hold **OCT+** and press a pad, or press **OCT+** while holding pads: they stay on with no key held (a white dot on the pad; **PERF** and how many in every header; their names on HOME). Press a latched pad again to turn it off. **OCT-** unlatches every pad (with pads held: only theirs); STOP does too. Latched pads keep playing after you leave PERFORM. The line under the header says it: [-] OFF [+] LATCH.

**MASTER** while FX is held turns **FX MIX**, the whole mix's effect return (the header's chip names it).

**The screen.** The strip's top row holds the seven mutes, each in its voice's colour (V1 purple, V2 blue, V3 coral, the drum mutes teal: a mute acts on that voice only); the row under it the four extras, white and grey like every effect (the whole mix), each over a column of the pads. Held: filled (a mute in its colour, an effect white); latched: dimmer with a white dot; played back by the STUTTER lane: a white bar under it; the knobs' pad outlined. The line under the header names the pad you pressed last and says HELD, LATCHED, FROM LANE or LAST PAD.

**The knobs** set the last pad pressed that has knobs (pressing a mute leaves the knobs where they were). KNOB 4 is AMOUNT (the wet level) wherever a pad has one.

| Pad | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| 1/4, 1/8, 1/16 | PITCH (-12 to +12) | DECAY | GATE | AMOUNT |
| HALF, REVERSE | LENGTH | | | AMOUNT |
| ECHO | | FDBK | | AMOUNT |
| VERB | | | | AMOUNT |
| ISO LO, MID, HI (shared) | LOW X (220 Hz) | HIGH X (1.1 kHz) | | AMOUNT |
| MOTION | RATE (1/4 to 2/1 bars; 1/1 to start) | DEPTH (how many octaves it travels) | RESO | AMOUNT |
| CRUSH | BITS (9 to start) | RATE (2: half the sample rate) | THIN (20 %: the lows a touch down) | AMOUNT |
| PHASER | RATE (1/4 to 4/1; 1/1) | DEPTH | FDBK | AMOUNT |
| JET | RATE (2/1: two bars a sweep) | FDBK (80 %) | | AMOUNT |
| VOWEL | VOWEL (U O A E I, and between) | RATE (OFF, or a sweep through the vowels and back in 1/4 to 2/1) | RESO | AMOUNT |
| STOP | TIME | | | AMOUNT |
| SWP DN, SWP UP | LENGTH (1/4 a beat to 4/1 four bars; 1/1 a bar) | RESO | RANGE (where it ends, in Hz) | SLOPE (12 or 24 dB) |
| 1/1, 1/2, the mutes | | | | |

At their start CRUSH is subtle (BITS 9, RATE 2, THIN 20 %: the difference from the dry mix measures about 18 dB down); turn BITS down and RATE up for more. MOTION at its start sweeps from 150 Hz up about four octaves and back once a bar.

The knob settings last until power-off and are not saved, except those recorded into the STUTTER lane.

**From an older project.** A STUTTER lane recorded before wave 13 holds pads that have moved or gone. On loading, each recorded pad becomes the pad it is now where that is honest: LP becomes SWP DN, HP becomes SWP UP, KILL DRUMS becomes SH+CL (the drum mute that leaves least), KILL BASS becomes V1 (or V2, V3 if its VOICE knob picked them), FLNG becomes JET, 1/32, 8T and 16T become the lane's own repeats at those lengths (section 7), and the rest keep their pad. GATE, DROP and LCUT have no pad now: they are dropped, and the screen says how many (LANE: 2 OLD PADS DROPPED).

---

## 7. The STUTTER lane

The STUTTER lane is PHEBUS's FX track: a sequencer part that plays no notes, and no voice. Its lit steps repeat a slice of what you just heard, and the PERFORM pads you play while recording are recorded into it.

**How to get there** (Ben, 23 Nov 2026, 15:55: "Both on FX as you've recommended but don't walk the PERFORM page"): **tap FX** until the tab row's **STUT** is lit: on a synth voice INS, SENDS, SLICER, MIX, then STUT (the fifth tap from anywhere else); on the drums FILTER, INS, SENDS, SLICER, MIX, then STUT. It is a whole-mix screen: the tag **MIX**, the title **STUTTER**, white and grey, whichever voice is selected. The voice stays selected (its name underlined in the strip): ALGORITHM still picks 1, 2, 3 or DRUM and the lane stays on screen. FX again goes round to the voice's first category; any other button leaves it as from any category. PERFORM is not on the way: it is **hold FX** only, from here too.

| You do | Result |
| --- | --- |
| **tap an unlit step** | it lights with the length for new steps (1/4 to start) |
| **tap a lit step** | it goes out when the key comes up, unless KNOB 1 turned |
| **hold a step + KNOB 1** | that step's length: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 (right: shorter) |
| **hold a step + OCT-** | the recorded pads over that step go (a step that press lit goes out again) |
| **KNOB 1 LEN** | the length for new steps |
| **KNOB 2 ON** | the lane ON or OFF, steps and recorded pads kept (HOME's KNOB 3 too) |
| **KNOB 3 PADS** | how many pads are recorded (up to 31); turn it: HOLD SAVE: CLEAR PADS; hold SAVE until the ring fills: every recorded pad is cleared |
| **KNOB 4 STEPS** | the lane's length, 1 to 64 |
| **OCT-**, **OCT+** tapped | page (they go round; the length is KNOB 4's) |
| **SAVE + OCT-** | undo an edit, an erase, a clear, or a take's pads |

**The black keys repeat** (Ben: "The black keys on the stutter sequencer should just do stutters / beat repeat fx"). While the lane is on screen, the white keys stay its steps and the black keys are hand-played repeats of the whole mix, exactly as PERFORM's stutter pads: hold one and the mix repeats at that length, let go and it comes back. The strip under the header names them:

| Black key | F#3 | G#3 | A#3 | C#4 | D#4 | F#4 | G#4 | A#4 | C#5 | D#5 | F#5 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Repeat | 1/1 | 1/2 | 1/4 | 1/8 | 1/16 | 1/4T | 1/8T | 1/16T | 1/8D | 1/16D | 1/32 |
| In the strip | 1/1 | 1/2 | 1/4 | 1/8 | 1/16 | 4T | 8T | 16T | 8D | 16D | 32 |

The five straight lengths sit on the first octave's five black keys (one hand shape); the triplets and the dotted lengths on the next octave's, 1/32 at the top. 1/1 and 1/2 replay the sequencer, as PERFORM's. Hold two and the last pressed plays; let it go and the other comes back. OCT+ held as you press one latches it (OCT- in PERFORM, or STOP, lets it go). They light while they play, and they are recorded into the lane while the sequencer records, like PERFORM's pads (the bar names the length: 1/8T). A key you play wins over the lane's own lit steps while you hold it (as a PERFORM pad does), and the steps take over again when you let go.

**The screen.** The 16 steps as tiles. Lit steps in a row with the same length are one loop and are drawn as **one span** across them, with the length named once at its start ("1/4"; a single step shows "4", "8", "16", "32", "2" or "1"). A line says what the lit steps do (or STEP n and its length), then the **recorded pads** as named bars under the steps they cover, in the same two rows of 8 and the same columns as the steps (steps 1 to 8 the first row, 9 to 16 the second); the line's right says **PADS** and how many, with "+n" for those a row cannot show; a bar lights while it plays and turns white over a held step.

**How it plays.** Steps are 1/16 and ignore swing. A lit run repeats the span played just before it, the whole mix. 1/4 to 1/32 are audio stutters; **1/2 and 1/1 replay the sequencer** (every voice plays the half bar or bar before again, and carries on afterwards from where it would have been). A pad you play live comes before the same pad recorded, and any pad before the lit steps.

**Recording on the lane.** **Tap REC on STUT** and the take is the lane's alone: no voice is armed (the strip shows no red dot; the header's red disc says it records). Playing, it records at once; stopped, the REC screen says **press play / stutters, pads: lane** (a key is a lane step, so no note starts it): PLAY starts the take. Turning ALGORITHM during it does not arm a voice. REC again stops it. REC anywhere else arms the selected voice, as ever, and its take records the pads into the lane too.

**What the other buttons do on STUT.** Hold **SEQ**, **ARP**, **EDIT**, **GLO**, **SAVE**, **ENV**, **LFO**: their layers, on the voice still selected, as on any FX page (hold ARP + OCT+: that voice's pattern up a semitone; hold EDIT + OCT-: HOLD SAVE: CLEAR VOICE 1). Hold SEQ's OCT- / OCT+ (halve, double) are refused on the lane, dark: its length is its own KNOB 4. **Hold FX**: PERFORM over it. **PRESETS** and **SELECT** do nothing here. **SAVE + OCT- / OCT+** undo and redo the lane's own edits. A **lane step held**: OCT- lit (its pads), the other steps and the black keys lit, every page button dark and ignored until you let go (as with a step held on the GRID).

**Recording pads.** While the sequencer records (REC while playing, any voice, or the lane's own take), each pad you play is written as a span: which pad, from press to release (to the nearest 1/16), and the knob you turned while it played. A latched pad records from latch to unlatch. A pad recorded over its own earlier span replaces it; other pads overdub. The 32nd says LANE FULL: 31 PADS.

---

## 8. Patterns A to D, recording and saving

### Patterns

A **pattern** holds all four voices' sounds and steps, the kit and the STUTTER lane. There are four, **A to D**. **SEQ plays them, SAVE stores them** (Ben: "The sequence button should be in charge of patterns"; storing is a keeping job, SAVE's).

| Control | Does |
| --- | --- |
| **SEQ, SEQ, SEQ** (the A-D category) | KNOB 1 the pattern to play; **OCT- / OCT+** the one before / after. Playing: on the next bar (NEXT: A). Stopped: loaded (LOADED A). Empty: EMPTY A |
| **hold SEQ + F#3, G#3, A#3, C#4** | play A, B, C, D (drawn A B C D in the strip; the one playing lit), the quick way |
| **SAVE five times** (the PATTERNS category) | the four patterns, STORED or EMPTY, the one playing marked. KNOB 1 picks one (OCT- / OCT+ too), **KNOB 2 STORE** turned right stores into it; over a stored one it asks (HOLD SAVE: STORE A): hold SAVE until the ring fills |
| **hold SAVE + key 1 to 4** | the shortcut: store into A to D. Over a used pattern it asks (HOLD SAVE: STORE A): SAVE is already held, so the ring fills at once; let go before and nothing is stored |

While a pattern plays, what you change in it is kept in it. **Tap SAVE** on its own for the SAVE pages.

### Recording live

| You do | Result |
| --- | --- |
| **tap REC, playing** | records now |
| **tap REC, stopped** | arms it (REC READY); recording starts on your first note, or after a count-in. On a synth voice's steps: STEP REC |
| **tap REC again** | stops or cancels |
| **hold REC** | nothing more (Ben: "I don't like the record button taking things away"). To clear the selected voice: **hold EDIT and press OCT-** (HOLD SAVE: CLEAR VOICE 1), then hold SAVE until the ring fills; SAVE + OCT- brings it back |

The REC screen: KNOB 3 **START** (NOTE or COUNT); the length is SEQ's PATTERN page (or hold SEQ). Notes are quantised to the voice's DIV. Rolls are recorded as retrigs. PERFORM pads go into the STUTTER lane. REC tapped on FX's STUT records the lane alone (no voice armed; stopped, PLAY starts it).

**Erase a note or a sound** (it was hold EDIT): hold its **key** on a screen where the keys play (HOME, a sound page), then press **OCT-**: playing, it goes from each step the playhead passes while you hold the key; stopped, from the whole pattern. SAVE + OCT- undoes it.

**Hold SEQ** (RHYTHM): the white keys **ROLL** (a held key repeats at KNOB 1's RATE: 1/8, 1/16, 1/32, 32T, 1/64; on the drums RETRIG, 1/1 to 1/80), **KNOB 2 SHIFT** moves the pattern a step later or earlier, **KNOB 3 GATE** (a synth voice; the drums have none), **KNOB 4 STEPS**; F#3 to C#4 play the patterns, D#4 the fill bar, **PLAY** the fill; **OCT-** halves the pattern, **OCT+** doubles it as a clone; **MASTER** the swing (its one place on this screen). The pattern's GATE, DIV and SWING are also SEQ's PATTERN page (and SHIFT is its 2/2 too).

**Hold ARP** (NOTES): OCT- / OCT+ transpose the selected voice's pattern a semitone (it was hold EDIT's KNOB 3 TRANSP); MASTER its TRANS. Tap ARP for the arpeggiator pages (ARP: MODE, RATE, OCT, GATE; ARP 2: SWG, PROB, HOLD, ORD).

### Saving, switching off, and the bank

Ben (22:45): "Just enough so that I can keep one in the bank. So one live one and one dead one. Or even just the option of turning the device off and on again without having lost what I was working on".

**What the FM-1 keeps by itself.** The **live project** (all four voices, the kit, the STUTTER lane, and patterns A to D) saves itself to flash, but **only while stopped**: a flash write stalls the sound for about 50 ms, so nothing is written while the sequencer plays.

| What | Written to flash | Switched off before that |
| --- | --- | --- |
| the working project (sounds, steps, kit, tempo, lane) | 0.3 s after you stop (once the sound has died away under -60 dB and the panel is untouched); a change made while stopped: 2.5 s after your last touch. At most one save every 5 s | lost back to the last save |
| patterns A to D (hold SAVE + key 1 to 4, or SAVE > PATTERNS) | stored at once in memory (SAVED A), written once stopped, quiet and untouched for 1.5 s | a pattern stored while playing and not yet written is lost |
| the bank (SAVE > PROJECT > KEEP) | at once, while stopped (it refuses while playing) | nothing to lose: KEEP writes before it says KEPT |
| your kits (SAVE > KITS), user presets (SAVE > USER) | at once, when you store them | nothing to lose |
| settings (lights, MIDI, SYNC, the MY HEX kits) | with the patterns: once stopped, quiet and untouched for 1.5 s | the last change |

So the safe habit is the one PHEBUS asks for anyway: **stop, wait a second, then switch off**. Switching off **while playing** loses everything since the last stop (edits, takes, a pattern stored while playing, a tempo change). The FM-1's power switch is not seen by the firmware (it cuts the power in hardware), so there is no last-moment save. An update or UBOOT writes everything first.

**How to tell.** The **white dot** in the header (bigger since the bank) means the project in memory is not yet in flash. **Hold SAVE** and the screen says it in words: PROJECT: SAVED, or PROJECT: NOT SAVED YET (in white), or STOP TO SAVE while playing. SAVE > PROJECT says the same next to LIVE.

**Hold SAVE** (the patterns' layer), in plain words since 22:45:

```
 PRESS KEY 1-4: STORE PATTERN                   [-] UNDO [+] REDO
 [ A STORED ] [ B PLAYING ] [ C EMPTY ] [ D EMPTY ]
 PROJECT: SAVED
 IT SAVES ITSELF WHEN YOU STOP
 KEEP A COPY: SAVE > PROJECT
```

**The bank: SAVE > PROJECT** (tap SAVE once: it is SAVE's first category). One more project, kept in flash beside the live one: "one live one and one dead one". The page shows both, so you can tell them apart:

```
 LIVE                    SAVED
 4 PATTERNS  92 BPM
 KIT: CUSTOM
 BANK
 2 PATTERNS  120 BPM            (or EMPTY: NOTHING KEPT YET)
 KIT: 808
```

| Knob, turned right | Does |
| --- | --- |
| **KNOB 1 KEEP** | copies the whole live project (its four patterns, kit, sounds, lane) into the bank. Into an empty bank: at once (KEPT IN THE BANK). Over a kept project it asks **HOLD SAVE: REPLACE THE BANK** (red ring) |
| **KNOB 2 SWAP** | the live project and the bank change places, so you can go back and forth between two sketches; nothing is ever lost (SWAP again is where you started). It asks **HOLD SAVE: SWAP WITH THE BANK** with a white ring. With an empty bank: THE BANK IS EMPTY: KEEP FIRST |
| **KNOB 4 NEW** | a fresh live project (the 606, 90 BPM, patterns A to D emptied); **the bank is untouched**. It asks **HOLD SAVE: NEW, BANK KEPT** (red ring) |

There is no LOAD: SWAP does it without losing the live one (to load the bank over the live one, SWAP, then KEEP). All three work only while stopped (STOP PLAYING FIRST). A power cut during KEEP or SWAP never loses either project: SWAP is one small write that turns a pointer, and KEEP writes the new copy beside the old one and switches over only at the end (tested with a cut after every single write).

- **Not in a project**: SYNC, the MIDI settings, the lights and the menu settings belong to the FM-1.

Projects saved by PHEBUS use a new format: PHEBUS reads older KARMA projects, stock KARMA cannot read PHEBUS's.

---

## 9. FX, GLO and the mixer

### The FX button: this voice, then the whole mix

Ben: "Voice FX, then mix FX". **Tap FX** for the selected voice's own effects, in its colour: **INS** (its three inserts, SEL: FX 1, 2, 3), then **SENDS**, then **SLICER** (on the drums the same order: the kit's INS, the selected sound's SENDS, the kit's SLICER). One more press is **MIX**: the whole mix's effects, white and grey with the tag **MIX**, after a divider in the tab row, whichever voice is selected. One more is **STUT**, the **STUTTER lane** ([section 7](#7-the-stutter-lane)), the whole mix's stutter sequence (Ben, 23 Nov 2026, 15:55: "Both on FX"). FX again goes round to INS. **Hold FX** for PERFORM ([section 6](#6-perform-the-pads)): PERFORM is never a tap category ("The perform page is only in the long press for FX button").

| FX taps (synth voice) | 1 | 2 | 3 | 4 | 5 | 6 |
| --- | --- | --- | --- | --- | --- | --- |
| category | INS | SENDS | SLICER | MIX | STUT | INS again |
| colour | the voice's | the voice's | the voice's | white | white | |

| MIX page (SEL) | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **DELAY** | TIME | FDBK | COLOUR | MIX (its return) |
| **REV/CHO** | SIZE | DAMP | RATE (the chorus's) | DEPTH (the chorus's) |
| **MASTER** | DUST | DUCK | | |

There is one delay, one reverb and one chorus for the whole FM-1. Each voice sends to them from its own **SENDS** page; each drum sound from its own FX > SENDS; PERFORM's ECHO and VERB throw into them. DUST is an old sampler and record; DUCK pumps the synth voices on every kick. On the drum screen the MIX pages show in the same place (the keys stay steps; the knobs are not lockable there); from a synth voice's steps, FX with nothing held opens the real MIX pages.

### GLO: the settings

Tap **GLO** for its categories, one a press: **GLOBAL**, **SYSTEM**, **MIDI** (SEL: MIDI 2, MIDI 3), **DEVICE** (the FM-1's own settings, [section 10](#10-settings-midi-and-usb)), then GLOBAL again; the tabs read GLOB SYS MIDI DEV. The tag is **GLO**, white and grey. Each knob is a row: its number, its name and its value at the right; the dials under say the same. Each setting has one home: MIDI OUT and IN are on MIDI 2 only.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **GLOBAL** | BPM (40 to 240): **the tempo, its one place** | CLICK (OFF, REC, ON) | TUNE | |
| **SYSTEM** | SYNC | CPU (or the USB state) | | |
| **MIDI** | CH1 | CH2 | CH3 | CH4 |
| **MIDI 2** | IN | OTHER | OUT | BEND |
| **MIDI 3** | KNOBS | **LEARN** (MIDI LEARN on / off) | **CLEAR** (forget every learned CC: turn right, hold SAVE) | |
| **SCREEN** (DEVICE) | COLOUR | ZOOM | | |
| **LIGHTS** (DEVICE) | DISCO | LIGHTS | KEYS | NOTES |
| **AUDIO** (DEVICE) | LOWCUT | USB (MIX, FULL, STEMS) | SERIAL | |
| **TOOLS** (DEVICE) | CALIB (the hardware calibration: turn right) | ABOUT (turn right) | | |

### The mixer (hold GLO)

Hold **GLO** for the mixer (hold it 0.7 seconds untouched and let go to keep it). Since wave 13 it is one map, whichever voice is selected (Ben, 14:19: "This global screen is kind of hard to figure out ... What voice is the keys?"): the **16 white keys are the 16 drum sounds**, the black keys **F#3 G#3 A#3 C#4 are voice 1, 2, 3 and the drums**.

The 16 tiles are the drum grid (Ben, 21:10: "make notes 1-4 mute the drum voices in the left column, 5-8 ... column 2, 9-12 ... column 3 and 13-16 ... the right hand column"), each key down its column, top to bottom:

| | Column 1 | Column 2 | Column 3 | Column 4 |
| --- | --- | --- | --- | --- |
| **top row** | HAT (key 1) | OP HAT (key 5) | RIDE (key 9) | CRASH (key 13) |
| **row 2** | PEDAL (key 2) | CONGA (key 6) | C-BELL (key 10) | SHAKER (key 14) |
| **row 3** | KICK 2 (key 3) | LO TOM (key 7) | HI TOM (key 11) | CLAP (key 15) |
| **bottom row** | KICK (key 4) | RIM (key 8) | SNARE 2 (key 12) | SNARE (key 16) |

```
 black keys   F#3 V1   G#3 V2   A#3 V3   C#4 DRUM          (the other seven black keys: nothing, not drawn)
 white keys   1-4      HAT  PEDAL  KICK 2  KICK            (the left column, top to bottom)
              5-8      OP HAT  CONGA  LO TOM  RIM          (column 2)
              9-12     RIDE  C-BELL  HI TOM  SNARE 2       (column 3)
              13-16    CRASH  SHAKER  CLAP  SNARE          (the right column)
```

**Not the order you play in.** On the mixer the keys count down the columns (Ben's 21:10 order, kept at 22:46), as PERFORM's pads do; when you play the drums by hand, RETRIG or turn SELECT, they count the Rytm's way, along the bottom row first (key 1 KICK, key 4 SNARE, key 13 HAT, key 16 CRASH). The tiles are the same picture either way. In the simulator, playing by hand: **Z** KICK, **X** RIM, **C** SNARE 2, **V** SNARE, **B** KICK 2, **N** LO TOM, **M** HI TOM, **,** CLAP, **.** PEDAL, **/** CONGA, **Q** C-BELL, **W** SHAKER, **R** HAT, **Y** OP HAT, **U** RIDE, **O** CRASH.

| Control (GLO held) | Does |
| --- | --- |
| **tap a key** | mute that drum sound (the table above: key 4 the KICK, key 16 the SNARE) or voice (yellow M); again: unmute |
| **hold OCT+ and tap a key** | solo it (ice-blue S): "plus = only this"; again: off. Several solos add up |
| **OCT-** | every mute and solo off |
| **SEL** | the second page: the knobs set the pans (L, C, R); SEL again: the levels |
| **KNOB 1 to 4** | the levels (or the pans) of voices 1, 2, 3 and the drums |
| **MASTER** | the **TEMPO** |

**A solo is a solo.** Solo anything, a voice or a drum sound, and only what is soloed is heard: solo the snare and voices 1 to 3 go quiet with the other drums; solo voice 1 and the drums go quiet. Solo the DRUM key (C#4) to hear every drum and no voice. A muted voice or sound fades out in a few milliseconds; a muted voice's pattern runs on silently. The mutes and solos are performance state (OCT- clears them all).

**The screen.** The line under the header says what a tap does: TAP A KEY: MUTE, and the -/+ legend at its right: [-] CLEAR, [+] SOLO. Under it, in the black-key strip's place, the four voices, each over its knob's column: its name in its colour and its level as a bar with the meter running in it. Then the 16 drum sounds as 4 x 4 tiles in the drum grid (cymbals and hats at the top, KICK bottom left, SNARE bottom right; keys 1 to 4 the left column, top to bottom), each named in teal; the tile that changes is the one under the key you pressed. Muted: the tile or cell yellow with an M in a black tab; soloed: ice blue with an S; silenced by a solo elsewhere: dark grey. The four dials are the levels (LEVEL) or, after SEL, the pans (PAN), each in its voice's colour.

**In the simulator.** Hold GLO: keep the **6** key down, or double-click (or shift-click) GLO on the page so it stays down (click it again to let go). Mute: click a key, or press its computer key (the white keys **Z X C V B N M , . /** then **Q W R Y U O**; F#3 G#3 A#3 C#4 are **S D F H**). For the drum sounds that is:

| Computer key | Z | X | C | V | B | N | M | , |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sound | HAT | PEDAL | KICK 2 | KICK | OP HAT | CONGA | LO TOM | RIM |
| **Computer key** | **.** | **/** | **Q** | **W** | **R** | **Y** | **U** | **O** |
| Sound | RIDE | C-BELL | HI TOM | SNARE 2 | CRASH | SHAKER | CLAP | SNARE |
 Solo: hold **]** (OCT+) as well, or shift-click OCT+ on the page so it stays down, then click the key (shift-click OCT+ again to let it up). Clear: **[** (OCT-). The pans: **2** (SEL).

---

## 10. Settings, MIDI and USB

### DEVICE: the FM-1's own settings

GLO's last category, **DEVICE** (tap GLO until it shows). Its four pages are normal GLO pages, with the header GLO, the tab row and the dials: **SEL** goes to the next page (round), KNOB 1 to 4 set the rows, OCT- / OCT+ step the knob last turned, GLO goes on to GLOBAL. These settings belong to the FM-1, not to a project, and are kept once the music stops.

| Page | Knobs |
| --- | --- |
| **SCREEN** | COLOUR (the palette, its colours shown), ZOOM |
| **LIGHTS** | DISCO, LIGHTS, KEYS (OFF, C, WHITE, ALL), NOTES |
| **AUDIO** | LOWCUT (the speaker's), USB (USB audio: MIX, FULL, STEMS), SERIAL (USB serial) |
| **TOOLS** | CALIB (the hardware calibration), ABOUT |

USB and SERIAL take effect at the next start (the row says RESTART until then). CALIB and ABOUT open with a turn right; ABOUT shows PHEBUS, "BASED ON KARMA 3.1.5" and the credits, and OCT- goes back to TOOLS. MIDI LEARN's CLEAR is on GLO > MIDI 3, beside LEARN.

### The lights and DISCO

Every button and key has an LED with three levels: **lit**, a **glow** (about a twenty-fourth as bright) and a **backlight** (fainter still; the menu's LIGHTS sets it). The knobs have none.

**While you hold something** (a button, OCT- or OCT+, a step, a key, a pad), every button and key that does something together with it is lit and everything else is dark, the backlight too; the button you hold stays lit. A dark button does nothing with what you hold (pressed, it is ignored). A question waiting (HOLD SAVE: ...) lights SAVE alone, pulsing. Black keys light only where the screen's strip draws them (and as notes where the keys play notes); a black key with no job stays dark even pressed.

| Hold | Lit |
| --- | --- |
| **SEQ** | PLAY (fill), OCT- OCT+ (halve, double; with PLAY held: unlatch, latch), the white keys (roll / retrig), F#3 G#3 A#3 C#4 D#4 (A B C D BAR) |
| **SAVE** | OCT- OCT+ (undo, redo), keys 1 to 4 (store) |
| **EDIT** | OCT- (clear the voice) |
| **ARP** | OCT- OCT+ (a semitone; synth voices) |
| **GLO** | the 16 white keys (the drum sounds), F#3 G#3 A#3 C#4 (the voices), SEL (the pans), OCT- while anything is muted or soloed (clear) |
| **FX** | every key (all 27 are pads since wave 13); OCT+ while a pad is held (latch), OCT- while something is latched |
| **LFO / ENV** (mapping) | SEL (the other source), OCT- (remove) |
| **OCT- / OCT+** | the other (octave 0) |
| **a step** | the page buttons with another page to show (EDIT ENV LFO FX SEQ), SEL, OCT- (erase), the other steps |
| **a key (a note, a sound)** | OCT- (erase it from the pattern) |

**DISCO** (GLO > DEVICE > LIGHTS, KNOB 1; kept with the device's settings, not the project; NORMAL to start): how much the lights move.

| Light | QUIET | NORMAL | SHOW |
| --- | --- | --- | --- |
| the page's button, the held button, REC recording, the octave | glow | lit | lit |
| the hints of a hold (above) | backlight | lit | lit (they win over the show) |
| the confirm's SAVE | glow, still | pulses | pulses |
| a layer kept open (LOCK) | glow, still | blinks | blinks |
| PERFORM: pads held / latched | glow, still | lit / blinking | lit / blinking |
| the steps (GRID, drum screen, lane), the step held, the keys held | glow | lit | lit |
| the play head over the steps, the lane's blinking steps | off | blinks | blinks |
| PLAY on each beat, the count-in's beats | off (PLAY glows while playing) | flashes | flashes |
| REC armed | glow, still | blinks | blinks |
| drum hits on the keys, NOTES (the synth's notes) | off | lit | lit |
| the landmarks (keys 1 5 9 13), the KEY page's chord keys | backlight | glow | glow |
| menu LIGHTS, KEYS backlight | at most LOW | as set | as set; every button's backlight HIGH |
| the keys dancing: every voice's notes, the drum hits, the step running along the white keys, the mix's level up from the lowest key | off | off | on (nothing held, keys playing notes) |
| HOME's FLOW band | stands still | flows | flows |
| the screensavers | only when opened | only when opened | open by themselves after a minute untouched on HOME while playing |
| the boot screen (3 s) | as always | as always | as always |

DISCO is a ceiling over LIGHTS, KEYS and NOTES: they still set what they set, but QUIET holds the backlight at LOW at most and turns NOTES off whatever it says (the menu shows NOTES grey then). **QUIET also dims the screen** (Ben's answer): everything on it is drawn at about half its light (9/16), still readable; NORMAL and SHOW bring it back. The FM-1's screen light itself is on or off only (a plain switch line in its wiring), so the dimming is done in the picture; it does not save battery.

### Screensavers

The screensaver is HOME's second category: on HOME tap **HOME** again (the **>** after the deck's FLOW band): one of the five opens, picked at random, never the one shown last, with a small grey caption for a couple of seconds ("1/5 FLOW   SEL: NEXT"). **SEL** goes to the next one: **FLOW**, **EYES**, **DESERT**, **THIRD EYE**, **BEACH**, each driven by the four voices' levels in their colours. **BEACH** is the four characters on a tropical beach by a trippy ocean: while the song plays each dances on the beat as long as its own voice sounds (the crocodile with the drums) and hops on its notes; otherwise they sway gently. With DISCO on QUIET the beach stands still. HOME again is HOME; any other page button closes it. Nothing is drawn across them as a line; once in a long while (about every half minute at most, a kick very rarely) a few rows slip sideways for a moment. At power-on a boot screen of about 3 seconds shows one of the five at random, a different one each time, with PHE and BUS in black (over BEACH: its sea and sky, without the characters); any key skips it.

### MIDI

- **CH1 to CH4** (GLO > MIDI): the channels of voices 1, 2, 3 and the drums (1, 2, 3 and 10 to start). OFF: nothing in or out.
- **IN** (MIDI 2): one channel plays the selected voice, NOTES (each channel its voice), or CLOCK only. **OTHER**: SEL or OFF. **OUT**: KEYS (what you play) or SEQ (the sequencer too). **BEND**: 1 to 12 semitones.
- **KNOBS** (MIDI 3): what a controller's knobs do: PANEL, AUTO, 1-8, 70-77 or GM.
- **SYNC** (GLO > SYSTEM): INT, USB or TRS. START plays from the top, CONTINUE carries on, STOP stops; with no clock pulse for half a second, the FM-1 goes back to its own tempo.
- **CCs** (with MIDI 3 KNOBS on GM; PANEL takes 70 to 77 for the FM-1's knobs): 5 GLIDE, 7 LEVEL, 10 PAN, 71 RESO, 72 release, 73 attack, 74 the voice filter's CUTOFF (on the drum channel: the drum voice's FILTER), 75 decay, 91 / 93 / 94 the reverb, chorus and delay sends (the SENDS page), 64 sustain, 120 and 123 all sound and all notes off, 121 reset.
- **Program Change** loads the voice's n-th sound (on the drum channel, kit n). **MIDI LEARN**: GLO > MIDI 3, KNOB 2 LEARN right (LEARN in every header), turn a knob (or SELECT) on screen, move the controller's knob; KNOB 2 left ends it.
- MIDI comes in on the 3.5 mm TRS jack (try the other adapter type if nothing plays) or USB from a computer or phone. A USB keyboard plugged straight in cannot work. No Bluetooth MIDI.

### USB audio

The FM-1 is a class-compliant USB audio input. MENU > AUDIO > USB AUDIO: **MASTER** follows the MASTER knob, **FULL** is a fixed level, **STEMS** (next start) gives six channels: each voice mono plus the effects return in stereo, at 44.1 kHz. The USB name is still **FM-1 KARMA**.

---

## 11. Backup and install

**Back up first.** The backup page (`web/editor.html`, Chrome or Edge, FM-1 on USB) has three buttons: **Connect**, **Save backup** and **Restore**. A backup holds the working project with its kit, the four patterns, the 32 user presets, the MY HEX kits, your drum kits MY KIT 1 to 6 (since wave 13), the project kept in the bank (since the bank: while it holds one) and the settings. A backup made before wave 13 (without your kits) still restores; it leaves the kits on the FM-1 as they are; one made before the bank leaves the bank as it is. Save one before you flash anything. A format 8 backup is refused by firmware older than its protocol.

**Build.** See [BUILDING.md](BUILDING.md): `./build.sh` with JieLi's toolchain and the AC79 SDK (on GitHub Actions or in Docker). The package is `build/felucca.fwsc`, with the identity **FM-1_926**.

**Install** (only after the checks in [section 14](#14-when-the-fm-1-arrives)):

- from the command line: `python3 tools/fm1_install.py build/felucca.fwsc` (needs `pip3 install mido python-rtmidi`); `--info` shows the connected FM-1's identity;
- or a local copy of the web installer, as BUILDING.md describes.

**Recovery.**

| Situation | What to do |
| --- | --- |
| it will not start | hold **OCT-** while switching on: PHEBUS USB RESCUE. Install again. Two failed starts in a row also come up in rescue |
| an install stopped half-way | the FM-1 waits in update mode: install again |
| buttons or knobs do the wrong thing | hold **OCT- and OCT+** while switching on, or MENU > SYSTEM > HARDWARE CALIBRATION |
| UBOOT is needed | hold OCT- and OCT+ for 5 seconds while stopped; let go to cancel |
| it no longer starts at all | recovery needs FM-1-transporter (see BUILDING.md) |
| back to the official firmware | save a backup, then M-VAVE's updater, M-UPGRADE, and the official firmware from m-vave.com (not tested with PHEBUS) |

---

## 12. Every gesture

The four ways ([section 2](#2-the-controls-and-the-screen)): a labelled button (again: its next category), SEL (PAGE: the next page inside), SELECT (knob 5: the type; on the drums the sound), and only then holds and combos. Every control has one idea (section 2's first table).

### The same gesture, the same meaning

| Gesture | Means, wherever it works |
| --- | --- |
| tap a labelled button | its group's first category; on it already, the next category (round). From the steps with nothing held: that button's real page |
| tap SEL | PAGE: the next page of the category shown (round); one page: nothing |
| SELECT | knob 5: the type of the page (synth); the sound (drum screen). No chip in the header: nothing |
| tap OCT- / OCT+ | the screen's minus / plus (the table below; the legend says what) |
| OCT- / OCT+ twice within 0.3 s | the octave down / up (CHROMATIC: its keys' octave); both at once: 0 |
| OCT- with a thing held | remove it: a step, its locks, a key's note or a drum sound's hits from the pattern, the held pads' latches; with EDIT held the voice (asks) |
| OCT+ with the thing held | latch it (PERFORM pads, the lane's repeats; the FILL: SEQ + PLAY + OCT+); on the mixer, OCT+ held with a key: solo it ("plus = only this") |
| hold a button | its layer: a performance layer or a shortcut; with OCT- / OCT+ its minus and plus; with MASTER its pair |
| hold a layer button 0.7 s, untouched, let go | keep the layer open (LOCK), every layer |
| hold SAVE (a question showing) | yes: the ring fills, it is done |
| hold a step + a page button / SEL + a knob | the lock view, a lock on that page: the only three-control gesture (Ben's Rytm lock) |
| a black key | its job only where drawn in the strip; elsewhere a note or nothing |

### What a single tap of OCT- / OCT+ does, screen by screen

| Screen | OCT- / OCT+ tapped | Legend |
| --- | --- | --- |
| the synth steps (TRIG, PATTERN), the drum screen (every tab), the STUTTER lane | the page of steps before / after (OCT+ on the last adds a copy of it; the lane goes round) | STEPS |
| SEQ's A-D | the pattern before / after (queued while playing, loaded stopped) | PATT |
| a step held (steps, drums) | OCT-: remove (the step on TRIG; on a lock view the last lock, every lock, then the step). OCT+: nothing | ERASE |
| a lane step held | OCT-: its recorded pads | ERASE |
| STEP REC | the step before / after (the red frame) | STEP |
| the drums' CHROMATIC | the sound before / after | SOUND |
| SAVE > PRESETS | the sound before / after (the drums: the kit) | SOUND (KIT) |
| SAVE > KITS | the kit place before / after | KIT |
| SAVE > USER | the slot before / after | SLOT |
| every other page (the sound pages, ARP, KEY, GLO, FX's MIX) | the knob last turned there (or SELECT) one fine step down / up; nothing turned yet: nothing | the knob's name (RESO, BPM, TYPE ...) |
| SAVE > TOOLS and SAVE > PROJECT (their knobs are GO knobs) | nothing | none |
| HOME, the screensaver, the REC screen | nothing | none |
| the menu | OCT- closes, OCT+ steps or opens a row (as written at its foot) | the menu's own line |
| a layer | the layer's own (the table in section 2) | at the right of the line under the header |
| the mixer (hold GLO) | OCT-: every mute and solo off; OCT+ held with a key: solo | CLEAR, SOLO |

### Anywhere

| Gesture | Does |
| --- | --- |
| tap PLAY | start or stop (with SEQ held: the FILL) |
| tap REC | record or arm (stopped, on a synth voice's steps: STEP REC) |
| hold REC | nothing more |
| tap HOME | HOME; on HOME the screensaver; on the screensaver HOME |
| hold HOME | nothing (the menu is GLO's DEVICE) |
| tap EDIT, ENV, LFO, FX, SEQ, ARP, GLO, SAVE | that button's categories, one a press (the button map, section 2) |
| tap SEL | the next page |
| ALGORITHM | the voice: 1, 2, 3, DRUM, on every screen (never the lane: FX's STUT) |
| PRESETS | the sound (drums: on the drum screen the selected slot's sound, on HOME and SAVE > PRESETS the whole kit) |
| MASTER | the volume; with a button held, its pair |
| a key held, then OCT- | that note or sound erased from the pattern |
| hold SAVE + OCT- / OCT+ | undo / redo |

### Sound pages and the synth steps

| Gesture | Does |
| --- | --- |
| EDIT / ENV / LFO / FX | OSC, FILT / AMP, FILT ENV / LFO 1, LFO 2, MATRIX / INS, SENDS, SLICER, MIX |
| SEL | OSC 1 to 4, FILT 1 to 2, LFO 1's sync page, FX 1 to 3, the MIX pages |
| SELECT | OSC: Type; FILT: TYPE; FX 1 to 3: TYPE; SLICER: mode; LFO 1, LFO 2: WAVE |
| SEQ | the steps: TRIG, again PATTERN, again A-D |
| a FILT ENV knob while LINK is AMP | the filter's envelope becomes OWN |
| keys | notes (sound pages); steps (SEQ) |
| steps: tap an empty / set step | set with the pen / clear on release |
| steps: a page button, nothing held | its real page (the steps go) |
| steps: hold a step + a page button / SEL | the lock view (LOCK: page); let go: TRIG |
| steps: hold a step + knob or SELECT | lock (TRIG: NOTE LEN VEL COND; SEL: FLAG RETRIG NUDGE) |
| steps: hold a step + OCT- | remove (TRIG: the step; a lock view: the last lock, every lock, the step) |
| steps: nothing held, TRIG 1/2 / 2/2 | NOTE (pen), STEP, HIT, VEL / SPICE, DICE |
| steps: OCT- / OCT+ tapped | pages of steps (OCT+ on the last adds one) |
| steps: tap REC | stopped: STEP REC; playing: LIVE REC |
| STEP REC: keys, SEQ, ARP, KNOB 2, OCT- / OCT+ | note or chord, rest, tie, the cursor, the step before / after |
| KEY pages, chord mode: black keys held | 3RD, 7TH, SUS4, 9TH, INV (drawn) |
| a sound page: hold LFO / ENV + a knob | map that LFO / envelope onto the knob (bipolar; 0 removes); SEL the other source; OCT- remove them here |

### Drum screen

| Gesture | Does |
| --- | --- |
| SELECT | the selected sound (the header's chip names it), in the Rytm order: KICK, RIM, SNARE 2, SNARE, KICK 2 ... CRASH, round |
| EDIT / ENV / LFO / FX / SEQ | SRC, FLTR, KIT FLT / AMP / LFO / INS, SENDS (the sound's), SLICER, MIX, STUT / TRIG, PATTERN, A-D (also with a step held) |
| SEL | the page inside (SRC 1/3 .., AMP 2/2, TRIG 2/2, PATTERN 2/2, the MIX pages) |
| tap an empty step / a hit | a hit / clear on release |
| hold a step + knob | lock on that sound's hit (on FX: the drum voice's own page) |
| hold a step + OCT- | remove (the last lock, every lock, the hit; on TRIG the hit) |
| OCT- / OCT+ tapped | pages of steps (A-D: the pattern) |
| tap ARP | CHROMATIC mode (OCT- / OCT+: the sound; twice quickly its octave; ARP again: off) |
| hold SEQ + key | RETRIG (KNOB 1 its rate) |
| hold SEQ + PLAY (+ OCT+) | FILL while held (latched) |
| PRESETS | the selected slot's sound (SOUND 808 SNARE; turn back: the sound you had; SAVE + OCT-: undo) |

### STUTTER lane

| Gesture | Does |
| --- | --- |
| tap FX (from INS, SENDS, SLICER, MIX) | reach it: STUT, after MIX; again: round to INS |
| tap a step | light it / put it out on release |
| a black key | repeat the mix at its length while held (1/1 1/2 1/4 1/8 1/16, 1/4T 1/8T 1/16T, 1/8D 1/16D, 1/32); OCT+ held: latch |
| hold a step + KNOB 1 / OCT- | its length / erase its pads |
| KNOB 1 to 4 | LEN for new steps, ON, PADS (turned: asks to clear; hold SAVE), STEPS |
| OCT- / OCT+ tapped | page |
| tap REC | the lane's own take (no voice armed); stopped: PLAY starts it |
| ALGORITHM | the voice changes, the lane stays |
| any other page button, HOME | leave, as from any category |

### PERFORM (hold FX)

| Gesture | Does |
| --- | --- |
| a white key | its effect while held (1/1 1/2 1/4 1/8 / 1/16 HALF ECHO VERB / ISO LO ISO MID ISO HI MOTION / CRUSH PHASER JET VOWEL) |
| F#3 G#3 A#3 / C#4 D#4 F#4 G#4 | mute voice 1, 2, 3 / KICKS, TOPS, SH+CL, HT+SN while held |
| A#4 C#5 D#5 F#5 | REVERSE, STOP, SWP DN, SWP UP while held |
| hold OCT+ + pad, or OCT+ with pads held | latch |
| a latched pad / OCT- | unlatch it / all (pads held: theirs) |
| KNOB 1 to 4 | the last pad's knobs |
| MASTER | FX MIX |
| FX (kept open), HOME, any page button | leave (latches play on) |
| hold FX 0.7 s untouched | keep it open |
| REC while playing | record the pads into the lane |

### Layers

| Hold | Gesture | Does |
| --- | --- | --- |
| SEQ | white key / black key 1 to 4 / D#4 / PLAY | roll (drums: RETRIG of that key's sound, the drum grid's order; the tiles drawn as the grid) / play A to D / fill bar / FILL |
| SEQ | KNOB 1 to 4 / OCT- OCT+ / MASTER | RATE, SHIFT, GATE, STEPS / halve, double (a clone) / SWING |
| ARP | OCT- OCT+ / MASTER | the pattern a semitone / TRANSP |
| GLO | white keys / F#3 G#3 A#3 C#4 / OCT+ held + a key / OCT- / SEL / KNOB 1 to 4 / MASTER | mute a drum sound (keys 1 to 4 the grid's left column, top to bottom: HAT PEDAL KICK 2 KICK ... key 16 SNARE) / mute a voice, the drums / solo it / clear all / the pans' page / levels (pans) / TEMPO |
| SAVE | keys 1 to 4 / OCT- OCT+ | store A to D / undo, redo |
| EDIT | OCT- / MASTER | clear the voice (asks) / LEVEL |
| ENV, LFO | MASTER | ENV LENGTH, MOD DEPTH |
| ENV, LFO (a synth sound page) | KNOB 1 to 4 / SEL / OCT- | the source's amount onto the knob (MAP) / the other LFO, envelope / remove its mappings on the page |
| any (a layer, a step, a key) | another button | lit: does something with it; dark: nothing (pressed, ignored) |
| SEL, HOME, REC | | nothing |
| any | hold 0.7 s untouched, let go | keep it open (LOCK) |

### Menu, screensaver, power-on

| Gesture | Does |
| --- | --- |
| SEL, KNOB 1 to 3, PRESETS, OCT+ / OCT- (menu) | section, rows, cursor, open / close |
| GLO, HOME (menu) | GLOBAL / HOME |
| SEL (screensaver) | the next one |
| hold OCT- while switching on | USB rescue |
| hold OCT- and OCT+ while switching on | hardware calibration (also DEVICE > SYSTEM) |
| hold OCT- and OCT+ 5 s, stopped | update mode (UBOOT) |

### Where a gesture means something else, and why

- **SELECT on the drum screen** picks the sound, not a type: Ben's choice. The drum sound's own WAVE, MODEL and filter TYPE stay on knobs.
- **The octave is a double tap** of OCT- or OCT+: the one double press, Ben's choice, so that a single tap is always the screen's minus or plus.
- **A drum key held, then OCT-** erases that sound's hits. OCT- held first and then a key is just the hit (no ghost hit since wave 12: ghosts come from TRIG's velocity).
- **OCT+ on the STUTTER lane's last page** goes round to page 1 instead of adding a page: the lane's length is KNOB 4's.
- **The drum voice's ARP** has one category, CHROMATIC: pressed again it ends it.
- **The mixer's keys** (wave 13) are the same whichever voice is selected: the white keys the 16 drum sounds, F#3 to C#4 the four voices. OCT+ is held with a key to solo (no SOLO mode to remember).
- **The drum screen draws no steps** (Ben, 23 Nov 13:35): the keys and their lights are the steps on every drum tab; the screen shows the sound instead. The synth voices' steps show on SEQ, where their note names and chords are drawn (the keys cannot show those).
- **Power-on holds** (rescue, calibration, update mode) are the loader's and the recovery's, not the UI's: left as they are.
- **Three controls at once**, the only ones: hold a step, change the page with a page button or SEL, turn a knob (a lock on another page). Everything else is one or two controls.

---

## 13. Known limits

- **Nothing has been run on a real FM-1.** Sound, timing, the processor's headroom and the screen's speed are unmeasured. Everything here comes from the code and the host tests.
- **Do not flash until the unit has arrived and its chip revision is checked.** Newer FM-1 units have a changed chip and can be bricked.
- **PHEBUS_ARENA_T12** (a setting of the device build only): tracks 1 and 2 have less memory for an oscillator than track 3. On the FM-1 itself, six Types (6-OP A, B, C, STRING, SPEECH, PARTICLE) play on track 3 only; on tracks 1 and 2 they are silent and the OSC page says NOT ON THIS TRACK. The simulator has no such limit.
- **Polyphony**: eight notes shared by the three synth voices; JUNO up to six; ACID and the heavy Types one.
- **No velocity or pressure on the keys**: they are plain switches. Set the drums' velocity on TRIG (KNOB 1 with nothing held: the keys and new hits; a step held: its VEL), or play PHEBUS from the MPC's pads over MIDI.
- **The PERFORM time effects and the STUTTER lane are mono** (16-bit, 12 dB of headroom kept); the buffer is 0.74 seconds, and below about 84 BPM a quarter-note loop is cut to 0.72 seconds.
- **Limits per pattern or voice**: 24 locks a synth voice, 128 drum sound locks and 64 hits with extras a pattern, 19 matrix cells a voice, 31 recorded pads in the lane, one kit of 16 slots a project, six kits of your own (SAVE > KITS).
- **Mutes, solos and PERFORM's knob settings** are not saved.
- **The newer, slower flash chip** (UC25HQ80, in FM-1s sold from late 2026): PHEBUS waits up to 2 seconds for it to erase and 50 ms to write a page, and the sound stops for as long as that takes. If the chip is still busy after that, the FM-1 restarts by itself rather than carry on (what was not yet saved is lost; your saved work is safe). The installer's own update loader still uses the old, shorter waits: on a very slow chip an install could stop half-way, and installing again carries on from where it stopped. After ChoralRoot 0.32's finding (by Quixotic7; the fix is felucca-bsp's, by Julius Vitkauskas).
- **Old projects** are converted when loaded; some conversions are approximate (listed in DECISIONS.md under "For Ben to confirm").
- The ABOUT page and the USB name still say KARMA (the version PHEBUS was forked from).

---

## 14. When the FM-1 arrives

Do these in order, before and after the first flash:

1. **Check the chip revision** of the unit. If it is a newer revision, do not flash.
2. **Note the official firmware version** the unit came with, and keep M-UPGRADE and that firmware to hand.
3. **Build** PHEBUS with the JieLi toolchain and check the build's report: the image, `.data+.bss` and the pool must fit, with the pool's 8 KiB rule met.
4. **Install**, then check the boot screen, the USB rescue (hold OCT- at power-on) and the hardware calibration.
5. **Listen**: each Type on each track (on tracks 1 and 2, the six Types above should say NOT ON THIS TRACK), the filter against a 303 and an SH-101, the 606 kit.
6. **Load**: the default mix with a six-note chord on voice 2, three heavy Types at once, five PERFORM pads stacked. Watch GLO > SYSTEM, KNOB 4 (CPU) and listen for dropouts.
7. **Timing**: drum nudges and retrigs, the MIDI clock to and from the MPC Sample, the STUTTER lane's 1/2 and 1/1 replays.
8. **Saving** (part one): that an autosave never stalls the screen or the sound, that the -60 dB silence threshold suits the real noise floor, patterns A to D across a power cycle, KEEP and SWAP and a power cycle after each, the backup page against the new firmware. **The flash chip:** if the unit has the newer UC25HQ80 flash (JEDEC 0xB36014), the flash driver's waits (500 ms an erase, 10 ms a page) may be too short for it (ChoralRoot 0.32 found so); until the driver is changed, a save may fail or the FM-1 may restart during one. The two copies of every project mean a failed save loses only that save, never the copy before it.
   **Saving, the flash chip:** with MENU > AUDIO > USB SERIAL on (it takes effect at the next start), the serial console's `status` names the flash chip and counts its slow writes (`FLASH: UC25HQ80 LATE 0 (max 0 ms)`); save a few times and check again: LATE above 0 means the slower chip at work, so note the time it shows. Then check that an autosave never stalls the screen or the sound, that the -60 dB silence threshold suits the real noise floor, patterns A to D across a power cycle, the backup page against the new firmware.
9. **The screen**: every screen at real size and speed, the screensavers and the boot's random pick.

---

## 15. Credits and licences

PHEBUS is Ben's fork of **KARMA** by Ariel Muchnik. KARMA is a fork of **SLOOP** by isod89, which is based on **Felucca** by Leo Kuroshita (Hügelton Instruments). It uses code from:

- **X0X** by Charles Vestal: the TB-303 voice (a port of Open303 by Robin Schmidt);
- **FM-106** by PatttF: the Juno-106 voice;
- **Mutable Instruments' Plaits and Elements** by Émilie Gillet: the PLAITS and ELEMENTS models, and DaisySP and Rings for the physical models;
- the **LXR-02 firmware** by Erica Synths: the HEX drum voices;
- **Melodee** by Kerem Kilic: the second core's audio;
- **JIANT FM** by Juan Jiant: the USB STEMS mode;
- msfa (Google, Pascal Gauthier), CrispyZebra (Leo Kuroshita) and the FM-1 Screen Reader's shared code (Charles Vestal), as listed in LICENSING.md.

The screen font is Terminus. The PHEBUS wordmark is drawn from TeX Gyre Adventor Bold.

**Licence**: GPL-3.0-only, with the additional permission and the other licences listed in [LICENSING.md](LICENSING.md); see also [LICENSE](LICENSE) and the files in `LICENSES/`. If you share PHEBUS or firmware made from it, share its complete source under the same licence. PHEBUS comes with no warranty. M-VAVE and FM-1 are trademarks of their owners. PHEBUS is not affiliated with or endorsed by M-VAVE, Arturia, Elektron, Akai, Mutable Instruments, Erica Synths or any of the projects above. The README of SLOOP is kept as [SLOOP-README.md](SLOOP-README.md).

---

## 16. Glossary

| Word | Means |
| --- | --- |
| **voice** | one of the four instruments: 1, 2, 3 (synth) and DR (drums). The screen always says VOICE, never TRACK |
| **step** | one place in a pattern, 1 to 64; a white key on the GRID, the drum screen and the lane. "STEP 5 HELD" |
| **trig** | what sits on a step: a note or a drum hit. TRIG is also the tab with the held step's own values |
| **category** | what a labelled button opens, one a press: EDIT's OSC and FILT, FX's INS SENDS SLICER MIX ... The tab row names them |
| **page** (of a category) | one screen of it: "2/4" at the right of the tabs; **SEL** walks them. (A page of steps: 16 steps, OCT- / OCT+) |
| **SEL (PAGE)** | the button that walks the pages of the category shown. Held: nothing |
| **SAVE** | keep and confirm: the SAVE pages (PROJECT, PRESETS, USER, KITS, PATTERNS, TOOLS); held, store a pattern, undo, redo, the confirm's ring |
| **SELECT** | the knob: knob 5, the type of a synth page (named in the header's chip); on the drum screen the sound |
| **type** | what SELECT turns: the oscillator Type, the filter's TYPE, an insert's TYPE, the LFO's WAVE, the SLICER's mode |
| **tab** | the row naming the categories of the last button pressed, the one shown filled |
| **lock** | a value set on one step only: hold the step and turn the knob. Drawn inverted |
| **condition** | when a step plays: -, FILL, NOFL, CHNC |
| **retrig** | a step repeated: a drum hit at a rate, a synth step 1 to 4 times in its time (RETRIG on both). Never RTRG or RATCH |
| **roll** | a held key repeated (hold SEQ on a synth voice; on the drums the same is RETRIG) |
| **layer** | what the keys, knobs, OCT- / OCT+ and MASTER do while a button is held |
| **PERFORM** | the pads layer, opened by holding FX: the effects on the white keys, the mutes and four extras on the black keys. Its effects are **pads** |
| **FX** | the button: this voice's own effects (INS, SENDS, SLICER; on the drums the kit's INS and SLICER around the selected sound's SENDS), then **MIX**, the whole mix's (DELAY, REV/CHO, MASTER), then **STUT**, the STUTTER lane; held, PERFORM |
| **MIX** | FX's whole-mix category, the shared effects, white; the tag of its pages and of the STUTTER lane; also the mixer's title (hold GLO) |
| **insert** | one of a voice's three FX slots, in series: its own effect |
| **send** | how much of a voice goes to the whole mix's delay, reverb or chorus: its SENDS page |
| **confirm** | hold SAVE until the ring fills, when a white bar says HOLD SAVE: ... (red ring: something goes; white: it only keeps) |
| **latch** | keep a pad or the FILL on with no finger on it: OCT+ with it held |
| **minus, plus** | a tap of OCT- / OCT+: the screen's minus and plus, named by the **-/+ legend** at the right of the tab row |
| **double tap** | OCT- or OCT+ twice within 0.3 s: the octave (the one double press) |
| **MASTER pair** | what MASTER turns while a button is held: FX MIX, SWING, TRANSP, LEVEL, ENV LENGTH, MOD DEPTH, TEMPO |
| **FX MIX** | the whole mix's effect return, 0 to 100 % (FX + MASTER) |
| **black-key strip** | the two rows under the header that name the black keys' jobs; a black key does something only where it is drawn |
| **mixer** | hold GLO: the 16 drum sounds on the white keys (the drum grid, keys down its columns), the voices on F#3 to C#4; a tap mutes, OCT+ with a key solos, OCT- clears, SEL the pans |
| **drum grid** | where the 16 drum sounds sit, as on an Analog Rytm: HAT OP HAT RIDE CRASH / PEDAL CONGA C-BELL SHAKER / KICK 2 LO TOM HI TOM CLAP / KICK RIM SNARE 2 SNARE; played by hand, by RETRIG and by SELECT the keys count the Rytm's way, the bottom row first (key 1 KICK, key 4 SNARE, key 16 CRASH); on the mixer they run down its columns (key 1 HAT, key 4 KICK, key 16 SNARE) |
| **bank** | SAVE > PROJECT: one more project kept in flash beside the live one; KEEP copies the live one into it, SWAP exchanges them, NEW starts the live one afresh |
| **KIT** | on the drums, the kit's own pages (INS, SLICER, EDIT's KIT FLT), acting on all 16 sounds: the header says KIT and SELECT does nothing there |
| **KIT FX** | each drum sound's switch on its SENDS page: ON through the kit's FILTER, INSERT and SLICER, OFF straight to the mix |
| **lock view** | a synth voice's steps with a step held and a page button pressed: that page's knobs under the steps (LOCK: in the tab row) |
| **A-D** | SEQ's third category: the patterns to play |
| **STUTTER lane** | the whole mix's stutter sequence, a part that plays no notes: FX's last category **STUT** (tag MIX, title STUTTER); on/off on HOME's KNOB 3 STUT |
| **pattern** | one of A to D: all four voices' sounds and steps, the kit and the lane. SEQ plays them, SAVE stores them |
| **kit** | the drum voice's 16 slots, the project's own; HOME names the kit you loaded, CUSTOM once a slot differs. Your kits: SAVE > KITS (MY KIT 1 to 6) |
| **slot** | one of the kit's 16 places (one per white key; SELECT picks it on the drum screen). It holds one whole sound of either machine |
| **sound** (drums) | what a slot holds: its machine, its parameters, sends, LFO and name ("808 SNARE"); PRESETS on the drum screen picks it |
| **HEX**, **CLASSIC** | the two drum machines: the LXR-02's voices, and the synthesised model of the 606, 808, 909 and the rest. Each slot plays with its own |
| **INIT HEX**, **INIT CLASSIC** | the plain starting sound of each drum machine, first in every slot's sound list |
| **Type** | the oscillator model, SELECT on OSC, 31 of them |
| **macro** | one of a Type's three knobs, M1 M2 M3 in the matrix |
| **mute**, **solo** | yellow M, ice-blue S. The mixer's: tap a key / OCT+ held with a key; a solo anywhere leaves only what is soloed. PERFORM's mutes (V1 V2 V3 KICKS TOPS SH+CL HT+SN) are separate, held or latched |
| **isolate**, **ISO** | PERFORM's ISO LO, ISO MID, ISO HI: hear only that band (below 220 Hz, 220 Hz to 1.1 kHz, above 1.1 kHz); two held: both; three: the whole mix |
| **MOTION** | PERFORM's band-pass that sweeps up and down in time with the beat |
| **sweep** | PERFORM's SWP DN (a low-pass closing) and SWP UP (a high-pass rising): to their RANGE in their LENGTH, then held |
| **repeat**, **stutter** | the mix replayed at a length (1/4, 1/8T ...): PERFORM's top pads, the lane's lit steps and its black keys |
| **map**, **mapping** | a source (LFO 1, LFO 2, the amp or filter envelope) moving a knob: hold LFO or ENV on a sound page and turn the knob; a cell of the MATRIX. Boxed while held, a small wave under the dial otherwise |
| **cell** | one source-to-destination amount in the MATRIX; 19 a voice besides the six fixed ones |
| **hint** | a button or key lit while you hold something: it does something with it |
| **DISCO** | GLO > DEVICE > LIGHTS: QUIET (dim, nothing moves), NORMAL, SHOW (the keys dance) |
| **glow**, **backlight** | the LEDs' two dim levels under lit |
| **voice colours** | 1 purple, 2 blue, 3 coral, drums teal |
| **GLO** | everything global (GLOBAL with the tempo, SYSTEM, MIDI with LEARN, DEVICE the menu) and, held, the mixer |
| **KEY** | ARP's second category: the key, scale and chords of the synth voices |

### Dictionary

One spelling for each word on the screen, six characters or fewer, UK English where the room allows (the code keeps them in one place, `firmware/src/words.h`). Tab labels are fixed too: PROJ PRST USER KITS PATT TOOL (SAVE), GLOB SYS MIDI DEV (GLO), INS SEND SLIC | MIX STUT (FX), OSC FILT (EDIT), AMP FILT (ENV), LFO 1 LFO 2 MTRX (LFO), TRIG PATT A-D (SEQ), SRC FLTR KFLT (the drums' EDIT), CHRM, HOME SAVR.

| Word | Means |
| --- | --- |
| **DETUNE**, **BRIGHT**, **STRUCT** | the synth knobs (never DTUNE, BRIT, STRC) |
| **PHASER**, **FLANGE**, **CHORUS**, **REVERB** | the effect types |
| **LP 24**, **LP 12** | the filter types |
| **FX 1**, **FX 2**, **FX 3**; **LFO 1**, **LFO 2** | always with the space |
| **COLOUR** | the delay's tone and the screen's palette |
| **SUS**, **REL** | the envelopes' sustain and release (ATTACK and DECAY in full) |
| **NS LVL**, **NS HLD**, **NS DEC**, **NS FRQ** | a drum sound's noise layer |
| **P AMT**, **P DEC** | a drum sound's pitch envelope |
| **SCALE**, **QUANT** | ARP > KEY |
| **DEPTH** | an FX slot's second knob (was INTENS) |
| **E SYNC** | the envelopes' times in note values |
| **ELEMNT** | the ELEMENTS engine where six letters are the room |
| **CLEAR** | SAVE > TOOLS's pattern clear, MIDI 3's LEARN clear |
| **SLICE** | the slicer's mode |
| **VOICE** | one of the four instruments, never TRACK |
