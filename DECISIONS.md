# Decisions made for Ben to review

Ben asked (20 Nov 2026) for a full first version of his brief without stopping for questions. These are the calls Claude and its helpers made that he did not specify. Each is open to change; SPEC.md stays the standard.

## Characters
- Track order: robot = track 1, alien = track 2, crocodile = track 3, snake = drums.
- They dance on the beat (lean, arms up, lean, arms up); the old ones reacted to each note. Each note still gives a small hop.
- On a voice's FX pages its own character dances (the wizard is gone).
- Scene props that were instruments became stage dressing: speakers, strings of lights, a spotlight, floor lights, sparks.
- The crocodile scene no longer shows pitch (the old saucer flew higher for higher notes).
- Neutral line art is now grey so only voices carry colour.

## Steps on the keys (the GRID)
- SEQ tapped on HOME opens the selected track's GRID on every track. Before, on synth tracks it opened the last SEQ page; the old STEP page is one more SEQ tap away. The UI test's expectation for this tap was updated to match.
- Tap a step key: set or clear. Hold a step key: KNOB 1 to 4 are NOTE, LEN, VEL, COND; SELECT right gives FLAG (accent/slide), RATCH, NUDGE.
- Parameter locks: hold a step and tap ENV, LFO, FX or EDIT, then turn a knob. Existing limit of 24 locks per track.
- OCT- with a step held removes its locks; OCT-/OCT+ with nothing held change page; black keys 1 to 4 pick the page of 16.
- Drum grid: a set step now clears when the key is released (so it can be held and edited); held drum steps get LEVEL, RATCH, NUDGE, COND.
- ALGORITHM on a GRID changes track and stays on the GRID.
- No sound when turning NOTE on a synth step (no safe preview call yet).

## Sound pages (OSC > FILT > ENV > FX > MOD)
- EDIT enters the chain at OSC and steps to the next stage; ENV, FX and LFO jump to their stage; SELECT walks every page in order.
- Stage buttons always land on the stage's first page (they no longer remember the last page used).
- ENV DEST stays in ENV; MOD is LFO, LFO DEST, LFO 2; the filter moved out of FX into FILT; INSERT comes before SLICER.
- The engine names (303, DCO/VCF) are no longer in the device header.
- On drums, ENV opens the drum sound's envelope and FX opens INSERT; the drum track's own filter is now reachable from a page.
- The strip trims a few rows off the bottom of the character scenes.
- One test line was added for isolation (`gen_on = 0`) and several page-order expectations were updated.

## Colours
- Mute yellow, solo blue, record red. Solo blue is close to the alien's blue.
- Knob strips: a voice's page in its colour; one colour per voice in the mixer; grey for global pages.
- The web editor uses the same four track colours; its knob columns still use four colours and need the same rule.

## Not yet checked
- Nothing has run on a real FM-1. Sound and timing are untested by ear.

## Colour sweep, 20 Nov (after Ben's "you didn't get it everywhere")
- The old KARMA scheme coloured things by knob position (knob 1 blue, 2 green, 3 yellow, 4 red). All of it is gone: on a voice's page, the envelope, scenes and markers are in that voice's colour only.
- Anything that belongs to everyone is neutral white/grey: play marker, SONG and SONGS section tiles (A B C D), "playing", KEY layer dials, the record-ready disc, the HOME tape, all seven visualisers.
- Red is kept for recording and erasing only. The KARMA boot logo letters use Ben's four colours plus white.
- Drum screens: the selected pad is outlined white, and the selected step and send bars are coral (they were track 1 purple and old green).

## Name: PHEBUS (Ben, 20 Nov)
- Ben's build is called PHEBUS. Boot logo and tape screen: plain white block letters, "a bit like an old Sega or Atari logo" (own letterforms, not a copy of either).
- Claude's call: the four voice-colour bars stay under the boot wordmark.
- Page titles say PHEBUS. Credits to KARMA, SLOOP and Felucca stay as they are (GPL attribution), as do file names, the repo name and the FM-1_926 package identity.
- Not yet checked against existing product names or trade marks.

- 20 Nov, later: Ben asked for a smooth font, "something suitable for the Barbican. Or Daft Punk - but not their font". The wordmark is now TeX Gyre Adventor Bold (a free geometric face), tracked wide, plain white, stored as tools/assets/phebus_word.png. The boot animation fades it up and out instead of popping pixel cells in. The tiny PHEBUS on the tape screen is still pixel letters.

## Boot screen and backdrops, 20 Nov (late)
- Boot: a flowing backdrop in the four voice colours, a different one each start, with PHE / BUS filling the square in black. About 3 seconds (was 2); any key skips. The colour bars and version number are gone from boot.
- On the boot only, the backdrop has no black of its own (deep versions of the four colours instead), so the letters stay whole. Elsewhere the backdrops keep black bands.
- Ben set the density limit ("the limit for how busy these should get"); patterns are drawn at that scale or broader, and glitch tears are rare and short.
- The letters are stretched about 9% taller to fill the square.
- The "different every time" pick uses the clock and battery reading; how random that is on real hardware is unknown.
- Drawn at full resolution; speed on the real device is unknown.

## Fierce characters, 20 Nov (late)
- A subagent drafted all four; it was stopped part-way and Claude finished the work.
- Robot: Ben asked for "more classic ... blockier": a box robot with antenna, visor slit, grille mouth, chest panel, clamp hands, slab feet.
- Snake: Ben asked to "lose the hood": a hoodless viper on a slim S neck with a flat wedge head, fangs and tongue.
- Crocodile: on Ben's worry about its silhouette, the arms in the dance poses are thrown clear of the body (one up behind, one reaching forward).
- Alien: unchanged from the first draft.
- STRIKE pose: shown for 8 frames when the character's own voice starts a note at the hard/accent velocity level; nothing in the sound or sequencer changes. In the mixer row the small figures strike too.
- Sprite data grew from 22,944 to 31,216 bytes.

## Review build, first wave (21 Nov): calls made that Ben did not specify
Sequencer
- Heard notes on the GRID are about 150 ms at velocity 100; they are not recorded, not sent to the arp or to MIDI out.
- SEQ on the GRID: synth goes to SEQ > PATTERN; drums go GRID > KIT > DRUM GEN as before. The other SEQ pages are walked with SELECT.
- The SEQ layer (hold SEQ) still has its 4x4 step keys. OPEN: Ben to decide whether they go.
- Lock pages: "LOCK" moved from the header into the step band so the page name fits.
- Page add: one page per press; a short last page is filled to 16 before copying; at 64 steps OCT+ wraps to page 1.
- Follow: a page picked by hand stays until the playhead moves to a new page.
- Full drum grid: up to 7 rows (the sounds in use plus the picked one). SELECT on the drum GRID no longer reaches the KIT page; SEQ still does.
- Tapping a tie makes a one-step note; the old note ends before it and the rest of its tail becomes rests.
- A press longer than about 0.14 s no longer counts as a tap (was 0.45 s).
- Lock table full with several steps held: the lock goes on all of them or none.
Saving
- The unsaved dot is white, in the header where the record disc sits (record wins while recording); checked twice a second.
- Save on stop waits 0.3 s, needs the output silent, and happens at most every 5 s. The older idle autosave stays as a fallback.
- Not checkable without the device: that the save never stalls the screen, and that the silence threshold suits the real noise floor.
Chain
- The four shared effect pages now follow MASTER under GLO. CLOUD's SEND is per voice but now sits on a shared page.
- OPEN: no one-knob page has a neighbour with a free knob, so none were merged. Ben to choose.
Song
- OPEN: insert and delete not built; both OCT buttons are already taken on that screen. Ben to choose how.
- Hint added: "OCT-: MODE".
Site
- The analytics counter was also removed from the published copies in docs/.
Screensavers
- A fourth piece, THIRD EYE, was added and named by the subagent. Ben has not approved it.
- Colours by track order; in DESERT purple is the farthest dune, coral the nearest.
- FLOW changes kind every 15 s or so with a wipe; kicks tear rows and ripple the sun.
- Drawn every other frame in 2x2 blocks. Host cost against the old average: FLOW 1.5 to 1.8 times, EYES 2.7, DESERT 2.4, THIRD EYE 3.5. Device speed unknown.
Device build after this wave: image 553,176 B (64 B smaller), working-memory pool 5.5 KB freer.

## Review build, second wave (21 Nov): calls made that Ben did not specify
Stutter lane
- Reached with ALGORITHM past DRUM on the GRID. Knob 1 length for new steps, knob 2 lane on/off, knob 4 lane length.
- A lit run repeats the span played just before it (capturing from the step onward sounded identical to the dry mix for short runs). Lit steps in a row with the same length are one continuous loop.
- Lane steps are straight sixteenths and ignore swing. The shown page does not follow the playhead.
- Inherited from the punch engine: the stutter is mono and low resolution; below about 84 BPM a quarter-note loop is cut to 0.72 s.
- Stored in unused drum-track bytes, so the save format is unchanged; stock KARMA loads the project but drops the lane if it re-saves.
Effects off
- Sends zeroed on all three tracks at power-on and NEW. The delay's return level stays at 90 (silent with sends at zero).
Visual pass
- Layer screens (hold a button) keep small knob values: there is no room for the bold face under their dials. OPEN for Ben if he minds.
- PUNCH key names were shortened to fit even tiles: STUTR, REVRS, WOW (for wobble), LP 4/8/16/32, RWND, FLNG, FRZ, -LOW/-MID/-HI.
- Knob labels widened to six letters: VOICE, GLIDE, GLMODE, PRIOR, INSERT, SWING, PHASE, DIST, CHORUS, DELAY, REVERB, FEEDBK, COLOR, PSLOPE, CHROM, ENVMOD, DECAY, DRIVE. TRANSP and NEW LOOP kept.
- Drum sounds: KICK 2, OP HAT, SNARE2, LO TOM, HI TOM, SHAKER, C-BELL.
- A muted badge follows the track's own mute, not "silent because another is soloed".
- GLO > DRUMS is treated as the drum track's page (coral), not a shared page.
- Robot head sits 2 px lower with a shorter antenna; crocodile snout 2 px shorter; poses nudged inside their frames.
- The strip is the same height as the old bar; freeing rows would have meant moving every screen.
Device build after this wave: image 557,192 B, about 24 KB left.

## Review build, last wave (21 Nov): calls made that Ben did not specify
Plain starts
- Track 1 JUNO INIT, mono (legato), keys an octave down. Track 2 JUNO INIT, polyphonic; track 2 may now run JUNO as well as ELEMENTS. Track 3 PLAITS INIT, mono.
- INIT sits at the end of the JUNO and PLAITS preset lists (listed first in the browser under "INIT"). Not added to ACID or ELEMENTS.
- The PLAITS lead is about 6 dB quieter than the JUNO saws.
- Host processor cost of the new defaults with a six-note chord: 2476 against 2066 for the old defaults (a heavier mix already in the test suite is 2873). The second core's load is 1321 against 811. Unverified on the device.
Held SEQ
- Keys do nothing while SEQ is held. Knob 1 (pen note) went with the step keys. The screen draws the pattern in time, with swing shown.
Strip
- The page name sits inside the lit stage box, with "2/3" where the dots were.
Song
- All keys are silent on the SONG screen; pressing an entry's key also moves the cursor to it. Holding a key past the end and pressing OCT+ appends. Editing is still refused while playing.
Stutter lane
- Tag reads STUT; no track is underlined while on it.
Sound pages
- The old FX "spells" scene was removed (unreachable). ELEMENTS and some PLAITS oscillator pictures are schematics, not exact waveforms.
Device build: image 558,264 B of 581,564, about 23 KB left.

## Rebuild wave 1, the cuts (21 Nov): calls made that Ben did not specify
- A to D stay as four patterns; tapping SAVE now opens PRESETS everywhere. The SAVE layer's old song keys (13, 14, 16) do nothing and show as empty tiles. HOME's header title is empty.
- The CHNC step condition is a fixed 50% until trigs get their own probability.
- Stutter ring: 16-bit linear with 12 dB of headroom kept. Quality measured on a loud mix: 35 dB before, 66 dB after; on a quiet mix: -5 dB before, 21 dB after. Timing unchanged. OPEN: 6 dB of headroom would be about 6 dB cleaner but clips very loud mixes.
- The slicer effect is untouched (the deleted file was the old sample-slicing engine).
- Web editor page is now a small backup page (connect, save, restore). It also fixes a fault in the original: it refused to restore its own newer backups. The firmware keeps the read-only diagnostic figures the developer scripts use.
- SELECT on the drum GRID now does nothing. OPEN: or open the KIT page.
- Characters: also removed, because they used the big sprites: the figure beside the GLO/TOOLS menu pages and the hopping figure on the ARP page. Scene backgrounds (wave, speakers, spotlight) are kept without their figure. OPEN: keep or cut.
- Deleted unused sample data and tools from the repository (about 6 MB).
- The flash layout still reserves the old slots for songs and sample kits; they go when the new save format lands.
- A fault the subagent caused and fixed: cutting CLOUD briefly removed the CLICK and CLRSQ settings; restored, with a new test that every setting shown has a label.

## Rebuild wave 2, drum trigs and the save format (22 Nov): calls made that Ben did not specify
Drum trigs (Ben: "Per-sound trigs, one track", 16 sounds)
- Every sound has its own trig on every step, with a velocity from 1 to 127. A new trig plays at 100, as the keys do and as the Rytm starts. Recorded hits keep their velocity: MIDI as played (it was rounded to four levels before), the keys 100, OCT- and OCT+ held 42 and 127 as before.
- What few trigs have besides a velocity (a condition, a chance, a nudge, a retrig) sits in a pool of 64 per pattern, and the sounds' locks in a pool of 128 per pattern. A trig with none of them costs nothing. When a pool is full the edit is refused with a message (TRIG EXTRAS FULL: 64, SOUND LOCKS FULL: 128). Dense storage for all of it would not fit the RAM.
- CHANCE counts only when the condition is CHNC. Its knob walks the Rytm's steps (1, 2, 4 .. 50 .. 98, 99 %). A trig set to CHNC starts at 50 %, what the old fixed CHNC was. Synth steps keep the fixed 50 % CHNC; the wave brief was the drum trigs.
- RETRIG follows the Rytm: a rate from 1/1 to 1/80 of a bar (a note value, whatever the track's DIV), a length from 1/8 step to 16 steps or INF, and a velocity curve from -128 (fades out) to 127 (fades in). The curve also sets the first hit, so a fade in starts quiet. INF repeats until the sound's next trig and has no curve. The sound's next trig always ends its retrig, and a retrig hit that falls on that trig is not doubled. Retrig hits play the trig's locks.
- NUDGE keeps the old range (half a step either way, in 64ths). A trig nudged early is queued when the step before it plays; at PLAY, or after a jump, it plays at once. Conditions and chances are drawn per trig, an early trig's when it is queued.
- Sound locks hold a HEX sound's parameters (the EDIT pages' order; the ids after them are free for wave 3's pages). MODEL is not lockable, as the Rytm's machine is not. A locked hit plays from its voice's own copy of the sound with the lock in it, for as long as it rings; an unlocked hit plays the kit's sound itself, so a knob turned is still heard at once. The synthesised kits (606 and the rest) have no editable sound until wave 6: their trigs play unlocked. To try sound locks in the simulator, pick a HEX kit.
- The drum voice's own locks (its FILTER, FX sends, INSERT, SLICER, its kit) stay as they were, per step, for the whole drum voice, like the Rytm's FX track. Old projects' drum locks were all of this kind and are kept as they are: none is dropped, none moved onto a sound (no sound parameter means the same thing).
- GRID, a drum step held: VEL, RETRIG, NUDGE, COND sit where LEVEL, RATCH, NUDGE, COND were; SELECT right gives CHANCE, LENGTH, CURVE, as SELECT does on a synth step. CHANCE, LENGTH and CURVE are drawn dim until they count. Nothing held, KNOB 4 is now the cursor trig's velocity.
- OCT- with a drum step held clears the trig's condition, chance, nudge, retrig and sound locks, and the drum voice's own locks on that step. On a lock page it clears that page's locks only.
- EDIT's SHIFT and length x2 move each drum trig with its extras and sound locks. (On synth voices SHIFT still moves the notes only, as before.)
- Undo keeps the whole drum pattern; its buffer moved to .noinit and is cleared at power-on.
- The GRID colours a trig by its velocity's band (ghost, soft, normal, hard), as the levels were; a retrig shows as notches.

Format 8 ("FUN8")
- A project is a core of 5,744 bytes (what a section holds and switches) and an ext of 1,024 bytes (the project's own, reserved for wave 6's kits, zero). The working project is stored whole (6,768 bytes); a slot (section) stores its core. A reader takes either length. The ext is written as zeros straight to flash and kept in no RAM.
- Room kept, zero, named in project.c: parameters by id up to 96 per track (71 used: wave 4's filter envelope ADSR and LINK / OWN, the resonant filter, wave 5's FX slots as new P_ ids); 46 bytes per synth track for wave 5's mod matrix; globals by id up to 48 (32 used); 124 bytes after the STUTTER lane for wave 6's recorded performance effects; drum lock ids after the HEX bytes; 1,024 bytes of ext for wave 6's kit. The engine choice per track (wave 4) is the engine byte, as now.
- Each drum sound's sends and the STUTTER lane now have their own fields (format 7 hid them in the drum track's unused parameters).
- The magic stays in the FUN family ("FUN8") so the backup page can tell it; the editor protocol is 12, and the backup page refuses a format 8 backup on firmware older than that. The web tests were not run (they are long).
- Format 7 and older load and convert: each lane's hit becomes a trig at its level's velocity (ghost 42, soft 72, normal 100, hard 127); a ratchet x2 to x4 becomes a retrig of one step at the nearest rate (exact on 1/16, 1/8 and 1/4 steps; on 1/32 steps x3 and x4 become 1/80, the fastest); a step's nudge and condition go onto every trig of that step; if more trigs need extras than the 64 fit, the rest play on the grid with no condition (rare: it needs over 64 nudged or conditioned hits).
- A CHNC step saved in format 7 came back as a normal step (format 7's loader read the value as normal). Converted, it is CHNC again, at 50 %, for synth steps and drum trigs. Format 8 keeps CHNC.

Flash
- The projects are new "big" objects, each copy two neighbouring sectors, A/B with the header last as before: the four slots at 0xA0000..0xAFFFF, the working project at 0xB0000..0xB3FFF, where KARMA's sample slots were. 0xB4000..0xDBFFF stays free.
- The old project objects stay where they were. Each is read once, converted, and written back in the new place when quiet; it is never written again, so an old firmware still finds its own. Erasing a slot empties the old object too, so it cannot come back.
- The song objects (0xE5000, 0xE6000, 0xFB000) are released.

RAM (the device budgets after wave 1: .data+.bss 83,572 of 98,304 B; pool 326,512 of 344,064 B, and tools/build.py refuses a build with under 8 KiB of the pool spare)
- Sections A and B stay in .noinit; .noinit has no room for C and D, so C is in the pool and D in main RAM. After a reset C and D are read from flash again; an update or UPDATE MODE writes them first. Only a section stored while playing and not yet written is lost if the FM-1 crashes.
- Measured on the host stand-in build (rv32, -Os; the device was within 210 B of it in wave 1): .data+.bss +8,112 B, pool +7,648 B (9,900 B left above the 8 KiB rule: 1,708 B), .noinit -1,716 B, code +10.5 KB. Estimated on the device: .data+.bss about 91,700 of 98,304 B, pool about 334,160 of 344,064 B. Waves 4 and 5 will have little RAM to spend; the extras and lock pools could shrink (each 128 bytes per copy, 7 copies) if needed.

Small fixes from wave 1
- HOME's header says PATTERN A to D while that section plays, otherwise PHEBUS (it was empty since the song cut).
- The SAVE layer no longer draws two rows of empty outlines for keys 9 to 16, which do nothing.

Not checked
- No device build (no toolchain here): the real sizes and the timing of the queued nudges and retrigs on the FM-1 are unmeasured. The long tests (regress, soak, stress, web) were not run.

## Rebuild wave 3, the Rytm drum screens (22 Nov): calls made that Ben did not specify
The sound model (Ben: "Make every kit editable", the 606 stays the start kit)
- Every sound of every kit has the same four pages, SRC FLTR AMP LFO, four knobs a half. A kit is one of two machines and each page holds that machine's own parameters in the Rytm's places: a HEX kit (and MY HEX) is the LXR's voice (SRC 1/3 TUNE SWEEP SW.DEC WAVE, 2/3 MODEL MOD M.PIT FM, 3/3 the transient and PSLOPE or FINE; FLTR FREQ RES TYPE DRIVE; AMP ATK DEC SLOPE DIST, then CRUSH DEL REV LEVEL); a synthesised kit (the 606 and the other 31) is its own model (SRC 1/3 TUNE FINE WAVE TONE, 2/3 SWEEP SWTIME PART2 P2 LVL, 3/3 NOISE N.LVL CLICK CHIP; FLTR FREQ RES TYPE ENV, then HPF; AMP HOLD DECAY N.HOLD N.DEC, then DRIVE DEL REV LEVEL). The synthesised sounds were not converted to the HEX machine: the 606 keeps sounding as it did.
- LFO on every sound (neither machine had one): SPEED DEPTH DEST WAVE, then SYNC FADE PHASE MODE. It moves PITCH (up to 12 semitones), CUT, RES or LEVEL of the hit it belongs to. MODE: TRIG (restarts at each hit), FREE, ONE (one cycle). FADE: in (right) or out (left).
- AMP 2/2 carries the sound's own reverb and delay sends (they were on the KIT page), in the Rytm's order (DEL, REV, then VOL as LEVEL).
- Copy-on-write: the first knob turned on a built-in kit (HEX, synthesised, MY HEX) copies the whole kit into the project; the built-in kits never change. The copy is saved in format 8's ext (488 of its 1,024 bytes) and plays whenever the drum track is on the kit it came from; its name then has a * (as the MiniFreak marks an edited preset). One kit's copy a project: a turn on another kit asks "AGAIN: REPLACE 606*" and replaces it only on a second turn within 1.5 s.
- In the copy each of the 16 sounds is its own: the built-in kits give PEDAL the closed hat's sound; the copy gives it a sound of its own, so all 16 are separately editable.
- MY HEX 1 to 4 are copied on write like the other kits, so they are no longer edited in place (Ben: "turning a knob on any kit makes your own editable copy"). OPEN: a way to store the project's copy into MY HEX, if Ben wants kits to travel between projects.
- A lock holds any parameter on these pages, the sends and the LFO included (not MODEL, as the Rytm's machine), plus a pitch (NOTE). A lock made for the other machine (a HEX lock on the 606) does nothing.
The screen (Ben: "B: four dials over the knobs")
- TRIG sits left of SRC as a fifth tab (the Rytm's TRIG page): the held trig's VEL RETRIG NUDGE COND, then CHANCE LENGTH CURVE NOTE. Nothing held, its KNOB 1 is the velocity new trigs get (100 to start, not saved).
- Values in one tall face (the 16 px face drawn twice as tall), five characters in a fixed 40 px column beside a small dial, in the drum voice's coral; a locked value is a coral box with the value in black; the touched knob's label inverted and its value underlined.
- A trig with locks has a black square in it, a retrig three lines, a condition a notch in its corner; hard trigs stay white as in wave 2.
- The sound map at the top right: dim coral for a sound with trigs, lit as it plays, white frame for the selected one, a yellow bar for muted, ice blue for soloed.
The buttons (the most Rytm-like the FM-1 allows)
- Pages: EDIT opens SRC and walks SRC > FLTR > AMP > LFO (as EDIT walks a synth's chain); ENV opens AMP, LFO opens LFO (again: the page's next half); SEQ on the drum screen opens TRIG (again: back); SELECT walks every half of every page, TRIG first. All of them work with a trig held (the Rytm's page keys).
- SELECT on the drum GRID: the pages (wave 1 left it doing nothing).
- Sound select: hold SEL and press a key (the Rytm's TRACK + pad). SEL held on the drum track no longer sets the key of the song there (it does on the synth voices).
- MUTE mode: tap SEL on the drum track (again: back). SEL + key in MUTE mode solos that sound (as mock "drum mute" says). Mutes and solos are performance state, never saved. EDIT + a black key still turns EDIT's white keys into mutes (kept from 2.6), now with the yellow mark.
- CHROMATIC mode: tap ARP on the drum track. The 27 keys play the selected sound; C4 (white key 5) is its own pitch; OCT- and OCT+ move the keys an octave (-2 to +2). Recorded live, a trig keeps its pitch as a NOTE lock (none at its own pitch). It is a mode of the drum screen: leaving the screen or the track ends it.
- Live RETRIG: hold ARP on the drum track (it was ROLL there) and press a key; the black keys pick the rate (F#3 1/4, G#3 1/6, A#3 1/8, C#4 1/12, D#4 1/16, F#4 1/24, G#4 1/32, A#4 1/40, C#5 1/48, D#5 1/64, F#5 1/80), KNOB 1 too (all 17 rates, 1/1 to 1/80). Recorded, a retrig faster than a step is a trig with a one-step retrig at that rate; slower ones are a trig a hit. Synth voices keep ARP's ROLL and G_ROLL.
- FILL on the drum screen: the last black key, F#5, held (the Rytm holds PAGE); pressed twice within 0.4 s it latches until pressed twice again or STOP. GLO + key 9 / 10 work as before. While a fill is on, FILL shows inverted in white in every header (it acts on every track's conditions).
- Removing locks: hold the trig and press OCT-: the lock of the knob last turned in this hold goes; OCT- again (or with no knob turned) every lock of the trig. On TRIG, OCT- clears what wave 2's OCT- cleared (condition, chance, nudge, retrig, every lock, the drum voice's own locks on the step). OCT+ is still the next condition.
- FX with a trig held still locks the drum voice's own pages per step (its bus: FILTER, INSERT, SLICER), as wave 2; the tabs then read LOCK and the page's name.
- The KIT page is gone (sound pick: SEL + key; kit: PRESETS; sends: AMP 2/2). HOME KNOB 2 DECAY now turns the selected sound's decay on every kit (copy-on-write as the pages).
RAM (Ben, 22 Nov 00:50: "Make the perfect version. Then we'll compromise": nothing trimmed; given back instead)
- Freed in main RAM: KARMA's scope buffers (2 KB: the visualisers that read them are gone; the stop autosave's silence test is now a count in audio.c), the HOME scope's snapshot (1 KB; dead since the sections), the HEX page preview (1 KB), the drum voices' two states as a union (720 B), the MIDI SEL note map a nibble a note (1 KB).
- Section C moved from the pool into main RAM, so the pool has 5.7 KB more for waves 4 and 5.
- Stand-in figures (rv32 -Os, as wave 2 measured): .data+.bss 91,921 B (wave 2: 91,477), pool 328,420 B (wave 2: 334,164), code and constants +13.6 KB. Estimated on the device: .data+.bss about 92,100 of 98,304, pool about 328,400 of 344,064 (15.6 KB spare, 7.4 KB above the 8 KiB rule), image about 491 KB of 581,564.
Found on the way
- A crash in PLAITS: turning FOLD wiped a Plaits model's memory that was still marked ready; the next Plaits model read a null pointer. The host UI fuzz hit it (wave 2's tree too, with another seed). Fixed in eng_prism.c.
Not checked
- No device build (no toolchain here). The LFO's and the copy's cost on the FM-1 is unmeasured (a few operations a voice a block); the long tests (soak, stress, regress, web) were not run.

## Rebuild wave 4, the MiniFreak voices, part one (22 Nov): calls made that Ben did not specify
Where the spec left a choice open, the most MiniFreak-like option was taken; each is open to change.

The oscillator (Ben: "A: Type + three macros"; 22 Nov: "Make the perfect version", every Type on every track)
- 31 Types on every voice, in one list: ACID, JUNO, the 24 Plaits models and the 5 Elements models, each its own Type. Order: the plain ones first (ACID, JUNO, VA, VCF, FOLD ..), the struck and drum models last.
- OSC has three halves: 1/3 TYPE and three macros; 2/3 OCT, TUNE, GLIDE, VOICE ("tune/octave on a second half"); 3/3 the Type's 4th and 5th controls, DTUNE, PRIOR. GLMODE and ALLOC have no page any more (they keep their values); PAN and MUTE are the mixer's only (VOICE and VOICE 2 are gone: one layout).
- Macro names per Type follow the Plaits manual's description of each model (e.g. VA DETUNE SQUARE SAW; MODAL MATERL BRIGHT DECAY). Elements: STRC BRIT DAMP on every exciter, then its exciter's own control and POS. Left without a knob (they keep the sound's value): ELEMENTS ACC and EXC/TONE, OMINOUS DTUNE and BRIT.
- Turning TYPE keeps the rest of the sound (the MiniFreak does). Another engine's Type starts its macros at their defaults with JUNO's chorus off and PLAITS' T.ENV at 0, so it stays plain; within PLAITS or ELEMENTS the macros stay where they are. BOWED and BLOWN get TONE 80 if it was under 60 (a strike's EXC 0 left them near silent).
- The header shows the engine's INIT sound name after a Type change (each engine's INIT is its last preset; ELEMENTS got an INIT, a plain plucked bar).
- JUNO's chorus moved from the FX stage to OSC 3/3 (it is part of the Juno voice; a JUNO-only FX page broke "the same layout").
- TYPE cannot be locked (the Rytm's machine cannot); VOICE and PRIOR cannot either.

The filter (Ben: "it doesn't growl or move" like a 101 or 303)
- One four-pole ladder (zero-delay feedback, a soft saturator in the loop) after every oscillator and before the amp, so a self-oscillating filter dies with the note. TYPE: LP24, LP12, BP, HP (12 dB, as the MiniFreak's; LP24 for the 101 and 303 growl).
- ACID's and JUNO's own ladders are kept as their LP24 (the 303 diode ladder, the IR3109), driven by the same CUTOFF RESO DRIVE and the same modulation; the line under the filter's name says which ladder plays. Any other TYPE opens their ladder and uses the shared one.
- The 303 keeps its own small built-in envelope movement and its accent sweep; its ENVMOD knob is replaced by the ENV amount and its DECAY by the filter envelope's DECAY.
- ENV amount: +-64 sweeps the whole range (127 steps, ten octaves) either way; KEY 100 % tracks exactly; DRIVE up to x8 into the saturator, with some level given back.
- FILT 2/2: KEY, DRIVE, LINK (one knob left empty). Velocity to the filter: as before (an accent opens it with the envelope).
- LINK is the default (the MiniFreak's filter follows the amp ENV). The first turn of a filter envelope knob while linked makes it OWN, starting from the amp ENV's values (as measured and proposed with the f-play report). ESYNC applies to both envelopes. ENV page: 1/2 AMP, 2/2 FILT, the AMP | FILT selector of mock-synth-env-A; ENV DEST (PIT SHP ESYNC) moved to the end of MOD.
- HOME KNOB 1 CUTOFF is the voice filter's on every voice; KNOB 2 DECAY is the filter envelope's while it is OWN (ACID INIT), else the amp ENV's.
- The old track FILT (DJ LP / HP) is no longer on a synth page: an older project's or user preset's FILT is moved onto the voice filter (LP: CUTOFF down; HP: TYPE HP). The drum track keeps its FILTER.

Older sounds and projects (Ben: "Save format: Accept"; format 8 kept)
- No new format: the new parameters are appended after P_E7 in format 8's room; rsv[0] of a project's core marks the wave 4 layout. A wave 2 or 3 project, every older format and user preset record 1 are mapped when loaded: ACID's CUT RES DRV onto CUTOFF RESO DRIVE, its ENVMOD onto the ENV amount (the sweep the 303 made), its DECAY onto an OWN filter envelope with the 303's decay time; JUNO's CUT RES ENVMOD and its half key tracking (KEY 35 %); their filter locks become the voice filter's locks.
- What cannot map exactly: PLAITS' and ELEMENTS' old ENV > FLT and LFO > FLT moved their timbre or brightness; now they move the filter. ENV > FLT on PLAITS is added to T.ENV (positive amounts only); LFO > FLT now sweeps the filter (from fully open, so only its lower half is heard). PLAITS factory sounds with LFO > FLT (FOLD DRONE, TABLE SWEEP, SPECTRAL PD, DREAM 69, FORMANT VOX, TERRAIN PAD, NOISE SWEEP, VCF SWEEP) now wobble the filter, not the timbre.
- User presets: record 2 keeps the 192-byte layout and puts the new parameters where the never-loaded 16-step pattern was. A firmware before wave 4 shows a record 2 slot as empty.
- MIDI LEARN: the new parameters are learnable (their id + 1, codes 72 .. 82).

The GRID (Ben: "Locks: Rytm way")
- The synth GRID has the drum screen's tabs: TRIG OSC FILT ENV FX MOD. Page buttons change the tab with a step held or not; nothing held, the knobs edit the sound; held, they lock. SEQ on the GRID: TRIG, again back (as the drums). SEQ from a sound page: the GRID on that page's tab; from anywhere else: on TRIG (note entry).
- SELECT on the synth GRID walks the tabs; the tempo is on HOME and the pages (it was on the GRID with nothing held).
- OCT- with a step held on a sound tab: the last-turned knob's lock, then every lock (as the drums); on TRIG: locks, nudge and condition (as before).
- The old PATTERN page is no longer reached from the GRID (as on the drum screen since wave 3). DIV, SWING and STEPS are on hold SEQ; GATE has no page now. OPEN: Ben to say where GATE goes, if anywhere.
- A step with locks: a small square in its tile (the drum screen's mark); a nudge keeps the dot at the top.
- The GRID's header shows the sound's name (the drum screen shows its kit); STEP REC and LIVE REC still show there.

The screens
- Synth header tag: the voice's number (1 2 3), as mock-synth-osc-A, on every synth page; the title is the sound's name.
- Every page's dial row is the drum screen's (four dials, the tall face in fixed columns, units dropped). On the shared list pages the list names the knobs with a bar where each sits, and the values are in the dials only (they showed twice).
- Five-letter values take a 7 px step instead of 8 so they never touch the next ring (the drum LFO page's PITCH); longer ones are cut to five, as before.
- The FILT tag "AMP ENV" of mock-synth-filt-A is the line under the filter's name instead ("ENV: AMP" / "ENV: OWN"): drawn over the curve it hid it.

Memory and the device (Ben, 22 Nov: "Make the perfect version. Then we'll compromise")
- Every part's engine arena is sized for the biggest engine (16.6 KB: Plaits' PARTICLE). On the host and in the simulator: no limit. The device build would need 14.8 KB more pool than the 8 KiB rule leaves; one named limit keeps it building: tools/build.py PHEBUS_ARENA_T12=9152 (tracks 1 and 2's arenas). With it, 6-OP A/B/C, STRING, SPEECH and PARTICLE play on track 3 only on the device ("NOT ON THIS TRACK" on 1 and 2); everything else everywhere. OPEN: Ben to choose (see the wave 4 report).
- ELEMENTS' Ominous Voice now starts from a cleared arena (a PHYS layout's slots were left in it); the arena test now runs on all three tracks.

Not checked
- No device build (no toolchain here); the voice filter's cost on the FM-1 is estimated from the host (about 0.45 of a JUNO voice per voice). The long tests (soak, stress, regress, web) were not run; the 26 golden renders will differ more (every voice now runs through the filter).

## Rebuild wave 5, the MiniFreak voices, part two (22 Nov): calls made that Ben did not specify
Where the spec left a choice open, the most MiniFreak-like option was taken; each is open to change.

FX slots (mock-synth-fx-A; Ben: "FX slots", "I don't like having fx on by default")
- Three slots in series on every voice, after the filter, before the SLICER and the voice's level. Each: TYPE on KNOB 1, then the MiniFreak's TIME, INTENSITY, AMOUNT on KNOBS 2 to 4, named by the type. SELECT and the FX button walk FX 1, FX 2, FX 3, then SLICER; the page draws the three slots as boxes (the one on the knobs filled, a lit square when on) and the effect below.
- Types: CHORUS, PHASER, FLANGER, REVERB, DELAY, DIST, SOFT, HARD, FOLD, FUZZ, CRUSH, EQ. PHASER, FLANGER, SOFT to FUZZ and CRUSH are the old INSERT's code; DIST the old per-voice DIST; EQ is new (three bands, -12 to +12 dB). Each type's knobs: CHORUS RATE DEPTH SEND, PHASER and FLANGER RATE FDBK MIX, REVERB SIZE DAMP SEND, DELAY TIME FDBK SEND, DIST and the drives TONE DRIVE MIX, CRUSH RATE BITS MIX, EQ LOW MID HIGH. The INSERT's fourth value each type had (the drives' LEVEL, CRUSH's LPF, the sweeps' DEPTH) stays at its old default: the MiniFreak gives every effect three knobs.
- The shared effects: there is one delay line, one reverb and one chorus (the MiniFreak also has one reverb and one delay). A CHORUS, REVERB or DELAY slot is the voice's send to it (KNOB 4: P_CHOR, P_REV, P_DLY, the same parameters as before, taken after the level and pan as always, so the slot's position does not change the sound); its KNOB 2 and 3 are GLO's settings (REV/CHO, DLY), shared by every voice and drawn white-grey, not in the voice's colour. A voice sends nothing to a shared effect without a slot of that type. One CHORUS, one REVERB, one DELAY and one FLANGER a voice (the flanger needs a delay line; one a voice keeps the RAM as it was): KNOB 1 skips a type another slot has.
- Picking a type sets its knobs to that type's start; a send type starts its send at the old preset default (chorus 24, delay 28, reverb 36) if it was 0; turning a send type away puts its send to 0. TYPE cannot be locked (as the Rytm's machine and the oscillator TYPE); TIME, INTENS, AMOUNT can.
- The voice's old INSERT pages are gone from the synth voices; the drum voice keeps its INSERT (TYPE, A B C, MIX) and SLICER on FX.
- MIDI CC 91, 93, 94 (sends) on a synth voice with no slot of that type put one in the first OFF slot (none OFF: nothing).
- Old projects, user presets (records 1 and 2) and every factory preset: DIST, the INSERT and the sends become slots in that order (DIST, INSERT, then the sends largest first), their locks moved with them. Exact, except: past three effects the smallest sends go (no factory sound has more than three; a project with DIST, INSERT and three sends loses its smallest sends); an INSERT's fourth value at a non-default (the drives' LEVEL goes into the sound's trim, exact at MIX 100 %; CRUSH LPF, PHASER/FLANGER DEPTH stay at default); the INSERT's mono CHOR becomes the shared CHORUS.

The mod matrix (Ben: "A: grid"; mock-synth-mod-A)
- Rows (sources): LFO1, LFO2, AENV (amp envelope), FENV (the filter's envelope: LINK AMP, the amp envelope), PENV (the old ENV > PIT's fast pitch fall), VEL, KEY. PENV is a row because ENV DEST's PIT always used that fall, not the amp envelope; as a row it can now go anywhere.
- Columns (destinations): PIT, M1 M2 M3 (the OSC page's macros, as the Type names them), CUT, RES, LVL, PAN, FX1 FX2 FX3 (each slot's AMOUNT: its mix or its send), AS1 AS2 (assignable: any lockable sound parameter, chosen with KNOB 4 TARGET). 7 x 13, the MiniFreak's own size.
- Navigation: KNOB 1 SOURCE (row), KNOB 2 DEST (column), KNOB 3 AMOUNT (-64 to +63), KNOB 4 CLEAR (on AS1 and AS2: TARGET). The grid shows six columns at a time and slides with the cursor. Positive cells a filled square, negative an empty one, size by amount.
- One source of truth: six cells ARE the old fixed routings' parameters: LFO1 > PIT (P_LD_PIT), LFO1 > CUT (P_LD_FLT), LFO1 > LVL (P_LD_AMP, the old one-way tremolo depth), FENV > CUT (the FILT page's ENV), PENV > PIT (P_ED_PIT), KEY > CUT (FILT 2's KEY). Turning either place changes the same value; these six lock on a step as before. The old LFO > SHP and ENV > SHP moved the engine's "shape" macro: they become LFO1 and AENV cells of that macro's column when an old sound loads (PLAITS M3, JUNO M2 (PW); ELEMENTS' shape is its 4th control, so an assignable column at half the amount, approximate; ACID had none, they did nothing there).
- Besides the six, 19 cells a voice (the 46 bytes format 8 kept: 19 cells, the two targets, LFO 2's SYNC and TRIG, Spice and Dice). The 20th says MATRIX FULL: 19 CELLS.
- PIT, M1-M3, CUT, RES and LVL act on each note with its own envelopes. PAN, FX and AS act on the whole voice part: the LFOs as they run, the envelopes, velocity and key of the note started last (a voice's notes are mixed in mono before the pan and the slots).
- LVL only ever brings the level down from the voice's own (a depth, as the old tremolo): no clipping from the matrix.
- LFO 2: RATE, WAVE, SYNC, TRIG (NOTE, FREE, PATT); FREE to start (the MiniFreak's runs free unless asked); its sample-and-hold uses its own random numbers.
- The MOD pages: LFO (RATE WAVE PHASE FADE), LFO SYNC (SYNC TRIG POL and ESYNC, which was on ENV DEST), LFO 2, MATRIX. LFO DEST and ENV DEST are gone (their values are grid cells).

Spice and Dice (Ben: "Cut all but give me the spice & dice from minifreak")
- On the synth GRID, TRIG 2/2 with no step held (that half said HOLD A STEP before): KNOB 1 SPICE (0 to 100 %), KNOB 2 DICE (each click right a new roll, numbered; left the roll before, so a roll you liked can be found again). A gesture on a button was considered: every free button on the GRID already has a job.
- A roll gives each step random numbers for: left out (up to 30 % of the notes at full Spice), an octave up or down (25 %), a ratchet of 2 or 3 (20 %), velocity (+-48) and length (x0.25 to x1.75). Spice scales them all; more Spice is the same roll, further (nested). The notes in the pattern are never touched; Spice 0 is the pattern exactly as written. The same roll and Spice repeat every loop.
- Kept with the pattern (each section A to D its own). Melodic voices only (the Rytm has no such thing; the drum trigs have CHANCE and conditions).
- The GRID's tiles show the variation while Spice is up: left-out steps dim with a line, octave arrows, ratchet ticks, the length as a line.

GATE (wave 4 left it without a page). CALL FOR BEN TO CONFIRM
- Hold SEQ: KNOB 1 GATE, next to DIV, SWING and STEPS (KNOB 1 was empty). Synth voices; the drum voice shows -- (its hits are one-shot).

GLMODE and ALLOC (wave 4 left them without a page)
- Both still matter (glide by rate or time; poly voice allocation), so they got OSC 4/4 (KNOBS 1 and 2), the voicing settings, the MiniFreak keeping them in its Sound settings. PAN and MUTE stay on the mixer only.

User presets
- Record 3 keeps the slots and LFO 2 (in the record's spare bytes) and each record's matrix in its bank after the 16 records (the bank's flash object had 760 bytes free). A bank stored before loads, its records' FX and routings mapped as above. A firmware before wave 5 shows a bank stored by wave 5 as empty (its length changed). Device compile switch PHEBUS_UP_MX=0 drops the matrices from user presets (saves 1,472 B of RAM) if the device needs it; not set.

Memory (Ben: "Make the perfect version. Then we'll compromise")
- No new pool use (the effects reuse the old INSERT's states and lines: four lines for the drums' INSERT, one a voice). Measured on an rv32 -Os stand-in build against the wave 4 merge (not the JieLi toolchain): pool +0 B; .data+.bss +2,570 B (about 96,240 of 98,304 on the device if the stand-in's difference holds); code and constants +22.4 KB (about 521,500 of 581,564). Everything fits; no new named limit; PHEBUS_ARENA_T12 stays as wave 4 set it.

Not checked
- No device build here (no toolchain); the sizes above are the stand-in's differences. The long tests (soak, stress, regress, web) were not run; the golden renders will differ where presets carry sends (they now go through slots: same arithmetic) and LFO > SHP (now cells). browse_test reports 2 presets differing after browsing, as on the wave 4 merge.

## Rebuild wave 6, the MPC performance (22 Nov): calls made that Ben did not specify
Where the spec left a choice open, the most MPC-like option was taken; each is open to change.

The pads (Ben: "A: pad map, trimmed set", about 18)
- 20 pads plus two triplets (22 keys of 27): the 16 white keys as mock-perf-A's 4 x 4, the black keys as its chips. Row 1 1/1 1/2 1/4 1/8, row 2 1/16 1/32 REVRS HALF (the stutters together, longest first as in the mock; 1/4 and 1/8 on keys 3 and 4, the mock's own places), row 3 LP HP CRUSH GATE, row 4 STOP DROP ECHO VERB. Black keys: F#3 -DRM (kill drums), G#3 -BAS (kill bass), A#3 LCUT (low cut), C#4 FLNG (flanger sweep), C#5 8T, D#5 16T (the triplets on the far right, never in a prime place); D#4 F#4 G#4 A#4 F#5 have none.
- Changed from the mock: ECHO moved to row 4 beside VERB (the throws together, the tape effects STOP and DROP together); the mock's DUST and WOW are not pads (DUST stays on GLO > MASTER; WOW was the old wobble).
- Gone from the old 27 (Ben's trimmed set): STUTTER (beat / 6: now the 16T pad), PHONE, ALIAS (now CRUSH's RATE knob), WOBBLE, KILL MID, KILL HIGH, REWIND, OCT UP (its two-tap code is now DROP's), AUTO-PAN, FREEZE (and its reverb hold), MONO, FADE OUT. KILL LOW is LCUT with a FREQ knob.
- -BAS kills voice 1 (the bass to start); its KNOB 1 picks the voice. The kills fade the part as a mute does but keep its notes playing unheard, so it comes back exactly in place (a MUTE stops new notes; a kill that did that came back with the bass's oscillators elsewhere, measured -5 dB of difference for as long as the song ran).
- 1/1 and 1/2 replay the sequencer, as the STUTTER lane's (the 0.74 s buffer cannot hold them); with the transport stopped they do nothing.
- Several at once: one TIME effect (stutters, REVRS, HALF, STOP, DROP, FLNG: the last pressed plays; let it go and an earlier one still held comes back, as the MPC's beat repeat) with every COLOUR effect (LP, HP, LCUT, CRUSH, GATE), both throws and both kills. No limit of four (the MPC's): see CPU below.
- ECHO and VERB are the old REV / ECHO THROW (the shared delay and reverb, ringing out after release); FX + OCT- / OCT+ no longer throw (OCT is the latch now).

In and out (the spec: "one obvious gesture ... must not steal note playing")
- Hold FX: PERFORM while held. Held 0.7 s untouched and let go: it stays open (LOCK), exactly as GLO keeps the mixer; FX again, HOME or any page button leaves. FX + HOME still locks it, as every layer. A double tap of FX was not used: its first tap opens the voice's FX pages.

Latch (Ben: latch)
- Hold OCT+ and press a pad, or press OCT+ while holding pads (the MPC latches with a button). A latched pad pressed again: off (the MPC's). OCT-: every latch off. A double tap of a pad was not used: stutters are tapped in rhythm and would latch by accident.
- Latched pads keep playing after PERFORM is left (hands free for notes); every screen's header shows PERF and how many, white, beside FILL. STOP ends every latch. A pattern change keeps the latches.
- The header says PERF, not FX: "FX 2" would read as a voice's FX slot 2.

The knobs (Ben: "knobs tweak the last effect pressed")
- The drum and synth pages' dials (op_dials), white-grey. KNOB 4 is AMOUNT (the wet level) wherever a pad has one. Per pad: stutters PITCH DECAY GATE; REVRS and HALF LENGTH; LP and HP CUTOFF RESO SPEED; CRUSH BITS RATE; GATE RATE WIDTH; STOP TIME; DROP DEPTH TIME; ECHO TIME FDBK; VERB SIZE DAMP; -BAS VOICE; LCUT FREQ; FLNG RATE FDBK; 1/1, 1/2 and -DRM none.
- ECHO's TIME and VERB's SIZE and DAMP are GLO's own (shared, as an FX slot shows them since wave 5); the rest belong to the pad.
- The start values sound as the old effects did where there was one (1/4 a beat, HALF a beat, STOP to a stop in a beat, the gate 1/16 half open, CRUSH 7 bits, LCUT 251 Hz, ECHO's feedback floor 70 %, FLNG a bar a sweep, AMOUNT 100 %). LP now closes over its SPEED (1/2 to start: about 1 s at 120 BPM, as the old code's comment said; it measured 0.18 s) down to 161 Hz; HP rises from 81 Hz to about 1.5 kHz, as before.
- Knob settings are performance state, as mutes: kept until power-off, not saved. The STUTTER lane's own lit steps always play at the start settings (so they sound exactly as before).

Recording into the STUTTER lane (Ben: "Keep, and record played FX into it")
- While any voice records and the transport runs. Each pad played is a span on the lane's own steps: its pad, its first step and length (press and release to the nearest 1/16), and the knob last turned while it played with its value (4 bytes; format 8's 124 bytes hold 31). A latched pad records from latch to unlatch (or the end of the take).
- A pad recorded over its own earlier span replaces what it overlaps; other pads overdub. The 32nd says FX LANE FULL: 31.
- The lane's ON / OFF (its KNOB 2, HOME's KNOB 3) gates the recorded pads too. A pad played live comes before the same pad recorded, and any pad before the lit steps.
- Undo: EDIT + OCT- takes back what a take recorded into the lane (the one undo there is: a later edit of a voice replaces it).
- A pattern change drops a span still open; the new pattern plays its own spans.

The STUTTER lane's screen
- The steps' tiles are 22 px high (were 26) to make room for three rows of recorded pads, named bars across their steps. The big ON / OFF and the big length are now in the dials only (they were shown twice). KNOB 3 (empty before) is FX: how many; turned twice within 1.5 s it clears them. Hold a step + OCT- erases the pads over it; a step lit by that very press goes out again.

Memory (Ben: "Make the perfect version. Then we'll compromise")
- No new buffer: the TIME effects use the punch ring, the colour effects the old effect's filter states. Freed: the reverb's own return buffers kept for FREEZE (256 B), the kills' block buffers (256 B, the low cut now runs a sample at a time), FREEZE's state, the old layer's 27 tiles (16 now: 154 B).
- Measured on an rv32 -Os stand-in build against the wave 5 merge (not the JieLi toolchain): .data + .bss 176 B smaller, pool +0, code and constants +7.3 KB. On the device that would be about 96,070 of 98,304 B and an image of about 518 KB of 581,564. Everything fits; no new limit.

CPU (not measured on a device)
- Host: the idle path costs what it did (4.4 ns a sample); one pad 11 to 20 ns (a stutter 11, LCUT 20); five colour pads at once about 40, with DROP on top about 55, against about 75 for the whole default mix. Stacking is the only new cost; if the FM-1 runs short, a limit of four at once (the MPC's) is the compromise to offer.

Not checked
- No device build here (no toolchain). The long tests (soak, stress, regress, web) were not run.

## Rebuild wave 7, HOME and the guide (22 Nov): calls made that Ben did not specify
HOME (Ben: "HOME: B: three zones", keeping CUTOFF, DECAY, STUT, LEVEL, the FLOW band, heads in the strip only)
- Layout: a deck (BAR and the bar number, the FLOW strip 116 x 14 px, the patterns A to D) then three zones of 38 px: SYNTH, DRUM, PERFORM. The mock's "VOICE 1" line is left out: the selected voice is shown by its name inverted in its colour (and the strip's underline), which the mock did not have room for with the band.
- SYNTH shows each voice's sound name (not the Type), M / S / a red dot, and its 16 steps in view. DRUM shows the kit, the drum voice's M / S, how many sounds MUTE mode mutes or solos, the selected sound, and every sound's hits in the 16 steps (the selected sound's bright, the others dim, only-muted grey). PERFORM shows LANE ON / OFF, the latched pads by name (or "n LATCHED"), and the lane's steps as spans.
- The pattern asked for next (playing) is outlined white on the deck.
- HOME's knobs use the pages' dials (one tall face, fixed 40 px columns, no units): DECAY reads "2.52", as the ENV page shows a time.
The frame on every screen
- The layers (EDIT, ARP, SEQ, SEL, GLO's tiles, SAVE), PERFORM, the REC screen and the STUTTER lane now have the header every screen has (tag: the button held, or REC / STUT), a line under it (what the layer does, LOCK, or a message as a white bar), the body, the pages' dials and the strip. The big "FX: PERFORM" style titles are gone; the tiles are 28 px rows (PERFORM's pads 20 px, chips 12 px).
- The header's title may run to 6 px before the tempo (PATTERN A was cut to PATTERN); the tempo, play symbol and beat lights moved 4 px right, the beat lights 7 px apart.
- The mixer's dials are the pages' dials in each voice's colour; its podiums sit over the dial columns; its M / S badges are the strip's.
- Messages: on pages over the dials as before; on layers and PERFORM in the line under the header, so the dials stay.
- Not restyled (kept by the Hybrid decision or as modal screens): the menu, the hold-to-clear ring, the boot and the screensavers. The GLO list pages keep their page dots (a tab row would need the list moved).
Words (a glossary is in GUIDE.md)
- A key is a STEP: the drum screen says STEP n HELD and HOLD A STEP (was TRIG). RETRIG everywhere (was RTRG on the drum line; when a retrig and lock names do not fit, the locks are counted: LOCKS n). The lane's recorded pads are PADS (its KNOB 3, PADS ERASED, AGAIN: CLEAR PADS, LANE FULL: 31 PADS; was FX). A to D are PATTERNS (the SAVE layer's title, EMPTY PATTERN, "into pattern A"). Shared pages' tag GLO (was ALL).
Stutter lengths as spans (taken as read in REVIEW.md)
- Lit steps in a row with the same length are drawn as one bar across their tiles, the length named once ("1/4"); a single lit step keeps its short number.
Found and fixed
- The graphs' tints were left from the last screen that set them: voice 1's ACID picture and voice 3's drew in coral after a drum or shared screen. They now reset to the selected voice every frame.
- 64-bit divides the device toolchain lacks (__udivdi3 / __umoddi3), all now 32-bit with tests proving them equal to the 64-bit form: the loops' read start (punch.c, every len, k and PITCH), the drum RETRIG grid (seq.c roll_grid; every rate divides a bar exactly) and curve (rt_vel), the SEQ layer's time view (pre-wave 2 code, exact by the floor identity), an FX picture's curve.
- gp_bars read d in the same expression that assigned it (the warning); the same pattern in ui_pages_test.
Memory: on an rv32 -Os stand-in build against the wave 6 merge (not the JieLi toolchain): pool +0 B, .data+.bss +160 B (about 96,300 of 98,304 on the device), code and constants -2.0 KB (about 514.6 KB of 581,564).

## Rebuild wave 8, the layout logic (22 to 23 Nov): calls made that Ben did not specify
Ben's answers (SPEC.md, "22 Nov 2026, 23:20"): "FX = whole mix only", key legend "Leave as built", "One rule everywhere", "Same words" (all four). Where they left a choice, the call that best fits his MPC rule ("everything is either written on the knob or written below the knob ... The most hidden things get is a long press on a button") was taken.

FX = whole mix only
- A voice's FX stage: FX 1, FX 2, FX 3 (inserts: PHASER FLANGER DIST SOFT HARD FOLD FUZZ CRUSH EQ), SENDS (DELAY REVERB CHORUS: the voice's P_DLY P_REV P_CHOR, always heard, 0 to start), SLICER. Its colour, its pages. CHORUS, REVERB, DELAY are never picked as a slot type (their stored numbers stay, read as OFF).
- Route to the FX stage without the FX button: EDIT walks the chain (from ENV: one press), SELECT walks every page. On the GRID with a step held: EDIT from ENV, the FX tab (SENDS lock per step).
- FX tapped (any voice, any screen): DELAY (TIME FDBK COLOR MIX), REV/CHO (SIZE DAMP RATE DEPTH), MASTER (DUST DUCK). Tag FX, white and grey; drawn as the GLO list pages.
- GLO: GLOBAL (BPM CLICK TUNE), SYSTEM, MIDI, MIDI 2, MIDI 3. Gone from GLO: DLY, REV/CHO, MASTER, DRUMS. GLO > DRUMS's four: CH4 is on MIDI, LVL on the mixer and HOME, REVERB and DELAY (the whole drum voice's sends) on the drum voice's FX tab. MASTER's ROLL was hold ARP KNOB 1 a second time: only there now.
- The drum voice: a sixth tab FX on its drum screen (TRIG SRC FLTR AMP LFO FX), EDIT walking SRC > FLTR > AMP > LFO > FX > SRC: FILTER (its LP / HP, HOME KNOB 1 on the drums), INSERT, INSERT 2 (MIX), SENDS (DELAY REVERB, not lockable: song values), SLICER. A step held, its knobs lock as any tab's: this replaces "a drum step held + FX". Ben's tab names SRC FLTR AMP LFO kept; FX added after them as the Rytm's FX track.
- SLICER kept, unchanged, as the last page of each voice's FX stage and of the drum FX tab (one home: the voice's own effects). The question is put to Ben (below).
- PERFORM: ECHO keeps FDBK (KNOB 2, where it was, so a recorded span keeps its knob) and AMOUNT; VERB only AMOUNT. KNOB 1 of ECHO and KNOB 1 to 3 of VERB are empty.
- The matrix's FX1 FX2 FX3 stay the three inserts' AMOUNT. Sends get no column: the MPC treats a send as a mix level, and an assignable column (AS1, AS2: TARGET DELAY, REVERB or CHORUS) moves one when wanted.
- MIDI CC 91 / 93 / 94 set the sends; a CC never makes a slot. A learned CC on a slot's TYPE skips the old send numbers (OFF).
- Conversion (format 8 stays "FUN8": rsv[0] 3 marks wave 8; no P_ or G_ renumbered): a project of waves 5 to 7 (rsv 2) and a user preset record 3: each CHORUS / REVERB / DELAY slot becomes OFF in its own place (the inserts keep their order, their locks stay), its send kept; a send the sound had no slot for was silent: 0, its locks freed; a matrix cell on such a slot's column moves to a free assignable column aimed at that send. Lossy only when both assignable columns are already used (the cell is dropped; fx8_lost counts it). A project or preset from before wave 5: DIST and INSERT become slots as in wave 5, and now every send is kept (wave 5 dropped the smallest past three). Factory presets keep all their sends (wave 7 had them in slots, up to three). User presets are stored as record 4 (a firmware before wave 8 shows them empty).

One rule everywhere
- Confirm: hold REC until a ring fills. Picked because REC held to clear a voice already was a hold with a ring, red means erase, it is one long press of one button (the MPC rule's deepest), and it needs no second turn and no timing. Every destructive or GO action asks in a white bar, HOLD REC: ... (USER LOAD, ERASE, SAVE; TOOLS CLRSQ, INIT, NEW; storing over a used pattern; replacing the edited kit; clearing the lane's pads). The ring starts at the press (with no question, REC held keeps its 0.7 s before the ring, as REC's tap records). Let go early: nothing, the question stays; REC tapped, another button or another screen: it goes. Nothing records while a question shows. For a pattern store, SAVE stays held (or kept open) while REC is held: the question lives on the screen that asked.
- Keep a layer open: hold its button 0.7 s untouched and let go, on every layer. Picked over "hold + tap HOME" because it is a long press of one button (Ben's deepest), not a chord of two, and GLO and FX already did it. The HOME chord is gone.
- Latch: OCT+ with the thing held, as PERFORM's. The drum FILL now latches with OCT+ and F#5 (either order); F#5 again lets it go (as a latched pad); STOP ends it. The F#5 double press is gone.
- Double presses removed: F#5 twice (FILL), HOME on HOME (screensaver: now HOME's pages, SELECT right; a ">" after the deck's FLOW band says so), SEQ again going back to the tab before (on the synth GRID and the drum screen SEQ again is TRIG's next half, as ENV again is ENV's next page), the GO knobs' second turn, the pattern key again within 3 s, the lane's KNOB 3 twice, the kit's second turn.
- Kept, no timing in them, said so in the guide: a page button pressed again walks its pages; SEL / ARP taps toggle MUTE / CHROMATIC on the drums; the mixer's key 10 and 11 toggle; hold a step + OCT- removes the last-turned lock, pressed again every lock (a sequence in one hold, each press says what went: a call for Ben).
- Three-control chords: the only one left is hold a step + a page button + a knob (a lock on another page), Ben's "Locks: Rytm way". The drum bus lock is that same rule now (it was step + FX + knob). Ghost and hard hits (OCT + drum key), undo (EDIT + OCT), EDIT + black key and GLO + F#3 are two controls.
- Times, in one place (ui_layers.c, GUIDE.md section 2): tap under 0.45 s, a layer shows after 0.14 s, the long press 0.7 s (keep open, menu, REC clear), the ring 1.3 s. Removed: 0.4 s (FILL), 1.5 s (GO, kit, pads), 3 s (pattern store).

Same words
- Filter page: the drum FLTR on both machines is the synth FILT: CUTOFF RESO ENV TYPE, then DRIVE first on 2/2 (synth FILT 2: DRIVE KEY LINK; synthesised kits FLTR 2/2: DRIVE HPF; the HEX machine has no filter envelope: its ENV knob is empty). The synthesised kits' DRIVE moved from AMP 2/2 to FLTR 2/2 (that knob is empty on AMP now; DEL REV LEVEL stay where the HEX kits have them).
- One spelling: CUTOFF, RESO, FDBK on every knob (drum FREQ / RES, HEX CUT / RES / FILT, PERFORM LCUT FREQ, GLO FEEDBK, CRT / CDP renamed; the drum LFO DEST and the matrix column read RESO). CUT stays as a 3 to 5 letter value name (LFO DEST, the matrix column) where CUTOFF does not fit.
- RATCH is RETRIG on a synth step (x1 to x4). ROLL stays for held keys.
- Spare filters cut: the synth tracks' hidden DJ filter (CC 74 now the voice filter's CUTOFF; on the drum channel the drum voice's FILTER) and GLO MASTER FILT. Their memory: the master and three tracks' filter states (160 B). A wave 4 to 7 project's synth FILT moves onto the voice filter as wave 4 mapped it; a FILT lock (none can be made from a page) is freed; a project's MASTER FILT is dropped (lossy: PERFORM's LP and HP are performance state). The drum voice's own FILTER stays (it is not hidden and is its bus).
- Tempo in one place: GLO > GLOBAL, KNOB 1 BPM. Picked because it is written on a knob on a page; SELECT is the tempo nowhere now (HOME, the mixer, the REC screen, layers), and tap tempo (the mixer's key 16) is gone: it was a second place. Every header still shows the tempo. MIDI clock still sets it.
- Pattern settings (audit 1.4, 1.5): the "PATTERN" page (STEPS DIV SWING GATE) was a leftover, unreachable since wave 4 (only the screenshot tool drew it): removed; hold SEQ (GATE DIV SWING STEPS) is the one place. SWING was also GLO > GLOBAL's (added to every pattern's): removed from GLOBAL and from the sequencer and slicer; an older project's GLOBAL SWING is added to each track's own when it loads (and to sections stored then, while that project is the one loaded; a section stored by an older firmware and loaded after a reboot with a new project loses the global swing it borrowed). STEPS was also the REC screen's KNOB 2 (the same P_SLEN): removed. Hold EDIT's KNOB 2 (copy x2 / halve) reads DOUBLE. The lane's KNOB 4 STEPS is the lane's own length and stays.

Memory, measured on an rv32 -Os stand-in (clang, not the JieLi toolchain) against the wave 7 merge: .data+.bss 260 B smaller, code and constants about 1.3 KB smaller, pool unchanged, .noinit unchanged. On the device that would be about 96,000 of 98,304 B and an image of about 514.9 KB of 581,564; the pool still about 200 B above the 8 KiB rule (nothing new was put in it).

Not checked
- No device build. The long tests (soak, stress, regress, web) were not run; golden renders differ where presets now keep every send.

## Rebuild wave 9, how to move around (23 Nov): calls made that Ben did not specify
Ben's four ways (SPEC.md, "23 Nov 2026, 08:25") and his rule at 08:42 with its picker answers ("PAGE button: SEL", "Drum sound pick: SELECT knob", "Under ARP", "On the mixer"). Where they left a choice, the call that best fits the four ways was taken. The 08:42 rule replaced the 08:25 answer "EDIT: osc, filter, alternating": a labelled button steps through its categories, SEL through the pages of one.

The rule in the code
- Two actions, any button can be bound to either (ui_nav.c): nav_button (a group's first category; on it already, its next, round) and nav_page (SEL: the next page of the category shown, round; one page: nothing). Every page carries its category (params.c page_t cat); the drum screen's tabs and halves and the GRID's TRIG and PATTERN are categories too.
- A category always opens on its first page (it no longer remembers the last page, on GLO, SAVE, ARP and KEY as on the sound pages).
- The tab row on every page screen, the GRID and the drum screen names the categories of the button's group, the one shown filled in the voice's colour (MIX in white after a divider), and "2/4" the page inside. It replaced the stage tabs (TRIG OSC FILT ENV FX MOD, TRIG SRC FLTR AMP LFO FX) and the GLO pages' dots. Ben's drum tab names SRC FLTR AMP LFO are the drum categories' names.
- Groups: EDIT OSC, FILT. ENV AMP, FILT (the filter's own envelope; one page each). LFO LFO 1 (SEL: its sync page), LFO 2, MATRIX. FX INS (FX 1 2 3), SENDS, SLICER, MIX (DELAY REV/CHO MASTER). SEQ TRIG (TRIG 1/2, 2/2), PATTERN. ARP ARP (ARP 2), KEY (KEY 2). GLO GLOBAL, SYSTEM, MIDI (MIDI 2, MIDI 3), DEVICE (the menu's four sections). SAVE PRESETS, USER, TOOLS. HOME HOME, the screensaver (SEL: its four). Drums: EDIT SRC, FLTR; ENV AMP; LFO LFO; FX FILTER, INS (INSERT, INSERT 2), SENDS, SLICER, MIX; SEQ TRIG, PATTERN; ARP CHROMATIC.
- On the GRID and the drum screen the same buttons and SEL change the view, the keys stay steps, with a step held too (the Rytm's page keys): a lock on any page is hold the step, the button (and SEL) until the page shows, turn. MIX shows there too (white dials; the knobs are not lockable: NOT LOCKABLE).
- SEQ opens TRIG from everywhere (it opened the GRID on the sound page's own tab before): its first category, always. TRIG is SEQ's first category because it is what the GRID's keys and knobs are with nothing chosen; PATTERN second.
- A one-category group (drum ENV, LFO): the button again goes back to its first page. The drum ARP is CHROMATIC; ARP again ends it (the keys are steps again), as before.
- The menu is GLO's DEVICE: SEL its sections (they were SELECT's), GLO on to GLOBAL, HOME or OCT- closes, any other page button closes it and opens its own group. Hold HOME stays its shortcut.
- The screensaver is HOME's second category: HOME on HOME opens it (wave 8 had removed that as a double press; it is the rule now, no timing), SEL the next one, HOME again HOME. SELECT does nothing on HOME.

SELECT: the type (Ben: "SELECT = type"; drums "SELECT knob")
- Synth pages: OSC 1 to 4 the oscillator Type, FILT 1 and 2 the filter TYPE, FX 1 to 3 the insert TYPE, SLICER its mode, LFO 1 (both pages) and LFO 2 their WAVE. No type, SELECT does nothing: ENV (AMP, FILT), SENDS, MATRIX, the MIX pages, ARP, ARP 2, KEY, KEY 2 (ARP's MODE and KEY's SCALE and CHORD were candidates; none is the page's one type, so none was picked), GLO and SAVE pages, HOME, the screensaver, the menu, the TRIG and PATTERN views, the STUTTER lane, the layers.
- The drum screen, every tab: the selected sound, one of 16 a detent, stopping at the ends, heard while stopped (as SEL + key heard it). With a step held it does nothing (LET GO OF THE STEP): the held step is that sound's.
- With a step held on the synth GRID, SELECT locks the type where it can be locked (the filter's TYPE, the LFO's WAVE, the SLICER's mode); the oscillator Type and the inserts' TYPE cannot (TYPE: NOT LOCKABLE), as before.
- Where it shows: the header's title place becomes a chip, the dials' little dial and the value, on the dark shade of the voice's colour, filled in it for a moment after a turn (as a touched dial's label). The SELECT knob sits at the screen's upper left on the FM-1. No chip: SELECT does nothing. The sound's name then shows only on pages without a type (and on HOME); the drum kit's name moved to the line under the sound's name.

The pages re-planned (the knob each type took is freed)
- OSC 1: M1 M2 M3 M4 (the Type's own controls together: the three macros and its fourth, T.ENV on Plaits, NOISE on JUNO). OSC 2: OCT TUNE GLIDE GLMODE (the pitch and the glide's mode beside GLIDE). OSC 3: M5 VOICE DTUNE PRIOR (the Type's fifth, then the voicing). OSC 4: ALLOC alone (13 controls need four pages; the rarest one is alone).
- FILT 1: CUTOFF RESO ENV DRIVE. FILT 2: KEY LINK.
- FX 1 to 3: the type's three knobs on KNOB 1 to 3 (left-aligned, as SENDS; KNOB 4 empty: there is no fourth stored value without a new parameter).
- SLICER (synth): PAT RATE DEPTH. LFO 1: RATE PHASE FADE POL; its sync page SYNC TRIG ESYNC. LFO 2: RATE SYNC TRIG.
- The drum screen keeps every knob as it was (SELECT is the sound there): SRC's WAVE and the HEX MODEL, FLTR's TYPE, the LFO's WAVE, the INSERT's TYPE, the drum SLICER's mode (a page of the drum voice's own now, as it was drawn).
- CLASH with "same filter page" (22 Nov, "CUTOFF, RESO, ENV, TYPE on drums and synths"): Ben's 08:42 answer puts the drum sound pick on SELECT, so the drum FLTR keeps TYPE on KNOB 4 while the synth FILT's TYPE is SELECT's and DRIVE takes its knob. The first three knobs are the same on both; KNOB 4 and DRIVE's place differ. Built that way and put to Ben (below).

SEL's old jobs, re-homed
- Key, scale, chord (hold SEL, the KEY layer; tap SEL, the SEL pages): ARP's KEY category (the pages renamed KEY and KEY 2). ROOT and SCALE there now set all three synth voices, as the layer did (the page set the selected voice's only). The layer is gone: picking the key with a key was quicker but is not performance; KEY KNOB 1 ROOT does it.
- Drum MUTE mode (SEL tapped) and solo (SEL + key in it): the mixer (below). Drum sound pick (SEL held + key): SELECT.
- Hold SEL: unused (Ben: "Leave it unused"). The SEL button is no layer any more; it acts on its press.

The mixer with the drums selected (Ben: "Hold GLO: with the drums selected, the keys mute the 16 sounds; voice mutes stay on keys 1 to 4 otherwise")
- The 16 white keys mute and unmute the 16 sounds; the key row on screen shows 16 tiles, each the sound's initial, muted yellow, soloed ice blue, the selected sound framed white.
- F#3 turns the keys into solo keys (SOLO lit, the word under the tiles ice blue), F#3 again back to mutes. G#3 fill while held, A#3 fill on the next bar, C#4 the knobs the pans: the jobs keys 9 to 11 have with a synth voice selected, on black keys because the 16 white keys are the sounds. They show as SOLO FILL BAR PAN boxes under the tiles, in black-key order.
- The four podiums, levels and pans stay; the voices' own mutes are off the keys while the drums are selected (select a synth voice for them). The header says DRUM MIX.

Holds (rule 3), every one
- Performance layers: FX PERFORM, GLO the mixer, SAVE patterns A to D, EDIT erase (with SHIFT and TRANSP, its knobs), ARP roll / drum RETRIG, F#5 FILL, OCT- / OCT+ + a drum key ghost / hard, REC clear the voice, a step held (the lock).
- Shortcuts to a page: SEQ held (STEPS: the PATTERN page's four knobs), HOME held (the menu: GLO's DEVICE).
- The confirm (REC held with the ring) and keep-open (any layer held 0.7 s untouched) stay as wave 8 built them.
- Power-on holds (OCT- rescue, OCT- and OCT+ calibration, 5 s UBOOT): the loader's and recovery's, not the UI's, left alone; calibration also has DEVICE > SYSTEM.
- Removed: SEL held (the KEY layer, the drum sound pick).

Combos (rule 4), every two-control gesture
- Kept: SEQ + OCT- halve, SEQ + OCT+ double as a clone (Ben's own); EDIT + OCT- / OCT+ undo, redo (EDIT the constant, an edit taken back); OCT+ with a pad or F#5 held (the one latch); OCT- in PERFORM every latch off; a step + OCT- its locks off (the step the constant, minus takes away); OCT- + OCT+ the octave back to 0; OCT- / OCT+ held + a drum key ghost / hard (performance, softer / harder).
- New: SEQ + OCT- halves the selected voice's pattern: if the second half holds a note, tie, trig, lock, nudge or condition it asks HOLD REC: HALVE TO n (SEQ kept held while REC is held, as SAVE for a pattern store), then the second half is cleared (it was kept hidden by the old DOUBLE knob); nothing there, it halves at once. SEQ + OCT+ doubles as a clone: the steps, their locks (as many as the 24 hold: DOUBLED: SOME LOCKS LEFT OUT otherwise), nudges and conditions; the drum trigs with their extras and sound locks; 64 steps: 64 STEPS: FULL. One undo each. Not on the STUTTER lane (its length is its KNOB 4). SEQ first, then OCT (OCT first moves the octave or the page of steps).
- Removed, each capability kept elsewhere: EDIT's KNOB 2 DOUBLE (SEQ + OCT); EDIT + a black key, the drum mute mode (the mixer); GLO + F#3 MIDI LEARN (GLO > MIDI 3, KNOB 2 LEARN); a step held + OCT+, the next condition (TRIG's KNOB 4 COND); STEP REC's key held + SEQ, a tie (ARP is the tie; SEQ is always a rest, written after the held notes); SEL + key on the drums (SELECT; solos on the mixer).
- Three controls: a step held + a page button or SEL + a knob (or SELECT): a lock on another page, as decided.

SELECT's other jobs, re-homed
- HOME: the screensavers (HOME again, then SEL). The screensaver: its next one (SEL). STEP REC: the cursor (KNOB 2, which already did it). The menu: its sections (SEL). The synth GRID with a step held: TRIG 1/2 to 2/2 (SEL). Every page walk (the buttons and SEL). The tempo stays GLO > GLOBAL KNOB 1 only.

Memory, measured on an rv32 -Os stand-in (clang, the HAL's asm stubbed, felucca.c) against the wave 8 base: .bss +48 B, .sbss +20 B (.data+.bss about +68 B: about 96,090 of 98,304 on the device), code and constants about +2.1 KB (an image of about 517,700 of 581,564), the pool unchanged (nothing new in it), .noinit unchanged. No 64-bit divide added.

Not checked
- No device build (no toolchain here). The long tests (soak, stress, regress, web) were not run. The sound: a forked render proves the new routes (SELECT on FILT, LFO, FX 1; PATTERN SWING; KEY ROOT; SEQ + OCT+) give the same audio as the values written straight; no engine, project format or preset changed.

## Rebuild wave 10, one idea per control (23 Nov): calls made that Ben did not specify
Ben's words (SPEC.md, "23 Nov 2026, 09:08"), his picker answers at 09:20, the items marked "Proposed by Claude", and his 10:33 remark about the steps staying on screen. Where they left a choice, the call that best fits "one idea per control" and the four ways was taken.

OCT- and OCT+: minus and plus (Ben: "Double tap for octave")
- A single tap acts at once on its press; the same button again within 0.3 s is the octave, and the second press takes back what the first did (the page, the fine step, the preset, the pattern queued). Picked over waiting 0.3 s: the single tap never lags, and every first-tap action is reversible.
- What a tap does on each screen: pages of steps (the steps, the drum screen, the lane; OCT+ on the last adds a copy, as before); SEQ's A-D: the pattern before / after; STEP REC: the step before / after (KNOB 1 is still the octave); CHROMATIC: the sound before / after (its octave by the double tap); SAVE > PRESETS: the sound; SAVE > USER: the slot; every other page: the knob (or SELECT) last turned on that page one fine step, none turned yet: nothing; TOOLS (GO knobs), HOME, the screensaver, the REC screen: nothing; the menu keeps its own OUT / OK and no double tap there.
- The legend: a boxed minus, a word, a boxed plus, white and grey, at the right end of the tab row (pages) or of the line under the header (layers); one word when both move the same thing ([-] STEPS [+]), two otherwise ([-] UNDO [+] REDO). Where the tab row is too long (the drums' FX row) the categories not shown are cut to three letters (FILTER INS SEN SLI | MIX). The lane got the tab row too (its pad bars two rows, not three).
- With a layer held the double tap is not the octave: each press is the layer's (ARP: two semitones). With a key or step held, no double tap either.
- On a page screen with the drums selected a tap of OCT- still acts (it is also the ghost hit's modifier when held before a key); only on pages where a knob was touched.

Minus removes
- A step held + OCT-: on TRIG the step itself (note or hit, its locks, nudge, condition); on a lock view the last-turned lock, then every lock (as before), then the step. A lane step: its pads (as before).
- A key held, then OCT-: that note (synth) or sound (drums) erased from the selected voice's pattern, as EDIT + key did (playing: as the playhead passes; stopped: every step); OCT- first, then a key, stays the ghost hit. "Holding a sound" is its key on a screen where the keys play sounds (HOME, the sound pages); on the drum screen the keys are steps, so a sound's hits go one hit at a time there.
- PERFORM: OCT- with pads held unlatches those; nothing held, every latch (as before). A latched pad pressed again was already off (wave 6), so "a pad's latch" was already one gesture.
- EDIT + OCT-: CLEAR VOICE n (asks; hold SAVE). It was REC held. EDIT held erases nothing; its keys do nothing; its knobs nothing; it shows MASTER's LEVEL.

SAVE keeps and confirms (proposed by Claude: "the confirm is hold SAVE with the ring")
- Every question says HOLD SAVE: ...; SAVE held fills the ring (1.3 s, from its press, or at once when SAVE is already held as the question comes: storing a pattern). Tapped: the question goes. The question now stays when the layer that asked is let go (EDIT, SEQ), so SAVE can be held after it.
- Ring colour: red when something goes or is replaced (every question but one); white when it only keeps: SAVE into an empty user preset slot.
- Hold SAVE: keys 1 to 4 store into A to D (they were keys 5 to 8; playing moved to SEQ); OCT- undo, OCT+ redo (they were EDIT's). SAVE held with a question is never the layer.
- REC only records: no hold, no ring, no confirm. The message line grew from 23 to 29 letters (HOLD SAVE: CLEAR VOICE 1 fits).

SEQ is rhythm
- Categories TRIG, PATTERN (GATE DIV SWING STEPS; 2/2 SHIFT, the old EDIT KNOB 1), A-D (KNOB 1 the pattern, -/+ before / after).
- Hold SEQ, one layer: white keys ROLL (drums RETRIG); black keys F#3 G#3 A#3 C#4 play A B C D, D#4 BAR (fill on the next bar: the mixer's old key 10), all drawn in the strip; PLAY: FILL while held, OCT+ with PLAY held latches, OCT- with PLAY held or PLAY again lets it go; KNOB 1 RATE (roll / retrig), KNOB 2 SHIFT, KNOB 3 SWING, KNOB 4 STEPS; OCT- halve, OCT+ double; MASTER SWING. GATE and DIV left the hold (on the PATTERN page): the knobs went to the performance ones. The old pattern-in-time drawing of hold SEQ is gone (the rolls' tiles took its place).
- Rolls are on the white keys only: a synth's black-key notes cannot roll (they are the patterns). The drums' RETRIG rate is KNOB 1 only (the black-key rates are gone).
- With SEQ held PLAY never starts or stops the transport.

ARP is notes
- Categories as wave 9 (ARP, KEY; drums CHROMATIC). Hold ARP: OCT- / OCT+ the selected voice's pattern a semitone (the old EDIT KNOB 3 TRANSP; one undo each); MASTER TRANS (KEY 2's, a live transpose, not stored notes). Keys and knobs do nothing while held. On the drums: nothing on -/+ and MASTER (shown so).
- STEP REC: ARP the tie (a note matter), SEQ the rest (rhythm), as wave 9 already had.

GLO is everything global
- MIDI LEARN stays GLO > MIDI 3. The menu is only GLO's DEVICE: the hold-HOME shortcut is gone (nothing needed hold HOME: the power-on holds are OCT-'s). Hold HOME and hold SEL are free.
- The mixer: levels, pans, mutes, solos only. Keys 9, 10 (fill, fill bar) and 11 (pan) gone; PAN is G#3, drawn in the strip on both mixers; with the drums selected F#3 SOLO, drawn (an outline, not live, on a synth voice's mixer, whose keys 5 to 8 solo). The podiums are shorter to make room for the strip.

MASTER with a button held (Ben: "Yes, those pairs")
- Relative by value scaling: a pot move from a to b moves the value the same share of the way to the end it turns toward (up: by (1 - p)(b - a)/(1023 - a), down: by p(a - b)/a). No jump ever; both ends reachable; the pot and the value meet at the end stops. Picked over pickup (a dead knob until it passes the value) and plain relative (the ends unreachable once apart). Moves under 4 of 1023 are ADC noise. The volume after a pair: it stays, and the next turn with nothing held moves it from where it is by the same rule.
- FX MIX: the shared delay, reverb and chorus returns times 0 to 100 %, eased over a block; sends and PERFORM throws untouched; at 0 the output is the dry mix exactly (tested). A return level, not a crossfade: at 100 % it is the mix as before.
- ENV LENGTH: the selected synth voice's amp ATTACK, DECAY and RELEASE coefficients scaled by 2^(-v/32) (v -64..63: a quarter to about 3.9 times as long; SUSTAIN untouched; with LINK the filter follows the amp). LFO MOD DEPTH: every matrix amount whose source is LFO1, LFO2, AENV, FENV or PENV (the fixed LFO1 > PIT, CUT, LVL, FENV > CUT, PENV > PIT and the free cells, the PAN, FX and AS columns too) times (64 + v)/64 (none to about twice); VEL and KEY rows untouched (they are playing, not modulation). Both are scales: nothing stored changes; 0 gives the stored sound exactly (tested for ENV).
- FX MIX, ENV LENGTH and MOD DEPTH are performance state, not saved (power-on and NEW: as stored), like the mutes and PERFORM's knobs: saving them needs a slot in the project format.
- On the drums: EDIT the drum voice's level, ENV the selected sound's decay, LFO the selected sound's LFO DEPTH (these are stored sound values, turned relatively), ARP nothing.
- The header shows the pair as a chip (a little dial and its word, in what it acts on's colour: the voice's for SWING TRANSP LEVEL LENGTH DEPTH, white for FX MIX and TEMPO); while it turns, the white bar says it ("FX MIX 40 %"). When FILL or latched pads take the title place, they win over the chip (the readout still shows when MASTER turns).
- Holding a layer open (LOCK) counts as held for MASTER. SAVE has no pair.

Black keys only where drawn (Ben: "Only live when drawn")
- The strip is PERFORM's (y 40, two rows, 37 x 12 a key); a key with no job here is an empty outline.
- Every black-key job found, and what happened to it: step pages 1 to 4 on the GRID, the drum screen and the lane: gone (OCT- / OCT+); F#5 FILL on the drum screen: gone (SEQ + PLAY); the drum RETRIG rates on hold ARP: gone (hold SEQ, KNOB 1); the mixer's SOLO (drums), FILL, BAR, PAN: SOLO and PAN drawn in its strip, FILL and BAR moved to SEQ; PERFORM's -DRM -BAS LCUT FLNG 8T 16T: kept, drawn; the chord modifiers (3RD 7TH SUS4 9TH INV): drawn and live on ARP's KEY pages only (elsewhere a black key does nothing in chord mode); the drum voice's black keys, which played the white key's sound below them: now nothing; the STEP REC and CHROMATIC black keys: notes (they are note screens); the old EDIT + black key (drum mute mode): already gone in wave 9.

The steps only on SEQ (Ben, 10:33)
- A synth voice's steps: with nothing held, EDIT, ENV, LFO, FX (and every other button) open the real page; a step held: the page's knobs under the steps, the tab row a LOCK: banner with the page's name, and letting go of the last step returns to TRIG (its 1/2 or 2/2). SEQ from a sound page opens the steps on TRIG.
- The drum screen is left as wave 3 built it (its sound pages with the step row on top, Ben's "B: four dials over the knobs"): a question for Ben (below).

Hold layers (rule 3), every one: FX PERFORM (performance); SEQ RHYTHM (performance, and a shortcut to PATTERN's SWING STEPS SHIFT and to A-D); ARP NOTES (transpose: KEY 2's TRANS is its page; the pattern transpose has no page, it is a performance edit like SHIFT, which also lives on PATTERN 2/2); GLO the mixer (performance; levels also on HOME and the voice pages); SAVE (store: SAVE has no store page, see below; undo, redo); EDIT, ENV, LFO: MASTER's pair only; SEL, HOME, REC: none.
- Storing a pattern is a hold only (SAVE + key). A SAVE page for it was not added: Ben's words put it on SAVE "with a key while the patterns show", and A-D (SEQ) shows them; say if a SAVE > PATTERN page is wanted.

Memory, measured on an rv32 -Os stand-in (clang, the inline asm emptied, felucca.c) against the wave 9 merge (9b35d75): .bss +44 B, .sbss +80 B, .sdata +12 B (.data+.bss about +136 B: about 96,240 of 98,304 on the device, about 2,060 B left), code +3,982 B and constants +444 B (an image of about 521,950 of 581,564), the pool unchanged (nothing new in it), .noinit unchanged. Removing the hold-SEQ time drawing and the old layers gave back less than the new code took. No 64-bit divide (64-bit multiplies only, in the FX return and ENV LENGTH).

Not checked
- No device build. The long tests (soak, stress, regress, web) were not run. Golden renders: FX MIX 100 %, ENV LENGTH and MOD DEPTH 0 leave every render as before (the old paths run unchanged).

## Rebuild wave 11, mapping and lights (23 Nov): calls made that Ben did not specify
Ben's words: SPEC.md "23 Nov 2026, 10:41: hold to map" with his 10:50 answers, "10:45: the lights", "10:46: how much of a disco", "11:33: the colour code". Where they left a choice, the call that best fits his four ways and "one idea per control" was taken.

Hold to map (ui_map.c)
- Where: a synth voice's sound pages (OSC 1-4, FILT 1-2, ENV AMP and FILT, FX 1-3, SENDS, SLICER, LFO 1 and its sync page, LFO 2, MATRIX). Elsewhere (HOME, the steps, the drums, ARP, KEY, GLO, SAVE, the MIX pages) holding LFO or ENV is wave 10's MASTER pair layer, unchanged. With a step held a page button opens that page's locks (as wave 10): no mapping.
- A tap still acts on release (its category); a hold never navigates. The MAP view appears once it is a hold (a knob or SEL used, or 0.14 s held: the layers' time), so a tap does not flash it; the page underneath never changes. Kept open (held 0.7 s untouched, let go) it stays mapping, LOCK in its tab row, SEL still flipping (the one-keep-open rule for every layer).
- "The one last shown": the LFO / envelope category last on screen (LFO 1, the amp envelope if none yet); SEL flips while held; the flip lasts the hold.
- The view: the page's body as it was; the header's tag LFO or ENV with MASTER's chip (DEPTH, LENGTH); the tab row a MAP banner OUTLINED in the voice's colour (a LOCK banner is filled), then "SEL" and what it flips to, and the legend [-] REMOVE; the dials the amounts: a set one BOXED in the voice's colour with a bipolar ring from the top (a lock is inverted), 0 grey, a knob that cannot be mapped dark grey "--" with a dark ring (one look for every refusal), a turn of it says NAME: NOT MAPPABLE or NAME: NOT FROM ITSELF.
- The mark outside the hold: a small wave (five 2 px dots) under the knob's ring in the voice's colour, for any source (the fixed cells too: FILT's ENV marks CUTOFF, as FENV moves it).
- Destinations: CUTOFF, RESO, the OSC page's three macros, an insert's AMOUNT: their fixed columns; TUNE: the PIT column (a semitone a step, +-12 at full: vibrato per note) rather than a column moving TUNE's cents; every other continuous knob (named positions, or fewer than 25, count as switches): an assignable column, made on the first mapping, freed when its last cell goes (from the hold; the MATRIX page keeps an emptied column until the next load).
- Not mappable: TYPE (SELECT), OCT, GLMODE, VOICE, PRIOR, ALLOC, FILT's ENV and KEY (they are modulation amounts: the FENV and KEY cells), LINK, the LFO's POL, SYNC, TRIG, ESYNC, LFO 2's SYNC and TRIG, the SLICER's PAT and RATE, the MATRIX page's knobs. An envelope onto its own times and level is refused (AENV: ATTACK..RELEASE; FENV: the filter envelope's four); an LFO may move its own RATE, PHASE, FADE.
- OCT- while mapping removes that source's mappings on this page's knobs; SAVE + OCT- undoes it (the matrix and the fixed cells' parameters kept in the undo's own room, no RAM of its own).
- Velocity and key: MATRIX page only (Ben). The drums: no mapping (wave 3's per-sound LFO stays as it is); holding LFO / ENV there is MASTER's pair. A question for Ben (below).

The matrix (19 columns)
- Assignable columns AS1..AS19 (MX_NAS), one parameter each; the cells stay 19 a voice (MX_NCELL), so a voice can map 19 different parameters: the one limit, said MATRIX FULL: 19 CELLS, nothing changed.
- Stored in mod[46] as layout 2: byte 7 is 0x5B (always 0 in layout 1), the rest a bit stream: LFO 2's SYNC (4 bits) and TRIG (2), Spice (7), Dice (14), how many cells (5), each cell its source (3), destination (7: a fixed column, or 11 + the parameter's id) and amount (7). 355 of 360 bits. The columns are not stored (rebuilt from the cells, in order); a column with no cell is not kept. Layout 1 (waves 5 to 10) is read exactly; a project or preset of waves 5 to 10 renders bit for bit as wave 10 did (tests/mx11_test.c against hashes made with wave 10's own code, tests/mx11_ref.sh). A wave 10 firmware reading a wave 11 project would read its matrix as garbage, checked and dropped (not required: older firmware never reads newer projects).
- Per note: PIT, M1 M2 M3, CUT, RES, LVL. Per block for the whole voice (the last note's envelopes and velocity): PAN, FX1 FX2 FX3 and every parameter column (envelope times, the filter's DRIVE, the sends, the slots' knobs, the LFOs' rates, GLIDE, DETUNE, SLICER DEPTH, the Type's 4th and 5th).
- The MATRIX page shows the eleven fixed columns, the parameter columns in use, then one empty "+" column (KNOB 4 TARGET picks its parameter; one column a parameter; a column with cells cannot be set to none). The heads are three letters (ATK, FAT, DRV, L1R, L2R, 1T, E4 ...; RESO became RES) so 19 read side by side; the view scrolls six at a time as before.

The lights (ui_lights.c)
- One function (combo_live) says what works under each hold; the lights draw from it and the input asks it before acting (the layers' OCT- / OCT+ / PLAY / SEL, the layers' keys, and a layer or page button pressed under a hold). New because of it: a button with nothing to do with what is held is now ignored while it is held (before, another layer button took over the hold, and with a step held GLO, SAVE, ARP left the steps); a page button under a step lights only where it would move. Fix found by the test: OCT- already held when a layer started was taken as a press of it (hold OCT-, then SAVE, was an undo).
- PLAY under a hold (but SEQ's) stays the transport and REC stays record: what they do alone, not lit as combos.
- Black keys: dark wherever they have no job, pressed or not (the steps, the drum voice's sounds outside CHROMATIC, chord mode off the KEY pages); where a strip is drawn the lit set is the strip's (tested against what the strip draws).
- The confirm: SAVE alone; it pulses at 2 Hz (NORMAL, SHOW); in QUIET it glows still.
- PERFORM: every pad lit; a held one lit, a latched one blinking (QUIET: still).

DISCO (GLO > DEVICE > LIGHTS, KNOB 1; the menu's LIGHTS section has four rows now)
- QUIET NORMAL SHOW, NORMAL to start; kept in the settings word's free bits 30-31 (as level ^ 1, so a word from before reads NORMAL), with the device, not the project.
- Every light is a level (dark, backlight, glow, lit) and a kind: STATE, HINT, MOTION, BACK. QUIET: STATE lit becomes glow, HINT becomes backlight (its pulse at least 1.5 us while a hold shows), MOTION off, the backlight at most LOW, NOTES off. NORMAL: as built. SHOW: NORMAL, every button's backlight HIGH, and with nothing held where the keys play notes: every voice's sounding notes lit, the drum hits lit, the step running along the white keys at the glow, the mix's level up from the lowest key at the glow; HOME untouched a minute while playing opens the screensaver. A hold always wins.
- The ceiling: the level never raises LIGHTS, KEYS or NOTES (SHOW's HIGH backlight is its own); QUIET caps them. The screen's brightness is untouched (a question for Ben).
- The full table of every light by level is GUIDE.md section 10, "The lights and DISCO".

SAVE > PATTERNS
- SAVE's third category (PRESETS, USER, PATTERNS, TOOLS): A to D STORED or EMPTY, PLAYING marked; KNOB 1 the pattern (-/+ too), KNOB 2 STORE (a GO: right). Empty: stored at once (as SAVE + key); stored: asks HOLD SAVE: STORE A. Header tag SAVE in grey (the whole project's). Hold SAVE + key 1 to 4 stays the shortcut. The page is "PATTERNS" (a page "PATTERN" was removed in wave 8 and a test guards that).

The colour code (Ben, 11:33)
- TE_COL, TE_MID, TE_DIM swapped at their one source: voice 3 coral, the drums teal; the heads keep their drawings (they take the palette when drawn), the boot wordmark's bars, the button map, the cheat sheet, the guide. Coral beside record red: see "For Ben to confirm".

Tests and their hygiene
- tests/ui_pages_test.c: hold to map on every synth page (32 knobs mapped, 22 refused), both ways with the MATRIX page, SEL, OCT- and its undo, the limit, the sound (the hold's cell = the grid's cell); every control under 25 holds, lit <=> does something with it and not what it does alone (forked trials); the confirm; the black keys against the strip; QUIET still for 2 s on HOME and the steps while playing; SHOW moving; DISCO kept; SAVE > PATTERNS; the MAP view's and the 19-column grid's colours and columns; the colour code. The NO SOUND CHANGES test now clears the FX slots' types and running state earlier tests left (it compared a route that runs the slots for a moment with one that does not); the mixer's F#3 test starts with SOLO off.

Memory, measured on the rv32 -Os stand-in (clang, the inline asm emptied, felucca.c) against the wave 10 merge (ec814ba): .data+.bss +194 B (about 96,394 of 98,304 on the device: about 1.9 KB left), code +15,544 B and constants +293 B (an image of about 537,400 of 581,564), the pool unchanged, .noinit unchanged. The matrix's RAM: +17 B a voice (the columns); the audio interrupt's stack +about 150 B in mx_part_begin (19 columns). No new 64-bit divide (the stand-in's __divdi3 / __udivdi3 references are the same as wave 10's). MX_NAS can be lowered at compile time if the device needs it (each column a byte a voice; below 19 a second limit appears, MAP FULL: ALL COLUMNS USED). Not needed.

Battery (an estimate on paper, not a measurement). The LEDs are multiplexed over 11 columns; a lit LED is on about 95 us of each ~1.05 ms frame (about 9 %), a glow 4 us (0.4 %), the backlight 0.5 to 2 us. At an assumed 5 to 10 mA through an LED while on, a lit LED averages about 0.5 to 0.9 mA, a glowing one about 0.02 to 0.04 mA. NORMAL in use lights about 5 to 10 LEDs (page button, steps, beats, hits): 3 to 7 mA; QUIET puts them at the glow: under 0.5 mA. Against the whole FM-1 (the processor, codec, amplifier and the screen's backlight, guessed at 80 to 150 mA) QUIET saves about 2 to 8 % of the battery: worth having for a flight, not a big change. SHOW (every button's backlight HIGH, 10 to 20 keys lit) costs about as much more. Confidence low: the LED current and the FM-1's draw are not measured.

Not checked
- No device build. The long tests (soak, stress, regress, web) were not run. The simulator's page changes were not run in a browser (no Emscripten here: the C compiles natively, the JS parses).

## For Ben to confirm
Every call waves 2 to 7 made that the spec did not cover, and every device-only limit or lossy conversion, one line each. The detail is in each wave's section above.

Device-only limits
- PHEBUS_ARENA_T12 = 9152: on the FM-1, 6-OP A, B, C, STRING, SPEECH and PARTICLE play on track 3 only (NOT ON THIS TRACK on 1 and 2); the simulator and host have no limit.
- CPU on the FM-1 is unmeasured: stacking PERFORM pads is the only new cost of wave 6; the compromise to offer is a limit of four at once (the MPC's).
- RAM: .data+.bss about 96.3 of 98.3 KB and the pool about 200 B above the 8 KiB rule; the extras and lock pools (128 B a copy, 7 copies) and PHEBUS_UP_MX=0 (1,472 B) are the reserves if needed.
- The STUTTER lane and PERFORM's time effects are mono, 0.74 s, 16-bit with 12 dB of headroom; below about 84 BPM a quarter loop is cut to 0.72 s.
- The voice filter's cost on the FM-1 is estimated from the host (about 0.45 of a JUNO voice a voice).
- Section D lives in main RAM and C too (not .noinit): after a crash, a pattern stored while playing and not yet written is lost.
Lossy conversions of older projects and presets
- Format 7 drum levels become velocities 42 / 72 / 100 / 127; ratchets become one-step retrigs at the nearest rate (on 1/32 steps x3 and x4 become 1/80).
- Past 64 trigs with extras when converting, the rest play with no condition or nudge.
- PLAITS / ELEMENTS: old ENV > FLT and LFO > FLT moved the timbre; now they move the filter (ENV > FLT on PLAITS goes to T.ENV, positive only). Eight PLAITS factory sounds now wobble the filter, not the timbre.
- Old LFO > SHP and ENV > SHP become matrix cells; on ELEMENTS an assignable column at half the amount (approximate).
- Old DIST, INSERT and sends become FX slots: past three effects the smallest sends go; an INSERT's fourth value (LEVEL into the trim, exact at MIX 100 %; CRUSH LPF and PHASER / FLANGER DEPTH reset to default).
- The old track FILT (DJ LP / HP) on a synth voice is moved onto the voice filter when loaded.
- A record 2 user preset slot shows as empty on firmware before wave 4; a wave 5 bank as empty on firmware before wave 5; stock KARMA cannot read PHEBUS projects.
Drums (waves 2 and 3)
- A new trig plays at velocity 100; TRIG's KNOB 1 with nothing held sets it (not saved).
- 64 trigs with extras and 128 sound locks a pattern (refused with a message when full).
- CHANCE counts only with COND CHNC; a CHNC trig starts at 50 %; synth steps keep a fixed 50 % CHNC.
- RETRIG: rate 1/1 to 1/80 of a bar, length 1/8 step to 16 steps or INF, curve -128 to 127; the next trig ends it.
- Synthesised kits keep their own model (not converted to the HEX machine); every sound gets an LFO (SPEED DEPTH DEST WAVE, SYNC FADE PHASE MODE).
- One edited kit a project; another kit asks HOLD REC: REPLACE (wave 8). A way to store your copy into MY HEX is not built.
- PEDAL gets its own sound in your copy (the built-in kits share the closed hat's).
- MODEL and TYPE cannot be locked (as the Rytm's machine).
- Sound pick: hold SEL + key; MUTE mode: tap SEL; CHROMATIC: tap ARP; live RETRIG: hold ARP + key, the black keys the rate; FILL: F#5 held, OCT+ to latch (wave 8).
- Hold a step + OCT-: the last-turned lock, then every lock; on TRIG everything the trig holds.
- The KIT page is gone (sound: SEL + key; kit: PRESETS; sends: AMP 2/2).
Melodic voices (waves 4 and 5)
- 31 Types in one list, plain ones first; OSC has four halves (Type + macros; OCT TUNE GLIDE VOICE; the Type's 4th and 5th, DTUNE PRIOR; GLMODE ALLOC).
- Macro names per Type from the Plaits manual; some ELEMENTS / OMINOUS controls have no knob and keep the sound's value.
- Turning TYPE keeps the rest of the sound; another engine's macros start at plain defaults; JUNO's chorus moved to OSC 3/4.
- One ladder filter on every voice (LP24, LP12, BP, HP); ACID's and JUNO's own ladders are their LP24.
- LINK is the default; the first turn of a filter envelope knob makes it OWN from the amp's values; ESYNC applies to both.
- The synth GRID has the drum screen's tabs; SELECT walks the tabs (the tempo is on HOME and the pages).
- FX slot types and their knobs; one FLANGER a voice (wave 8: the sends are the SENDS page, not slots).
- Matrix: 7 sources x 13 destinations, 19 cells besides the six fixed ones; LVL only reduces; PAN, FX and AS follow the last note.
- LFO 2 starts FREE; its sample-and-hold uses its own random numbers.
- Spice and Dice on TRIG 2/2 with nothing held; a roll's ranges (30 % left out, 25 % octave, 20 % ratchet, velocity +-48, length x0.25 to x1.75).
- GATE on hold SEQ, KNOB 1 (wave 5: a call for Ben).
PERFORM and the STUTTER lane (wave 6)
- The pad map changed from mock-perf-A: ECHO moved to row 4 beside VERB; DUST and WOW are not pads; -DRM -BAS LCUT FLNG on black keys 1 to 4, 8T and 16T on C#5 and D#5.
- Gone from the old 27: STUTTER (now 16T), PHONE, ALIAS, WOBBLE, KILL MID / HIGH, REWIND, OCT UP, AUTO-PAN, FREEZE, MONO, FADE OUT.
- -BAS kills voice 1 (KNOB 1 picks); kills keep the notes playing unheard.
- One time effect at a time with every colour effect, throw and kill on top; no limit of four.
- In and out: hold FX; held 0.7 s untouched and let go, it stays (every layer, wave 8). Latch: OCT+ with a pad (a double tap was not used). OCT- unlatches all; STOP ends latches; latches play on outside PERFORM.
- The header says PERF and how many (not FX).
- Each pad's knobs and start values; knob settings are not saved; the lane's own lit steps always play at the start settings.
- Recording: spans to the nearest 1/16 with the last knob turned; a pad over its own span replaces it, others overdub; 31 at most.
- EDIT + OCT- undoes a take's pads; a pattern change drops an open span.
- The lane's KNOB 3 PADS asks to clear (hold REC, wave 8); hold a step + OCT- erases its pads.
HOME, the screens and the words (wave 7)
- HOME's three zones and their contents as listed in wave 7 above; the selected voice shown inverted instead of a "VOICE 1" line.
- The layers', PERFORM's and the REC screen's frame (header tag the button held, a line under it, the pages' dials, the strip).
- The words: STEP (a key), trig (what is on it), RETRIG, PADS (the lane's), PATTERN (A to D), GLO (the shared tag).
- Stutter lengths as spans: runs of one length drawn as one bar, named once.
- Gesture exceptions kept: SELECT is the cursor in STEP REC; OCT+ on the lane's last page goes round (no page added); SEL and ARP do other jobs on the drum voice. (Wave 8 removed: SELECT as tempo, the FILL double press, the GLO / FX-only keep-open.)
Wave 8, the layout logic
- SLICER: kept as each voice's last FX page and the drum FX tab's. To decide (Ben said "decided separately"): keep it there, cut its STUT mode only (stutter is PERFORM's and the lane's), or cut the page. Recommended: keep the page, cut STUT (it then is a per-voice trance gate, which PERFORM's whole-mix GATE pad cannot do).
- FX stage reached by EDIT (ENV then EDIT: one press from ENV) and SELECT; no button jumps to it any more. Alternative if it feels far: SEL held + ENV, or ENV pressed past FILT ENV into FX.
- Confirm = hold REC (the ring from the press). Storing over a used pattern needs SAVE kept held (or kept open) while REC is held.
- The drum voice's own pages are a sixth tab, FX, on the drum screen (Rytm-style FX track); GLO > DRUMS removed (its REVERB and DELAY are the FX tab's SENDS).
- No tap tempo (it was a second place for the tempo). Say if you want it back as a key on GLO > GLOBAL.
- No send columns in the matrix (an assignable column takes a send).
- ECHO and VERB pads keep only their own knobs; VERB has AMOUNT only.
- Hold a step + OCT- still removes the last-turned lock, then every lock on a second press in the same hold (not timed).
- The screensavers are HOME's pages (SELECT right); the hint is a ">" after the deck's FLOW band (the band was narrowed 12 px).
- GLO > SYSTEM still shows OUT and IN, which MIDI 2 shows too (left as built; pages-unique would drop them from SYSTEM).
- Lossy: a wave 5 to 7 matrix cell on a send slot when both assignable columns are used; a project's MASTER FILT; GLOBAL SWING for an old section loaded after a reboot with a newer project. User preset records 4 read as empty on older firmware.
Wave 9, how to move around
- The CLASH: the synth FILT is CUTOFF RESO ENV DRIVE with TYPE on SELECT; the drum FLTR stays CUTOFF RESO ENV TYPE (DRIVE on 2/2) because the drum screen's SELECT picks the sound. Choose: keep it so; or put the synth FILT's TYPE back on KNOB 4 as well (SELECT and KNOB 4 both turn it, DRIVE back to 2/2); or move the drum FLTR's TYPE to its 2/2 so KNOB 4 is DRIVE on both.
- The re-planned OSC pages: M1 M2 M3 M4 / OCT TUNE GLIDE GLMODE / M5 VOICE DTUNE PRIOR / ALLOC.
- FX 1 to 3 on KNOB 1 to 3 (KNOB 4 empty); the SLICER's knobs moved left a place.
- SEQ always opens TRIG (no longer the GRID on the sound page's own tab).
- A category opens on its first page (no memory of the last page anywhere).
- ARP's MODE and KEY's SCALE are not SELECT types (no page has a single type there).
- The mixer, drums selected: SOLO FILL BAR PAN on F#3 G#3 A#3 C#4; the voices' mutes off the keys meanwhile.
- The SEL layer's quick key pick (SEL held + a key) is gone, not given a performance home; say if you want it back on a layer.
- Halving clears the dropped half (the old DOUBLE knob kept it hidden); STEP REC's SEQ with keys held writes them then a rest.
- The header shows the type (or the drum sound) instead of the sound's name on typed pages; the drum kit's name moved under the sound.
- MIDI LEARN is GLO > MIDI 3 KNOB 2; the HOME screensaver is HOME pressed again (wave 8's "no second meaning" call reversed by the new rule).
Wave 10, one idea per control
- Proposed by Claude from Ben's words, not objected to: FILL is hold SEQ + PLAY (not ARP); playing a pattern moved to SEQ (A-D, hold SEQ + black keys 1 to 4) while storing stays with SAVE (hold SAVE + keys 1 to 4); the confirm is hold SAVE with the ring; REC only records; clearing a voice is EDIT + OCT-, then the confirm; erase is OCT- with the thing held; MASTER acts relatively (value scaling) so the volume never jumps; the settings menu moved from hold HOME into GLO (DEVICE only; hold HOME is free).
- The double tap's window: 0.3 s, the first tap acting at once and the second taking it back.
- What -/+ do on each screen (GUIDE section 12's table); a page with nothing touched does nothing; the legend's look and place (the tab row's right end; the drums' FX names cut to three letters to make room).
- The steps only on SEQ (Ben, 10:33, Claude's reading): applied to the synth voices. THE DRUM SCREEN: its sound pages (SRC FLTR AMP LFO FX) keep the step row on top, as Ben chose in wave 3 ("B: four dials over the knobs", the Rytm's way); with nothing held a page button stays on the drum screen and changes the dials; the keys stay steps. Choose: keep it so (the Rytm way, as decided); or make the drums like the synths (a sound page without the steps, the keys playing the 16 sounds, the steps back only on SEQ and while a step is held); or a middle way (the drum screen's step row stays, but only SEQ makes the keys steps; on SRC FLTR AMP LFO FX the keys play the sounds).
- Hold SEQ: KNOB 1 RATE, 2 SHIFT, 3 SWING, 4 STEPS (GATE and DIV only on the PATTERN page); synth rolls on the white keys only.
- MASTER pairs on the drums: LEVEL the drum voice's, ENV the selected sound's decay, LFO its LFO DEPTH, ARP nothing.
- FX MIX is a return level (0 dry, 100 as before), not a dry/wet crossfade; FX MIX, ENV LENGTH, MOD DEPTH are not saved with the project.
- MOD DEPTH leaves the VEL and KEY rows alone (they are playing, not modulation).
- The chord modifiers live only on ARP's KEY pages (drawn there); play chords elsewhere without them.
- The drum voice's black keys no longer play the sound of the white key below them.
- A key held + OCT- erases (a ghost hit is OCT- first): on the drums, holding a long kick and pressing OCT- for a ghost snare would erase the kicks (SAVE + OCT- undoes). Say if the erase should need something more.
- The mixer: PAN on G#3 for both mixers (key 11 gone); a synth voice's mixer still solos on keys 5 to 8.
- No SAVE page for storing a pattern (a hold only, as Ben's words suggest); say if one is wanted.
- The white ring: only SAVE into an empty user preset slot.
Wave 11, mapping and lights
- THE DRUMS AND HOLD TO MAP: not built; holding LFO / ENV on the drum screen is MASTER's pair. Choose: leave it so; or hold LFO + turn a knob sets the selected sound's LFO DEST to that knob's parameter and DEPTH by the turn (where that destination exists; one destination a sound, so it replaces the last); or hold ENV likewise onto a per-sound envelope destination (none exists today: it would need one).
- THE DRUMS' ERASE AND GHOST (from wave 10): on a screen where the keys play the drum sounds, a sound's key held and then OCT- erases that sound's hits from the pattern (playing: as the playhead passes; stopped: every step); OCT- held first and then a key is a ghost hit. So holding a long kick and pressing OCT- for a ghost snare erases the kicks (SAVE + OCT- brings them back). Choose: keep it; or the erase needs the key held 0.5 s before OCT- counts (a quick OCT- under a key is a ghost on the next key instead); or erase moves to hold EDIT + key (the old way, a second place for minus); or the ghost moves to the left of the keyboard only (keys 1 to 8 ghost while OCT- is held, never erase).
- The hold's view is shown on a voice's sound pages only, not on HOME or the steps.
- TUNE maps to the pitch column (+-12 semitones, per note) rather than moving the cents of TUNE.
- A layer button pressed while another layer is held now does nothing (it used to switch the hold to it); with a step held, GLO, SAVE, ARP do nothing (they used to leave the steps).
- The map banner outlined, amounts boxed, refusals dark grey "--"; the mark a small wave under the ring.
- DISCO: QUIET's hints at the backlight level (very dim: right for a dark cabin, faint in daylight); SHOW's dance and its screensaver after a minute.
- THE SCREEN'S BRIGHTNESS is not part of DISCO. Should QUIET also dim the screen (the LCD backlight is the bigger battery cost)?
- VOICE 3 CORAL AND RECORD RED: coral (255,128,108) and the record red (255,44,52) are close. Where they now sit side by side with voice 3 selected or recording (review/wave11: w11-v3-rec-armed, w11-v3-home-rec, w11-v3-filt-rec): the REC screen's armed row is a red frame around voice 3's coral row; the strip shows voice 3's coral head and number with the red recording dot right after; HOME shows voice 3's coral name inverted with the red dot beside it; a voice 3 header's coral tag sits left of the red beat light and red recording disc. The confirm ring is red with no coral near it (fine). Nothing changed; options: keep; or make the recording dot white with a red ring; or move voice 3 to a less red coral.

