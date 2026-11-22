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
