# PHEBUS polish review, 26 Nov 2026

Reviewed: the whole instrument on branch `ben` (8381c63) after waves 1 to 14 and the 23 to 26 Nov work. Depth: standard (asked in passing; no long test runs). Three read-only reviewers: visual design (393 screens rendered and looked at), using it (nine journeys driven through the real UI code, every on-screen string, the guide against the build) and engine and code (every fast test, sanitisers, a flash and RAM read). Each finding below was checked by Claude against a screenshot or the code. Screenshots: `review/polish-2026-11-26/`.

Overall: the instrument works end to end. Every button walks its categories, SELECT never changes page, every journey (first ten minutes, 808 snare into the 606, hold-to-map, mixer, patterns and fill, the bank, the stutter lane, DISCO) did what the guide says. All fast tests pass. What is left: four engine faults that matter on the real unit, a drum side that does not yet mirror the synths in three places, uneven words and tab labels, and docs that lag the build. 18 decisions are waiting.

The previous review (the 22 Nov direction review) is in `review/REVIEW-2026-11-22-direction.md`.

## Before the first install (engine)

Done 26 Nov (368d95f, CI run 109, RAM unchanged at 97,076 B): E1, E2, E3 (warning THIS WILL STOP THE AUDIO, ERASE included), E4, E5.

E1. **A long save can trip the watchdog on the slower chip.** `felucca.c` `st_erase` never feeds the ~8 s watchdog; `ota_erase` was given a feed on 26 Nov but saves were not. KEEP makes about 22 erases in one pass, NEW about 10, a stop with four changed patterns 9 or more. On a chip averaging over ~360 ms per erase, KEEP restarts the FM-1 every time (no work lost, KEEP just never finishes) and the reset lands mid-erase. Fix: one line, `fm1_wdt_feed()` at the top of `st_erase`. Small.

E2. **A backup can hold the bank's project instead of the live one.** `editor.c:263` captures the live project into `proj_tmp`; listing the bank's objects then calls `bank_scan`, which loads the bank's project into the same `proj_tmp` (`project.c:1807`) before the checksum is taken. Reproduced: after a power cycle with something in the bank, the backup's object 0 is byte-identical to the bank. Restoring that backup replaces the live sketch with the bank's. Fix: `bank_scan` reads only the header, tempo and kit name into small locals, never `proj_tmp`. Small.

E3. **User kits are written to flash while playing.** `ui_kit.c:285` and `:298` call `ukit_put` with no playing check; every other writer refuses. One erase and 15 pages with interrupts off: a ~50 ms hole in the sound on the old chip, up to 2.75 s of silence (sequencer, MIDI clock and USB frozen) on the slow one. Fix: refuse with STOP PLAYING FIRST, as KEEP does. Alternative: queue it until stopped, as patterns are. Small.

E4. **A double tap of OCT on SAVE > PRESETS loses an edited sound.** The first tap loads the next preset at once (`ui_input.c:1093`), overwriting the voice; the second tap reverts to the previous preset by list position, not to the edited sound. Sound edits are not in undo. Likely the same on SEQ > A-D while stopped (edits made while stopped are not kept in the pattern). Fix: on pages where a tap loads something, wait out the 0.3 s double-tap window before acting. Alternative: keep a 192 B copy of the voice (RAM is very tight). Small.

E5. **The FX 1 to 3 pages read past the parameter array** (`ui_synth.c:421`: `t->p[pg->id[0]]` with id 246 since wave 9; ASan stops here). The PHASER and FLANGER slot pictures do not animate. Fix: `t->p[P_XS(pg_slot(pg), 0)]`, as line 495 does. Small.

Also from the engine pass, for the device checks after install (no decision): the CPU's heaviest case is every PERFORM pad stacked (x1.5 of the plain mix on the host); CPU1's mix frame grew from 192 B to 768 B (stack 3,968 B); the web tests (`node web/test_web.mjs`, long) are the one test worth running before using the web installer.

## Using it, consistency and the spec

U1. **The drums' MIX pages are drawn as the drum screen** (tag DRUM in teal, the selected sound large, the kit grid, "THE WHOLE MIX"), where a synth voice shows the one white MIX screen. Breaks the colour rule and "mirror the synths". Fix: one MIX screen whichever voice is selected, as STUT already does. Small. `v03`, `u-j603`.

U2. **The drum kit's INS and SLICER put the type on KNOB 1; the synths put it on SELECT.** Kit INS: K1 "INSERT", K2 to K4 named "-" but showing 50 76 76 in full colour (and lockable); no "EMPTY: SELECT PICKS A TYPE". Kit SLICER: K1 SLCR, then PAT RATE DEPTH, where a synth has the mode on SELECT and PAT RATE DEPTH on K1 to K3. SELECT does nothing on kit pages, so it is free. Fix: SELECT turns the kit insert's type and the slicer's mode, values move to the synth positions, the empty state matches. Medium. `v20`, `u-j600`.

U3. **The drum sound pages show the same picture on every page** (the decay graph on SRC, FLTR, AMP and LFO, mostly an empty box for short sounds) and a giant word on INS and SENDS, where synth pages draw the page's own picture. Fix: reuse the synth drawings in teal: filter curve on FLTR, wave on LFO, the insert tiles on INS, the send thumbnails on SENDS; keep the decay on SRC and AMP with its time axis scaled to the sound. Large. `v04`, `v04b`.

U4. **SEQ's A-D page does not show the patterns.** The body is still the steps and KNOB 1 reads "--"; you find an empty slot only by turning to it. Fix: four tiles, PATTERN A to D, STORED / PLAYING / QUEUED / EMPTY, as hold SAVE and HOME already draw them. Small to medium. `v09`.

U5. **GLO > DEVICE (the old menu) is outside the frame and the flow.** Header MENU not GLO, no tab row or strip, a third value font, "K1 COLOR" labels, two footers worded differently, a second SYSTEM beside GLO's SYSTEM with other contents, MIDI LEARN CLEAR there while LEARN is on MIDI 3, and OCT- "BACK" lands on HOME (so the next HOME press opens a screensaver). Fix: draw its four pages in the standard GLO frame (DISCO LIGHTS KEYS NOTES as dials etc.), rename its SYSTEM (TOOLS), move LEARN CLEAR beside LEARN, one footer, OCT- returns to GLO. Medium to large. `v08`.

U6. **The same setting in two places** (Ben, 22 Nov). GLO > SYSTEM repeats MIDI 2's OUT and IN; hold SEQ shows SWING on KNOB 3 and on MASTER on the same screen, and STEPS and SHIFT repeat the PATTERN page; whole kits load from HOME PRESETS, SAVE > PRESETS and SAVE > KITS. Fix: drop OUT / IN from SYSTEM; hold SEQ's KNOB 3 gets no second swing (leave it empty or give it GATE); on the drums SAVE > PRESETS points to KITS. Small each.

U7. **SAVE > USER on the drum voice is a dead end** that names screens which no longer exist ("DRUM TRACK / SEQ TRACKS GLO DRUMS", `ui_draw.c:792`). Fix: skip USER in SAVE's walk on the drums. Alternative: one plain line, "USER PRESETS ARE FOR VOICES 1-3. YOUR KITS: SAVE > KITS". Small. `u-j415`.

## Visual design

V1. **Tab labels shorten differently on every page and some touch** ("PRO PRE USE ... TOO", "GLOBA", "DEVIC", "SENDSSLI", "MIXSTUT"). Fix: one fixed short label per category used everywhere (PROJ PRST USER KITS PATT TOOL; GLOB SYS MIDI DEV; INS SEND SLIC | MIX STUT), a fixed 6 px gap; when room runs out the legend drops its word first, then the page count. Medium. `v02`.

V2. **Words: one spelling and plain forms.** DETUNE / DTUNE, BRIGHT / BRIT, PHASR / PHASER and FLANG / FLANGER on one screen, LP24 / LP 24, FX1 / FX 1, LFO 1 / LFO1, KICK 2 / SNARE2, COLOR (US) against COLOUR, the LFO 1 page titled "LFO"; cryptic SUSTN RELEAS N.HOLD N.DEC P.AMT P.DEC SCL QNT INTENS ESYNC STRC CLRSQ SLCR and the cut "ELEME"; "VOICE" and "TRACK" both used for a voice ("NOT ON THIS TRACK", "MIDI PLAYS TRACK 2", "LOCKS FULL: 24 A TRACK"). Fix: one dictionary, six characters or fewer, applied everywhere (DETUNE, BRIGHT, PHASER, FLANGE, LP 24, FX 1, LFO 1, SNARE 2, COLOUR, SUS, REL, NS HLD / NS DEC, P AMT / P DEC, SCALE, QUANT, DEPTH, STRUCT, ELEMNT, CLEAR, SLICE, VOICE). Small to medium. `v05`.

V3. **MIX and GLO pages show every value twice**: a numbered list with a bar per parameter in the body, the same four dials below, and slider bars on settings that are not ranges (SYNC INT, OUT SEQ). Fix: MIX pages get a picture like the voice pages (delay taps, reverb tail, a master meter); GLO pages drop the bars and print the value right-aligned in each row. Alternative: keep the list and drop the dials' values. Medium. `v07`.

V4. **Said twice on one screen** (Ben: "too dense"): hold SAVE's header "SAVE SAVE"; hold ENV / LFO's white bar repeating the big MASTER readout; the AMP / FILT chips repeating the tab row on ENV; the drum screen naming the sound three times (chip, title, "SOUND 606 SNARE") and the browse readout repeating the body. Fix: title PATTERNS on hold SAVE, no bar while the MASTER readout shows, drop the AMP / FILT chips, the drum line becomes "606 · 8/41". Small. `v15`.

V5. **Small visual faults, one bundle** (each verified on a capture):
- Drum kit INSERT with a step held: "THE KIT: 16 OF 16 SOUNDS" and the lock line drawn on top of each other (`v01`).
- List pages show the top of a seventh row above the tab rule (`v06`).
- Mixer M / S badges clipped by the tile edge, "SNARE2" running into its M (`v10`).
- REC screen line "PLAY STARTS, REC CANCEL" runs to the last pixel (`v11`).
- Long header titles touch the tempo ("JUNO BRAS127", "PATTERN B120") (`v12`).
- The red record disc touches the beat lights and nearly the battery (`v23`).
- PERFORM's and the mixer's right-hand tiles run to the screen edge; pad text fills its tile (`v24`).
- ABOUT: lines 1 to 2 px apart, the last on the bottom edge (`v25`).
- Hold SEQ: an empty second row of boxes under A B C D BAR, SHIFT drawn as a lone "‹ ›" (`v22`).
- The screensaver caption is unreadable over FLOW and THIRD EYE (`v19`).
- Storing over a used pattern: the line "KEEP A COPY: SAVE > PROJECT" stays half-drawn above the divider, and the ask reads "LET GO: NO" (`u-j100`).
- The mixer puts its messages in the header title, cut to "MUTES AND" (`u-j304`).
- While a fill is on, "FILL" replaces the header chip (the drum sound, or MASTER's SWING) (`u-j412`).
Fix: each as described in the screenshots' notes in this report's source list. Small.

V6. **Envelope times have no units and three scales** (AMP 1.0 / 1.0 / 100 / 2.1; FILT own 21 / 21 / 33 / 21; FILT linked 4.3 / 18 / 55 / 8.8). Fix: one format on both envelopes, ms under a second then "1.2S", sustain in %. Small. `v13`.

V7. **The STUTTER lane's recorded pads are on a different time axis from its steps** (one 16-step line at 15 px a step under two rows of 8 at 30 px), so a span from step 5 sits under step 2. Fix: draw spans in the same two rows of 8 under the same columns, with a PADS label. Medium. `v14`.

V8. **Pictures that do not match their page or the colour rule:** the MATRIX grid drawn white and grey on a voice page (`v18`); FOLD's wave 80 px wide where others span 228 px, and OSC 3/4 and 4/4 repeating page 1's wave (`v21`); ARP > KEY's keyboard drawn as thin bars where CHROMATIC draws a real piano (`v17`). Fix: matrix labels and cursor in the voice colour (dim when idle); every wave full width, OSC 3/4 and 4/4 show voices / unison dots; KEY reuses the CHROMATIC piano in the voice colour. Small.

## Options (taste, not defects)

O1. **HOME gives the three synth voices the least room**: their steps are 2 px dashes while DRUM and PERFORM get 14 px boxes. Option: put SYNTH inline and give each voice a 4 px step bar. `v16`.

O2. **Drum sounds browse differently from synth sounds**: a single big name against the synths' six-row list. Option: the same six-row list on the drums, same-kind first. `v26`.

## Docs

D1. **The docs lag the build**: GUIDE line 5 and the cheat sheet still say "Do not flash until your FM-1 has arrived and its chip revision has been checked"; GUIDE says the loader still has the short waits (changed 26 Nov) and step 8 contradicts itself; drum FX order, the tab example, the drum lock walkthrough, PERFORM's black row (one row, not 7 + 4), ENV 2/2, CHROMATIC's ends, the glossary's TRACK, the missing STUT and GATE are out of date; the button map is missing PROJECT, says four screensavers and "wave 14"; about 27 "wave n" history notes remain in the guide and cheat sheet; DECISIONS' "For Ben to confirm" still asks about things long settled (hold REC confirm, SEL + key, the 7 + 4 strip, "lowest key is HAT", the flash permission). Fix: bring all of it to today's build and prune "For Ben to confirm" to what is open. Medium, docs only.

## For Ben to reconsider (his own decisions, with evidence)

R1. **Drum FX order.** SPEC.md 21:40's applied reading says SENDS first, then the kit's INS and SLICER; his 21:41 "mirror the synths" led the build to INS, SENDS, SLICER (`ui_nav.c:31`). SPEC and build disagree; one should win and the other be corrected.

R2. **Three reading orders of the 16 white keys on 4 x 4 grids:** the mixer and PERFORM down the columns from the top; hand play, RETRIG and SELECT along the rows from the bottom (both Ben's choices); and hold SEQ's roll tiles on a synth voice in plain rows from the top, which nobody specified. Lining the last one up with one of the others is cheap.

R3. **ARP on the drums** is the only button whose second press is not its next category (CHROMATIC, then off).

## Came up clean

Navigation (every button's categories, SEL's pages, SELECT never changing page); the synth SEQ grid and lock view; hold-to-map; PERFORM's columns and one-row black keys; the confirm rings; SAVE > PROJECT and the plain-words hold SAVE; the drum grid picture and both key orders; the colour code everywhere; red only for record and erase; the artwork (black eye, no outlines, DESERT, BEACH, EYES); STEP REC; QUIET dim; all fast tests; no new 64-bit divide; no floating point on the audio path; the simulator compiles; the flash A/B design and the bank's power-cut safety; MASTER's relative scaling; no em dashes in the docs.

## Not seen

No capture exists for the synth ARP pages, the drums' KIT FLT, kit SLICER and INSERT 2, the HEX engine's AMP and LFO, SAVE > TOOLS and a drum sound list.
