# PHEBUS: the guide

PHEBUS is firmware for the M-VAVE FM-1 groovebox. This guide is for the person playing it. It says what each button, key and knob does, in the order you are likely to need it.

> **Read this first.** PHEBUS is unreleased, personal firmware. Nothing in this guide has been checked on a real FM-1 yet: it was written from the firmware's source code and its tests. Some FM-1 hardware revisions can be bricked by custom firmware. Do not flash PHEBUS (or any custom firmware) until you have checked that your unit is a revision known to be safe.

PHEBUS is Ben's fork of KARMA. It is based on [KARMA](README.md) by Ariel Muchnik, itself a fork of [SLOOP](https://github.com/isod89/sloop-fm1) by isod89, based on [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments). It is free software under GPL-3.0-only: see [LICENSING.md](LICENSING.md) for the licence and for the other code it uses.

---

## How to read this guide

| Written | Means |
| --- | --- |
| **tap FX** | press FX and let go without touching anything else, in under about half a second |
| **hold FX** | keep FX pressed |
| **FX + key 3** | hold FX, and while it is held press white key 3 |
| **FX + KNOB 2** | hold FX, and while it is held turn KNOB 2 |
| **GO** | a value that does something (LOAD, SAVE, NEW and so on). Turn it one click and *AGAIN: ...* shows. Turn it one more click within about 1.5 seconds to do it |
| **key 1 to key 16** | the 16 white keys, from the lowest (F3) to the highest (G5) |

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Note | F3 | G3 | A3 | B3 | C4 | D4 | E4 | F4 | G4 | A4 | B4 | C5 | D5 | E5 | F5 | G5 |

The 11 black keys are F#3, G#3, A#3, C#4, D#4, F#4, G#4, A#4, C#5, D#5 and F#5. "Black keys 1 to 4" means the first four: F#3, G#3, A#3 and C#4.

The FM-1 has these controls: the buttons PLAY, REC, SAVE, HOME, ENV, LFO, FX, SEL, EDIT, ARP, SEQ, GLO, OCT- and OCT+; the knobs ALGORITHM, PRESETS, SELECT, KNOB 1 to KNOB 4 and MASTER; and 27 keys. SEL is the button between FX and ENV, not the SELECT knob.

## Contents

1. [What PHEBUS is](#1-what-phebus-is)
2. [The screen at a glance](#2-the-screen-at-a-glance)
3. [Your first sound](#3-your-first-sound)
4. [The GRID: placing steps](#4-the-grid-placing-steps)
5. [Drums](#5-drums)
6. [The STUTTER lane](#6-the-stutter-lane)
7. [Sound design along the chain](#7-sound-design-along-the-chain)
8. [Performance effects and global effects](#8-performance-effects-and-global-effects)
9. [Mixer, mute and solo](#9-mixer-mute-and-solo)
10. [Sections](#10-sections)
11. [Recording live, erasing and clearing](#11-recording-live-erasing-and-clearing)
12. [Saving](#12-saving)
13. [Tempo, swing, key and scale](#13-tempo-swing-key-and-scale)
14. [Screensavers](#14-screensavers)
15. [Settings and menus](#15-settings-and-menus)
16. [MIDI and USB](#16-midi-and-usb)
17. [The engines and the sound list](#17-the-engines-and-the-sound-list)
18. [Kits and the web editor](#18-kits-and-the-web-editor)
19. [Every button gesture, screen by screen](#19-every-button-gesture-screen-by-screen)
20. [Troubleshooting and recovery](#20-troubleshooting-and-recovery)
21. [Credits](#21-credits)

---

## 1. What PHEBUS is

PHEBUS turns the FM-1 into a four-voice groovebox: three synth voices and one drum voice, each with its own sequencer track of up to 64 steps.

Each voice is a character with its own colour:

| Voice | Character | Colour | Plays |
| --- | --- | --- | --- |
| **1** | robot | purple | ACID (a 303 voice) or JUNO (a Juno-106 style synth) |
| **2** | alien | blue | JUNO or ELEMENTS (physical modelling) |
| **3** | crocodile | teal | PLAITS (24 synthesis models) |
| **Drums** | snake | coral | the drum synth, with 16 sounds on the keys |

**Colour has a meaning.** A voice's colour always means "this acts on that voice". White and grey mean shared: the screen acts on everything, and its header tag says ALL. Yellow is for **M** (mute). Ice blue is for **S** (solo). Red is only for recording and erasing.

**Tap and hold.** Most buttons do two jobs. Tap one and it opens its pages. Hold one and it becomes a layer: the keys and knobs do something else for as long as you hold it. The layer names are:

| Hold | Layer | Used for |
| --- | --- | --- |
| FX | PUNCH | performance effects on the keys |
| EDIT | ERASE | erase, shift, undo and redo |
| ARP | ROLL | note-repeat rolls |
| SEQ | STEPS | pattern time: division, swing, length |
| SEL | KEY | key, scale and chord |
| GLO | MIX | levels, mute, solo, fills, tap tempo |
| SAVE | SECTION | sections A to D |

A layer shows its name in the header ("FX: PUNCH") once you have held it for a moment. To keep a layer open without holding it, hold its button and tap HOME. Any other button lets it go again, except PLAY, REC, OCT- and OCT+. If you hold GLO for about 0.7 seconds without touching anything else and then let go, the mixer stays open.

After a layer lets go, the knobs ignore you for a quarter of a second, so a knob still turning does not change the page underneath.

---

## 2. The screen at a glance

**The header** runs along the top:

- a boxed **tag** showing what the screen acts on: the engine name (ACID, JUNO, ELEM, PLTS), DRUM, HOME, STUT (the stutter lane) or ALL (shared pages, in grey);
- the **page name**;
- the **tempo** (BPM) and a play or stop symbol;
- four **beat lights**. While you record, only the light on beat 1 turns red;
- a red **record disc** while recording, or a small white **unsaved dot** when the project in memory differs from what is saved (see [Saving](#12-saving));
- the **battery**, three bars, red when low.

**The bottom strip** shows the four characters' heads, numbered 1 2 3 DR. The selected voice is underlined and its number is in its colour. A yellow **M** on a head means muted, an ice-blue **S** means soloed, a red dot means it is recording. On the STUTTER lane no voice is underlined.

**The knob band** at the bottom of a page shows what KNOB 1 to KNOB 4 do. Values are drawn in one bold face. The knob you are touching has an inverted label and an underlined value. A dim grey value means it does nothing in the current setting. A short message appears as a white bar over the band.

**The chain strip.** On sound pages, a strip above the knobs lists the five stages of the sound chain, OSC, FILT, ENV, FX and MOD. The current stage is lit and carries the page name. If the stage has more than one page, "2/3" and so on appears at the right. See [Sound design](#7-sound-design-along-the-chain).

**Graphs.** Sound pages draw what their knobs do: the filter's response curve (with the envelope's sweep), the insert effect's curve, the oscillator's waveform, the envelope, the LFO, and bars for the rest. A small dancing character sits at the top right, and the sound's name is at the top left.

### The HOME screen

Tap **HOME** for the HOME screen. At the top: the bar counter, a strip of the **FLOW** backdrop, and the sections A to D. In the strip each voice has a band in its colour that swells as that voice gets louder and sinks to a dim line when it is quiet. It flows while the song plays or a voice sounds, and stands still otherwise. Under them, the selected voice's sound and engine (and REC, SOLO or MUTE), then a lane for each voice: its character, the 16 steps in view, the step playing and its level.

The four knobs play the selected voice, for quick changes while it plays:

| Knob | Does |
| --- | --- |
| **KNOB 1 CUTOFF** | the voice's filter: the engine's own cutoff on ACID and JUNO (CUT); the voice's FILTER on PLAITS and ELEMENTS (left of centre a low-pass, right a high-pass); the drum track's FILTER on the drums |
| **KNOB 2 DECAY** | the decay of the envelope that shapes the sound: ACID's own DECAY (the 303's envelope, which moves its filter); the ENV page's DEC on JUNO, PLAITS and ELEMENTS; on the drums, the selected sound's DEC on a HEX kit ("--" on the other kits) |
| **KNOB 3 STUT** | the STUTTER lane **ON** (turn right) or **OFF** (turn left). It is drawn in white and grey because it acts on the whole mix. See [The STUTTER lane](#6-the-stutter-lane) |
| **KNOB 4 LEVEL** | the voice's level. On a muted voice the first turn unmutes it |

Swing and the pattern length are on **hold SEQ**. Pan is on the mixer (**GLO + key 11**) and on VOICE 2.

---

## 3. Your first sound

A new project (and a power-on with nothing saved) starts with plain sounds and **no effects on**, so you hear the raw voice:

| Voice | Starts as |
| --- | --- |
| 1 | ACID INIT: a plain sawtooth through the 303-style filter, the filter part open with a little resonance and a short envelope sweep so each note speaks. No accent, slide or drive. One note at a time, an octave down. Good for bass |
| 2 | JUNO INIT, a plain sawtooth, polyphonic. Good for chords |
| 3 | PLAITS INIT, a plain sawtooth (the VA model), one note at a time, about 6 dB quieter. Good for lead |
| Drums | the 606 kit |

All the effect sends (DIST, CHORUS, DELAY, REVERB) start at zero. **SAVE > TOOLS > NEW** puts a project back to this starting point.

To make a first sound:

1. Press **PLAY**. The beat lights run.
2. Play the keys. On a synth voice, the white and black keys are notes. On the drum voice, the keys are 16 drum sounds.
3. Turn **ALGORITHM** to pick a voice: 1, 2, 3 or DRUM.
4. Turn **PRESETS** to browse that voice's sounds. The ACID, JUNO and PLAITS sounds each have an **INIT** sound: the plain starting point again.
5. Tap **EDIT** to open the sound's first page, and turn the knobs.
6. Tap **OCT-** or **OCT+** to change the octave of a synth voice, from -3 to +3. Press both together to go back to 0.

**Drum keys:** hold **OCT-** while you hit a drum key for a ghost (soft) hit, or **OCT+** for a hard hit.

---

## 4. The GRID: placing steps

The GRID is the step sequencer, and it is the only step editor. **Tap SEQ** (from HOME or from any other page) and the selected voice's GRID opens. This works the same on every voice.

On the GRID the 16 white keys are the **16 steps of the page shown**. They play nothing. The title reads "GRID 1/2" (page 1 of 2, and so on). **ALGORITHM** changes voice and stays on the GRID. **HOME** goes back.

### The tiles

Each step is a tile in the voice's colour:

- a set step shows its note name (without the octave) and 1 to 4 small octave ticks;
- white text means an accent, a small mark means a slide;
- a bar along the tile is its velocity;
- a line joining notes is a chord, and a bar from the note before it is a **tie**;
- a dot means a parameter lock or a nudge;
- a notch means a condition;
- the cursor is framed in white, and the playhead is a white bar.

### Placing and clearing steps

| You do | Result |
| --- | --- |
| **Tap an empty step** | it is set with the **pen** (see below) at velocity 100, and you hear it for about 150 ms |
| **Tap a set step** | it is cleared when you let go of the key, unless you turned a knob while holding it |
| **Hold a step** | the step is selected. The screen shows STEP n with its notes, length, flags, ratchet, condition, nudge and up to four lines of locks (a "+" if there are more) |

The **pen** is the note or chord a new step gets: the last note or chord you played (on the keys elsewhere, or from a MIDI keyboard), or whatever KNOB 1 sets. A chord goes onto a step only on a polyphonic voice.

### Editing a held step

While you hold a step, the knobs edit it. A step you edited is kept, never cleared when you let go. Hold several steps and they are edited together.

| Synth voice | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| first set | **NOTE**, by degrees of the scale (you hear it) | **LEN**, in steps (the following steps become ties) | **VEL**, 1 to 127 (turning it clears the accent) | **COND**: -, FILL, NOFL, CHNC |
| after **SELECT** right | **FLAG**: -, ACC, SLD, A+S | **RATCH**: x1 to x4 | **NUDGE** | |

SELECT left goes back to the first set. On a drum step the knobs are **LEVEL**, **RATCH**, **NUDGE** and **COND**.

- **Conditions.** FILL plays only during a fill. NOFL plays except during a fill. CHNC plays about one time in two. A skipped step is silent in full: no note, no MIDI, no ratchet, no lock. Start a fill with **GLO + key 9** (while held) or **GLO + key 10** (the whole next bar; again cancels it).
- **Ratchets** repeat the step 1 to 4 times inside its time.
- **Nudge** moves a step slightly off the grid.
- Held step + **OCT+** goes round the conditions (the message says NORMAL, FILL ONLY, NO FILL or CHANCE).
- Held step + **OCT-** clears. On the GRID it clears the step's locks, nudge and condition together. On a lock page (below) it clears only that page's locks.

### Parameter locks

A lock gives one sound parameter a different value on one step only.

1. Hold a step.
2. Tap **EDIT**, **ENV**, **LFO** or **FX**. That button's page goes onto the knobs. The title reads "LOCK ..." in the step band.
3. Turn a knob. That parameter is locked on the held step (or steps).

Tap the same button again to go to the next page that can be locked. EDIT walks OSC, FILT and ENV only. A value drawn in full colour is a lock. A dim value is the voice's own value.

You can also hold a step and turn **PRESETS** to turn the lock parameter on (the last sound knob you touched, which starts as ENV > FLT). With the step still held, **ALGORITHM** chooses which parameter.

There is a limit of **24 locks per voice**. Messages you may see: NOTHING TO LOCK HERE, NOT LOCKABLE, NO LOCK LEFT, LOCKS FULL: HOLD FEWER STEPS, LOCKS FULL: 24 A TRACK.

### Pages, length and follow

A pattern is up to 64 steps, in four pages of 16.

- **Black keys 1 to 4** pick page 1 to 16, 17 to 32, 33 to 48 or 49 to 64.
- The slot after the last used page adds a page: it is a copy of the last page (the message says "32 STEPS" and so on). Other empty slots say "ADD PAGE n".
- **OCT-** and **OCT+** with no step held go to the previous and next page. **OCT+** on the last page adds a page. At 64 steps the message says "64 STEPS: THE MOST" and OCT+ wraps to page 1.
- Under the tiles, four page slots: the used pages are filled, the unused ones outlined, and the page that is playing is marked.
- **Follow.** While playing, the view follows the playhead onto a new page. A page you pick by hand stays until the playhead moves to a different page.
- The pattern length can also be set with **hold SEQ + KNOB 4** (see [section 13](#13-tempo-swing-key-and-scale)), or with KNOB 4 during STEP REC.

### With no step held (synth voice)

| Control | Does |
| --- | --- |
| KNOB 1 **NOTE** | the pen (you hear it) |
| KNOB 2 **STEP** | the cursor |
| KNOB 3 **HIT** | right sets the cursor's step, left clears it |
| KNOB 4 **VEL** | the velocity of the cursor's step |
| **PRESETS** | picks the sound |
| **SELECT** | the tempo |

### SEQ pages after the GRID

On a synth voice, **tap SEQ on the GRID** to open the PATTERN page. Tap SEQ again to return to the GRID.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **PATTERN** | STEPS (1 to 64) | DIV | SWING | GATE |

**DIV** values: 1/4, 1/8, 1/16 (the usual), 1/32, 8T, 16T, 1/2, 1BAR, 2BAR.

### Step record (STEP REC)

To write a melody or a chord part note by note, use STEP REC on a synth voice's GRID.

1. Open the voice's GRID (**SEQ**) with the song **stopped**. The white frame is the cursor: STEP REC starts there (turn **KNOB 2** first to start somewhere else).
2. Tap **REC**. The title reads **STEP REC**, a red dot shows in the header, and the cursor turns into a **red frame**: the step the next note goes on.
3. Play the keys. They are notes again and you hear them. When you let go, the note goes onto the red step (replacing whatever was there) and the cursor moves on one step.
4. For a **chord**, hold the keys down together: the chord is written when the last key comes up. A chord is kept whole only on a polyphonic voice (on a mono voice the first note is written).
5. Tap **REC** again to finish. The keys are steps again.

| While in STEP REC | Does |
| --- | --- |
| **SEQ** (the button beside PLAY) | a **rest**: the step is emptied and the cursor moves on |
| **ARP** | a **tie**: the note before is made one step longer and the cursor moves on. "NO NOTE TO TIE" after a rest |
| hold a key + **SEQ** | the held note is written and tied (press SEQ again for a longer note) |
| **OCT-** / **OCT+** | the octave of the keys, as on HOME (both together: 0) |
| **SELECT** or **KNOB 2** STEP | move the cursor, for example back to fix a step |
| **KNOB 1** OCT | the octave |
| **KNOB 3** VEL | the velocity of the notes written (100 to start) |
| **KNOB 4** STEPS | the pattern length, 1 to 64 |
| **PRESETS** | the sound |
| **EDIT + OCT-** | undo the **whole STEP REC pass** (everything since REC was tapped) |

- At the end of the pattern the cursor goes back to step 1. On a longer pattern it moves on through the later pages, and the view follows it. To make room for more pages, turn KNOB 4 first.
- The screen shows the next step ("NEXT: STEP 5"), the notes held, and SEQ REST, ARP TIE, REC DONE.
- A MIDI keyboard on the voice's channel writes notes the same way.
- STEP REC ends by itself if you leave the GRID or change voice. **PLAY** works during STEP REC: the pattern plays while you write.
- **With the song playing, REC on the GRID records live** (as everywhere): the title reads **LIVE REC**, the keys play notes and what you play is recorded into the pattern. Tap REC again to stop recording.
- STEP REC is for the synth voices. On the drum GRID and the STUTTER lane, REC works as before.

### Undo and redo

**EDIT + OCT-** undoes and **EDIT + OCT+** redoes. A full undo restores note, length, locks, nudge and condition. Everything you change by knob during one EDIT hold counts as one undo. See [section 11](#11-recording-live-erasing-and-clearing).

---

## 5. Drums

The drum voice (coral, the snake) has 16 sounds on the 16 white keys:

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sound | KICK | KICK 2 | SNARE | CLAP | HAT | OPEN HAT | PEDAL | RIM |

| Key | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sound | SNARE 2 | LOW TOM | HI TOM | CRASH | RIDE | SHAKER | CONGA | COWBELL |

(The closed hat cuts the open hat.)

### The drum GRID: the snake view

Tap **SEQ** with the drum voice selected. The default view is the **snake**: the steps wind round the screen as a snake would. **SELECT** right shows the **full grid**: up to seven rows, the sounds in use plus the one you picked. **SELECT** left goes back to the snake.

| Control | Does |
| --- | --- |
| KNOB 1 **SOUND** | picks the drum sound (you hear it) |
| KNOB 2 **STEP** | moves the cursor (you hear that step) |
| KNOB 3 **HIT** | right sets the step, left clears it |
| KNOB 4 **LEVEL** | ghost, soft, norm, hard |
| **PRESETS** | picks the kit |
| step keys | the 16 steps of the page, as on synth voices |

Hold a drum step to edit it (LEVEL, RATCH, NUDGE, COND). Pages, length and follow work as in [section 4](#4-the-grid-placing-steps).

### The KIT page

Tap **SEQ** on the drum GRID for the KIT page, and again to go back to the GRID. On the KIT page the keys pick a sound (the 16 pads are on screen), and:

| Knob | Does |
| --- | --- |
| KNOB 1 | the sound |
| KNOB 2 | the kit |
| KNOB 3 | that sound's reverb send |
| KNOB 4 | that sound's delay send |

### Kits

The kit list is: five HEX kits (HEX BOOM, HEX ACID, HEX METAL, HEX GLITCH, HEX ROOM), then 32 synthesised kits (808, 909, **606** (the power-on kit), 80S, VINTAGE, TRAP, DRILL, BOOMBAP, LO-FI, PHONK, HOUSE, D.HOUSE, TECHNO, MINIMAL, ELECTRO, DISCO, GARAGE, JUNGLE, DUBSTEP, DEMBOW, AMAPIANO, AFRO, LATIN, TRIBAL, SYNTHWV, CHIP, ARCADE, GLITCH, INDUSTR, HYPER, AMBIENT, JAZZ), then MY HEX 1 to 4. A project saved on one of KARMA's user sample kits (USR1 to USR4, USR3+4, no longer in PHEBUS) plays the 606.

### Drum sounds and mutes

- **Edit a sound.** Tap **EDIT** on the drum voice for HEX OSC. EDIT again steps to the next page of the sound (HEX MOD, HEX CLICK, HEX MORE, then HEX FILT). **ENV** opens HEX AEG. **LFO** says "NO LFO ON DRUMS". Turning a knob on a built-in HEX kit copies the kit into the next free MY HEX kit.
- **Drum mutes.** Hold **EDIT** and press a **black key** on the drum voice to switch between erase mode and mute mode. In mute mode the white keys mute sounds (shown "off" in red).
- **HOME knobs** for the drum voice: CUTOFF (the drum track's own FILTER), DECAY (the selected sound's DEC on a HEX or MY HEX kit; "--" on the other kits, which have none), STUT and LEVEL. See [The HOME screen](#the-home-screen).

---

## 6. The STUTTER lane

The STUTTER lane is a fifth sequencer lane that does not play notes. It repeats a slice of what you just heard, on the steps you light.

On the GRID, turn **ALGORITHM** past 1, 2, 3 and DRUM to reach **STUTTER** (turn left to go back). Its tag is STUT, in neutral white and grey.

| You do | Result |
| --- | --- |
| **Tap an unlit step** | it lights, with the length last set (new steps start at **1/4**) |
| **Tap a lit step** | it goes out when you let go of the key, unless you turned KNOB 1 meanwhile |
| **Hold a step + KNOB 1** | that step's length: **1/1, 1/2**, 1/4, 1/8, 1/16 or 1/32. Turn right for shorter, left for longer |
| **KNOB 1** (no step held) | the length new steps take |
| **KNOB 2** | the whole lane ON or OFF (the steps are kept). **KNOB 3 on HOME** does the same |
| **KNOB 4** | the lane's length, 1 to 64 steps |
| **Black keys 1 to 4**, **OCT-**, **OCT+** | change page |

How it behaves:

- Steps are 1/16 on the grid and ignore swing. The page shown does not follow the playhead.
- A lit step repeats the span that played just before the run of lit steps. It repeats the whole mix, not one voice. Lit steps in a row with the same length are one continuous loop.
- **1/4, 1/8, 1/16 and 1/32** repeat the sound itself (an audio stutter). Their tiles read 4, 8, 16, 32.
- **1/2 and 1/1 replay the sequencer** (tiles 2 and 1): for as long as the run of lit steps lasts, every voice and the drums play again the steps of the half bar (1/2) or the bar (1/1) just before the run, with their notes and locks, in a loop. The song position keeps counting underneath, so when the run ends every voice carries on from the step it would have played without the stutter. Notes ring on naturally (it is not a recording), so the replay is not sample-identical. A voice whose steps are longer than the span (a bar's steps on 1/2) is not replayed.
- Holding **FX** and a key (see [Performance effects](#8-performance-effects-and-global-effects)) takes over while you hold it.
- The lane is mono (16-bit). Below about 84 BPM a quarter-note loop is cut to 0.72 seconds.
- It is saved with the project and with each section (it lives in unused drum-track bytes). Stock KARMA loads such a project but drops the stutter lane if it saves again. A lane saved before 1/2 and 1/1 existed loads as it was.
- **EDIT + OCT-** undoes a stutter edit.

---

## 7. Sound design along the chain

Every synth sound is one **chain of five stages**, in signal order:

**OSC** > **FILT** > **ENV** > **FX** > **MOD**

| Button | What it does on the chain |
| --- | --- |
| **EDIT** | from anywhere else it enters at OSC. When you are already on the chain it steps to the next stage and wraps from MOD back to OSC |
| **ENV**, **FX**, **LFO** | jump to that stage's first page. Press again for the stage's next page |
| **SELECT** | walks every page of the chain, one by one, and stops at both ends |

A page the sound does not have is skipped. On the drum voice, **ENV** opens HEX AEG, **LFO** says "NO LFO ON DRUMS", and **EDIT** opens HEX OSC. A page that does not apply says "NOT ON THIS SOUND".

### Pages by stage

| Stage | Pages and knobs (KNOB 1 to 4) |
| --- | --- |
| **OSC** | per engine (see [engines](#17-the-engines-and-the-sound-list)). Then **VOICE**: VOICE, GLIDE, GLMODE, PRIOR. Then **VOICE 2**: ALLOC, DTUNE, PAN, MUTE |
| **FILT** | ACID **FILTER**: CUT, RES, ENV, DEC; **FILTER 2**: FILT. JUNO **FILTER**: CUT, RES, ENV, FILT. ELEMENTS and PLAITS: **FILTER** with FILT. Drums: HEX FILT |
| **ENV** | **ENV**: ATK, DEC, SUS, REL. **ENV DEST**: FLT, PIT, SHP, ESYNC. Drums: HEX AEG |
| **FX** | **FX**: DIST, CHORUS, DELAY, REVERB (this voice's sends). **CHORUS** (JUNO's own chorus). **INSERT**: TYPE, A, B, C. **INSERT 2**: MIX. **SLICER** |
| **MOD** | **LFO**: RATE, WAVE, PHASE, FADE. **LFO DEST**: PIT, FLT, SHP, AMP. **LFO SYNC**: SYNC, TRIG (NOTE, FREE, PATT), POL |

On the drum voice, the sound pages are HEX OSC, HEX MOD, HEX CLICK, HEX MORE, HEX FILT and HEX AEG (see [Drums](#5-drums)).

**FILT** (in ELEMENTS, PLAITS and the JUNO and ACID pages above) is the voice's own one-knob filter: left of centre is a low-pass, right of centre is a high-pass, centre is off.

**ESYNC** (ENV DEST) makes ATK, DEC and REL note values of the tempo. **LFO SYNC**: SYNC locks the LFO to a note value; TRIG chooses what restarts it; POL chooses bipolar or unipolar.

| TRIG | The LFO restarts (at its PHASE) |
| --- | --- |
| **NOTE** | at each new phrase: a note played when no other note of the voice is held (as before) |
| **FREE** | never: it runs on by itself |
| **PATT** (pattern) | each time the voice's pattern comes round to step 1, and when PLAY starts. Notes in between do not restart it, so the LFO lines up with the pattern |

### What the graphs show

| Page | Graph |
| --- | --- |
| FILTER | the filter's response curve, with a wedge showing how far the envelope sweeps it |
| INSERT | the effect's transfer curve, or a swept-notch plot |
| VOICE | a note diagram |
| CHORUS | chorus lines |
| FX | the sends as drawn boxes |
| OSC | the waveform (a schematic for ELEMENTS and some PLAITS models) |
| ENV | the envelope (with a dim ghost of the character) |
| LFO | the LFO shape |
| OSC 2, ENV DEST, LFO DEST, VOICE 2 | bar graphs |

### INSERT types

| Type | A | B | C |
| --- | --- | --- | --- |
| SOFT, HARD, FOLD, FUZZ | DRIVE | TONE | LEVEL |
| CRUSH | BITS | RATE | LPF |
| PHASR, FLANG, CHOR | RATE | DEPTH | FDBK |

TYPE OFF (or MIX at 0) costs nothing. A, B, C and MIX can be locked on a step.

### Presets and INIT

- **PRESETS** (on HOME, on the SAVE > PRESETS page, or on any sound page) browses the selected voice's sounds, grouped by kind (BASS, KEYS, ORGN, PAD, LEAD, PLCK, STAB, FX, INIT), then your user presets. On the drum voice it picks the kit.
- **SAVE > PRESETS**: KNOB 1 the sound, KNOB 2 the engine (ACID or JUNO on voice 1, JUNO or ELEMENTS on voice 2).
- **SAVE > USER**: KNOB 1 SLOT (1 to 32), KNOB 2 LOAD, KNOB 3 ERASE, KNOB 4 SAVE (all GO).
- **SAVE > TOOLS**: KNOB 1 CLRSQ (clear the pattern), KNOB 2 INIT (reset the sound to the engine's defaults and first preset; the message says "SOUND INIT"), KNOB 4 NEW (all GO).

Changing a sound never changes the voice's key, chord mode, pattern or mix.

---

## 8. Performance effects and global effects

### Hold FX: PUNCH

Hold **FX** and press keys. The effect runs on the whole mix while you hold the key, and lets go cleanly when you release it. Keys pressed with FX held never play a note and are never recorded. Loops and the gate start on the grid.

**The 16 white keys** (four rows of four):

| Key | Effect | Key | Effect |
| --- | --- | --- | --- |
| 1 (F3) | **LOOP 4**: loops the last **quarter note** | 9 (G4) | **LOW**: low-pass sweep |
| 2 (G3) | **LOOP 8**: loops the last **eighth note** | 10 (A4) | **HIGH**: high-pass sweep |
| 3 (A3) | **LOOP 16**: loops the last sixteenth | 11 (B4) | **PHONE** |
| 4 (B3) | **LOOP 32**: loops the last thirty-second | 12 (C5) | **CRUSH** |
| 5 (C4) | **STUTTER** (beat divided by six) | 13 (D5) | **ALIAS** |
| 6 (D4) | **REVERSE** | 14 (E5) | **GATE** |
| 7 (E4) | **STOP** (tape stop) | 15 (F5) | **ECHO** |
| 8 (F4) | **HALF** (half speed) | 16 (G5) | **WOBBLE** |

**The 11 black keys:**

| Key | Effect | Key | Effect |
| --- | --- | --- | --- |
| F#3 | **KILL LOW** | F#4 | **FLANGER** |
| G#3 | **KILL MID** | G#4 | **AUTO-PAN** |
| A#3 | **KILL HIGH** | A#4 | **FREEZE** |
| C#4 | **REWIND** | C#5 | **LOOP 8T** (an eighth-note triplet loop) |
| D#4 | **OCT UP** | D#5 | **MONO** |
| | | F#5 | **FADE OUT** |

The loops replay the span you just heard, locked to the grid. The effect's buffer holds 0.74 seconds. On the layer's screen the keys show short names: lp 4, lp 8, lp 16, lp 32, stutr, revrs, stop, half, low, high, phone, crush, alias, gate, echo, wow, then -low, -mid, -hi, rwnd, oct+, flng, pan, frz, lp8t, mono, fade.

**Knobs while FX is held:**

| Knob | Does |
| --- | --- |
| KNOB 1 | master **FILTER**: left a low-pass, right a high-pass |
| KNOB 2 | **DUST** |
| KNOB 3 | **DUCK** |
| KNOB 4 | **TRK FILT**: the selected voice's own filter |

**FX + OCT-** is a **REV THROW** and **FX + OCT+** is an **ECHO THROW**. Both at once throw both. **FX + HOME** locks the layer open.

### Global effects (GLO pages)

Tap **GLO**. Tap again, or turn SELECT, for the next page. These pages are shared (tag ALL), except DRUMS, which counts as the drum voice (coral).

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **GLOBAL** | BPM | SWING | CLICK (OFF, REC, ON) | TUNE |
| **MASTER** | DUST | DUCK | FILT | ROLL |
| **DLY** | TIME | FEEDBK | COLOR | MIX |
| **REV/CHO** | SIZE | DAMP | CRT (chorus rate) | CDP (chorus depth) |
| **SYSTEM** | OUT | SYNC | IN | CPU (or USB state) |
| **DRUMS** | CH4 | LVL | REVERB | DELAY |
| **MIDI** | CH1 | CH2 | CH3 | CH4 |
| **MIDI 2** | IN | OTHER | OUT | BEND |
| **MIDI 3** | KNOBS | | | |

- **DUST** runs the mix through an old sampler and record: lower sample rate, fewer bits, a darker top end, and a little hiss and crackle while playing.
- **DUCK** makes every kick pump the synth voices down and up over an eighth note.
- **FILT** is a DJ filter: left low-pass, right high-pass, centre off.
- **ROLL** is the note-repeat rate of ARP + key.
- **DLY** TIME values include 1/4, 1/8, 1/16, 1/32, 8T, 16T and dotted values.

---

## 9. Mixer, mute and solo

**Hold GLO** for the mixer (or hold GLO untouched for about 0.7 seconds and let go to keep it open). It shows a podium for each voice, the characters, and M and S badges. The key legend reads "123D123DFBP T".

| Control | Does |
| --- | --- |
| **KNOB 1 to KNOB 4** | the levels of voices 1, 2, 3 and the drums |
| **keys 1 to 4** | mute voices 1, 2, 3, drums (yellow M) |
| **keys 5 to 8** | solo voices 1, 2, 3, drums (ice-blue S). Several solos add up |
| **key 9** | fill while held |
| **key 10** | fill on the next bar (again cancels it) |
| **key 11** | the knobs set the **pans** instead of the levels (title PAN); again: levels |
| **key 16** | tap tempo |
| **F#3** (black key) | MIDI LEARN on or off (the title reads "LEARN") |
| **SELECT** | tempo |

**Pan.** With GLO held (or the mixer kept open), press **key 11** (P): the title reads **PAN** and KNOB 1 to 4 set the pans of voices 1, 2, 3 and the drums (L, C, R). Press key 11 again for the levels. When the mixer closes, its knobs are the levels again. A synth voice's pan is also on **VOICE 2** (KNOB 3).

A muted voice fades out in a few milliseconds and plays no new notes, but its pattern runs on in time. On the HOME screen, KNOB 4 sets the selected voice's level (on a muted voice the first turn unmutes it). **VOICE 2** has PAN and MUTE. **GLO > DRUMS** has the drum level, reverb and delay.

---

## 10. Sections

A **section** is a stored loop: all four voices' sounds, patterns and kit. There are four, **A to D**.

### The SAVE layer (hold SAVE)

| Control | Does |
| --- | --- |
| **keys 1 to 4** | play section A to D. While playing, it starts on the next bar (the message says "NEXT: A"; the sub line reads "next bar"). A second tap in the same hold asks for that one instead. Stopped, it is loaded as the loop ("LOADED A"). An empty section says "EMPTY A" |
| **keys 5 to 8** | store the loop into A to D. Over a used section, press again within 3 seconds ("AGAIN: A", then "SAVED A") |

While a section plays, what you change in it is kept in it a moment after you stop touching the panel.

Holding and releasing SAVE with nothing else does not save anything. Tap **SAVE** on its own for the SAVE pages (PRESETS, USER, TOOLS), on every screen.

---

## 11. Recording live, erasing and clearing

**REC**

| You do | Result |
| --- | --- |
| **Tap REC while playing** | recording starts at once (on a synth voice's GRID too: the keys play notes there while recording; the title reads LIVE REC) |
| **Tap REC while stopped** | arms it ("REC READY"). Recording starts when you play. **On a synth voice's GRID it starts STEP REC instead** (see [Step record](#step-record-step-rec)) |
| **Tap REC again** | cancels ("REC OFF") |
| **Tap REC on an empty project** | a **free take**: it records without a tempo, and REC closes it |
| **Hold REC** (about 0.7 s, then the ring fills over about 1.3 s more) | **clears the selected voice** ("TRACK 1 CLEARED"). Let go before the ring is full and nothing happens. EDIT + OCT- brings it back |

On the REC screen: KNOB 1 **MODE** (on an empty project), KNOB 2 the length (1, 2 or 4 bars), KNOB 3 **START** (on a note, or with a count-in). The click setting is GLO > GLOBAL > CLICK (OFF, REC, ON). The beat lights turn red on beat 1 only while recording.

Notes are recorded as you play them, quantised to the voice's DIV. Rolls (hold ARP and a key) are recorded as ratchets.

### The EDIT layer: erase, shift, undo

Hold **EDIT**:

| Control | Does |
| --- | --- |
| **a key** | erases that note or drum sound ("ERASED"). While playing: from every step the playhead passes while you hold the key. Stopped: from the whole pattern |
| **KNOB 1** SHIFT | rotates the pattern |
| **KNOB 2** STEPS | turn right doubles the pattern (copying it after itself, up to 64 steps), left halves it |
| **KNOB 3** TRANSP | transposes (synth voices) |
| **OCT-** | undo |
| **OCT+** | redo |
| **a black key** (drums) | switch between erase and mute mode |
| **HOME** | lock the layer open |

The sub line reads "oct- undo", "oct+ redo" or "no undo yet". All the knob turns of one EDIT hold are one undo.

### The ARP layer: rolls

Hold **ARP** and a key for a note-repeat roll. KNOB 1 sets the rate (1/8, 1/16, 1/32, 32T, 1/64). On the drum voice, OCT- and OCT+ give ghost and hard rolls. Tap **ARP** for the arpeggiator pages: ARP (MODE, RATE, OCT, GATE) and ARP 2 (SWG, PROB, HOLD, ORD).

### Clearing a pattern

**SAVE > TOOLS > CLRSQ** (GO) clears the selected voice's pattern.

---

## 12. Saving

- **The unsaved dot.** A small white dot in the header means the project in memory differs from the one in flash. The firmware compares them twice a second.
- **Autosave.** It saves the working project by itself:
  - after you stop playing: 0.3 seconds after your last input, once the output is silent (under -60 dB), and at most every 5 seconds;
  - as a fallback: after 2.5 seconds with no input, at most every 20 seconds.
- **At power-on**, the working project comes back as you left it.
- **Saved only when you ask:** the sections A to D (see [section 10](#10-sections)) and your user presets (SAVE > USER).
- **Not part of a project** (they belong to the FM-1): SYNC, MIDI settings, the lights, the menu settings, the REC screen's MODE and START.
- **Presets and projects are written to flash only while stopped.**

---

## 13. Tempo, swing, key and scale

**Tempo** (40 to 240 BPM): **SELECT** on HOME, inside any layer or on the REC screen; **GLO + key 16** (tap tempo); or GLO > GLOBAL > BPM. With SYNC set to USB or TRS, the tempo follows the master clock.

**Swing:** hold SEQ + KNOB 3 (per voice), or GLO > GLOBAL > SWING (global).

**Metronome:** GLO > GLOBAL > CLICK.

### The SEQ layer (hold SEQ)

The keys do nothing. The screen draws the pattern in time with the swing, and the sub line reads "16 steps = 1 bar".

| Control | Does |
| --- | --- |
| KNOB 2 **DIV** | 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1BAR, 2BAR |
| KNOB 3 **SWING** | 0 to 100, per voice |
| KNOB 4 **STEPS** | 1 to 64 |

### The SEL layer (hold SEL): key, scale and chord

| Control | Does |
| --- | --- |
| **any key** | sets the **key of the song** (the root of all three synth voices) |
| KNOB 1 **CHORD** | OFF, TRIAD, 7TH, 9TH, SUS4, POWER |
| KNOB 2 **SCALE** | CHROM, MAJ, MIN, DOR, MIX, PEN, MPEN, HARM, PHRY, LYD, LOC, MEL, BLUES, WHOLE, DIMHW, DIMWH |
| KNOB 3 **KEYS** | OFF, SNAP (keys round down to the scale), WHITE (white keys walk the scale; black keys are silent) |
| KNOB 4 **TRANSP** | -24 to +24 semitones |

Tap **SEL** for the SEL pages: **SEL** (ROOT, SCALE, QNT, CHORD) and **SEL 2** (TRANS, STRUM, VLEAD). With a chord mode on, the white keys walk the scale from C4, each playing the chord of that degree. This is how a single finger plays a chord, and it is recorded as a chord. With a chord mode on, black keys change the chord while you play (F# major or minor, G# adds a 7th, A# sus4, C# adds a 9th, D# inversion). *This behaviour is carried over from KARMA's guide and was not re-checked in this pass.*

---

## 14. Screensavers

Tap **HOME** while you are on the HOME screen to open a screensaver. It fills the screen with no header. **HOME** again, or any other page button, closes it. **SELECT** steps through four: **FLOW**, **EYES**, **DESERT** and **THIRD EYE**. Its name shows for about a second, with "n/4". The choice is kept with your settings.

Keys, PLAY, REC and layers work as usual. A held layer shows its own screen and then the screensaver comes back. In the screensavers, each of the four voice colours is driven by that voice's own level.

At power-on, a boot screen of about 3 seconds shows a flowing backdrop (a different one each start) with PHE and BUS in black. Any key or note skips it.

---

## 15. Settings and menus

**Hold HOME** (about 0.7 seconds) to open the menu. **SELECT** changes section. **KNOB 1 to 3** set the section's rows. **PRESETS** moves the cursor. **OCT+** steps a setting or opens it. **OCT-** closes (from ABOUT it goes back one level).

| Section | Rows |
| --- | --- |
| **SCREEN** | COLOR, ZOOM |
| **LIGHTS** | LIGHTS, KEYS, NOTES |
| **AUDIO** | SPEAKER LOWCUT, USB AUDIO (MASTER, FULL, STEMS), USB SERIAL |
| **SYSTEM** | HARDWARE CALIBRATION, MIDI LEARN CLEAR, ABOUT |

These are settings of the FM-1: they are kept when you load a project or start a new one.

- **LIGHTS** makes the buttons glow so you can read them in the dark. **KEYS** lights some or all of the keys. **NOTES** lights the keys of the notes sounding.
- **USB AUDIO** and **USB SERIAL** take effect at the next start.
- **ABOUT** shows "PHEBUS" with four colour squares and "BASED ON KARMA 3.1.5", then the credits: KARMA by Ariel Muchnik, Felucca by Leo Kuroshita, Hügelton Instruments, "GPL-3.0, NO WARRANTY", and further credits. (The version text still says KARMA 3.1.5, which is the version PHEBUS was forked from.)
- **HARDWARE CALIBRATION**: press each button and turn each knob as the screen asks. 30 seconds without input cancels it.

Other settings live on pages: GLO > GLOBAL (BPM, SWING, CLICK, TUNE), GLO > SYSTEM and the MIDI pages (see [MIDI and USB](#16-midi-and-usb)).

---

## 16. MIDI and USB

*This section is carried over from KARMA's guide. The CC numbers, Program Change, sustain, pitch bend, channel handling and the USB port name were spot-checked against the code; the rest was not re-tested.*

On USB the FM-1 shows up as a MIDI device and an audio input. Its USB product name is still **FM-1 KARMA**.

### Where MIDI comes in

- The **MIDI IN jack** (3.5 mm TRS), through a TRS-to-DIN adapter. If nothing plays, try the other adapter type (A or B).
- **USB**, from a computer or phone. A USB keyboard plugged straight into the FM-1 cannot work, because both are USB devices.

### Settings (GLO > MIDI, MIDI 2, MIDI 3)

| Setting | Does |
| --- | --- |
| **CH1 to CH4** | the MIDI channel of voices 1, 2, 3 and the drums (defaults 1, 2, 3 and 10). OFF means nothing in or out for that voice. Two voices never share a channel |
| **IN** | **CH 1 to CH 16**: that channel plays the selected voice. **NOTES**: each channel plays its own voice. **CLOCK**: only clock and start, continue, stop |
| **OTHER** | what a channel no voice has does: SEL (plays the selected voice) or OFF |
| **OUT** | **KEYS**: only what you play goes out. **SEQ**: the sequencer, arpeggiator and rolls go out too |
| **BEND** | pitch bend range, 1 to 12 semitones |
| **KNOBS** | what a controller's eight knobs do: PANEL (the FM-1's eight rotaries), AUTO, 1-8, 70-77 (the sound's EDIT knobs) or GM |
| **SYNC** (GLO > SYSTEM) | INT (own tempo), USB or TRS (follow that MIDI clock) |

### CCs (KNOBS = GM, plus always-on ones)

| CC | Sets |
| --- | --- |
| 5 | GLIDE |
| 7 | LEVEL (on the drum channel: the drum level) |
| 10 | PAN |
| 71 | RES (engines that have one) |
| 72, 73, 75 | release, attack, decay |
| 74 | the voice's FILTER (64 is off) |
| 91, 93, 94 | reverb, chorus and delay sends |
| 64 | sustain pedal |
| 120, 123 | all sound off, all notes off |
| 121 | reset all controllers |

A **Program Change** on a voice's channel loads that voice's n-th sound (or kit n on the drum channel). **MIDI LEARN:** GLO + F#3, turn a knob on the screen, move a knob on your controller, then GLO + F#3 again. The menu's MIDI LEARN CLEAR forgets all learned CCs.

### Clock

START plays from the top, CONTINUE carries on, STOP stops. The tempo follows the master's clock. With no pulse for half a second, the FM-1 goes back to its own tempo. Bluetooth MIDI is not supported.

### USB audio

The FM-1 is also a class-compliant USB audio input (no driver). **MENU > AUDIO > USB AUDIO**:

- **MASTER** (default): the recording follows the MASTER knob.
- **FULL**: a fixed level, as with MASTER all the way up.
- **STEMS** (at the next start): six channels, each voice on its own (mono) plus the effects return (stereo), at 44.1 kHz.

---

## 17. The engines and the sound list

| Engine | Voice | What it is |
| --- | --- | --- |
| **ACID** | 1 | a TB-303 style voice (from X0X), one note at a time. A step's accent and slide are the 303's |
| **JUNO** | 1 and 2 | a Juno-106 style synth (from FM-106): DCO, filter, chorus, up to 6 notes |
| **ELEMENTS** | 2 | physical modelling (after Mutable Instruments Elements) |
| **PLAITS** | 3 | 24 synthesis models (after Mutable Instruments Plaits) |

OSC page knobs:

| Engine | OSC (KNOB 1 to 4) | OSC 2 |
| --- | --- | --- |
| ACID | WAVE, ACC, SLIDE, DRV | |
| JUNO | WAVE, PW, SUB, NOISE | |
| ELEMENTS | MODEL, STRC, BRIT, DAMP | POS, ACC, BOW, EXC |
| PLAITS | MODEL, HARM, TIMB, MRPH | ENV, SUB |

The factory sounds are listed by kind in PRESETS. Voice 1 offers the ACID and JUNO sounds, voice 2 the JUNO and ELEMENTS sounds, voice 3 the PLAITS sounds. Each list also has your user presets after the factory ones. A few examples: ACID has TB ACID, TB SQUARE, TB DIRTY, TB DEEP, TB SQUELCH and TB RUBBER. JUNO has JUNO BASS, SUB 106, JUNO KEYS, JUNO ORGAN, JUNO STRING, JUNO PAD, JUNO SWEEP, JUNO LEAD, JUNO HOOVER, JUNO BRASS and JUNO STAB. The INIT sounds are listed at the start of the ACID, JUNO and PLAITS sounds. ACID INIT (the power-on bass) is a plain saw with the filter part open, RES 45 %, ENVMOD 55 % and DECAY 55 %.

Engine details carried over from KARMA: ELEMENTS models are BOUNC, PICK, BOW, BLOW and OMIN (the Ominous Voice, one note at a time). PLAITS has FOLD, SWARM, WAVE, HARM, CHORD, VA, VCF, PHDIS, FM, 6OP A, B and C, GRAIN, TERRN, STRMC, CHIP, STRNG, MODAL, NOISE, SPECH, PARTC, KICK, SNARE and HIHAT. The heavy models (6OP, STRMC, STRNG, SPECH, PARTC) play one note at a time.

Where they come from: ACID is Charles Vestal's TB-303 voice from X0X (after Open303). JUNO is PatttF's Juno-106 voice from FM-106. PLAITS and ELEMENTS are built on Émilie Gillet's open-source code for Mutable Instruments. The drum voices come from the LXR-02 firmware by Erica Synths. PHEBUS is not affiliated with or endorsed by any of them.

---

## 18. Kits and the web editor

*This section is carried over from KARMA and was not re-checked against PHEBUS's colours or the editor's current pages in this pass.*

- **MY HEX 1 to 4** are the last four kits. A copy of a HEX kit is made the first time you turn a knob on a built-in HEX kit.
- The **web editor** (Chrome or Edge, FM-1 on USB, press Connect) follows the device live and edits sounds, sequences, songs, drum kits and settings. It also makes **backups** (Projects > Backup).
- Try the editor without hardware by adding `?mock=1` to its address. Its protocol is in [web/EDITOR_PROTOCOL.md](web/EDITOR_PROTOCOL.md).
- The browser simulator and its cheat sheet are in `web/emu/`.

A backup holds the music you are working on, the four projects, the 32 user presets, the MY HEX kits and the settings. Save a backup before you flash anything.

---

## 19. Every button gesture, screen by screen

Each row was checked against the input code (`ui_input.c`, `ui_layers.c`, `ui_grid.c`, `ui_stut.c`, `ui_studio.c`, `ui_menu.c`). "Twice" means two taps or two clicks within the stated time.

### Anywhere

| Gesture | Does |
| --- | --- |
| tap PLAY | start or stop |
| tap REC | record, arm, or close a free take (stopped, on a synth voice's GRID: STEP REC) |
| hold REC about 2 s | clear the selected voice |
| tap HOME | HOME screen (on the HOME screen: a screensaver) |
| hold HOME 0.7 s | the menu |
| tap ENV, LFO, FX, EDIT, ARP, SEL, GLO | open that button's pages |
| tap SEQ | the GRID (on the GRID: the SEQ pages; on a SEQ page: back to the GRID) |
| tap SAVE | the SAVE pages (PRESETS, USER, TOOLS), on every screen |
| hold a layer button, then tap HOME | lock that layer open |
| ALGORITHM | choose the voice (not during a free take or in the menu) |
| PRESETS | the sound list (drums: the kit) |
| SELECT | the next or previous page; on screens with no pages, the tempo |
| OCT- or OCT+ | octave -3 to +3 (both together: 0) |
| hold OCT- or OCT+, hit a drum key | ghost or hard hit |
| EDIT + OCT- or OCT+ | undo or redo |

### HOME

| Gesture | Does |
| --- | --- |
| KNOB 1 to 4 | CUTOFF, DECAY, STUT (the STUTTER lane on or off), LEVEL of the selected voice |
| SELECT | tempo |
| PRESETS | the sound list |
| tap EDIT (drums) | the drum GRID |

### Sound pages

| Gesture | Does |
| --- | --- |
| tap EDIT | enter OSC; on the chain, the next stage |
| tap ENV, FX, LFO | that stage's first page; again, its next page |
| SELECT | the previous or next page of the chain |
| KNOB 1 to 4 | the four values shown |

### GRID (synth voice)

| Gesture | Does |
| --- | --- |
| tap an empty step key | set it with the pen |
| tap a set step key | clear it on release (unless a knob was turned) |
| hold a step | select it and show its detail |
| hold a step + KNOB 1 to 4 | NOTE, LEN, VEL, COND |
| hold a step + SELECT right or left | FLAG, RATCH, NUDGE, or back |
| hold a step + tap EDIT, ENV, LFO, FX | parameter locks |
| hold a step + PRESETS or ALGORITHM | lock on or off, or choose the lock parameter |
| hold a step + OCT- | clear locks (and nudge and condition on the GRID) |
| hold a step + OCT+ | next condition |
| hold several steps | edit them together |
| black keys 1 to 4 | page 1 to 4 |
| OCT- or OCT+ (nothing held) | previous or next page (OCT+ on the last page adds one) |
| KNOB 1 to 4 (nothing held) | NOTE (pen), STEP, HIT, VEL |
| PRESETS or SELECT (nothing held) | the sound, or the tempo |
| ALGORITHM | change voice and stay on the GRID (past DRUM: STUTTER) |
| tap SEQ | the SEQ pages |
| tap REC, stopped | STEP REC on or off |
| tap REC, playing | record live (the keys play notes) |
| STEP REC: keys | a note, or a chord (keys held together), on the red step; the cursor moves on |
| STEP REC: SEQ, ARP | rest, tie (a key held + SEQ: tie) |
| STEP REC: OCT- or OCT+ | the octave |
| STEP REC: SELECT, KNOB 1 to 4 | the cursor; OCT, STEP, VEL, STEPS |
| STEP REC: EDIT + OCT- | undo the whole pass |

### GRID (drum voice)

| Gesture | Does |
| --- | --- |
| step keys | as on a synth voice, with the sound KNOB 1 picks |
| hold a step + KNOB 1 to 4 | LEVEL, RATCH, NUDGE, COND |
| KNOB 1 to 4 (nothing held) | SOUND, STEP, HIT, LEVEL |
| SELECT right or left | full grid, or back to the snake |
| PRESETS | the kit |
| tap SEQ | KIT, then back to the GRID |
| EDIT + black key | erase or mute mode |

### STUTTER lane

| Gesture | Does |
| --- | --- |
| tap an unlit or lit step | light it, or put it out (on release) |
| hold a step + KNOB 1 | that step's length (1/1 to 1/32; right: shorter) |
| KNOB 1 | the length for new steps (1/4 to start) |
| KNOB 2 | lane on or off (also KNOB 3 on HOME) |
| KNOB 4 | lane length |
| black keys 1 to 4, OCT-, OCT+ | page |
| hold FX + key | a performance effect while held |
| EDIT + OCT- | undo |

### Layers

| Hold | Gesture | Does |
| --- | --- | --- |
| FX | keys | punch effects |
| FX | KNOB 1 to 4 | master FILTER, DUST, DUCK, TRK FILT |
| FX | OCT- or OCT+ | REV THROW, ECHO THROW |
| EDIT | key | erase that note or sound |
| EDIT | KNOB 1 to 3 | SHIFT, STEPS (double or halve), TRANSP |
| EDIT | OCT- or OCT+ | undo or redo |
| ARP | key | note-repeat roll |
| ARP | KNOB 1 | roll rate |
| ARP | OCT- or OCT+ | ghost or hard rolls (drums) |
| SEQ | KNOB 2 to 4 | DIV, SWING, STEPS |
| SEL | key | key of the song |
| SEL | KNOB 1 to 4 | CHORD, SCALE, KEYS, TRANSP |
| GLO | KNOB 1 to 4 | levels of 1, 2, 3, drums |
| GLO | keys 1 to 4, 5 to 8 | mute, solo |
| GLO | keys 9, 10, 16 | fill while held, fill next bar, tap tempo |
| GLO | key 11 | the knobs set the pans (again: levels) |
| GLO | F#3 | MIDI LEARN |
| SAVE | keys 1 to 4 | play A to D (playing: on the next bar) |
| SAVE | keys 5 to 8 | store into A to D |

### REC screen

| Gesture | Does |
| --- | --- |
| KNOB 1 to 3 | MODE, length, START |
| REC | record, or cancel |

### Screensaver and menu

| Gesture | Does |
| --- | --- |
| SELECT (screensaver) | the next screensaver |
| HOME or any page button (screensaver) | close it |
| SELECT (menu) | the next section |
| KNOB 1 to 3 (menu) | the rows |
| PRESETS (menu) | move the cursor |
| OCT+ or OCT- (menu) | step or open; close |

### At power-on and special

| Gesture | Does |
| --- | --- |
| hold OCT- while switching on | USB rescue (PHEBUS USB RESCUE) |
| hold OCT- and OCT+ while switching on | hardware calibration |
| hold OCT- and OCT+ for 5 seconds, stopped | update mode (UBOOT); the screen counts down, let go to cancel |
| any key or note during the boot screen | skips it |

---

## 20. Troubleshooting and recovery

| Situation | What to do |
| --- | --- |
| The unit will not start | hold **OCT-** alone while switching on. The screen reads "PHEBUS USB RESCUE". Then install again. If the firmware fails to start twice in a row it also comes up in rescue by itself |
| An install stopped half-way | the FM-1 waits in update mode: install again |
| Buttons or knobs do the wrong thing | hold **OCT- and OCT+** while switching on, or MENU > SYSTEM > HARDWARE CALIBRATION |
| A developer needs UBOOT | hold OCT- and OCT+ for 5 seconds while stopped ("UPDATE MODE IN 3..."); let go to cancel |
| The black keys are silent on a synth voice | the voice plays a scale or chord: SEL layer, CHORD OFF and KEYS OFF |
| "STOP FIRST" | the change waits until the transport is stopped |
| "EMPTY SECTION" | the section has nothing in it. Store a loop into it first |
| Notes fade on a dense part | the processor is at its limit. Mute a voice, use fewer held notes, or a lighter sound. The load shows on GLO > SYSTEM, KNOB 4 |
| Nothing from the MIDI IN jack | try the other adapter type, and check the channel and that IN is not CLOCK |

**Going back to the official firmware.** *Carried over from KARMA's guide, not tested with PHEBUS.* Save a backup with the web editor first. Download M-VAVE's updater, M-UPGRADE, and the official FM-1 firmware from m-vave.com, close every app that uses MIDI, and run the update. An FM-1 goes back only to the official version it last had.

**Backups.** Because PHEBUS is unreleased and untested on hardware, back up everything the FM-1 holds before you flash anything, and read [BUILDING.md](BUILDING.md) before you build.

### Checks still to do

These could not be confirmed from the code and are not described above as fact:

- how it all behaves on a real FM-1;
- that a flash save never stalls the screen or the audio;
- the -60 dB silence threshold for the stop autosave against a real noise floor;
- the web editor's pages against the new firmware.

---

## 21. Credits

PHEBUS is Ben's fork of **KARMA** by Ariel Muchnik. KARMA is a fork of **SLOOP** by isod89, which is based on **Felucca** by Leo Kuroshita (Hügelton Instruments). It also uses code from X0X (Charles Vestal), FM-106 (PatttF), Melodee (Kerem Kilic), Mutable Instruments' Plaits and Elements (Émilie Gillet), the LXR-02 firmware (Erica Synths), and JIANT FM (Juan Jiant). The screen font is Terminus.

Licence: GPL-3.0-only. See [LICENSING.md](LICENSING.md) and [LICENSE](LICENSE). PHEBUS comes with no warranty. M-VAVE and FM-1 are trademarks of their owners; PHEBUS is not affiliated with M-VAVE. The README of SLOOP is kept as [SLOOP-README.md](SLOOP-README.md).
