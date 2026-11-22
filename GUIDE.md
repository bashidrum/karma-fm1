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
17. [The oscillator Types](#17-the-oscillator-types)
18. [Kits and the backup page](#18-kits-and-the-backup-page)
19. [Every button gesture, screen by screen](#19-every-button-gesture-screen-by-screen)
20. [Troubleshooting and recovery](#20-troubleshooting-and-recovery)
21. [Credits](#21-credits)

---

## 1. What PHEBUS is

PHEBUS turns the FM-1 into a four-voice groovebox: three synth voices and one drum voice, each with its own sequencer track of up to 64 steps.

Each voice has its own colour, and a small head in the bottom strip and on HOME:

| Voice | Head | Colour | Plays |
| --- | --- | --- | --- |
| **1** | robot | purple | any of the 31 oscillator Types (starts as ACID, a 303 bass) |
| **2** | alien | blue | any of the 31 oscillator Types (starts as JUNO, polyphonic chords) |
| **3** | crocodile | teal | any of the 31 oscillator Types (starts as PLAITS VA, a lead) |
| **Drums** | snake | coral | the drum synth, with 16 sounds on the keys |

The three synth voices work the way the Arturia MiniFreak does: the same pages on every voice, an oscillator **Type** with three macro knobs, one resonant **filter** with its own envelope, the amp **envelope**, then FX and modulation. See [Sound design](#7-sound-design-along-the-chain).

**Colour has a meaning.** A voice's colour always means "this acts on that voice". White and grey mean shared: the screen acts on everything, and its header tag says ALL. Yellow is for **M** (mute). Ice blue is for **S** (solo). Red is only for recording and erasing.

**Tap and hold.** Most buttons do two jobs. Tap one and it opens its pages. Hold one and it becomes a layer: the keys and knobs do something else for as long as you hold it. The layer names are:

| Hold | Layer | Used for |
| --- | --- | --- |
| FX | PUNCH | performance effects on the keys |
| EDIT | ERASE | erase, shift, undo and redo |
| ARP | ROLL | note-repeat rolls (on the drum voice: RETRIG) |
| SEQ | STEPS | pattern time: division, swing, length |
| SEL | KEY | key, scale and chord (on the drum voice: SOUND, the sound pick) |
| GLO | MIX | levels, mute, solo, fills, tap tempo |
| SAVE | SECTION | sections A to D |

A layer shows its name in the header ("FX: PUNCH") once you have held it for a moment. To keep a layer open without holding it, hold its button and tap HOME. Any other button lets it go again, except PLAY, REC, OCT- and OCT+. If you hold GLO for about 0.7 seconds without touching anything else and then let go, the mixer stays open.

After a layer lets go, the knobs ignore you for a quarter of a second, so a knob still turning does not change the page underneath.

---

## 2. The screen at a glance

**The header** runs along the top:

- a boxed **tag** showing what the screen acts on: the synth voice's number (**1**, **2**, **3**), DRUM, HOME, STUT (the stutter lane) or ALL (shared pages, in grey);
- the **sound's name** on a synth voice's sound pages and GRID (the kit on the drum screen; elsewhere the page name; while a fill is on: **FILL** in white, see [Fill](#fill));
- the **tempo** (BPM) and a play or stop symbol;
- four **beat lights**. While you record, only the light on beat 1 turns red;
- a red **record disc** while recording, or a small white **unsaved dot** when the project in memory differs from what is saved (see [Saving](#12-saving));
- the **battery**, three bars, red when low.

**The bottom strip** shows the four voices' heads, numbered 1 2 3 DR. The selected voice is underlined and its number is in its colour. A yellow **M** on a head means muted, an ice-blue **S** means soloed, a red dot means it is recording. On the STUTTER lane no voice is underlined.

**The dials** at the bottom of a page show what KNOB 1 to KNOB 4 do: four dials, each over its knob, with its name above and its value beside it in one tall face, in a fixed column (a value never runs into the next one). The knob you are touching has an inverted label and an underlined value. A dim grey value means it does nothing in the current setting (or cannot be locked while a step is held). A **locked** value is drawn **inverted**, in a box of the voice's colour. A short message appears as a white bar over the dials. On the shared list pages (GLO, SAVE > TOOLS) the list names the four knobs with a bar for where each one sits; the values are in the dials only.

**The tabs.** On a synth voice's sound pages, a row above the dials lists the five stages of the sound chain, OSC FILT ENV FX MOD, the current one inverted in the voice's colour; "1/3" and so on at the right is the page (the "half") within it. On the GRID the row starts with TRIG, as on the drum screen. See [Sound design](#7-sound-design-along-the-chain).

**Pictures.** A synth sound page shows its subject large in the voice's colour (OSC the Type, FILT the filter type, ENV AMP or FILTER, FX and MOD the page), a line under it, and a drawing of what the knobs do: the oscillator's wave, the filter's response with the envelope's sweep, the envelope or envelopes, the FX slots and their effect, the LFOs, and the mod matrix's grid. No character and no scene behind it.

### The HOME screen

Tap **HOME** for the HOME screen. The header reads PATTERN A to D while one of the sections plays, otherwise PHEBUS. At the top: the bar counter, a strip of the **FLOW** backdrop, and the sections A to D. In the strip each voice has a band in its colour that swells as that voice gets louder and sinks to a dim line when it is quiet. It flows while the song plays or a voice sounds, and stands still otherwise. Under them, the selected voice's sound and engine (and REC, SOLO or MUTE), then a lane for each voice: its character, the 16 steps in view, the step playing and its level.

The four knobs play the selected voice, for quick changes while it plays:

| Knob | Does |
| --- | --- |
| **KNOB 1 CUTOFF** | the voice filter's CUTOFF, on every synth voice; the drum track's FILTER on the drums |
| **KNOB 2 DECAY** | the decay of the envelope that moves the sound: the filter's own envelope's DECAY while it is OWN (as ACID starts), else the amp ENV's DECAY; on the drums, the selected sound's decay on every kit (the first turn on a built-in kit makes your copy, see [Drums](#5-drums)) |
| **KNOB 3 STUT** | the STUTTER lane **ON** (turn right) or **OFF** (turn left). It is drawn in white and grey because it acts on the whole mix. See [The STUTTER lane](#6-the-stutter-lane) |
| **KNOB 4 LEVEL** | the voice's level. On a muted voice the first turn unmutes it |

Swing and the pattern length are on **hold SEQ**. Pan is on the mixer (**GLO + key 11**).

---

## 3. Your first sound

A new project (and a power-on with nothing saved) starts with plain sounds and **no effects on**, so you hear the raw voice:

| Voice | Starts as |
| --- | --- |
| 1 | ACID INIT: a plain sawtooth through the 303's own filter, part open with a little resonance; the filter has its own envelope (OWN) that sweeps it on each note so it speaks. No accent, slide or drive. One note at a time, an octave down. Good for bass |
| 2 | JUNO INIT, a plain sawtooth, polyphonic, the filter open. Good for chords |
| 3 | PLAITS INIT, a plain sawtooth (the VA Type), one note at a time, the filter open, about 6 dB quieter. Good for lead |
| Drums | the 606 kit |

Every voice's three FX slots start OFF (no drive, no chorus, delay or reverb send) and the mod matrix empty. **SAVE > TOOLS > NEW** puts a project back to this starting point.

To make a first sound:

1. Press **PLAY**. The beat lights run.
2. Play the keys. On a synth voice, the white and black keys are notes. On the drum voice, the keys are 16 drum sounds.
3. Turn **ALGORITHM** to pick a voice: 1, 2, 3 or DRUM.
4. Turn **PRESETS** to browse that voice's sounds (every voice lists every sound). Each engine's sounds start with an **INIT** sound: the plain starting point again.
5. Tap **EDIT** to open the sound's first page, OSC. **KNOB 1** picks the oscillator **Type**; KNOB 2 to 4 are its three macros. Tap EDIT again for the filter.
6. Tap **OCT-** or **OCT+** to change the octave of a synth voice, from -3 to +3. Press both together to go back to 0.

**Drum keys:** hold **OCT-** while you hit a drum key for a ghost (soft) hit, or **OCT+** for a hard hit.

---

## 4. The GRID: placing steps

The GRID is the step sequencer, and it is the only step editor. **Tap SEQ** (from HOME or from any other page) and the selected voice's GRID opens. This works the same on every voice.

On the GRID the 16 white keys are the **16 steps of the page shown**. They play nothing. The header shows the sound's name (STEP REC or LIVE REC while recording). Under the tiles: the notes of the step held or under the cursor, the pen, and which steps are shown ("1-16 OF 32"). **ALGORITHM** changes voice and stays on the GRID. **HOME** goes back.

On a synth voice the GRID works like the drum screen, the Elektron way: under the steps a row of tabs, **TRIG OSC FILT ENV FX MOD**, and four dials. The dials show the tab's page:

- **TRIG**: the steps themselves (the pen and the cursor with nothing held; the held step's own values with a step held).
- **OSC** to **MOD**: the sound's pages, the same as on the sound pages ([section 7](#7-sound-design-along-the-chain)). With nothing held the knobs edit the sound. With a step held they **lock** (below).

| Button on the synth GRID | Tab |
| --- | --- |
| **EDIT** | OSC; again: the next stage (FILT, ENV, FX, MOD, round) |
| **ENV**, **FX**, **LFO** | ENV, FX, MOD; again: that tab's next page |
| **SEQ** | TRIG; again: back to the tab before |
| **SELECT** | every page of every tab in turn, TRIG first |

These work the same with a step held. **SEQ** on a sound page opens the GRID on that page's tab; SEQ from HOME or anywhere else opens it on TRIG.

### The tiles

Each step is a tile in the voice's colour:

- a set step shows its note name (without the octave) and 1 to 4 small octave ticks;
- white text means an accent, a small mark means a slide;
- a bar along the tile is its velocity;
- a line joining notes is a chord, and a bar from the note before it is a **tie**;
- a small square in the lower right means the step has parameter locks; a dot at the top means a nudge;
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

While you hold a step on the **TRIG** tab, the knobs edit it. A step you edited is kept, never cleared when you let go. Hold several steps and they are edited together.

| TRIG (synth voice) | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **TRIG 1/2** | **NOTE**, by degrees of the scale (you hear it) | **LEN**, in steps (the following steps become ties) | **VEL**, 1 to 127 (turning it clears the accent) | **COND**: -, FILL, NOFL, CHNC |
| **TRIG 2/2** (SELECT right) | **FLAG**: -, ACC, SLD, A+S | **RATCH**: x1 to x4 | **NUDGE** | |

On the drum voice each sound has its own trigs: a held trig's own values are on the TRIG page, and the knobs on the other pages lock the sound (see [Drums](#5-drums)).

- **Conditions.** FILL plays only during a fill. NOFL plays except during a fill. CHNC plays about one time in two. A skipped step is silent in full: no note, no MIDI, no ratchet, no lock. Start a fill with **GLO + key 9** (while held) or **GLO + key 10** (the whole next bar; again cancels it), or on the drum screen with **F#5** (held; twice quickly: latched).
- **Ratchets** repeat the step 1 to 4 times inside its time.
- **Nudge** moves a step slightly off the grid.
- Held step + **OCT+** goes round the conditions (the message says NORMAL, FILL ONLY, NO FILL or CHANCE).
- Held step + **OCT-** clears. On TRIG it clears the step's locks, nudge and condition together. On a sound tab it clears the lock of the knob you last turned in this hold; press it again (or with no knob turned) to clear every lock of the step.

### Parameter locks

A lock gives one sound parameter a different value on one step only. It works the Rytm's way, as on the drum screen, with no extra press:

1. Hold a step.
2. Turn any knob on a sound tab (OSC, FILT, ENV, FX or MOD). That parameter is locked on the held step (or steps). Its value is drawn **inverted**; the step's line under the tiles lists its locks, and its tile gets a small square.

The page buttons still change the tab while the step is held, so you can lock the cutoff, then tap ENV and lock the decay, on the same hold. The Type (OSC, KNOB 1) cannot be locked, as the Rytm's machine cannot; nor can VOICE and PRIOR (they are drawn grey while a step is held). Locking a filter envelope value while the filter is LINKed to the amp ENV keeps the lock but has no effect until the filter has its OWN envelope.

There is a limit of **24 locks per voice**. Messages you may see: NOT LOCKABLE, NO LOCK LEFT, LOCKS FULL: HOLD FEWER STEPS, LOCKS FULL: 24 A TRACK, LOCK OFF: (name), n LOCKS OFF.

### Pages, length and follow

A pattern is up to 64 steps, in four pages of 16.

- **Black keys 1 to 4** pick page 1 to 16, 17 to 32, 33 to 48 or 49 to 64.
- The slot after the last used page adds a page: it is a copy of the last page (the message says "32 STEPS" and so on). Other empty slots say "ADD PAGE n".
- **OCT-** and **OCT+** with no step held go to the previous and next page. **OCT+** on the last page adds a page. At 64 steps the message says "64 STEPS: THE MOST" and OCT+ wraps to page 1.
- Under the tiles, four page slots: the used pages are filled, the unused ones outlined, and the page that is playing is marked.
- **Follow.** While playing, the view follows the playhead onto a new page. A page you pick by hand stays until the playhead moves to a different page.
- The pattern length can also be set with **hold SEQ + KNOB 4** (see [section 13](#13-tempo-swing-key-and-scale)), or with KNOB 4 during STEP REC.

### With no step held (synth voice)

On a sound tab the knobs edit the sound, as on the sound pages. On **TRIG 1/2**:

| Control | Does |
| --- | --- |
| KNOB 1 **NOTE** | the pen (you hear it) |
| KNOB 2 **STEP** | the cursor |
| KNOB 3 **HIT** | right sets the cursor's step, left clears it |
| KNOB 4 **VEL** | the velocity of the cursor's step |
| **PRESETS** | picks the sound |
| **SELECT** | the tabs (the tempo: on HOME) |

### Spice and Dice (synth voice)

The MiniFreak's way of varying a pattern without rewriting it. On **TRIG 2/2** with no step held (turn SELECT right on the TRIG tab):

| Control | Does |
| --- | --- |
| KNOB 1 **SPICE** | how much of the variation you hear, 0 to 100 %. At 0 the pattern plays exactly as written |
| KNOB 2 **DICE** | each click to the right rolls a new variation (DICE #n); each click left goes back to the roll before |

A roll gives every step its own random changes: left out (up to about 30 % of the notes at full Spice), an octave up or down (25 %), a ratchet of two or three hits (20 %), a different velocity (up to 48 either way) and a different length (a quarter to 1.75 times). More Spice is the same roll, further: the steps that vary at a little Spice keep varying the same way as you turn it up. The same roll at the same Spice plays the same variation every loop. **The notes you wrote never change**: turn Spice back to 0 and the pattern is as it was. The tiles show the variation while Spice is up: a step left out is drawn dim with a line through it, an octave as a small arrow at its top left, a ratchet as ticks at its right edge, the length as a thin line under it. Spice and Dice belong to the pattern (each A to D section keeps its own) and to the melodic voices only (the Rytm has nothing like it on its drums).

### Pattern settings

The pattern's GATE, DIV, SWING and STEPS are on **hold SEQ** ([section 13](#13-tempo-swing-key-and-scale)): KNOB 1 GATE (how long each note sounds within its step), KNOB 2 DIV, KNOB 3 SWING, KNOB 4 STEPS. **DIV** values: 1/4, 1/8, 1/16 (the usual), 1/32, 8T, 16T, 1/2, 1BAR, 2BAR.

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
- STEP REC is for the synth voices. On the drum screen and the STUTTER lane, REC works as before.

### Undo and redo

**EDIT + OCT-** undoes and **EDIT + OCT+** redoes. A full undo restores note, length, locks, nudge and condition. Everything you change by knob during one EDIT hold counts as one undo. See [section 11](#11-recording-live-erasing-and-clearing).

---

## 5. Drums

The drums work the way the Elektron Analog Rytm MKII does: pick a sound, the 16 keys are its trigs, hold a trig and turn a knob to lock that value on it, and every sound has the same four pages, **SRC FLTR AMP LFO**. The drum voice (coral, the snake) has 16 sounds:

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sound | KICK | KICK 2 | SNARE | CLAP | HAT | OPEN HAT | PEDAL | RIM |

| Key | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Sound | SNARE 2 | LOW TOM | HI TOM | CRASH | RIDE | SHAKER | CONGA | COWBELL |

(The closed hat cuts the open hat.)

### The drum screen

Tap **SEQ** or **EDIT** with the drum voice selected. The screen shows:

- the **selected sound's name**, large, and under it how many trigs it has (or "TRIG 5 HELD");
- top right, the **sound map**: 16 tiles, one a sound, in the order of the keys. A sound with trigs is dim coral and lights up as it plays; the selected one is framed in white; a yellow bar means muted, ice blue soloed;
- the selected sound's **16 trigs** of the page shown. A trig is filled (its shade is its velocity: dim ghost, coral normal, white hard). A black square in it means it carries locks, three lines a retrig, a notch in its corner a condition. A held trig is framed white, the playhead is a white bar under the trig playing, and small dots mark the quarters;
- a line with what the held trig holds (RTRG 1/16, LOCK DECAY ...), or which steps are shown, and the four page slots at the right;
- the **tabs** TRIG SRC FLTR AMP LFO, the page shown filled, and "1/2" when the page has more than one half;
- **four dials**, each over its knob: the name and the value in one size. A value locked on the held trig is drawn inverted (a coral box). The knob you are turning has its name inverted and its value underlined.

### Picking a sound

**Hold SEL and press a key**: that sound is selected (you hear it). The steps, the pages and chromatic mode then act on it.

### Placing trigs

The 16 white keys are the selected sound's steps of the page shown. Tap an empty step to set a trig (velocity 100, or what TRIG's KNOB 1 sets); tap a trig to clear it when you let go. **Black keys 1 to 4** pick the page of steps (1-16 .. 49-64), **OCT-** and **OCT+** go to the page before and after (OCT+ on the last page adds a copy of it). Pages, length and follow work as in [section 4](#4-the-grid-placing-steps).

### The sound pages

| Button | Opens |
| --- | --- |
| **EDIT** | SRC. On the drum screen, EDIT again goes to the next page: SRC > FLTR > AMP > LFO > SRC |
| **ENV** | AMP. Again: its next half |
| **LFO** | LFO. Again: its next half |
| **SEQ** | TRIG (the held trig's own values). Again: back to the page before |
| **SELECT** | every half of every page, TRIG first, both ways |

These all work while you hold a trig, as the Rytm's page keys do.

With nothing held, a knob changes the selected sound. **Every kit can be edited.** The first knob you turn on a built-in kit makes your own copy of that kit in the project (the message says YOUR COPY OF THE KIT, and the kit's name in the header gets a *, as "606*"). The built-in kit itself never changes. Your copy is saved with the project and plays whenever the drum voice is on the kit it came from; PRESETS to another kit and back finds it again. A project holds one copy: if it holds the 606's and you turn a knob on the 909, the message says AGAIN: REPLACE 606*, and only a second turn within 1.5 seconds replaces it. In your copy all 16 sounds are separate (in the built-in kits PEDAL shares the closed hat's sound).

What the pages hold depends on the kit's machine. The page order is the same on every sound:

| Page | HEX kits (HEX BOOM .. HEX ROOM, MY HEX 1 to 4) | Synthesised kits (606, 808, 909 and the rest) |
| --- | --- | --- |
| **SRC 1/3** | TUNE, P.AMT (pitch sweep), P.DEC (its time), WAVE | TUNE, FINE, WAVE, TONE (the tone's level) |
| **SRC 2/3** | MODEL, MOD (the modulator's wave, or REPT on SNARE and CLAP), M.PIT, FM (named per model) | SWEEP (semitones of pitch drop), SWTIME, PART2 (a second partial), P2 LVL |
| **SRC 3/3** | TRANS (the transient), T.LVL, T.PIT, PSLOPE (FINE on CYM and HAT) | NOISE (OFF, WHITE, METAL, CYM, CHIP, CLAP), N.LVL, CLICK, CHIP |
| **FLTR** | FREQ, RES, TYPE, DRIVE | 1/2: FREQ, RES, TYPE (LP, BP, HP; LP+ and so on filter the tone too), ENV. 2/2: HPF (the noise's high-pass) |
| **AMP 1/2** | ATK, DEC, SLOPE, DIST | HOLD, DECAY, N.HOLD, N.DEC (the noise's) |
| **AMP 2/2** | CRUSH, DEL, REV, LEVEL | DRIVE, DEL, REV, LEVEL (dB) |
| **LFO 1/2** | SPEED, DEPTH, DEST, WAVE | the same |
| **LFO 2/2** | SYNC, FADE, PHASE, MODE | the same |

- **DEL** and **REV** are the sound's own delay and reverb sends, added to the drum voice's (GLO > DRUMS).
- **LFO**: every sound has its own. DEST is what it moves: PITCH (up to 12 semitones), CUT, RES or LEVEL. DEPTH 0 is off; left of centre it moves the other way. SYNC OFF uses SPEED; otherwise one cycle lasts a note value of the tempo (4BAR to 1/32). FADE right fades it in after the hit, left fades it out. PHASE is where a hit starts it. MODE: TRIG restarts it at each hit, FREE lets it run, ONE plays one cycle and holds.

### Locks: hold a trig and turn

Hold a trig and turn any knob on the page shown: that value is **locked on that trig** of that sound only (the kick on the same step and the snare on other steps are not changed). It is drawn inverted while you hold the trig. Change page with EDIT, ENV, LFO, SEQ or SELECT while holding to lock values on other pages. Hold several trigs to lock them together. Every kit's sounds lock, the 606 too, and the kit is not copied for a lock. MODEL cannot be locked (NOT LOCKABLE). A pattern holds up to 128 sound locks (SOUND LOCKS FULL: 128).

- **Held trig + OCT-**: the lock of the knob you last turned goes (LOCK OFF: DECAY). OCT- again, or with no knob turned, takes every lock off the trig (3 LOCKS OFF).
- On **TRIG**, held trig + OCT- clears everything the trig holds: its condition, chance, nudge, retrig, every lock, and the drum voice's own locks on that step.
- **Held trig + OCT+** goes round the trig's conditions.
- **Held trig + FX**: the drum voice's own pages (its FILTER, INSERT, SLICER) on the knobs, locking per step for the whole drum voice. The tabs read LOCK and the page's name. SELECT goes back to the sound's pages.

### TRIG: the trig's own values

| TRIG | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **1/2** | **VEL**, 1 to 127 | **RETRIG**: OFF, 1/1, 1/2, 1/3, 1/4, 1/5, 1/6, 1/8, 1/10, 1/12, 1/16, 1/20, 1/24, 1/32, 1/40, 1/48, 1/64, 1/80 | **NUDGE** | **COND**: -, FILL, NOFL, CHNC |
| **2/2** | **CHANCE**, 1 to 99 % (what CHNC plays) | **LENGTH** of the retrig, 1/8 step to 16 steps, or INF | **CURVE** of the retrig's velocity, -128 (fades out) to 127 (fades in) | **NOTE**: the trig's pitch, in semitones (a lock) |

- With nothing held, TRIG's KNOB 1 sets the velocity new trigs get (100 to start; not saved). The other knobs say HOLD A TRIG.
- **Retrig** repeats the hit at its rate for its length. INF repeats it until that sound's next trig. CHANCE, LENGTH and CURVE show grey until they count.
- **Nudge** and **condition** are the trig's own: two sounds on one step can sit at different nudges and conditions.
- A pattern has room for 64 trigs with a condition, nudge or retrig (TRIG EXTRAS FULL: 64).

### Fill

On the drum screen, **hold F#5** (the last black key): a fill plays for as long as you hold it. **Press F#5 twice quickly** to latch the fill on; press it twice again (or STOP) to end it. A fill makes the trigs set to FILL play and those set to NOFL rest. While a fill is on, **FILL** shows in white in the header of every screen. GLO + key 9 (held) and GLO + key 10 (the next bar) work as before.

### MUTE mode

**Tap SEL** on the drum voice. The screen shows the 16 sounds as tiles and the keys mute and unmute them: a muted sound is dark with a yellow **M**. **Hold SEL and press a key** to solo a sound (ice-blue **S**; several solos add up; again: off). Muted sounds keep their trigs and the keys and MIDI still play them; only the sequencer skips them. Mutes and solos belong to the performance: they are not saved. Tap SEL again for the steps. (Hold EDIT and press a black key still turns EDIT's white keys into mutes too.)

### CHROMATIC mode

**Tap ARP** on the drum voice. The 27 keys play the selected sound at pitches, as a keyboard: C4 (white key 5, marked with a dot) is the sound's own pitch, every key a semitone from it. **OCT-** and **OCT+** move the keys an octave (-2 to +2; both: 0). The knobs still edit the page shown. **Recording live**, each trig keeps the pitch it was played at (a NOTE lock; one at the sound's own pitch has none). Tap ARP again for the steps. Leaving the drum screen ends chromatic and MUTE mode.

### RETRIG (hold ARP)

**Hold ARP** on the drum voice and press a key: the sound retriggers at the rate shown for as long as you hold it. The **black keys** pick the rate: F#3 1/4, G#3 1/6, A#3 1/8, C#4 1/12, D#4 1/16, F#4 1/24, G#4 1/32, A#4 1/40, C#5 1/48, D#5 1/64, F#5 1/80 (of a bar). **KNOB 1** goes through all of them, 1/1 to 1/80. OCT- and OCT+ held give ghost and hard hits. Recorded, a retrig faster than a step becomes a trig with a one-step retrig at that rate; slower, a trig on each hit.

### Recording

Recording on the drum voice keeps each hit's velocity (from MIDI as played, from the keys 100, ghost and hard with OCT- and OCT+ held). EDIT + KNOB 1 (SHIFT) and the length x2 take each trig's extras and locks with it.

### Kits

**PRESETS** picks the kit. The list: five HEX kits (HEX BOOM, HEX ACID, HEX METAL, HEX GLITCH, HEX ROOM), then 32 synthesised kits (808, 909, **606** (the power-on kit), 80S, VINTAGE, TRAP, DRILL, BOOMBAP, LO-FI, PHONK, HOUSE, D.HOUSE, TECHNO, MINIMAL, ELECTRO, DISCO, GARAGE, JUNGLE, DUBSTEP, DEMBOW, AMAPIANO, AFRO, LATIN, TRIBAL, SYNTHWV, CHIP, ARCADE, GLITCH, INDUSTR, HYPER, AMBIENT, JAZZ), then MY HEX 1 to 4. A project saved on one of KARMA's user sample kits (no longer in PHEBUS) plays the 606. **SAVE > TOOLS > NEW** goes back to the built-in 606 (your copy goes).

### The drum voice's own pages

**FX** on the drum voice opens its own pages, which act on the whole drum voice: **INSERT**, **INSERT 2** and **SLICER** (SELECT left from INSERT reaches its **FILTER**). **HOME knobs** for the drum voice: CUTOFF (the drum voice's FILTER), DECAY (the selected sound's DEC or DECAY, on every kit; the first turn makes your copy of a built-in kit), STUT and LEVEL.

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

Every synth sound is one **chain of five stages**, in signal order, the same on every voice whatever its Type (the MiniFreak's layout):

**OSC** > **FILT** > **ENV** > **FX** > **MOD**

The oscillator makes the sound, the filter shapes it (before the amp, so a ringing filter dies with the note), the amp envelope shapes its level.

| Button | What it does on the chain |
| --- | --- |
| **EDIT** | from anywhere else it enters at OSC. When you are already on the chain it steps to the next stage and wraps from MOD back to OSC |
| **ENV**, **FX**, **LFO** | jump to that stage's first page. Press again for the stage's next page |
| **SELECT** | walks every page of the chain, one by one, and stops at both ends |

On the drum voice, **EDIT**, **ENV** and **LFO** open the drum screen's sound pages (SRC, AMP, LFO; see [Drums](#5-drums)), and **FX** the drum voice's own FILTER, INSERT and SLICER.

### Pages by stage

Each stage's pages are shown as halves, "1/3" at the right of the tabs.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **OSC 1/4** | **TYPE** (31, one a detent) | macro 1 | macro 2 | macro 3 |
| **OSC 2/4** | **OCT** (-3 to +3 octaves) | **TUNE** (-99 to +99 cents) | **GLIDE** | **VOICE** (POLY, MONO, LEG, UNI) |
| **OSC 3/4** | the Type's 4th control | the Type's 5th control | **DTUNE** (unison spread) | **PRIOR** (LAST, LOW, HIGH) |
| **OSC 4/4** | **GLMODE** (RATE: GLIDE is the time an octave takes; TIME: the time any glide takes) | **ALLOC** (ROT: poly notes take the voices in turn; REUSE: the nearest free voice) | | |
| **FILT 1/2** | **CUTOFF** | **RESO** | **ENV** (how far the filter's envelope moves it, either way) | **TYPE**: LP24, LP12, BP, HP |
| **FILT 2/2** | **KEY** (the cutoff follows the note) | **DRIVE** | **LINK**: AMP or OWN | |
| **ENV 1/2** (AMP) | **ATTACK** | **DECAY** | **SUSTN** | **RELEAS** |
| **ENV 2/2** (FILT) | **ATTACK** | **DECAY** | **SUSTN** | **RELEAS** (the filter's own envelope) |
| **FX 1/4**, **2/4**, **3/4** | **TYPE** | TIME | INTENS | AMOUNT (named by the type: see [FX slots](#fx-slots)) |
| **SLICER** | **SLCR** | **PAT** | **RATE** | **DEPTH** |
| **LFO** | **RATE** | **WAVE** | **PHASE** | **FADE** |
| **LFO SYNC** | **SYNC** | **TRIG** (NOTE, FREE, PATT) | **POL** | **ESYNC** (both envelopes in note values) |
| **LFO 2** | **RATE** | **WAVE** | **SYNC** | **TRIG** (NOTE, FREE, PATT; FREE to start) |
| **MATRIX** | **SOURCE** (the row) | **DEST** (the column) | **AMOUNT** | **CLEAR** (on AS1, AS2: **TARGET**) |

Where the LFO and the envelopes go is the matrix's: see [The mod matrix](#the-mod-matrix).

### The oscillator: Type and macros

**KNOB 1 on OSC** picks the Type: one list of 31, the same on every voice. Turning it changes only the oscillator; the filter, envelopes, FX and voice stay as they are. Changing to another engine's Type starts its macros at their plain defaults (no chorus, no envelope on the timbre); changing within PLAITS or within ELEMENTS keeps the macros where they are. The page shows the Type's name large and "TYPE n/31". See [section 17](#17-the-oscillator-types) for every Type and its macros.

### The filter

One resonant filter on every voice, after the oscillator and before the amp, whatever the Type: a four-pole ladder, like the SH-101's and the Juno's. Its **TYPE** takes the 24 dB or 12 dB low-pass, a 12 dB band-pass or a 12 dB high-pass from it. On **ACID** and **JUNO**, **LP24** is their own ladder (the 303's filter, the Juno's IR3109), moved by the same knobs; the line under the filter's name says which (303 LADDER, IR3109 LADDER, LADDER).

- **RESO** at the top makes the filter sing at the cutoff on its own (self-oscillation).
- **ENV** moves the cutoff with the filter's envelope, up (+) or down (-); at the ends it sweeps the whole range.
- **DRIVE** pushes the sound into the filter for grit.
- **KEY** at 100 % makes the cutoff follow the note exactly.
- The LFO moves it too (the matrix's LFO1 > CUT), and an accented note opens it a little more.
- **ENV** and **KEY** are the matrix's FENV > CUT and KEY > CUT cells: turning either here or in the grid changes the same value.

**LINK** chooses the filter's envelope:

| LINK | The filter follows |
| --- | --- |
| **AMP** | the amp ENV (ENV 1/2): one envelope shapes level and tone, the quick MiniFreak way |
| **OWN** | its own ADSR (ENV 2/2), just for the filter |

While it is linked, ENV 2/2 shows the amp ENV's values in grey and says LINKED TO THE AMP ENV. **Turning any of its knobs makes it OWN**, starting from the amp ENV's values (the message says FILTER ENV: OWN). LINK on FILT 2/2 switches back. Both shapes are drawn: the one you are on in the voice's colour, the other faint behind.

**ESYNC** (MOD > LFO SYNC, KNOB 4) makes ATTACK, DECAY and RELEASE note values of the tempo, for both envelopes. **LFO SYNC**: SYNC locks the LFO to a note value; TRIG chooses what restarts it; POL chooses bipolar or unipolar.

| TRIG | The LFO restarts (at its PHASE) |
| --- | --- |
| **NOTE** | at each new phrase: a note played when no other note of the voice is held (as before) |
| **FREE** | never: it runs on by itself |
| **PATT** (pattern) | each time the voice's pattern comes round to step 1, and when PLAY starts. Notes in between do not restart it, so the LFO lines up with the pattern |

### What the graphs show

| Page | Drawing |
| --- | --- |
| OSC 1/4, 3/4, 4/4 | the waveform (a schematic for ELEMENTS and some PLAITS models; FOLD, SWARM, WAVETABLE, ADDITIVE and CHORD their own picture) |
| OSC 2/4 | the pitch on a scale of +-3 octaves, and a note diagram of the voice mode and glide |
| FILT | the response curve at its TYPE, the resonance peak, a shaded wedge for how far the envelope sweeps it; DRIVE lifts the curve, KEY tilts a line through the cutoff |
| ENV | the envelope (AMP | FILT selector top right); the other envelope faint behind when the filter is OWN |
| FX | the three slots as boxes (the one on the knobs filled, a lit square when a slot is on), then the effect: the delay's echoes, the reverb's tail, the chorus or the sweep, a drive's curve, the EQ's three bands |
| SLICER | its 16-step pattern |
| LFO, LFO 2 | the LFO shape, running |
| MATRIX | the grid |

### FX slots

Three slots in series on every voice, after the filter and before the voice's level, as on the MiniFreak. **All three are OFF to start**, and every INIT sound starts with none. KNOB 1 picks a slot's **TYPE**; the other three knobs are the MiniFreak's TIME, INTENSITY and AMOUNT, named by the type. Picking a type sets its knobs to that type's start.

| Type | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- |
| **CHORUS** | RATE (shared) | DEPTH (shared) | SEND |
| **PHASER** | RATE | FDBK | MIX |
| **FLANGER** | RATE | FDBK | MIX |
| **REVERB** | SIZE (shared) | DAMP (shared) | SEND |
| **DELAY** | TIME (shared) | FDBK (shared) | SEND |
| **DIST** | TONE | DRIVE | MIX |
| **SOFT**, **HARD**, **FOLD**, **FUZZ** | TONE | DRIVE | MIX |
| **CRUSH** | RATE (the held sample's) | BITS | MIX |
| **EQ** | LOW | MID | HIGH (each -12 to +12 dB) |

**The shared effects.** There is one chorus, one reverb and one delay line for the whole FM-1 (the MiniFreak also has only one reverb and one delay). A CHORUS, REVERB or DELAY slot is this voice's **send** to it: KNOB 4 is how much this voice sends. Their KNOB 2 and KNOB 3 are the shared effect's own settings, the same values as GLO > REV/CHO and GLO > DLY, so they change it for every voice; they are drawn in white, not the voice's colour, to say so. A voice sends nothing to a shared effect unless it has a slot of that type. A voice can have one CHORUS, one REVERB, one DELAY and one FLANGER; KNOB 1 skips a type another slot of the voice already has. Turning a send slot to another type puts its send back to 0.

MIX at 0, a send at 0, an EQ with all three bands at 0 dB, or OFF: the slot costs nothing and the voice is exactly as without it. TIME, INTENS and AMOUNT can be locked on a step; TYPE cannot (as the MiniFreak's). The drum voice keeps its own INSERT (TYPE, DRIVE TONE LEVEL / BITS RATE LPF / RATE DEPTH FDBK, and MIX) on its FX pages.

### The mod matrix

MOD's last page is the MiniFreak's matrix as a grid: **sources** down the side, **destinations** across. Each cell is an amount, plus or minus; a filled square is a positive amount, an empty square a negative one, bigger for more; a grey dot is empty. Six columns show at a time: the view slides when the cursor goes past them (a < or > says there are more).

| Sources (rows) | |
| --- | --- |
| **LFO1** | the LFO page's LFO |
| **LFO2** | the second LFO (MOD > LFO 2) |
| **AENV** | the amp envelope |
| **FENV** | the filter's envelope (LINK AMP: the amp envelope) |
| **PENV** | the pitch envelope: a fast fall from each note's start (about 40 ms) |
| **VEL** | the note's velocity |
| **KEY** | the note, from C4 |

| Destinations (columns) | |
| --- | --- |
| **PIT** | pitch, up to 12 semitones either way |
| **M1**, **M2**, **M3** | the OSC page's three macros, as the Type names them |
| **CUT**, **RES** | the filter's cutoff and resonance |
| **LVL** | the voice's level (it only ever comes down from the voice's own) |
| **PAN** | the voice's pan |
| **FX1**, **FX2**, **FX3** | each slot's AMOUNT (its MIX, or a shared effect's send) |
| **AS1**, **AS2** | any sound parameter you choose with KNOB 4 **TARGET** |

**KNOB 1 SOURCE** and **KNOB 2 DEST** move the white frame; **KNOB 3 AMOUNT** sets that cell; **KNOB 4 CLEAR** sets it back to 0 (on AS1 and AS2, KNOB 4 is **TARGET**, the parameter the column moves). PIT, the macros, CUT, RES and LVL act on each note with its own envelopes; PAN, FX and AS act on the whole voice, following the note played last.

**Six cells are the fixed routings of before**, one value shown in both places: **LFO1 > PIT**, **LFO1 > CUT**, **LFO1 > LVL** (a tremolo depth, one way only), **FENV > CUT** (the FILT page's ENV), **PENV > PIT**, **KEY > CUT** (FILT 2's KEY, one way). Besides those, a voice can set 19 cells; the 20th says MATRIX FULL. A fixed cell's AMOUNT can be locked on a step (hold the step on the GRID's MOD tab); the other cells cannot.

### Presets and INIT

- **PRESETS** (on HOME, on the SAVE > PRESETS page, or on any sound page) browses the selected voice's sounds, grouped by kind (BASS, KEYS, ORGN, PAD, LEAD, PLCK, STAB, FX, INIT), then your user presets. On the drum voice it picks the kit.
- **SAVE > PRESETS**: KNOB 1 the sound, KNOB 2 the engine (any of the four, on every voice).
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
- **DLY** and **REV/CHO** are the shared effects a voice sends to through a CHORUS, REVERB or DELAY FX slot; such a slot shows their TIME and INTENSITY too (in white: the same for every voice).

---

## 9. Mixer, mute and solo

**Hold GLO** for the mixer (or hold GLO untouched for about 0.7 seconds and let go to keep it open). It shows a level bar for each voice with its meter inside, and M and S badges. The key legend reads "123D123DFBP T".

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

**Pan.** With GLO held (or the mixer kept open), press **key 11** (P): the title reads **PAN** and KNOB 1 to 4 set the pans of voices 1, 2, 3 and the drums (L, C, R). Press key 11 again for the levels. When the mixer closes, its knobs are the levels again.

A muted voice fades out in a few milliseconds and plays no new notes, but its pattern runs on in time. On the HOME screen, KNOB 4 sets the selected voice's level (on a muted voice the first turn unmutes it). **GLO > DRUMS** has the drum level, reverb and delay.

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
| **Tap REC while stopped** | arms it ("REC READY"). Recording starts when you play, at the tempo set (an empty project too). **On a synth voice's GRID it starts STEP REC instead** (see [Step record](#step-record-step-rec)) |
| **Tap REC again** | cancels ("REC OFF") |
| **Hold REC** (about 0.7 s, then the ring fills over about 1.3 s more) | **clears the selected voice** ("TRACK 1 CLEARED"). Let go before the ring is full and nothing happens. EDIT + OCT- brings it back |

On the REC screen: KNOB 2 the length (1, 2 or 4 bars), KNOB 3 **START** (on a note, or with a count-in). The click setting is GLO > GLOBAL > CLICK (OFF, REC, ON). The beat lights turn red on beat 1 only while recording.

Notes are recorded as you play them, quantised to the voice's DIV. Rolls (hold ARP and a key) are recorded as ratchets (on the drum voice, as a retrig).

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

Hold **ARP** and a key for a note-repeat roll. KNOB 1 sets the rate (1/8, 1/16, 1/32, 32T, 1/64). On the drum voice, hold ARP is **RETRIG**: the rates are the Rytm's (1/1 to 1/80; the black keys pick 1/4 to 1/80), and OCT- and OCT+ give ghost and hard hits (see [RETRIG](#retrig-hold-arp)). Tap **ARP** for the arpeggiator pages: ARP (MODE, RATE, OCT, GATE) and ARP 2 (SWG, PROB, HOLD, ORD).

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
| KNOB 1 **GATE** | how long each note of the pattern sounds within its step, 1 to 100 % (synth voices; the drums: --) |
| KNOB 2 **DIV** | 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1BAR, 2BAR |
| KNOB 3 **SWING** | 0 to 100, per voice |
| KNOB 4 **STEPS** | 1 to 64 |

### The SEL layer (hold SEL): key, scale and chord

| Control | Does |
| --- | --- |
| **any key** | sets the **key of the song** (the root of all three synth voices). On the drum voice: picks the sound (in MUTE mode: solos it), see [Drums](#5-drums) |
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
| 91, 93, 94 | reverb, chorus and delay sends (a synth voice with no slot of that type gets one in its first OFF slot; with none OFF the CC does nothing) |
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

## 17. The oscillator Types

Every synth voice can play every Type. A Type is an engine and, where the engine has several, one of its models. KNOB 1 on OSC picks it; KNOB 2 to 4 are its three macros (OSC 1/4); the 4th and 5th controls, where it has them, are on OSC 3/4.

| # | Type | Engine | Macro 1 | Macro 2 | Macro 3 | OSC 3/4 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | ACID 303 | ACID | WAVE (saw, square) | ACCENT | SLIDE | |
| 2 | JUNO DCO | JUNO | WAVE (SAW, PLS, S+P) | PW | SUB | NOISE, CHORUS |
| 3 | VA | PLAITS | DETUNE | SQUARE | SAW | T.ENV, SUB |
| 4 | VA VCF | PLAITS | F.RES | F.CUT | WAVE | T.ENV, SUB |
| 5 | FOLD | PLAITS | SHAPE | FOLD | SKEW | T.ENV, SUB |
| 6 | PHASE D | PLAITS | FREQ | AMOUNT | ASYM | T.ENV, SUB |
| 7 | 2-OP FM | PLAITS | RATIO | INDEX | FDBK | T.ENV, SUB |
| 8 to 10 | 6-OP A, B, C | PLAITS | PATCH | LEVEL | ENV | T.ENV, SUB |
| 11 | WAVETABLE | PLAITS | BANK | ROW | COLUMN | T.ENV, SUB |
| 12 | TERRAIN | PLAITS | TERRN | SIZE | OFFSET | T.ENV, SUB |
| 13 | ADDITIVE | PLAITS | BUMPS | CENTRE | WIDTH | T.ENV, SUB |
| 14 | CHORD | PLAITS | CHORD | INVRT | SOUND | T.ENV, SUB |
| 15 | STR MACH | PLAITS | CHORD | COLOUR | WAVE | T.ENV, SUB |
| 16 | SWARM | PLAITS | SPREAD | RATE | SIZE | T.ENV, SUB |
| 17 | FORMANT | PLAITS | RATIO | FORMNT | WIDTH | T.ENV, SUB |
| 18 | CHIPTUNE | PLAITS | CHORD | ARP | PW | T.ENV, SUB |
| 19 | STRING | PLAITS | INHARM | BRIGHT | DECAY | T.ENV, SUB |
| 20 | MODAL | PLAITS | MATERL | BRIGHT | DECAY | T.ENV, SUB |
| 21 | BOUNCE | ELEMENTS | STRC | BRIT | DAMP | LAST, POS |
| 22 | PLUCKED | ELEMENTS | STRC | BRIT | DAMP | GAP, POS |
| 23 | BOWED | ELEMENTS | STRC | BRIT | DAMP | BOW, POS |
| 24 | BLOWN | ELEMENTS | STRC | BRIT | DAMP | BLOW, POS |
| 25 | OMINOUS | ELEMENTS | RATIO | FM | FDBK | SHAPE, SPACE |
| 26 | NOISE | PLAITS | FILTER | CLOCK | RESO | T.ENV, SUB |
| 27 | SPEECH | PLAITS | MODE | FORMNT | WORD | T.ENV, SUB |
| 28 | PARTICLE | PLAITS | RANDOM | DENSTY | FILTER | T.ENV, SUB |
| 29 | KICK | PLAITS | PUNCH | TONE | DECAY | T.ENV, SUB |
| 30 | SNARE | PLAITS | NOISE | TONE | DECAY | T.ENV, SUB |
| 31 | HI-HAT | PLAITS | METAL | TONE | DECAY | T.ENV, SUB |

**T.ENV** is how far the amp ENV opens a PLAITS model's timbre; **SUB** adds a sine an octave down. ACID is one note at a time (the 303); JUNO up to 6 notes; the heavy PLAITS models (6-OP, STR MACH, STRING, SPEECH, PARTICLE) and BOWED, BLOWN and OMINOUS one note; the others several. Some ELEMENTS and OMINOUS controls the macros leave out (ACC, EXC or TONE; OMINOUS's DTUNE and BRIT) keep the values the sound loaded with.

**On the FM-1 itself** (the device build, not the simulator): tracks 1 and 2 have less memory for an oscillator than track 3, so 6-OP A, B, C, STRING, SPEECH and PARTICLE play only on track 3 there; on tracks 1 and 2 they are silent and the OSC page says NOT ON THIS TRACK.

The factory sounds are listed by kind in PRESETS, every engine's on every voice, then your user presets. A few examples: ACID has TB ACID, TB SQUARE, TB DIRTY, TB DEEP, TB SQUELCH and TB RUBBER. JUNO has JUNO BASS, SUB 106, JUNO KEYS, JUNO ORGAN, JUNO STRING, JUNO PAD, JUNO SWEEP, JUNO LEAD, JUNO HOOVER, JUNO BRASS and JUNO STAB. Each engine has an INIT sound, listed first. A sound made before the voice filter (a factory sound, an older project or user preset) has its own filter settings moved onto the FILT page: ACID INIT is a saw with CUTOFF and RESO at 35 %, its filter envelope OWN, decaying as the 303's did.

Where they come from: ACID is Charles Vestal's TB-303 voice from X0X (after Open303). JUNO is PatttF's Juno-106 voice from FM-106. PLAITS and ELEMENTS are built on Émilie Gillet's open-source code for Mutable Instruments. The drum voices come from the LXR-02 firmware by Erica Synths. PHEBUS is not affiliated with or endorsed by any of them.

---

## 18. Kits and the backup page

*The kits part is carried over from KARMA and was not re-checked against PHEBUS's colours in this pass.*

- **MY HEX 1 to 4** are the last four kits. Like every kit, turning a knob on one makes the project's own copy of it (see [Drums](#5-drums)); the MY HEX kits themselves are not changed.
- The **backup page** (`web/editor.html`, at the web editor's old address; Chrome or Edge, FM-1 on USB) has three buttons: **Connect**, **Save backup** (downloads a file) and **Restore** (writes a backup file back to the FM-1, after a confirmation). It edits nothing. Its protocol is in [web/EDITOR_PROTOCOL.md](web/EDITOR_PROTOCOL.md).
- A KARMA backup restores too; its user sample slots are skipped.
- The browser simulator and its cheat sheet are in `web/emu/`.

A backup holds the music you are working on (with its own kit), the four projects, the 32 user presets, the MY HEX kits and the settings. Save a backup before you flash anything.

---

## 19. Every button gesture, screen by screen

Each row was checked against the input code (`ui_input.c`, `ui_layers.c`, `ui_grid.c`, `ui_drum.c`, `ui_stut.c`, `ui_studio.c`, `ui_menu.c`, and the keys in `seq.c`). "Twice" means two taps or two clicks within the stated time.

### Anywhere

| Gesture | Does |
| --- | --- |
| tap PLAY | start or stop |
| tap REC | record or arm (stopped, on a synth voice's GRID: STEP REC) |
| hold REC about 2 s | clear the selected voice |
| tap HOME | HOME screen (on the HOME screen: a screensaver) |
| hold HOME 0.7 s | the menu |
| tap ENV, LFO, FX, EDIT, ARP, SEL, GLO | open that button's pages (drum voice: EDIT, ENV, LFO the drum screen's SRC, AMP, LFO; SEL MUTE mode; ARP CHROMATIC mode) |
| tap SEQ | the GRID (from a synth sound page: on that page's tab; on the GRID, synth or drum: TRIG, again back) |
| tap SAVE | the SAVE pages (PRESETS, USER, TOOLS), on every screen |
| hold a layer button, then tap HOME | lock that layer open |
| ALGORITHM | choose the voice (not in the menu) |
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
| tap EDIT (drums) | the drum screen, at SRC |

### Sound pages

| Gesture | Does |
| --- | --- |
| tap EDIT | enter OSC; on the chain, the next stage |
| tap ENV, FX, LFO | that stage's first page; again, its next page (ENV: AMP, then FILT; FX: FX 1, FX 2, FX 3, SLICER; LFO: LFO, LFO SYNC, LFO 2, MATRIX) |
| SELECT | the previous or next page of the chain |
| KNOB 1 to 4 | the four values shown |
| OSC KNOB 1 | the oscillator Type (31), one a detent |
| FX 1 to 3, KNOB 1 | the slot's TYPE, one a detent (skips CHORUS, REVERB, DELAY, FLANGER when another slot of the voice has it) |
| FX 1 to 3, KNOB 2 to 4 | TIME, INTENS, AMOUNT as the type names them (a shared effect's KNOB 2 and 3: GLO's, shown white) |
| MATRIX, KNOB 1, KNOB 2 | move the cursor: SOURCE (row), DEST (column) |
| MATRIX, KNOB 3 | the cell's AMOUNT (MATRIX FULL past 19 cells besides the fixed ones) |
| MATRIX, KNOB 4 | CLEAR the cell; on AS1 or AS2, TARGET (the parameter that column moves) |
| a FILT ENV knob, LINK AMP | the filter's envelope becomes OWN, from the amp ENV's values, then turns |
| tap SEQ | the GRID, on this page's tab (the keys become steps) |
| keys | play notes |

### GRID (synth voice)

| Gesture | Does |
| --- | --- |
| tap an empty step key | set it with the pen |
| tap a set step key | clear it on release (unless a knob was turned) |
| hold a step | select it and show its notes and locks |
| tap EDIT | the OSC tab; again the next stage (FILT ENV FX MOD, round); with a step held too |
| tap ENV, FX, LFO | the ENV, FX, MOD tab; again its next page; with a step held too |
| tap SEQ | the TRIG tab; again back to the tab before |
| SELECT | every page of every tab, TRIG first, stopping at both ends |
| KNOB 1 to 4, nothing held, a sound tab | edit the sound (as on the sound pages) |
| KNOB 1 to 4, nothing held, TRIG 1/2 | NOTE (pen), STEP, HIT, VEL |
| KNOB 1, nothing held, TRIG 2/2 | SPICE (0: the pattern as written) |
| KNOB 2, nothing held, TRIG 2/2 | DICE: right a new roll, left the roll before |
| hold a step + KNOB 1 to 4, a sound tab | lock that value on the step (drawn inverted; the TYPE, an FX slot's TYPE, a shared effect's settings, VOICE, PRIOR and the matrix's cursor cannot be locked; on MATRIX a fixed cell's AMOUNT can) |
| hold a step + KNOB 1 to 4, TRIG | NOTE, LEN, VEL, COND; TRIG 2/2 FLAG, RATCH, NUDGE |
| hold a step + OCT-, a sound tab | the lock of the knob last turned off; again, every lock of the step |
| hold a step + OCT-, TRIG | clear the step's locks, nudge and condition |
| hold a step + OCT+ | next condition |
| hold several steps | edit or lock them together |
| black keys 1 to 4 | page 1 to 4 |
| OCT- or OCT+ (nothing held) | previous or next page (OCT+ on the last page adds one) |
| PRESETS (nothing held) | the sound |
| ALGORITHM | change voice and stay on the GRID (past DRUM: STUTTER) |
| tap REC, stopped | STEP REC on or off |
| tap REC, playing | record live (the keys play notes) |
| STEP REC: keys | a note, or a chord (keys held together), on the red step; the cursor moves on |
| STEP REC: SEQ, ARP | rest, tie (a key held + SEQ: tie) |
| STEP REC: OCT- or OCT+ | the octave |
| STEP REC: SELECT, KNOB 1 to 4 | the cursor; OCT, STEP, VEL, STEPS |
| STEP REC: EDIT + OCT- | undo the whole pass |

### Drum screen (drum voice)

| Gesture | Does |
| --- | --- |
| tap an empty step key | a trig of the selected sound (velocity 100, or TRIG's KNOB 1) |
| tap a trig | clear it on release (unless a knob was turned) |
| hold SEL + key | pick the sound (heard) |
| KNOB 1 to 4 (nothing held) | the selected sound's values on the page shown (the first turn on a built-in kit makes your copy; another kit's copy: turn again within 1.5 s) |
| hold a trig + KNOB 1 to 4 | lock that value on the trig (TRIG: VEL RETRIG NUDGE COND, then CHANCE LENGTH CURVE NOTE) |
| tap EDIT | SRC; again: the next page (FLTR, AMP, LFO, SRC) |
| tap ENV, LFO | AMP, LFO; again: the page's next half |
| tap SEQ | TRIG; again: back |
| SELECT | every half of every page, TRIG first |
| (the page buttons and SELECT) | also with a trig held |
| hold a trig + OCT- | the lock of the knob last turned off; again (or none turned): every lock (TRIG: everything the trig holds) |
| hold a trig + OCT+ | next condition |
| hold a trig + tap FX | the drum voice's own page locks per step (SELECT: back) |
| hold several trigs | lock and edit them together |
| black keys 1 to 4 | page of steps 1 to 4 |
| OCT- or OCT+ (nothing held) | previous or next page of steps (OCT+ on the last page adds one) |
| F#5 | FILL while held; twice quickly: latched (again twice: off) |
| PRESETS (nothing held) | the kit |
| ALGORITHM (nothing held) | change voice and stay on the GRID (past DRUM: STUTTER) |
| tap SEL | MUTE mode on or off |
| MUTE mode: keys | mute or unmute the 16 sounds |
| MUTE mode: hold SEL + key | solo the sound (again: off) |
| tap ARP | CHROMATIC mode on or off |
| CHROMATIC mode: keys | the selected sound at the key's pitch (C4: its own); recorded live as a NOTE lock |
| CHROMATIC mode: OCT- or OCT+ | the keys' octave, -2 to +2 (both: 0) |
| hold ARP + key | RETRIG the sound while held |
| hold ARP + black key | the RETRIG rate (1/4 to 1/80) |
| EDIT + black key | erase or mute mode for EDIT's white keys |

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
| ARP | key | note-repeat roll (drums: RETRIG) |
| ARP | KNOB 1 | roll rate (drums: the RETRIG rate, 1/1 to 1/80) |
| ARP | black key (drums) | the RETRIG rate |
| ARP | OCT- or OCT+ | ghost or hard hits (drums) |
| SEQ | KNOB 1 to 4 | GATE (synth voices), DIV, SWING, STEPS |
| SEL | key | key of the song (drums: pick the sound; in MUTE mode: solo it) |
| SEL | KNOB 1 to 4 | CHORD, SCALE, KEYS, TRANSP (drums: nothing) |
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
| KNOB 2, 3 | length, START |
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

**Going back to the official firmware.** *Carried over from KARMA's guide, not tested with PHEBUS.* Save a backup with the backup page first. Download M-VAVE's updater, M-UPGRADE, and the official FM-1 firmware from m-vave.com, close every app that uses MIDI, and run the update. An FM-1 goes back only to the official version it last had.

**Backups.** Because PHEBUS is unreleased and untested on hardware, back up everything the FM-1 holds before you flash anything, and read [BUILDING.md](BUILDING.md) before you build.

### Checks still to do

These could not be confirmed from the code and are not described above as fact:

- how it all behaves on a real FM-1;
- that a flash save never stalls the screen or the audio;
- the -60 dB silence threshold for the stop autosave against a real noise floor;
- the backup page against the new firmware;
- the voice filter's processor cost on the FM-1 (about half a JUNO voice per voice on the host), and three heavy Types at once;
- how the filter sounds against a real 303 and SH-101.

---

## 21. Credits

PHEBUS is Ben's fork of **KARMA** by Ariel Muchnik. KARMA is a fork of **SLOOP** by isod89, which is based on **Felucca** by Leo Kuroshita (Hügelton Instruments). It also uses code from X0X (Charles Vestal), FM-106 (PatttF), Melodee (Kerem Kilic), Mutable Instruments' Plaits and Elements (Émilie Gillet), the LXR-02 firmware (Erica Synths), and JIANT FM (Juan Jiant). The screen font is Terminus.

Licence: GPL-3.0-only. See [LICENSING.md](LICENSING.md) and [LICENSE](LICENSE). PHEBUS comes with no warranty. M-VAVE and FM-1 are trademarks of their owners; PHEBUS is not affiliated with M-VAVE. The README of SLOOP is kept as [SLOOP-README.md](SLOOP-README.md).
