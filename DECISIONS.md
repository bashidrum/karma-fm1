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
