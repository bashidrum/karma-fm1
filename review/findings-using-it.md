# PHEBUS review: using it, flow and words (subagent report, saved by orchestrator)

Lens: a musician on a plane with no manual. Reproduced in the host UI harness (screenshots in /home/claude/review-work/use/, sheets in use/sheets/, drivers j1.h to j10.h, `./run.sh jN.h shots/jN`) or cited to file:line in /home/claude/rv-use (branch ben, 63f20d1). Sizes: small = an hour or less, medium = about a day, large = several days or a design change.

## Journey step counts
| # | Journey | Actions | Pain point |
|---|---|---|---|
| 1 | Power on to first sound | 1 (press a key). Pick track + sound + play: 3 | HOME has no preset list. The list is SAVE on any page except HOME; SAVE on HOME opens SONG |
| 2a | Enter GRID, set 16 steps of one note | 1 + 16 | fine |
| 2b | New step with a different note | 2 per pitch (KNOB 1 blind, then the key) | silent (U1) |
| 2c | Change one step's note | 1 (hold step + KNOB 1) | silent |
| 2d | Parameter lock | 3 (hold + page button + knob) for OSC/VOICE/ENV/FX/LFO. Filter: no lock page, about 6 actions by a hidden route | U3 |
| 2e | Remove a step | 1 | fine |
| 2f | Change length / go past 16 steps | Not possible on the GRID. Hold SEQ + KNOB 4 (hidden), or HOME + KNOB 3 + SEQ | U4 |
| 3 | Drums: kick, snare, hat | About 21 (5 to reach the drum GRID: HOME, ALGORITHM x3, SEQ; then lanes via KNOB 1) | U5 |
| 3b | Accent one hit / change kit | 1 / 1 | Kit is a number on the KIT page but a name in the header |
| 4 | OSC > FILT > ENV > FX > MOD | EDIT x5. Every page: 18 SELECT clicks on ACID | Page stays put when changing track (good) |
| 5 | Mute / solo / level | 1 each (hold GLO + key or knob) | Mixer key legend is single letters |
| 6 | Store A and B, switch, copy A to B, chain | 1 / 1 / 2 or 3 / hold SAVE + 2 or more taps | Key 16 tile mislabelled (U7) |
| 7 | Arrange song, play, save, reload | Open SONG 1, KNOB 1/2/3 per entry, KNOB 4 count, OCT- song mode (unlabelled), PLAY. Save: 4. Load: 4 | No insert or delete; no save while playing |
| 8 | Record, undo, clear | 3 / 1 (EDIT + OCT-) / 1 (hold REC 2 s) | Good signposting |
| 9 | Tempo, swing, key, scale | 1 each | SELECT is tempo on HOME/GRID but turns pages on pages |
| 10 | Home from anywhere | 1. From a locked layer: 2 | Leaving the menu needs HOME held again |

## Findings (most valuable first)

U1. No note audition in the synth GRID. ui_grid.c:91-115 (sg_set auditions drums only); sg_free_knob and sg_held_knob silent. Step tap, PEN turn and held-step NOTE turn are all silent; keys are steps. Fix: play a short note when NOTE turns or a step is set. Alt: while a step is held, black keys (or OCT held + white key) play and set the note. Medium. grid-synth-empty.png.

U2. Three step editors with different layouts and knob orders. GRID 2x8 NOTE STEP HIT VEL; STEP page 2x8 STEP NOTE TIME/LEN FLAG; SEQ layer 4x4 NOTE DIV SWING STEPS. Only STEP page has heard key entry. Second SEQ press goes to STEP on synths, KIT on drums (ui_input.c:678-686). Fix: one editor; fold STEP page key entry into GRID as a mode, keep SEQ layer for DIV/SWING/STEPS, same second press on every track. Alt: same knob order and 2x8 everywhere. Large. seq-layer-4x4-vs-grid-2x8.png, step-page-knob-order-differs.png.

U3. Filter cannot be locked from the GRID's lock pages. ui_input.c:672 passes FAM_EDIT; ui_grid.c:196-208 only cycles that family (OSC, VOICE, VOICE 2). FILTER pages are FAM_FLT. Only route hidden: touch CUT on its page, return, hold step, turn PRESETS; parameter not named. Fix: held step + EDIT walks OSC > FILT > ENV as EDIT does elsewhere. Alt: held step + SELECT walks every lockable page. Small. lock-page-title-truncated-VOIC.png.

U4. No pattern length on the GRID; going past 16 steps silently does nothing. ui_grid.c:155-161, 405-413. Fix: show 4 page slots, unused dim; OCT+ or black key past the end adds a page (copy of last) with "LENGTH 32". Alt: LENGTH on KNOB 4 when no step held. Small. grid-synth-oct-does-nothing-at-16.png.

U5. Drum GRID shows one lane; snake takes the space. ui_studio.c:607-626; others only a tiny dot matrix; GUIDE.md line 240 promises "16 sounds x 16 steps". Fix: readable multi-lane grid of the sounds in use. Alt: SELECT toggles full 16x16 view. Medium. drum-grid-one-lane-only.png.

U6. Peeking at a layer then releasing fires the tap action. ui_layers.c:31-32 (TAP_MS 450 / SHOW_MS 140); ui_input.c:758-760. Reproduced SEQ, EDIT, ARP, FX (sheets/j5b.png). Fix: no tap once the layer has shown. Alt: do not show the layer before TAP_MS. Small.

U7. SAVE layer key 16 tile says CHAIN but opens SONG. ui_layers.c:1014 vs 418. Fix: label "song"; sub line "tap 2+ = chain" while SAVE held. Small. save-layer-chain-label.png.

U8. No way back to the GRID from a sound page. ui_input.c:704. Synth GRID > EDIT > SEQ gives STEP; drum GRID > FX > SEQ gives a dead "DRUM TRACK" STEP page. Fix: SEQ returns to GRID if that was the last sequencing screen. Alt: HOME acts as back. Small.

U9. Sound pages show the character, not the sound (OSC, VOICE, FILTER, FX, INSERT, LFO DEST, ARP, PLAY, GLOBAL; only ENV, LFO, SLICER draw their shape). "ACID TB ACID" repeats the header tag. Fix: draw filter curve, wave, send bars, insert curve; characters for HOME and screensavers. Alt: shrink character to an icon. Large. filter-page-shows-robot-not-filter.png.

U10. 18 pages in the ACID chain: OSC, VOICE, VOICE 2, FILTER, FILTER 2, ENV, ENV DEST, FX, INSERT, INSERT 2, SLICER, CLOUD, DLY, REV/CHO, CLOUD 2, LFO, LFO DEST, LFO 2. FILTER 2 and INSERT 2 one knob each (params.c:519, 534). Fix: merge single-knob pages, move global FX pages to GLO. Alt: page names not dots in the strip. Medium.

U11. Global pages drawn in the voice colour: FX-stage DLY, REV/CHO, CLOUD, CLOUD 2 and GLO > GLOBAL (params.c:536-539 SC_GLOBAL). Fix: neutral with a "GLOBAL" tag. Small. global-delay-page-in-voice-colour.png. [orchestrator: verify, draw_columns sets neutral for SC_GLOBAL; may be the strip/scene only]

U12. Bottom "1 2 3 DRUM" bar reads as tabs, takes 40 of 240 rows, appears on global pages; only place solo/mute show there. Fix: slim strip of four character heads (yellow M, blue S) or label "TRACK". Alt: hide on global pages and the GRID. Small.

U13. No autosave while playing (project.c:767); no saved/unsaved indicator. "Save" used five ways (SAVE layer "save A", SONG "REC: STORE", SONGS "SAVE", USER "SAVE", TOOLS). Fix: "unsaved" dot in header and autosave on stop. Alt: autosave between bars (needs hardware timing check). Medium.

U14. ENV and LFO do nothing on the drum GRID (ui_grid.c:414-419, synth only). Fix: behave as from HOME. Small. drum-grid-env-does-nothing.png.

U15. Default kit cannot be sound-designed yet pages show live-looking values (ui_op.c:867 "A HEX KIT ONLY: PRESETS: KIT 1-5"). Fix: blank the knobs, plain words naming editable kits. Alt: default to a HEX kit. Small. hex-kit-only-shows-values.png.

U16. SONG screen: no insert or delete (ui_song.c:274-292); OCT- toggles song/loop (ui_song.c:250) but not in hints (97-98); edits refused while playing. Fix: hold entry + OCT+ inserts, + OCT- deletes; add "oct-: song/loop". Medium. song-screen-hints.png.

U17. GRID does not follow the playhead across pages. Option: follow mode. Small. grid-does-not-follow-playhead.png.

U18. HOME shows "LEVEL 0" for a muted track (stored 81); first KNOB 2 turn only unmutes (ui_input.c:268-271). Fix: real level dimmed plus MUTE. Small. home-level-0-when-muted.png.

U19. Downbeat light is red while only playing (ui_op.c:59), against DECISIONS (red = record/erase only). Fix: white. Small. downbeat-red-while-not-recording.png.

U20. Stray pixels under SONG header: rows 40-41 never cleared (ui_song.c). Small. song-screen-stray-pixels.png.

U21. ABOUT says KARMA, version "KARMA 3.1.5"; "KARMA USB RESCUE", "FELUCCA CRASH" (ui_menu.c:90, ui.c:6, recovery.c:62, main.c:100). Fix: PHEBUS title, "BASED ON KARMA" in credits. Small. page-about.png.

U22. GUIDE.md describes the old firmware: titled "KARMA 3.1"; old track colours and "knob of the same colour" (108-113); "FX wizard" (707); "16 sounds x 16 steps" (240). Medium.

## WORDS
W1. COND vs FILL for the same feature: "COND: NORMAL" / "LOCKS, NUDGE, COND CLEARED" (ui_grid.c:390/395) vs "FILL: NORMAL" / "NUDGE, LOCKS, FILL CLEARED" (ui_layers.c:280/286). Use COND.
W2. Section vs part: "EMPTY SECTION: REC", "EMPTY PART" (ui_song.c:207), "PICK A PART" (ui_layers.c:416), "into part A", "song: parts order loop". Use SECTION.
W3. Pattern length named STEPS (HOME, SEQ layer), LEN (PATTERN), LENGTH (EDIT layer). SONG "LENGTH" = entries. GRID "LEN" = note length. Use STEPS / LEN / ENTRIES.
W4. Tracks named "1 2 3 DRUM", "SYN1-3 DRUM", "TRACK 1", "= T", sound name (REC), engine (header tag), "DRUMS" on HOME. Pick one.
W5. Layer titles differ from buttons: FX=PUNCH, EDIT=ERASE, ARP=ROLL, SEQ=STEPS, SEL=KEY, GLO=MIX, SAVE=SONG. Use "SEL: KEY" style. layer-names-differ-from-buttons.png.
W6. Truncated/cryptic: headers "PRESE", "LOCK VOIC"; knobs VCE, GLD, GLMOD, PRIO, DRV, DST, CHO, PHS, COLR, FDBK, CHANC, STRID, SWG, INSRT, INS A/B/C, P.SLP, TRANSP; drum lanes (drums.c:118) KICK2, OPEN, SNR 2, TOM L, TOM H, SHAKE, BELL; punch STUTT, K LOW/MID/HIGH, LOOP8T, OCT UP; mixer key strip "MMMMSSSSFB T"; "CHR".
W7. Same label, different thing: FILTER page "ENV"/"DEC" next to ENV stage; "DLY" send (params.c:130) and global delay page (537); "LFO 2" is LFO 1's sync page (543) -> LFO SYNC (lfo2-is-lfo1-sync.png); KIT knob number vs header name (drum-kit-page-number-vs-name.png).
W8. "TB ACID ACID" on HOME and "ACID TB ACID" on sound pages: engine name repeated.
W9. Leftovers: KARMA/FELUCCA strings (U21). Possibly dead: "clear the sequence?", "clear track n?" (ui.c), "SEQ  TRACKS  GLO DRUMS", "U07  NAME" (ui_draw.c).
W10. "STOP FIRST" vs "STOP THE SONG FIRST". "NEW LOOP" for TOOLS > NEW. Confirm prompts mixed: "EDIT AGAIN: ERASE A", "AGAIN: SAVE", "OCT+ AGAIN: LOAD", "REC AGAIN: REPLACE".
W11. Toasts carry over to the next screen ("KEY A" over STEPS layer; "AGAIN: SAVE" over HOME; "NO LFO ON DRUMS" over INSERT knobs). Option: clear on screen change.

## Hidden gestures (Shown = the screen tells the user when it matters)
| Gesture | Does | Shown? |
|---|---|---|
| HOME tap on HOME | visualiser | no |
| HOME held 0.7 s (again) | menu (leave) | leaving not shown |
| Layer held + HOME tap | lock the layer | "LOCK" once locked; never offered |
| GLO held 0.7 s untouched | mixer stays | no |
| Release a layer between 0.14 and 0.45 s | counts as a tap | no (U6) |
| REC held 0.7 s, then to 2 s | clear track | yes |
| EDIT + OCT- / OCT+ | undo / redo | yes, on ERASE layer |
| EDIT + black key (drums) | mute mode | sub line |
| FX + OCT- / OCT+ held | reverb / echo throw | only while active |
| OCT- + OCT+ together | octave 0 | no |
| OCT held (drums) / ARP + key + OCT | ghost/hard hits / rolls | no |
| SAVE held + 2+ section taps | chain | no; tile mislabelled |
| SAVE + 5-8 on a used section, twice in 3 s | overwrite | yes |
| SAVE on a page (not HOME) | PRESETS/USER/TOOLS | no |
| SONG: REC (x2), OCT+ x2, EDIT x2, SAVE | store / load / erase / songs | yes |
| SONG: OCT- | song/loop mode | no |
| SONGS: KNOB 2/3/4 twice | erase / load / save | yes |
| USER/TOOLS knobs: two detents in 1.5 s | GO | yes, after the first |
| GRID: hold step + SELECT | FLAG/RATCH/NUDGE knobs | no |
| GRID: hold step + ENV/LFO/FX/EDIT | lock page | title once in |
| GRID: hold step + OCT- / OCT+ | clear locks / cycle condition | message after |
| GRID: hold step + PRESETS / ALGORITHM | lock last-touched parameter / pick it | no; not named |
| GRID: black keys 1-4, OCT-/OCT+ | pages | no; silent at 16 steps |
| SEQ layer: step + PRESETS/ALGORITHM/OCT | lock / parameter / condition / clear | sub line |
| GLO + F#3 | MIDI LEARN | no |
| HOME KNOB 2 on a muted track | unmute | no |
| KIT page key | pick sound | yes |
| Drum black key | plays the white key to its left | no |
| Key on GEN page | GEN on + set KEY | no |
| OCT- at power on / both at power on / both held 5 s stopped / any key at boot | USB rescue / calibration / update mode / skip boot | own screens only |

## Keep
One press to first sound. ALGORITHM keeps the chain page across tracks. Recording screens. SONG and SONGS print their own double-press gestures. The held GRID step view.

Not verified: after a clear-hold released early, EDIT + OCT- emptied track 1's recorded loop; may be correct one-level undo.
