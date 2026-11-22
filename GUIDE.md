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
9. [GLO pages and the mixer](#9-glo-pages-and-the-mixer)
10. [Settings, MIDI and USB](#10-settings-midi-and-usb)
11. [Backup and install](#11-backup-and-install)
12. [Every gesture](#12-every-gesture)
13. [Known limits](#13-known-limits)
14. [When the FM-1 arrives](#14-when-the-fm-1-arrives)
15. [Credits and licences](#15-credits-and-licences)
16. [Glossary](#16-glossary)

---

## 1. What PHEBUS is

PHEBUS has four **voices**: three melodic voices and one drum voice. Each voice has its own sequencer part of up to 64 steps. On top of the four voices sit the **PERFORM** pads, effects you play on the keys like an MPC's pads, and the **STUTTER lane**, a fifth sequencer part that plays stutters and the pads you recorded.

| Voice | Head in the strip | Colour | Starts as |
| --- | --- | --- | --- |
| **1** | robot | purple | ACID INIT: a plain saw through the 303 filter, mono, an octave down. A bass |
| **2** | alien | blue | JUNO INIT: a plain saw, polyphonic. Chords |
| **3** | crocodile | teal | PLAITS VA: a plain saw, mono, about 6 dB quieter. A lead |
| **DR** | snake | coral | the 606 kit, 16 sounds |

Every voice starts with no effects on and an empty mod matrix. **SAVE > TOOLS > NEW** puts a project back to this start.

### The colour rule

Colour always means something:

- A voice's colour (purple, blue, teal, coral) means "this acts on that voice". On a voice's own pages everything is in its colour.
- White and grey mean shared: the screen acts on everything (PERFORM, the STUTTER lane, the GLO pages, the patterns).
- A yellow **M** means muted. An ice-blue **S** means soloed.
- Red is only for recording and erasing.

### Three things and nothing else

| Pillar | Works like | What you do |
| --- | --- | --- |
| **SYNTH** | MiniFreak | pick an oscillator Type, turn three macros, shape it with the filter and envelopes, add FX slots, route the mod matrix. The same pages on every voice |
| **DRUM** | Analog Rytm MKII | pick a sound, the 16 keys are its steps, hold a step and turn a knob to lock a value on it. Every sound has the pages SRC, FLTR, AMP, LFO |
| **PERFORM** | MPC pad effects | hold FX and the keys are effect pads on the whole mix: stutters, sweeps, tape stop, throws and kills, with latch |

---

## 2. The controls and the screen

### The FM-1's controls

| Control | Name in this manual | Main job |
| --- | --- | --- |
| Buttons | **PLAY**, **REC** | transport and recording |
| Buttons | **HOME**, **SAVE** | the HOME screen; the SAVE pages (presets, tools) |
| Buttons | **ENV**, **LFO**, **FX**, **SEL**, **EDIT**, **ARP**, **SEQ**, **GLO** | tap: open pages. Hold: a layer (below) |
| Buttons | **OCT-**, **OCT+** | octave; pages of steps; latch on PERFORM |
| Knob | **ALGORITHM** | choose the voice: 1, 2, 3, DRUM |
| Knob | **PRESETS** | the voice's sounds (the drums: the kit) |
| Knob | **SELECT** | walk the pages, tabs and halves; the tempo where there are none |
| Knobs | **KNOB 1** to **KNOB 4** | the four values on the screen, under the four dials |
| Knob | **MASTER** | the output volume |
| Keys | **key 1** to **key 16** | the 16 white keys, F3 to G5 |
| Keys | the **black keys** | F#3, G#3, A#3, C#4, D#4, F#4, G#4, A#4, C#5, D#5, F#5. "Black keys 1 to 4" are the first four |

SEL is the button between FX and ENV. SELECT is the knob.

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Note | F3 | G3 | A3 | B3 | C4 | D4 | E4 | F4 | G4 | A4 | B4 | C5 | D5 | E5 | F5 | G5 |

**Tap** means press and let go in under about 0.45 seconds without touching anything else. **Hold** means keep it pressed. **FX + key 3** means hold FX and press key 3. A value marked **GO** (LOAD, SAVE, NEW and so on) asks first: turn it one click and AGAIN: ... shows, turn it one more click within about 1.5 seconds to do it.

### Tap and hold: the layers

Tap a button to open its pages. Hold it and the keys and knobs do something else for as long as you hold it. That is a **layer**.

| Hold | Layer | For |
| --- | --- | --- |
| **FX** | PERFORM | the effect pads ([section 6](#6-perform-the-pads)) |
| **EDIT** | ERASE | erase notes or sounds, shift, double, transpose, undo |
| **ARP** | ROLL | note repeat (on the drums: RETRIG) |
| **SEQ** | STEPS | the pattern's GATE, DIV, SWING and STEPS |
| **SEL** | KEY | key, scale and chord (on the drums: SOUND, the sound pick) |
| **GLO** | MIX | levels, mute, solo, fill, pan, tap tempo |
| **SAVE** | PATTERN | play and store the patterns A to D |

A layer shows after about 0.14 seconds, so a tap does not flash it. To keep a layer open without holding it, hold its button and tap **HOME**: the line under the header says **LOCK**. Any other button lets it go, except PLAY, REC, OCT- and OCT+. **GLO** and **FX** also stay open if you hold them for about 0.7 seconds without touching anything and then let go.

### The screen

Every screen has the same frame, top to bottom:

1. **The header.** A boxed **tag** says what the screen acts on: **1**, **2**, **3** (a synth voice), **DRUM**, **HOME**, **STUT** (the STUTTER lane), **GLO** (shared pages), **REC**, or on a layer the button you hold (**FX**, **EDIT**, **ARP**, **SEQ**, **SEL**, **GLO**, **SAVE**). Then the title (the sound's name, the kit, the page or the layer), the tempo, a play or stop symbol, four beat lights (beat 1 red only while recording), a red disc while recording or a small white **unsaved dot**, and the battery. While a fill is on, **FILL** shows inverted in white; while PERFORM pads are latched, **PERF** and how many.
2. **The body.** What the screen is about: the steps, a graph, the pads.
3. **The tabs** (on the sound pages, the GRID and the drum screen): the pages of the voice, the one shown inverted, and "1/3" at the right for the half of the page shown. On a layer, a line here says what it does now; a message shows there as a white bar.
4. **The dials.** Four dials, one over each knob: the name above, the value beside it in one tall face, always in a fixed 40-pixel column, so a value never runs into the next one. The knob you are turning has its name inverted and its value underlined. A grey value does nothing in the current setting. A value **locked** on the step you hold is drawn **inverted**. A short message replaces the dials as a white bar.
5. **The strip.** The four voices' heads, numbered 1 2 3 DR. The selected voice is underlined and its number is in its colour. A yellow **M** means muted, an ice-blue **S** soloed, a red dot recording.

### HOME: three zones

Tap **HOME**. The header says **PATTERN A** (to D) while a pattern plays, otherwise PHEBUS. The body has a deck and three zones, one per pillar:

| Part | Shows |
| --- | --- |
| **deck** | BAR and the bar of the selected voice's loop; a strip of the **FLOW** backdrop, a band per voice in its colour that swells as that voice gets louder (it flows while something plays and stands still otherwise); the patterns **A B C D**: the one playing filled white, the one asked for next outlined white, the stored ones outlined grey |
| **SYNTH** | the three melodic voices side by side: each sound's name in its voice's colour, with **M**, **S** or a red dot, and under it the 16 steps in view (a note in the voice's colour, a tie dim, the step playing white). **The selected voice's name is inverted.** A muted voice's name is grey |
| **DRUM** | DRUM (inverted when the drums are selected), M or S for the drum voice, the kit (a * after the name when your copy plays), how many sounds MUTE mode has muted (yellow M and a number) or soloed (ice-blue S), the selected sound's name, and the 16 steps in view: a step with the selected sound's hit in coral, with other sounds' hits dim coral, with only muted sounds grey, the step playing underlined white |
| **PERFORM** | **LANE ON** or **LANE OFF** for the STUTTER lane, the latched pads by name at the right (or "3 LATCHED" when the names do not fit), and the lane's 16 steps in view: lit steps grey, a run of lit steps of one length drawn as one bar, the step playing underlined |

The HOME knobs play the selected voice while it plays:

| Knob | Does |
| --- | --- |
| **KNOB 1 CUTOFF** | the voice filter's cutoff (on the drums: the drum voice's own FILTER) |
| **KNOB 2 DECAY** | the decay of the envelope that moves the sound: the filter's own envelope's DECAY while it is OWN (as ACID INIT is), else the amp envelope's; on the drums the selected sound's decay, on every kit |
| **KNOB 3 STUT** | the STUTTER lane ON (turn right) or OFF (turn left). White-grey: it acts on the whole mix |
| **KNOB 4 LEVEL** | the voice's level. On a muted voice the first turn unmutes it |

All four values are in the same tall face as on every page. On HOME, **SELECT** sets the tempo and **PRESETS** browses the selected voice's sounds. Tap **HOME** again on HOME for a screensaver ([section 10](#10-settings-midi-and-usb)).

---

## 3. A first session, ten minutes

This takes you from a new project to a saved groove. Start with **SAVE > TOOLS > NEW** (turn KNOB 4 two clicks) if the FM-1 has something on it already.

**1. A drum pattern (two minutes).**

1. Turn **ALGORITHM** to **DRUM** and tap **SEQ**. The drum screen opens: the 16 white keys are the steps of the selected sound.
2. Hold **SEL** and press **key 1**: KICK is selected (you hear it). Let go of SEL. Tap keys **1, 5, 9 and 13**: four kicks.
3. Hold **SEL** and press **key 3**: SNARE. Tap keys **5 and 13**.
4. Hold **SEL** and press **key 5**: HAT. Tap keys 3, 7, 11 and 15.
5. Press **PLAY**.

**2. Lock a hit (one minute).** Hold **SEL** + **key 3** to select the snare again. **Hold key 13** (the second snare), tap **ENV** to show the AMP page, and turn **KNOB 2** (DECAY) right. Let go. Only that snare is longer: its value was drawn inverted while you held it, and its step now has a small black square. To take the lock off, hold the step and press **OCT-**.

**3. A bass line (two minutes).** Turn **ALGORITHM** to **1**. You stay on the GRID, now voice 1's. Press **PLAY** to stop. Tap **REC**: the title reads STEP REC and a red frame marks the step the next note goes on. Play a note on the keys: it lands on that step and the frame moves on. Press **SEQ** for a rest, **ARP** to tie the note before. Tap **REC** to finish. Press **PLAY**.

**4. Open the filter (one minute).** Tap **HOME**. While it plays, turn **KNOB 1 CUTOFF** and **KNOB 2 DECAY**. For the full filter page tap **EDIT** twice (OSC, then FILT): KNOB 2 is RESO.

**5. A chord track (two minutes).** Turn **ALGORITHM** to **2** (JUNO, polyphonic). Tap **SEQ** for its GRID, stop the song and tap **REC**. Hold three keys together and let go: the chord goes onto the step when the last key comes up. Write four chords, using **SEQ** for rests. Tap **REC** to finish and press **PLAY**.

**6. PERFORM pads, played and recorded (one minute).** While it plays, tap **REC** (recording starts at once). **Hold FX**: the keys are now pads. Press **key 3** (the **1/4** pad) for a beat, then **key 9** (**LP**, a low-pass sweep). Let go of FX and tap **REC** to stop. The pads you played are now in the STUTTER lane and play back every loop. **KNOB 3 STUT** on HOME switches them off and on.

**7. Save (one minute).** Stop the song. The working project saves itself a moment later: the white unsaved dot in the header goes out. To keep this groove as pattern A, **hold SAVE** and press **key 5**. Hold SAVE and press **key 1** to play pattern A again later.

---

## 4. SYNTH: the melodic voices

### The sound chain

Every synth sound is one chain of five stages, in signal order, the same on every voice whatever its Type:

**OSC** > **FILT** > **ENV** > **FX** > **MOD**

The oscillator makes the sound, the filter shapes it (before the amp, so a ringing filter dies with the note), the amp envelope shapes its level, three FX slots colour it, and the mod matrix moves it.

| Button | On the chain |
| --- | --- |
| **EDIT** | enters at OSC; on the chain, the next stage (MOD goes round to OSC) |
| **ENV**, **FX**, **LFO** | that stage's first page; again, its next page |
| **SELECT** | every page of the chain, one by one, stopping at both ends |
| **SEQ** | the GRID, on this page's tab (the keys become steps) |

On a sound page the keys play notes. Each page shows its subject large in the voice's colour and draws what the knobs do: the wave, the filter's response and its envelope's sweep, the envelopes, the FX slots, the LFOs, the matrix grid.

### Pages

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **OSC 1/4** | **TYPE** (31) | macro 1 | macro 2 | macro 3 |
| **OSC 2/4** | **OCT** (-3 to +3) | **TUNE** (-99 to +99 cents) | **GLIDE** | **VOICE** (POLY, MONO, LEG, UNI) |
| **OSC 3/4** | the Type's 4th control | the Type's 5th control | **DTUNE** (unison spread) | **PRIOR** (LAST, LOW, HIGH) |
| **OSC 4/4** | **GLMODE** (RATE: GLIDE is the time an octave takes; TIME: the time any glide takes) | **ALLOC** (ROT: notes take the voices in turn; REUSE: the nearest free one) | | |
| **FILT 1/2** | **CUTOFF** | **RESO** | **ENV** (how far the filter's envelope moves it, either way) | **TYPE**: LP24, LP12, BP, HP |
| **FILT 2/2** | **KEY** (the cutoff follows the note) | **DRIVE** | **LINK**: AMP or OWN | |
| **ENV 1/2** AMP | **ATTACK** | **DECAY** | **SUSTN** | **RELEAS** |
| **ENV 2/2** FILT | **ATTACK** | **DECAY** | **SUSTN** | **RELEAS** (the filter's own envelope) |
| **FX 1**, **FX 2**, **FX 3** | **TYPE** | TIME | INTENS | AMOUNT (named by the type) |
| **SLICER** | **SLCR** | **PAT** | **RATE** | **DEPTH** |
| **LFO** | **RATE** | **WAVE** | **PHASE** | **FADE** |
| **LFO SYNC** | **SYNC** | **TRIG** (NOTE, FREE, PATT) | **POL** | **ESYNC** (both envelopes in note values) |
| **LFO 2** | **RATE** | **WAVE** | **SYNC** | **TRIG** (NOTE, FREE, PATT; FREE to start) |
| **MATRIX** | **SOURCE** (row) | **DEST** (column) | **AMOUNT** | **CLEAR** (on AS1, AS2: **TARGET**) |

### The oscillator

KNOB 1 on OSC picks the **Type** from one list of 31, the same on every voice: ACID (the 303), JUNO (the Juno DCO), the Plaits models (VA, VA VCF, FOLD, PHASE D, 2-OP FM, 6-OP A, B, C, WAVETABLE, TERRAIN, ADDITIVE, CHORD, STR MACH, SWARM, FORMANT, CHIPTUNE, STRING, MODAL, NOISE, SPEECH, PARTICLE, KICK, SNARE, HI-HAT) and the Elements models (BOUNCE, PLUCKED, BOWED, BLOWN, OMINOUS). KNOB 2 to 4 are its three macros, renamed for each Type (VA: DETUNE, SQUARE, SAW; ACID: WAVE, ACCENT, SLIDE; JUNO: WAVE, PW, SUB). Turning TYPE changes only the oscillator: the filter, envelopes, FX and voice stay. Another engine's Type starts its macros at plain defaults. ACID plays one note at a time, JUNO up to six, the heavy models (6-OP, STR MACH, STRING, SPEECH, PARTICLE, BOWED, BLOWN, OMINOUS) one.

### The filter

One resonant filter on every voice: a four-pole ladder whose TYPE gives 24 or 12 dB low-pass, band-pass or high-pass. On ACID and JUNO, LP24 is their own ladder (the 303's, the Juno's IR3109); the line under the filter's name says which. RESO at the top self-oscillates. DRIVE pushes the sound into it. KEY at 100 % follows the note exactly.

**LINK** chooses the filter's envelope: **AMP** follows the amp ENV (one envelope for level and tone, the MiniFreak's quick way) or **OWN** uses its own ADSR on ENV 2/2. While it is linked, ENV 2/2 shows the amp values in grey; turning any of its knobs makes it OWN, starting from the amp values (FILTER ENV: OWN).

**LFO SYNC TRIG**: NOTE restarts the LFO at each new phrase, FREE never restarts it, PATT restarts it each time the pattern comes round to step 1.

### FX slots

Three slots in series after the filter, all OFF to start. KNOB 1 picks a slot's TYPE; KNOB 2 to 4 are named by the type:

| Type | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- |
| **CHORUS** | RATE (shared) | DEPTH (shared) | SEND |
| **PHASER**, **FLANGER** | RATE | FDBK | MIX |
| **REVERB** | SIZE (shared) | DAMP (shared) | SEND |
| **DELAY** | TIME (shared) | FDBK (shared) | SEND |
| **DIST**, **SOFT**, **HARD**, **FOLD**, **FUZZ** | TONE | DRIVE | MIX |
| **CRUSH** | RATE | BITS | MIX |
| **EQ** | LOW | MID | HIGH (-12 to +12 dB) |

There is one chorus, one reverb and one delay for the whole FM-1. A CHORUS, REVERB or DELAY slot is this voice's **send** to it (KNOB 4); its KNOB 2 and 3 are the shared effect's own settings (the GLO pages'), drawn white-grey because they change it for every voice. A voice can have one of each of CHORUS, REVERB, DELAY and FLANGER; KNOB 1 skips a type another slot already has.

### The mod matrix

MOD's last page is a grid of 7 sources by 13 destinations, six columns in view at a time.

- **Sources**: LFO1, LFO2, AENV (amp envelope), FENV (the filter's envelope), PENV (a fast pitch fall at each note's start), VEL, KEY.
- **Destinations**: PIT, M1 M2 M3 (the macros), CUT, RES, LVL (only ever down), PAN, FX1 FX2 FX3 (each slot's AMOUNT), AS1 AS2 (any lockable sound value, chosen with KNOB 4 TARGET).

A filled square is a positive amount, an empty one negative, bigger for more. Six cells are the routings shown on other pages too: LFO1 > PIT, LFO1 > CUT, LFO1 > LVL, FENV > CUT (FILT's ENV), PENV > PIT, KEY > CUT (FILT 2's KEY). Besides them a voice holds 19 cells; one more says MATRIX FULL: 19 CELLS.

### The GRID: steps on the keys

**Tap SEQ** for the selected voice's GRID. The 16 white keys are the 16 steps of the page shown. They play nothing. Under the steps: a line about the step held or under the cursor, the tabs **TRIG OSC FILT ENV FX MOD**, and the dials.

| You do | Result |
| --- | --- |
| **tap an empty step** | it is set with the **pen** (the last note or chord you played, or TRIG's KNOB 1) at velocity 100; you hear it |
| **tap a set step** | it is cleared when the key comes up, unless you turned a knob while holding it |
| **hold a step** | the step is selected: the line shows STEP n HELD, its note and its locks |
| **hold a step and turn a knob on a sound tab** | that value is **locked** on the step (drawn inverted; the tile gets a small square) |
| **hold a step + OCT-** | on a sound tab: the lock of the knob last turned goes; again: every lock. On TRIG: the step's locks, nudge and condition |
| **hold a step + OCT+** | the next condition: NORMAL, FILL ONLY, NO FILL, CHANCE |
| **EDIT**, **ENV**, **FX**, **LFO** | the OSC, ENV, FX, MOD tab; again, the next stage or page. Also with a step held |
| **SEQ** | the TRIG tab; again, back to the tab before |
| **SELECT** | every half of every tab, TRIG first |
| **black keys 1 to 4** | page 1 to 4 (steps 1-16 .. 49-64) |
| **OCT-**, **OCT+** (nothing held) | the page before or after; OCT+ on the last page adds a copy of it (up to 64 steps) |

**TRIG** with a step held: KNOB 1 NOTE (by scale degrees), LEN (in steps; the steps after become ties), VEL, COND (-, FILL, NOFL, CHNC). TRIG 2/2: FLAG (-, ACC, SLD, A+S), RATCH (x1 to x4), NUDGE. With nothing held: NOTE (the pen), STEP (the cursor), HIT (right sets the cursor's step, left clears it), VEL.

**Locks.** The Rytm's way: hold a step, turn whatever is on the knobs. The page buttons still change the tab while you hold it, so you can lock the cutoff, tap ENV and lock the decay in one hold. TYPE, an FX slot's TYPE, VOICE, PRIOR, the shared effects' settings and the matrix's cursor cannot be locked (grey while a step is held). A voice holds **24 locks**.

**Spice and Dice.** On TRIG 2/2 with nothing held (turn SELECT right on TRIG): KNOB 1 **SPICE** (0 to 100 %: how much variation you hear; 0 is the pattern as written) and KNOB 2 **DICE** (each click right a new roll, left the roll before). A roll leaves notes out, moves some an octave, adds ratchets and changes velocities and lengths; the notes you wrote never change. Each pattern A to D keeps its own.

**STEP REC.** With the song stopped, on a synth voice's GRID, tap **REC**. A red frame marks the step the next note goes on. Play a note or a chord (keys held together: written when the last comes up); the frame moves on. **SEQ** writes a rest, **ARP** a tie (a key held + SEQ: written and tied). OCT- and OCT+ set the octave, SELECT or KNOB 2 moves the frame, KNOB 3 the velocity, KNOB 4 the pattern length. EDIT + OCT- undoes the whole pass. Tap **REC** again to finish. With the song playing, REC on the GRID records live instead (LIVE REC).

**Undo.** **EDIT + OCT-** undoes, **EDIT + OCT+** redoes.

### Presets

**PRESETS** browses the voice's sounds, grouped by kind (BASS, KEYS, ORGN, PAD, LEAD, PLCK, STAB, FX, INIT), then your user presets. Each engine has an INIT sound. **Tap SAVE** for the SAVE pages: **PRESETS** (KNOB 1 the sound, KNOB 2 the engine), **USER** (KNOB 1 SLOT 1 to 32, KNOB 2 LOAD, KNOB 3 ERASE, KNOB 4 SAVE, all GO), **TOOLS** (KNOB 1 CLRSQ clears the pattern, KNOB 2 INIT resets the sound, KNOB 4 NEW, all GO).

### Key, scale and chords (hold SEL)

| Control | Does |
| --- | --- |
| **a key** | the key of the song (the root of all three synth voices) |
| KNOB 1 **CHORD** | OFF, TRIAD, 7TH, 9TH, SUS4, POWER |
| KNOB 2 **SCALE** | CHROM, MAJ, MIN, DOR, MIX, PEN, MPEN, HARM, PHRY, LYD, LOC, MEL, BLUES, WHOLE, DIMHW, DIMWH |
| KNOB 3 **KEYS** | OFF, SNAP (keys round to the scale), WHITE (white keys walk the scale, black keys silent) |
| KNOB 4 **TRANSP** | -24 to +24 semitones |

With a chord mode on, the white keys walk the scale from C4 and each plays the chord of its degree; the black keys change it while held: F# flips the third, G# adds the 7th, A# makes it sus4, C# adds the 9th, D# inverts it. Tap **SEL** for the SEL pages (ROOT, SCALE, QNT, CHORD; TRANS, STRUM, VLEAD).

---

## 5. DRUM: the drums

The drums work the Rytm's way. The drum voice has 16 sounds, one a key:

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sound | KICK | KICK 2 | SNARE | CLAP | HAT | OPEN HAT | PEDAL | RIM |
| **Key** | **9** | **10** | **11** | **12** | **13** | **14** | **15** | **16** |
| Sound | SNARE 2 | LOW TOM | HI TOM | CRASH | RIDE | SHAKER | CONGA | COWBELL |

The closed hat cuts the open hat. Holding OCT- while you hit a drum key gives a ghost hit (velocity 42), OCT+ a hard hit (127).

### The drum screen

Tap **SEQ** or **EDIT** with DRUM selected. The header shows DRUM and the kit. The body shows the **selected sound's name** large and how many steps it plays on (or STEP n HELD), the **sound map** top right (16 tiles; a sound with hits dim coral, lit as it plays, the selected one framed white, a yellow bar muted, ice blue soloed), the selected sound's **16 steps** (a hit filled, its shade its velocity; a black square: locks; three lines: a retrig; a notch: a condition), a line about the held step (RETRIG 1/32, LOCK DECAY ...) or which steps are shown with four page slots, the tabs **TRIG SRC FLTR AMP LFO**, and the dials in coral.

| You do | Result |
| --- | --- |
| **hold SEL + key** | select that sound (heard) |
| **tap an empty step** | a hit of the selected sound (velocity 100, or TRIG's KNOB 1 with nothing held) |
| **tap a hit** | it goes when the key comes up, unless a knob was turned |
| **hold a step and turn a knob** | lock that value on that sound's hit only (drawn inverted) |
| **hold a step + OCT-** | the lock of the knob last turned goes; again (or none turned): every lock. On TRIG: the hit's condition, chance, nudge, retrig, every lock and the drum voice's own locks on that step |
| **hold a step + OCT+** | the next condition |
| **EDIT** | SRC; again FLTR, AMP, LFO, SRC |
| **ENV**, **LFO** | AMP, LFO; again the page's next half |
| **SEQ** | TRIG; again back |
| **SELECT** | every half of every page, TRIG first |
| **black keys 1 to 4**, **OCT-**, **OCT+** | pages of steps, as on the synth GRID |
| **PRESETS** | the kit |

The page buttons and SELECT also work with a step held, as the Rytm's page keys do. With nothing held, a knob changes the selected sound.

### The sound pages

| Page | HEX kits (HEX BOOM .. HEX ROOM, MY HEX 1 to 4) | Synthesised kits (606, 808, 909 and the rest) |
| --- | --- | --- |
| **SRC 1/3** | TUNE, P.AMT, P.DEC, WAVE | TUNE, FINE, WAVE, TONE |
| **SRC 2/3** | MODEL, MOD, M.PIT, FM | SWEEP, SWTIME, PART2, P2 LVL |
| **SRC 3/3** | TRANS, T.LVL, T.PIT, PSLOPE (FINE on CYM, HAT) | NOISE, N.LVL, CLICK, CHIP |
| **FLTR** | FREQ, RES, TYPE, DRIVE | FREQ, RES, TYPE, ENV; 2/2 HPF |
| **AMP 1/2** | ATK, DEC, SLOPE, DIST | HOLD, DECAY, N.HOLD, N.DEC |
| **AMP 2/2** | CRUSH, DEL, REV, LEVEL | DRIVE, DEL, REV, LEVEL |
| **LFO 1/2** | SPEED, DEPTH, DEST, WAVE | the same |
| **LFO 2/2** | SYNC, FADE, PHASE, MODE | the same |

DEL and REV are the sound's own sends. Every sound has its own LFO: DEST is PITCH (up to 12 semitones), CUT, RES or LEVEL; MODE is TRIG (restarts at each hit), FREE or ONE (one cycle).

**Every kit can be edited.** The first knob you turn on a built-in kit makes your own copy of it in the project (YOUR COPY OF THE KIT; the kit's name gets a *, "606*"). The built-in kit never changes. A project holds one copy: turning a knob on another kit asks AGAIN: REPLACE 606*, and only a second turn within 1.5 seconds replaces it. Locks do not make a copy. MODEL cannot be locked.

### TRIG: the hit's own values

| TRIG | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **1/2** | **VEL** 1 to 127 | **RETRIG**: OFF, 1/1 .. 1/80 | **NUDGE** | **COND**: -, FILL, NOFL, CHNC |
| **2/2** | **CHANCE** 1 to 99 % | **LENGTH** of the retrig, 1/8 step to 16 steps or INF | **CURVE** of the retrig's velocity, -128 to 127 | **NOTE** (a pitch lock) |

With nothing held, TRIG's KNOB 1 is the velocity new hits get (100 to start); the other knobs say HOLD A STEP. A pattern holds 64 hits with a condition, chance, nudge or retrig (TRIG EXTRAS FULL: 64) and 128 sound locks (SOUND LOCKS FULL: 128).

### Modes and live play

- **MUTE mode**: tap **SEL**. The keys mute and unmute the 16 sounds (yellow M); **SEL + key** solos one (ice-blue S). Mutes and solos are not saved. Tap SEL again for the steps.
- **CHROMATIC mode**: tap **ARP**. The 27 keys play the selected sound at pitches, C4 (key 5) its own; OCT- and OCT+ move them an octave (-2 to +2). Recorded live, each hit keeps its pitch as a NOTE lock. Tap ARP again to leave.
- **RETRIG**: hold **ARP** and press a key: the sound repeats while held. The black keys pick the rate: F#3 1/4, G#3 1/6, A#3 1/8, C#4 1/12, D#4 1/16, F#4 1/24, G#4 1/32, A#4 1/40, C#5 1/48, D#5 1/64, F#5 1/80; KNOB 1 goes through all 17 (1/1 to 1/80).
- **FILL**: on the drum screen, hold **F#5** for a fill while held; press it twice quickly to latch it (twice again, or STOP, to end). FILL shows in every header.
- **The drum voice's own pages**: tap **FX** on the drum voice for its INSERT, INSERT 2 and SLICER, acting on the whole drum voice (SELECT left from INSERT reaches its FILTER). Hold a step and tap FX to lock those per step.

### Kits

**PRESETS** picks the kit: five HEX kits (HEX BOOM, HEX ACID, HEX METAL, HEX GLITCH, HEX ROOM), 32 synthesised kits (808, 909, **606**, 80S, VINTAGE, TRAP, DRILL, BOOMBAP, LO-FI, PHONK, HOUSE, D.HOUSE, TECHNO, MINIMAL, ELECTRO, DISCO, GARAGE, JUNGLE, DUBSTEP, DEMBOW, AMAPIANO, AFRO, LATIN, TRIBAL, SYNTHWV, CHIP, ARCADE, GLITCH, INDUSTR, HYPER, AMBIENT, JAZZ), then MY HEX 1 to 4.

---

## 6. PERFORM: the pads

PERFORM works the way the MPC's pad effects do: each key is one effect on the whole mix, on while you hold it; several play at once; a latch keeps one on; the four knobs set the last pad you pressed.

**In and out.** Hold **FX**: PERFORM shows while you hold it and the keys are pads. Let go and the keys play notes again. Hold FX for about 0.7 seconds untouched and let go, or hold FX and tap HOME, and it stays open (LOCK on the line under the header); tap FX again or any page button to leave. PLAY and REC work while it is open. Pads never play or record notes.

**The pad map.** The 16 white keys are four rows of four (keys 1 to 4 the top row); the black keys are the chips above them.

| Row | Key 1, 5, 9, 13 | Key 2, 6, 10, 14 | Key 3, 7, 11, 15 | Key 4, 8, 12, 16 |
| --- | --- | --- | --- | --- |
| 1 | **1/1** replay the bar | **1/2** replay the half bar | **1/4** repeat | **1/8** repeat |
| 2 | **1/16** repeat | **1/32** repeat | **REVRS** reverse | **HALF** half speed |
| 3 | **LP** low-pass sweep | **HP** high-pass sweep | **CRUSH** bit crush | **GATE** trance gate |
| 4 | **STOP** tape stop | **DROP** pitch drop | **ECHO** echo throw | **VERB** reverb throw |

| Black key | Pad |
| --- | --- |
| F#3 | **-DRM** kill the drums |
| G#3 | **-BAS** kill the bass (voice 1; KNOB 1 picks the voice) |
| A#3 | **LCUT** low cut |
| C#4 | **FLNG** flanger sweep |
| C#5, D#5 | **8T**, **16T** triplet repeats |

D#4, F#4, G#4, A#4 and F#5 have no pad.

- The repeats lock to the grid. **1/1** and **1/2** are too long for the 0.74-second buffer, so the sequencer replays the bar or half bar before; they need the transport running.
- **ECHO** and **VERB** throw the mix into the shared delay and reverb; they ring out after you let go.
- **-DRM** and **-BAS** fade the part out but keep its notes playing unheard, so it comes back in place.
- One time effect plays at a time (a stutter, REVRS, HALF, STOP, DROP or FLNG: the last pressed; let it go and an earlier one still held comes back), with every colour effect (LP, HP, LCUT, CRUSH, GATE), throw and kill on top.

**Latch.** Hold **OCT+** and press a pad, or press **OCT+** while holding pads: they stay on with no key held (a white dot on the pad; **PERF** and how many in every header; their names on HOME). Press a latched pad again to turn it off. **OCT-** unlatches every pad; STOP does too. Latched pads keep playing after you leave PERFORM.

**The screen.** Held pads are white, latched ones grey with a white dot, a pad the STUTTER lane is playing back grey with a white bar, the knobs' pad outlined. The line under the header names the knobs' pad and says HELD, LATCHED, FROM LANE or LAST PAD.

**The knobs** set the last pad pressed. KNOB 4 is AMOUNT (the wet level) wherever a pad has one.

| Pad | KNOB 1 | KNOB 2 | KNOB 3 |
| --- | --- | --- | --- |
| 1/4 .. 1/32, 8T, 16T | PITCH (-12 to +12) | DECAY | GATE |
| REVRS, HALF | LENGTH | | |
| LP, HP | CUTOFF | RESO | SPEED |
| CRUSH | BITS | RATE | |
| GATE | RATE | WIDTH | |
| STOP | TIME | | |
| DROP | DEPTH | TIME | |
| ECHO | TIME (shared) | FDBK | |
| VERB | SIZE (shared) | DAMP (shared) | |
| -BAS | VOICE | | |
| LCUT | FREQ | | |
| FLNG | RATE | FDBK | |

The knob settings last until power-off and are not saved, except those recorded into the STUTTER lane.

---

## 7. The STUTTER lane

The STUTTER lane is PHEBUS's FX track: a fifth sequencer part that plays no notes. Its lit steps repeat a slice of what you just heard, and the PERFORM pads you play while recording are recorded into it.

On the GRID, turn **ALGORITHM** past DRUM to reach it (left goes back). The tag is **STUT**, white and grey.

| You do | Result |
| --- | --- |
| **tap an unlit step** | it lights with the length for new steps (1/4 to start) |
| **tap a lit step** | it goes out when the key comes up, unless KNOB 1 turned |
| **hold a step + KNOB 1** | that step's length: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 (right: shorter) |
| **hold a step + OCT-** | the recorded pads over that step go (a step that press lit goes out again) |
| **KNOB 1 LEN** | the length for new steps |
| **KNOB 2 ON** | the lane ON or OFF, steps and recorded pads kept (HOME's KNOB 3 too) |
| **KNOB 3 PADS** | how many pads are recorded (up to 31); turn it, then again within 1.5 seconds: every recorded pad is cleared |
| **KNOB 4 STEPS** | the lane's length, 1 to 64 |
| **black keys 1 to 4**, **OCT-**, **OCT+** | page (OCT- and OCT+ go round; the length is KNOB 4's) |
| **EDIT + OCT-** | undo an edit, an erase, a clear, or a take's pads |

**The screen.** The 16 steps as tiles. Lit steps in a row with the same length are one loop and are drawn as **one span** across them, with the length named once at its start ("1/4"; a single step shows "4", "8", "16", "32", "2" or "1"). A line says what the lit steps do (or STEP n and its length), then the **recorded pads** as named bars across the steps they cover, in up to three rows; a bar lights while it plays and turns white over a held step.

**How it plays.** Steps are 1/16 and ignore swing. A lit run repeats the span played just before it, the whole mix. 1/4 to 1/32 are audio stutters; **1/2 and 1/1 replay the sequencer** (every voice plays the half bar or bar before again, and carries on afterwards from where it would have been). A pad you play live comes before the same pad recorded, and any pad before the lit steps.

**Recording pads.** While the sequencer records (REC while playing, any voice), each pad you play is written as a span: which pad, from press to release (to the nearest 1/16), and the knob you turned while it played. A latched pad records from latch to unlatch. A pad recorded over its own earlier span replaces it; other pads overdub. The 32nd says LANE FULL: 31 PADS.

---

## 8. Patterns A to D, recording and saving

### Patterns

A **pattern** holds all four voices' sounds and steps, the kit and the STUTTER lane. There are four, **A to D**. **Hold SAVE** for the PATTERN layer:

| Control | Does |
| --- | --- |
| **keys 1 to 4** | play pattern A to D. Playing: on the next bar (NEXT: A). Stopped: loaded (LOADED A). Empty: EMPTY A |
| **keys 5 to 8** | store into A to D. Over a used pattern, press again within 3 seconds (AGAIN: A, then SAVED A) |

While a pattern plays, what you change in it is kept in it. **Tap SAVE** on its own for the SAVE pages.

### Recording live

| You do | Result |
| --- | --- |
| **tap REC, playing** | records now |
| **tap REC, stopped** | arms it (REC READY); recording starts on your first note, or after a count-in. On a synth voice's GRID: STEP REC |
| **tap REC again** | stops or cancels |
| **hold REC** | after about 0.7 seconds a ring fills over about 1.3 seconds more: the selected voice is cleared. Let go early and nothing happens. EDIT + OCT- brings it back |

The REC screen: KNOB 2 **STEPS** (the length: 1BAR, 2BARS, 4BARS), KNOB 3 **START** (NOTE or COUNT). Notes are quantised to the voice's DIV. Rolls are recorded as ratchets (on the drums, as retrigs). PERFORM pads go into the STUTTER lane.

**Hold EDIT** (ERASE): a key erases that note or sound (playing: from each step the playhead passes while you hold it; stopped: from the whole pattern). KNOB 1 SHIFT rotates the pattern, KNOB 2 STEPS doubles or halves it, KNOB 3 TRANSP transposes (synth voices). OCT- undo, OCT+ redo. On the drums, a black key switches EDIT's keys between erasing and muting.

**Hold SEQ** (STEPS): KNOB 1 GATE (synth voices), KNOB 2 DIV (1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1BAR, 2BAR), KNOB 3 SWING (0 to 100), KNOB 4 STEPS (1 to 64). The keys do nothing; the screen draws the pattern in time with its swing.

**Hold ARP** (ROLL): hold a key for a note-repeat roll; KNOB 1 the rate (1/8, 1/16, 1/32, 32T, 1/64). Tap ARP for the arpeggiator pages (ARP: MODE, RATE, OCT, GATE; ARP 2: SWG, PROB, HOLD, ORD).

### Saving

- **The unsaved dot**: a small white dot in the header when the project in memory differs from flash (checked twice a second).
- **Autosave**: 0.3 seconds after you stop with no input, once the output is silent (under -60 dB), at most every 5 seconds; as a fallback after 2.5 seconds with no input, at most every 20 seconds. Flash is written only while stopped.
- **At power-on** the working project comes back.
- **Saved only when you ask**: patterns A to D (hold SAVE) and user presets (SAVE > USER).
- **Not in a project**: SYNC, the MIDI settings, the lights and the menu settings belong to the FM-1.

Projects saved by PHEBUS use a new format: PHEBUS reads older KARMA projects, stock KARMA cannot read PHEBUS's.

---

## 9. GLO pages and the mixer

### GLO pages

Tap **GLO** for the shared pages; tap again or turn SELECT for the next. The tag is **GLO**, white and grey (DRUMS counts as the drum voice, coral). The list names the four knobs with a bar where each sits; the values are in the dials.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **GLOBAL** | BPM (40 to 240) | SWING | CLICK (OFF, REC, ON) | TUNE |
| **MASTER** | DUST | DUCK | FILT | ROLL |
| **DLY** | TIME | FEEDBK | COLOR | MIX |
| **REV/CHO** | SIZE | DAMP | CRT (chorus rate) | CDP (chorus depth) |
| **SYSTEM** | OUT | SYNC | IN | CPU (or the USB state) |
| **DRUMS** | CH4 | LVL | REVERB | DELAY |
| **MIDI** | CH1 | CH2 | CH3 | CH4 |
| **MIDI 2** | IN | OTHER | OUT | BEND |
| **MIDI 3** | KNOBS | | | |

DUST is an old sampler and record; DUCK pumps the synth voices on every kick; FILT is a DJ filter (left low-pass, right high-pass); ROLL is the ARP layer's rate. DLY and REV/CHO are the shared effects the FX slots send to and PERFORM's ECHO and VERB throw into.

### The mixer (hold GLO)

Hold **GLO** for the mixer (hold it 0.7 seconds untouched and let go to keep it). It shows each voice's level as a podium with its meter, M and S badges, the 16 keys' jobs, and four dials, each in its voice's colour.

| Control | Does |
| --- | --- |
| **KNOB 1 to 4** | levels of voices 1, 2, 3 and the drums |
| **keys 1 to 4** | mute (yellow M) |
| **keys 5 to 8** | solo (ice-blue S; several add up) |
| **key 9** | fill while held |
| **key 10** | fill on the next bar (again cancels) |
| **key 11** | the knobs set the pans (title PAN); again: levels |
| **key 16** | tap tempo |
| **F#3** | MIDI LEARN on or off |
| **SELECT** | tempo |

A muted voice fades out in a few milliseconds and its pattern runs on silently.

---

## 10. Settings, MIDI and USB

### The menu

**Hold HOME** (about 0.7 seconds). SELECT changes section, KNOB 1 to 3 set its rows, PRESETS moves the cursor, OCT+ steps a setting or opens it, OCT- closes.

| Section | Rows |
| --- | --- |
| **SCREEN** | COLOR, ZOOM |
| **LIGHTS** | LIGHTS, KEYS, NOTES |
| **AUDIO** | SPEAKER LOWCUT, USB AUDIO (MASTER, FULL, STEMS), USB SERIAL |
| **SYSTEM** | HARDWARE CALIBRATION, MIDI LEARN CLEAR, ABOUT |

USB AUDIO and USB SERIAL take effect at the next start. ABOUT shows PHEBUS, "BASED ON KARMA 3.1.5" and the credits.

### Screensavers

Tap **HOME** on HOME. **SELECT** steps through **FLOW**, **EYES**, **DESERT** and **THIRD EYE**, each driven by the four voices' levels in their colours. HOME or any page button closes it. At power-on a boot screen of about 3 seconds shows a flowing backdrop with PHE and BUS in black; any key skips it.

### MIDI

- **CH1 to CH4** (GLO > MIDI): the channels of voices 1, 2, 3 and the drums (1, 2, 3 and 10 to start). OFF: nothing in or out.
- **IN** (MIDI 2): one channel plays the selected voice, NOTES (each channel its voice), or CLOCK only. **OTHER**: SEL or OFF. **OUT**: KEYS (what you play) or SEQ (the sequencer too). **BEND**: 1 to 12 semitones.
- **KNOBS** (MIDI 3): what a controller's knobs do: PANEL, AUTO, 1-8, 70-77 or GM.
- **SYNC** (GLO > SYSTEM): INT, USB or TRS. START plays from the top, CONTINUE carries on, STOP stops; with no clock pulse for half a second, the FM-1 goes back to its own tempo.
- **CCs**: 5 GLIDE, 7 LEVEL, 10 PAN, 71 RES, 72 release, 73 attack, 74 the track's DJ filter (64 is off; on a synth voice it has no page since the voice filter came, so CC 74 does not move CUTOFF), 75 decay, 91 / 93 / 94 the reverb, chorus and delay sends (a voice with no slot of that type gets one in its first OFF slot), 64 sustain, 120 and 123 all sound and all notes off, 121 reset.
- **Program Change** loads the voice's n-th sound (on the drum channel, kit n). **MIDI LEARN**: GLO + F#3, turn a knob on screen, move the controller's knob, GLO + F#3 again.
- MIDI comes in on the 3.5 mm TRS jack (try the other adapter type if nothing plays) or USB from a computer or phone. A USB keyboard plugged straight in cannot work. No Bluetooth MIDI.

### USB audio

The FM-1 is a class-compliant USB audio input. MENU > AUDIO > USB AUDIO: **MASTER** follows the MASTER knob, **FULL** is a fixed level, **STEMS** (next start) gives six channels: each voice mono plus the effects return in stereo, at 44.1 kHz. The USB name is still **FM-1 KARMA**.

---

## 11. Backup and install

**Back up first.** The backup page (`web/editor.html`, Chrome or Edge, FM-1 on USB) has three buttons: **Connect**, **Save backup** and **Restore**. A backup holds the working project with its kit, the four patterns, the 32 user presets, the MY HEX kits and the settings. Save one before you flash anything. A format 8 backup is refused by firmware older than its protocol.

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

### The same gesture, the same meaning

| Gesture | Means, wherever it works |
| --- | --- |
| tap a button | open its pages |
| hold a button | its layer |
| hold a layer + tap HOME | keep the layer open (LOCK) |
| hold a step + turn a knob | change that value on that step: a lock (synth GRID, drum screen) or the step's length (STUTTER lane) |
| hold a step + OCT- | remove from that step: the last lock, then every lock (synth, drums); its recorded pads (STUTTER lane) |
| hold a step + OCT+ | the next condition (synth, drums) |
| OCT- / OCT+, nothing held | page of steps before / after (GRID, drums, lane); elsewhere the octave |
| SELECT | walk the halves, tabs and pages; the tempo where there are none |
| hold SEQ | the pattern's settings (GATE, DIV, SWING, STEPS) |
| EDIT + OCT- / OCT+ | undo / redo |
| OCT+ with a pad | latch it (PERFORM) |

### Anywhere

| Gesture | Does |
| --- | --- |
| tap PLAY | start or stop |
| tap REC | record or arm (stopped, on a synth voice's GRID: STEP REC) |
| hold REC | clear the selected voice (a ring fills) |
| tap HOME | HOME (on HOME: a screensaver) |
| hold HOME | the menu |
| tap ENV, LFO, FX, EDIT, ARP, SEL, GLO | that button's pages (drums: EDIT, ENV, LFO the drum screen's SRC, AMP, LFO; FX the drum voice's own pages; SEL MUTE mode; ARP CHROMATIC mode) |
| tap SEQ | the GRID (from a synth sound page: on that page's tab; on the GRID: TRIG, again back) |
| tap SAVE | the SAVE pages |
| ALGORITHM | the voice (on the GRID, past DRUM: the STUTTER lane) |
| PRESETS | the sound (drums: the kit) |
| OCT- / OCT+ | octave -3 to +3 (both: 0) |
| hold OCT- / OCT+ + drum key | ghost / hard hit |

### HOME

| Gesture | Does |
| --- | --- |
| KNOB 1 to 4 | CUTOFF, DECAY, STUT (lane on / off), LEVEL |
| SELECT | tempo |

### Sound pages (synth voice)

| Gesture | Does |
| --- | --- |
| tap EDIT | OSC; on the chain, the next stage |
| tap ENV, FX, LFO | that stage; again, its next page (ENV: AMP, FILT; FX: FX 1, FX 2, FX 3, SLICER; LFO: LFO, LFO SYNC, LFO 2, MATRIX) |
| SELECT | the page before or after |
| tap SEQ | the GRID on this tab |
| a FILT ENV knob while LINK is AMP | the filter's envelope becomes OWN |
| keys | notes |

### GRID (synth voice)

| Gesture | Does |
| --- | --- |
| tap an empty / set step | set with the pen / clear on release |
| hold a step + knob, a sound tab | lock |
| hold a step + knob, TRIG | NOTE LEN VEL COND; 2/2 FLAG RATCH NUDGE |
| hold a step + OCT- / OCT+ | remove locks (TRIG: locks, nudge, condition) / next condition |
| nothing held, TRIG 1/2 | NOTE (pen), STEP, HIT, VEL |
| nothing held, TRIG 2/2 | SPICE, DICE |
| EDIT, ENV, FX, LFO, SEQ, SELECT | the tabs (also with a step held) |
| black keys 1 to 4, OCT- / OCT+ | pages (OCT+ on the last adds one) |
| tap REC | stopped: STEP REC; playing: LIVE REC |
| STEP REC: keys, SEQ, ARP | note or chord, rest, tie |

### Drum screen

| Gesture | Does |
| --- | --- |
| hold SEL + key | pick the sound |
| tap an empty step / a hit | a hit / clear on release |
| hold a step + knob | lock on that sound's hit |
| hold a step + OCT- / OCT+ | remove locks (TRIG: everything) / next condition |
| hold a step + FX | lock the drum voice's own pages on that step |
| EDIT, ENV, LFO, SEQ, SELECT | SRC FLTR AMP LFO, TRIG, every half (also with a step held) |
| tap SEL / SEL + key in MUTE mode | MUTE mode / solo |
| tap ARP | CHROMATIC mode (OCT- / OCT+: its octave) |
| hold ARP + key / black key | RETRIG / its rate |
| F#5 | FILL while held; twice: latched |
| EDIT + black key | EDIT's keys erase or mute |

### STUTTER lane

| Gesture | Does |
| --- | --- |
| tap a step | light it / put it out on release |
| hold a step + KNOB 1 / OCT- | its length / erase its pads |
| KNOB 1 to 4 | LEN for new steps, ON, PADS (twice: clear), STEPS |
| black keys 1 to 4, OCT- / OCT+ | page |

### PERFORM (hold FX)

| Gesture | Does |
| --- | --- |
| a white key, F#3 G#3 A#3 C#4, C#5 D#5 | its pad while held |
| hold OCT+ + pad, or OCT+ with pads held | latch |
| a latched pad / OCT- | unlatch it / all |
| KNOB 1 to 4 | the last pad's knobs |
| FX (kept open), HOME, any page button | leave (latches play on) |
| REC while playing | record the pads into the lane |

### Layers

| Hold | Gesture | Does |
| --- | --- | --- |
| EDIT | key / KNOB 1 to 3 / OCT- OCT+ | erase / SHIFT, STEPS, TRANSP / undo, redo |
| ARP | key / KNOB 1 | roll (drums: RETRIG) / rate |
| SEQ | KNOB 1 to 4 | GATE, DIV, SWING, STEPS |
| SEL | key / KNOB 1 to 4 | song key (drums: the sound) / CHORD, SCALE, KEYS, TRANSP |
| GLO | keys and knobs | the mixer (section 9) |
| SAVE | keys 1 to 4 / 5 to 8 | play / store A to D |

### Menu, screensaver, power-on

| Gesture | Does |
| --- | --- |
| SELECT, KNOB 1 to 3, PRESETS, OCT+ / OCT- (menu) | section, rows, cursor, open / close |
| SELECT (screensaver) | the next one |
| hold OCT- while switching on | USB rescue |
| hold OCT- and OCT+ while switching on | hardware calibration |
| hold OCT- and OCT+ 5 s, stopped | update mode (UBOOT) |

### Where a gesture means something else, and why

- **SELECT on HOME, the mixer and the REC screen** sets the tempo: those screens have no pages. In STEP REC it moves the red frame, the step the next note goes on.
- **OCT- and OCT+** are the octave on HOME, the sound pages and in STEP REC and CHROMATIC mode (the keys are notes there), and pages of steps where the keys are steps.
- **OCT+ on the STUTTER lane's last page** goes round to page 1 instead of adding a page: the lane's length is KNOB 4's.
- **Latch** is OCT+ with a pad on PERFORM but a double press of F#5 for the drum FILL. A double tap of a pad would latch stutters tapped in rhythm by accident; the FILL is the Rytm's.
- **Staying open** by holding 0.7 seconds untouched works on GLO and FX only; every layer stays open with its button held and HOME tapped.
- **SEL and ARP on the drum voice** are MUTE mode, the sound pick, CHROMATIC mode and RETRIG (the Rytm's), not the key, scale and arpeggiator of the synth voices.
- **TRIG's knobs with nothing held**: on a synth voice the pen, cursor, hit and velocity; on the drums KNOB 1 is the velocity new hits get and the others say HOLD A STEP.

---

## 13. Known limits

- **Nothing has been run on a real FM-1.** Sound, timing, the processor's headroom and the screen's speed are unmeasured. Everything here comes from the code and the host tests.
- **Do not flash until the unit has arrived and its chip revision is checked.** Newer FM-1 units have a changed chip and can be bricked.
- **PHEBUS_ARENA_T12** (a setting of the device build only): tracks 1 and 2 have less memory for an oscillator than track 3. On the FM-1 itself, six Types (6-OP A, B, C, STRING, SPEECH, PARTICLE) play on track 3 only; on tracks 1 and 2 they are silent and the OSC page says NOT ON THIS TRACK. The simulator has no such limit.
- **Polyphony**: eight notes shared by the three synth voices; JUNO up to six; ACID and the heavy Types one.
- **No velocity or pressure on the keys**: they are plain switches. Use OCT- and OCT+ for ghost and hard drum hits, set velocity per step, or play PHEBUS from the MPC's pads over MIDI.
- **The PERFORM time effects and the STUTTER lane are mono** (16-bit, 12 dB of headroom kept); the buffer is 0.74 seconds, and below about 84 BPM a quarter-note loop is cut to 0.72 seconds.
- **Limits per pattern or voice**: 24 locks a synth voice, 128 drum sound locks and 64 hits with extras a pattern, 19 matrix cells a voice, 31 recorded pads in the lane, one edited kit a project.
- **Mutes, solos and PERFORM's knob settings** are not saved.
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
8. **Saving**: that an autosave never stalls the screen or the sound, that the -60 dB silence threshold suits the real noise floor, patterns A to D across a power cycle, the backup page against the new firmware.
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
| **voice** | one of the four instruments: 1, 2, 3 (synth) and DR (drums). Messages call a voice's part a TRACK ("TRACK 1 CLEARED") |
| **step** | one place in a pattern, 1 to 64; a white key on the GRID, the drum screen and the lane. "STEP 5 HELD" |
| **trig** | what sits on a step: a note or a drum hit. TRIG is also the tab with the held step's own values |
| **page** | 16 steps (black keys 1 to 4); also one screen of a voice's settings |
| **half** | a page's next four knobs, "1/2" at the right of the tabs; SELECT walks them |
| **tab** | the row naming a voice's pages: TRIG OSC FILT ENV FX MOD (synth), TRIG SRC FLTR AMP LFO (drums) |
| **lock** | a value set on one step only: hold the step and turn the knob. Drawn inverted |
| **condition** | when a step plays: -, FILL, NOFL, CHNC |
| **retrig** | a drum hit repeated at a rate (RETRIG). Never RTRG |
| **ratchet** | a synth step repeated 1 to 4 times in its time (RATCH) |
| **layer** | what the keys and knobs do while a button is held |
| **PERFORM** | the pads layer, opened by holding FX. Its effects are **pads** |
| **FX** | the button; on a synth voice, its three **FX slots**. Never a PERFORM pad |
| **send** | how much of a voice goes to the shared chorus, reverb or delay: a slot's KNOB 4 |
| **latch** | keep a pad (OCT+) or the FILL (F#5 twice) on with no finger on it |
| **STUTTER lane** | the fifth sequencer part, tag STUT; STUT on HOME's KNOB 3 |
| **pattern** | one of A to D: all four voices' sounds and steps, the kit and the lane |
| **kit** | the drum voice's 16 sounds; **your copy** is the project's edited one (a * after its name) |
| **Type** | the oscillator model, OSC's KNOB 1, 31 of them |
| **macro** | one of a Type's three knobs, M1 M2 M3 in the matrix |
| **mute**, **solo** | yellow M, ice-blue S |
| **GLO** | the shared pages and, held, the mixer |
