# PHEBUS review, 21 November 2026

**What was reviewed:** PHEBUS as it stands on branch `ben` (the firmware, its simulator, the editor and docs), against SPEC.md.
**Depth:** full overnight pass. Ten journeys walked in a test harness, every screen rendered and judged, 29 test suites run, plus research into the best-regarded grooveboxes.
**Overall state:** the bones are good and it already reads as one instrument. The new step GRID is the weakest part: it is silent, incomplete and has three real bugs. Nothing has run on a real FM-1.
**Decisions waiting:** 22, most valuable first. Pictures are in `review/shots/`. The full lens reports and the research are in `review/`.

## Ben's decisions (21 Nov, by picker)

| # | Finding | Decision |
|---|---|---|
| 1 | Silent synth GRID | Play on change |
| 2 | Three step editors | GRID only |
| 3 | Filter locks | EDIT walks the chain |
| 4 | Length and pages | Page slots, view follows the playhead |
| 5 | Drum GRID one lane | SELECT toggles between the snake view and a full grid |
| 6 | Saving | Unsaved dot, save on stop |
| 7 | Undo | Full undo, shown on screen |
| 8 | Sound pages | Sound big, character small |
| 9 | Chain length | Merge one-knob pages, LFO SYNC, global effects to GLO |
| 10 | Song screen | Insert and delete |
| 11 | Four sequencer bugs | Fix all four |
| 12 | Boot | Keep 3 s at full detail (the broken test still gets fixed) |
| 13 | Strike pose | Fix |
| 14 | Knob numbers | A: one bold face |
| 15 | Bottom bar | A: heads with underline; mute and solo as badges |
| 16 | Colour rule leftovers | Fix all |
| 17 | Colour-blind | Skip |
| 18 | Headers | Fix |
| 19 | Small legibility fixes | Do all |
| 20 | Screensavers | Drop all 11, build only the new pieces |
| 21 | Words | Fix all |
| 22 | Docs and site | Rewrite guide and cheat sheet, remove the analytics counter. Power-on kit: 606 |

Status, 21 Nov midday: built, merged and live in the simulator: 1 to 7, 11 to 16, 18 to 21, the 606 kit, the analytics removal, plus (asked for on 21 Nov) effects off by default and the stutter sequencer. Being built now: 8 (sound pages draw the sound), 9 (page names in the strip), 10 (song insert and delete by holding an entry's key), held SEQ without step keys, plain sawtooth starts with JUNO on track 2. Then: the guide and cheat sheet. 17 skipped at Ben's choice.

---

Two facts that shape everything below:

- **Space is nearly gone.** The firmware is 553,240 bytes of a 581,564 limit, so about 28 KB is left. The fierce characters used 12.7 KB of it. The working-memory pool has 16 bytes spare before the build fails. New screensavers must be code, not stored pictures, and must not need a buffer.
- **Today's work changed no sound.** All 117 reference renders are identical before and after.

Sizes: small is an hour or less, medium about a day, large several days or a design change.

---

## Using it

### 1. The synth GRID is silent
- **Where:** GRID on tracks 1 to 3 (`ui_grid.c:91`).
- **Wrong:** placing a step, turning the note knob and changing a held step's note all make no sound, and the keys are steps. You have to leave the GRID to hear a pitch. Your brief says "Choose a note, and place it into a step".
- **Fix:** play a short note whenever the note changes or a step is placed.
- **Alternative:** while a step is held, the black keys play and set its note.
- **Size:** medium.

### 2. Three step editors for the same 16 steps
- **Where:** GRID (2 rows of 8), the old STEP page (2 rows of 8, first two knobs swapped), the SEQ layer (4 by 4). Picture: `seq-layer-4x4-vs-grid-2x8.png`.
- **Wrong:** three layouts and knob orders. Only the old STEP page lets you hear notes. A second SEQ press means STEP on synths and KIT on drums. From a sound page, SEQ drops you on the old STEP page (a dead "DRUM TRACK" page on drums), not back on the GRID.
- **Fix:** the GRID becomes the only step editor. Remove the old STEP pages, move their heard note entry into the GRID, keep the SEQ layer for division, swing and length, and make SEQ always return to the GRID.
- **Alternative:** keep all three but give them one layout and knob order.
- **Size:** large.

### 3. The filter cannot be locked to a step
- **Where:** GRID with a step held (`ui_input.c:672`, `ui_grid.c:196`).
- **Wrong:** holding a step and pressing EDIT only offers OSC and VOICE pages. Cutoff and resonance are unreachable except by a hidden six-press route, and the screen never names the parameter being locked. Filter locks are the heart of acid lines.
- **Fix:** with a step held, EDIT walks OSC, FILT, ENV as it does everywhere else, and the title names the parameter.
- **Alternative:** with a step held, SELECT walks every lockable page.
- **Size:** small.

### 4. Pattern length and pages are missing from the GRID
- **Where:** GRID (`ui_grid.c:155`). Picture: `grid-synth-oct-does-nothing-at-16.png`.
- **Wrong:** at 16 steps, the page keys silently do nothing. Length lives elsewhere under three names. The GRID also does not follow the playhead onto later pages.
- **Fix:** show four page slots with unused ones dim. Pressing past the end adds a page (a copy of the last) and says "32 STEPS". The view follows the playhead while playing.
- **Alternative:** put length on knob 4 when no step is held.
- **Size:** small.

### 5. The drum GRID shows one sound at a time
- **Where:** drum GRID. Picture: `drum-grid-one-lane-only.png`.
- **Wrong:** you see one lane and a tiny dot thumbnail, with the snake taking most of the screen. Kick, snare and hat cannot be seen together, and a three-part beat takes about 21 presses.
- **Fix:** a readable multi-lane grid of the sounds in use, with the snake reduced to a small figure.
- **Alternative:** SELECT toggles between the snake view and a full grid.
- **Size:** medium.

### 6. Nothing autosaves, and nothing shows unsaved work
- **Where:** everywhere (`project.c:767`).
- **Wrong:** no save happens while playing and no screen shows whether work is saved. "Save" means five different things. The research ranks lost work as the most resented groovebox failing.
- **Fix:** an unsaved dot in the header and an autosave whenever you stop.
- **Alternative:** autosave between bars while playing (needs a timing check on the real device).
- **Size:** medium.

### 7. Undo forgets parameter locks, and is two hidden buttons
- **Where:** EDIT + OCT- (verified in a harness).
- **Wrong:** undo after clearing a locked step restores the note but not its locks, nudge or condition. The GRID makes clearing a step one tap, so this will bite.
- **Fix:** undo restores the whole step. Show "UNDO" on screen while EDIT is held.
- **Alternative:** leave undo as it is and ask for a second tap to clear a step that has locks.
- **Size:** medium.

### 8. Sound pages show the character, not the sound
- **Where:** OSC, VOICE, FILTER, FX, INSERT and LFO destination pages. Picture: `filter-page-shows-robot-not-filter.png`.
- **Wrong:** half the screen is the character. There is no filter curve and no waveform, so turning a knob shows a number change and nothing else. Only ENV and LFO draw their shape. This is the gap between PHEBUS and the MiniFreak flow you love.
- **Fix:** draw what the knobs do (filter curve, wave shape, send levels) with the character small beside it.
- **Alternative:** keep the big character and add a thin live graph under it.
- **Note:** this one is partly taste. You asked for the characters, so it is your call how much room they keep.
- **Size:** large.

### 9. The sound chain is 18 pages long
- **Where:** EDIT chain on ACID.
- **Wrong:** FILTER 2 and INSERT 2 hold one knob each, "LFO 2" is really LFO 1's sync page, and four global effect pages sit inside each voice's chain.
- **Fix:** merge the one-knob pages, rename LFO 2 to LFO SYNC, and move global effects to the GLO button.
- **Alternative:** keep the pages and show their names in the strip in place of dots.
- **Size:** medium.

### 10. The song screen cannot insert or delete
- **Where:** SONG (`ui_song.c:274`). Picture: `song-screen-hints.png`.
- **Wrong:** entries can only be added or removed at the end, edits are refused while playing, and the song/loop switch (OCT-) is not in the on-screen hints.
- **Fix:** hold an entry and press OCT+ to insert or OCT- to delete, and add the hint.
- **Alternative:** rebuild the song screen as rows of section, repeats and mutes, which is what the research says people want. Large.
- **Size:** medium.

---

## Engine

### 11. Four real bugs in the new sequencer
All verified with test harnesses. One decision: fix them all.
- **Memory overflow:** a held step with several locks writes 49 characters into a 31-character buffer (`ui_grid.c:542`). On the device this silently corrupts memory.
- **Wrong step deleted:** hold a step, leave the GRID, change track, let go: the same step on the new track is deleted.
- **Tie tap:** tapping a tied step creates a note that steals the previous note's tail.
- **Peek counts as a tap:** holding a layer button just long enough to see it, then letting go, also fires the button's tap action. Reproduced on SEQ, EDIT, ARP and FX.
- **Size:** small each.

### 12. The boot screen is heavy and has got longer
- **Where:** boot (`ui_vis.c`).
- **Wrong:** each boot frame costs about 13 times an ordinary visualiser frame. The boot is timed in frames, not seconds, so on the real device 3 seconds may stretch. I lengthened it from 2 seconds to 3 last night, which also broke one test that waits for boot to finish.
- **Fix:** time the boot by the clock at 2 seconds, draw the pattern in 2 by 2 blocks under the smooth letters, and fix the test.
- **Alternative:** keep full resolution and 3 seconds, and judge it on the device.
- **Size:** small.

### 13. The strike pose misses some notes
- **Where:** character STRIKE.
- **Wrong:** it never fires in UNISON mode or on legato accent-and-slide notes, because of how those notes are started.
- **Fix:** trigger from the sequencer's accent flag as well as from loud notes.
- **Size:** small.

---

## Visual design

### 14. Knob values use three typefaces on four baselines
- **Where:** every knob row. Pictures: `F01_value_type_now.png`, `F01_value_type_options.png`.
- **Principle:** typography.
- **Wrong:** values switch between segment digits, a large face and a small face depending on length, so neighbours sit at different sizes and heights. The digit 1 is a bare bar, so 11 reads as "||".
- **Fix (A):** one bold face for everything on one baseline. Most legible.
- **Alternative (B):** keep the segment digits, on one baseline, with a proper 1.
- **Size:** medium.

### 15. The bottom bar reads as page tabs, and hides mute and solo
- **Where:** "1 2 3 DRUM" bar on every screen. Pictures: `F07_trackbar_options.png`, `F03_mute_solo_options.png`.
- **Principle:** component meaning, colour.
- **Wrong:** four boxes with one filled is the tab pattern, which is why you read it as pages. Mute only dims the tile, with no yellow. Solo is a tiny square in a blue almost identical to the alien's.
- **Fix (A):** a slim status strip of the four character heads with numbers, an underline on the selected track, a yellow M badge and an ice-blue S badge.
- **Alternative (B):** keep four boxes but put each head inside, with the same badges.
- **Size:** small.

### 16. Your colour rule is still broken in a few places
One decision: fix them all. Picture: `F02_voice_colour_mock.png`.
- Menus colour their three knobs purple, blue and teal.
- GLOBAL, MASTER, MIDI and SYSTEM pages, and the global delay and reverb pages, carry a purple tag and the purple robot.
- The PUNCH layer has a coral title and voice-coloured stripes.
- The downbeat light is red while only playing; red should mean record or erase.
- A touched value turns white on a voice's page, which makes it look shared.
- A per-voice SEND is drawn white.
- The web editor's knob columns and the cheat sheet's colour legend still show the old scheme.
- **Size:** small each.

### 17. Purple and blue merge for colour-blind eyes
- **Where:** everywhere. Picture: `F04_colour_blind.png`.
- **Principle:** colour.
- **Wrong:** with the commonest colour blindness the robot and alien are nearly identical, the crocodile turns grey, and the snake, mute yellow and record red all go olive. A few elements rely on colour alone.
- **Fix:** add track numbers to the colour-only elements. No change to your palette.
- **Alternative:** also shift the purple slightly deeper and the blue slightly lighter so they separate. This changes colours you approved, so it needs your eye.
- **Size:** small.

### 18. Sound page headers repeat the engine and bury the preset
- **Where:** every sound page. Picture: `F06_header_redundancy.png`.
- **Principle:** hierarchy.
- **Wrong:** the tag says ACID and the next line says "ACID TB ACID". The preset name, the thing you change most, is the dimmest text. The FILTER page says "SYN1" where the others name the engine.
- **Fix:** engine once in the tag, preset alone in white underneath, same on every page.
- **Size:** small.

### 19. Small legibility and tidiness fixes
One decision: fix them all. Pictures: `F09_grid.png`, `F11_low_contrast.png`, `F15_envelope.png`, `F05_character_clipping.png`, `F16_characters_small_sizes.png`.
- GRID note names touch their tile edges; show the note without its octave and mark the octave with ticks.
- A held step lays out one way on synths and the other way on drums.
- Several things are drawn too dim to read at 3 cm: muted tiles, unlocked lock values, HOME step dots, the one-pixel value bars.
- The envelope curve is one pixel and tangles with the ghost character; make it two and dim the ghost.
- Robot, crocodile and snake poses touch the edge of their frame and are cut flat; the strip trims their feet.
- At small sizes the alien is faint, the crocodile is speckled and the snake's head reads as a duck.
- The SONG screen leaves two rows of stray pixels under its header.
- PUNCH tiles are uneven and their labels have no padding.
- HOME shows "LEVEL 0" for a muted track whose level is really 81.
- **Size:** small each, medium together.

### 20. Screensavers: 11 exist, 4 are worth keeping
- **Where:** HOME pressed on HOME. Picture: `F13_visualisers_keep_drop.png`.
- **Wrong:** there are 11 visualisers, not the 7 I told you. Seven are now grey and dull. Four already do what you asked, with each voice in its own colour: ORBIT, WIRES, BOUNCE and the meters. The flowing backdrop only appears at boot, and nothing yet moves with each voice's level. The eyes and desert pieces do not exist.
- **Fix:** keep the four, drop the seven, add the flowing backdrop driven by the four voices, then build eyes and desert as code. Dropping seven frees space for the new ones.
- **Alternative:** drop all 11 and build only your new pieces.
- **Size:** large.

---

## Words

### 21. One name per thing
One decision: fix them all. Full list in `review/findings-using-it.md`.
- COND and FILL name the same feature. Use COND.
- "Section" and "part" name the same thing. Use SECTION.
- Pattern length is STEPS, LEN and LENGTH; LEN also means note length. Use STEPS for the pattern, LEN for the note.
- Tracks are named five different ways.
- Layer titles differ from their buttons (FX opens PUNCH, GLO opens MIX, SEL opens KEY). Title them "FX: PUNCH" and so on.
- The SAVE layer's CHAIN tile opens SONG. Label it SONG and say how to chain.
- Truncated labels: PRESE, LOCK VOIC, VCE, GLD, PRIO, CHANC, STRID and more.
- "LFO 2" is LFO 1's sync page.
- ABOUT, USB rescue and the crash screen still say KARMA or FELUCCA. Make them PHEBUS, with "based on KARMA" in the credits.
- Messages linger onto the next screen.
- **Size:** small each.

---

## Consistency and spec

### 22. Docs, the public site and the default kit
- **GUIDE.md and the cheat sheet** still describe KARMA: old colours, "FX wizard", a drum grid that no longer exists. Rewrite for PHEBUS. Medium.
- **Your public simulator and installer pages report visits to the original author's analytics counter.** Remove it. Small.
- **The installer page says "install KARMA"** and has no chip check. It stays unused until your unit is checked by hand.
- **Power-on kit is the 808.** You said you prefer 606 and 707 sounds, then chose to leave the kits alone. A 606 kit exists; making it the default is one line. Your call.
- **SPEC.md** was missing last night's wordmark and backdrop instructions in your words. Now added.

---

## Held for a second round

**From the research: what the best grooveboxes do that PHEBUS does not.** Each is a feature, not a fix, so they wait until the list above is settled. The full research with sources is in `review/RESEARCH.md`.
- **Capture:** keep a rolling record of what you just played on the keys, so a good idea is never lost (Ableton Move).
- **Duplicate and switch:** one press copies the current section to the next slot and moves you there, for building variations (EP-133).
- **Queued changes:** section changes wait for the end of the bar, with a modifier to jump at once (Circuit, OP-XY).
- **Turn the playing chain into a song** with one press (Elektron).
- **A perform page** on the 27 keys for mutes, section launches and punch-in effects together.
- **Show what the keys do while a button is held**, so no gesture has to be remembered. PHEBUS has about 30 hidden gestures today; the table is in `review/findings-using-it.md`.

**Housekeeping with no decision needed**, to do alongside whatever you approve: rename leftover KARMA and SLOOP names in the code, delete unused drawing code, correct stale comments, repair two tests that were already broken before this fork.

**Cannot be known until the device arrives:** real drawing speed, how random the boot pattern is, timing of any autosave, and how it sounds.

**Clean, no findings:** the recording screens, the KEY layer, the mixer layout, the preset browser, the boot wordmark and its balance, save and load (old patterns still load, new ones still load on stock KARMA).
