# Ben's KARMA: spec

Build against this. Do not change specced behaviour to make a test, timing or event count pass: stop and report instead. Anything marked "Claude's notes" is not spec.

Base: KARMA for the M-VAVE FM-1 (eatyourgreensstudio-dotcom/karma-fm1, GPL-3.0-only), forked at commit ba0e39d (18 Nov 2026, version 3.1.x).

## What Ben wants (20 Nov 2026)

> "I want it in Claude in a project where we can start making upgrades and changing the visuals to something more helpful. I go for useability over all else, speed and user experience so I want my version of Karma to be incredible to use"

Earlier, about the travel rig in general:

> "I like things where I can see them."

## Role in the rig

FM-1 is the synth and drum machine; the MPC Sample is the sampler and arranger. Sound target: Daft Punk / Justice, French touch (Juno chords, acid and mono bass, 909/Linn-style drums).

## Changes requested

Ben, 20 Nov 2026, after playing the simulator:

### Step sequencers on the keys

> "I like the step sequencer being on the keys. Would be cool if all the other step sequencers worked like that or if that was selectable."

> "Then the step sequencers should copy Elektron as much as possible. Press a key to enter a step. Choose a note, and place it into a step. You can param lock per step which would be super powerful on a small device like this."

### Graphics and characters

> "The graphics bum me out. I'm up for whimsical characters but they're kind of gross looking. I would want a robot, an alien, a crocodile and a snake. And I would want them to dance rather than play. We could have a white robot, green alien, blue crocodile, and a red snake? What are the Juno colours, the roland colours, let's steal those."

Colours, later the same evening (with a photo of a Roland Jupiter-08 Boutique):

> "I like the purple and sage green colours that I use on everything else too and the icy kind of baby blue with a coralish red / orange pink."

Decided 20 Nov (Ben: "Yeah let's confirm those colour swaps for the alien and crocodile. Here's the palette."), from his Ableton track colours:

| Track character | Colour | Hex (sampled from his screenshot) |
|---|---|---|
| Robot | purple | #7D61E0 |
| Alien | blue | #119AEB |
| Crocodile | green | #00B7A5 |
| Snake | coral | #DE5C50 |

> "Also feel free to improve the colours so they feel like a cohesive palette" (20 Nov: the hex values above are the starting point, not fixed)

> "The Red for what I assume is Mute? is very similar to the coral... perhaps we could pair any of the controls (if they exist with the same colours as Ableton which would make Solo a blue S, Mute a yellow M etc. ? And maybe we could slightly persuide the coral colour out brighter with a bit more orange / pink rather than Red" (20 Nov)

### Colour means voice (Ben, 20 Nov, about the FX screen)

> "I realise that this is a confusing layout for me on the FX screen... it lists 1,2,3, DRUM along the bottom, I thought they were pages no percentages of how much track had of each page's effect as a wet dry control. Stupid design there. Also they've used the same colours for the FX as they have for the voices. The voices' colours should be a unique signifier that whatever you are interacting with is being applied to THAT specific voice. Otherwise, we can use a generic colour to mean general or global controls. And if you're editing lots of parameters that effect only one voice, all the controls on that page would be in THAT specific voice's colour"

### Drums

Decided 20 Nov: keep KARMA's own drum kits; no new drum engine for now ("Let's stay put with the drum machines. I didn't realise there were lots of options to choose from!").


> "I don't love the 808.. truth be told I'd rather 707 or 606 sounds. Reckon you could hunt one down for us to steal and replace this kit? Or just if there's a REALLY good sounding drum synth out for the FM1 maybe we can steal its brain and stick it in our one?"

### Sound design layout

> "The menu diving is intense and doesn't yield significantly better results. I would want a sound design layout that matches the flow and ease of the Minifreak. Oscillator engine > Filter > ADSR > FX + MOD matrix. We could leave the mod matrix to the end. But their concepts for small builds on synths are definitely the best and most fluid sound design."

## Claude's notes

- GPL-3.0-only: if this build is shared with anyone, the full source must be shared too. A public fork covers that.
- The hardware build needs JieLi's Linux toolchain and the AC79 SDK. Neither is reachable from Claude's cloud workspace, so builds run on GitHub Actions in Ben's fork (or on his Mac with Docker).
- Newer FM-1 units have a changed chip; check Ben's unit before the first flash.

## 20 Nov 2026, evening: name, characters, backdrops (Ben's words)

Name: "I want it to be called something that sounds a bit like a greek or roman god's name. Maybe meaning something that encapsulates a euphoric french touch feeling. But could also be the name of a space probe" ... "Phebus?" ... "Just do it in plain white. PHEBUS." ... "Can you do it in a smooth font? Rather than pixelated? Something suitable for the barbican. Or Daft Punk - but not their font."

Characters and backdrops (with six reference pictures: a realistic grey alien with huge black eyes; two muscular crocodile-men, one roaring; a tall chrome humanoid robot with a blank visor head; two pencil drawings of a reared cobra, fangs and tongue out): "I want a cross between this kind of ferocity in the characters. Will screensavers or backdrops that have really specific acid psychedelic modern glitch art fused with alex gray's psychedellic eyes and desert art vibe"

His picker answers:
- How the fierce characters move: "Both" (dance while playing, strike poses on accents).
- Backdrop colours: "Four voice colours" (purple, blue, teal and coral only, each driven by its own voice's level).
- Where backdrops appear: "Screensavers only" (they replace the visualiser screens; working screens stay clean).
- Order: "Characters first".

Claude's notes: the references set a mood, they are not to be traced or copied; the robot reference is a real product, so the robot is an original design. Backdrops are original work, not copies of any artist's paintings.

Backdrops, further (Ben, 20 Nov, 22:42, with six reference pictures: a jagged horizontal-wave glitch in neon purple, green and blue on black; flat two-colour wavy stripe posters; three flat-colour illustrated faces; three liquid marble swirls in neon colours on black, one with thin drips): "These but flowing and morphing"

Characters, first draft feedback (Ben, 20 Nov, 23:08): "Let's go more classic on the robot, blockier. And lose the hood of the cobra, it just looks like he's eaten something too big for his throat"

## The dream (Ben, 20 Nov 2026, 23:39)
"Go deep and maybe do some research first on the best groove box UI's and what most people complain about when using them. And how we can make ours the best one ever. I love the flow on the minifreak. If it could do drums, sequencing, patterns, song mode. It would be the dream"

## Boot wordmark and backdrop, further (Ben, 20 Nov 2026, 22:45 to 23:00; added 21 Nov, missing until the review caught it)
- "Can you make the word mark - I like the font. Can you use a screensaver visual as the background, a different one every time ofc.. and then use the PHEBUS work mark but the phebus is all black so it appears as the negative space in the trippy background visual?"
- "Phebus should be heavier to stand up against the noise. Make it bold"
- (of one boot picture) "This is also the limit for how busy these should get." (of a denser one) "This feels too dense"
- "We don't need that light band of translucent white that runs horizontally behind the Phebus text."
- "You're right. The title font should fill the square. PHE / BUS"
- Characters: "I like them."
This supersedes "Just do it in plain white" for the boot screen.

## Review decisions (Ben, 21 Nov 2026, by picker; the questions are in REVIEW.md)
1 Hear notes on the GRID: "Play on change". 2 Step editors: "GRID only". 3 Filter locks: "EDIT walks the chain". 4 Length: "Page slots". 5 Drum grid: "SELECT toggles views". 6 Saving: "Dot + save on stop". 7 Undo: "Full undo, shown". 8 Sound pages: "Sound big, character small". 9 Chain: "Merge and move". 10 Song screen: "Insert and delete". 11 Bugs: "Fix all four". 12 Boot: "Keep 3 s full detail". 13 Strike: "Fix". 14 Knob numbers: "A: one bold face". 15 Bottom bar: "A: heads, underline"; mute/solo "A: badges". 16 Colour rule: "Fix all". 17 Colour-blind: "Skip". 18 Headers: "Fix". 19 Small fixes: "Do all". 20 Screensavers: "Drop all 11, new only". 21 Words: "Fix all". 22 Docs and site: "Rewrite and remove"; default kit: "Start on 606".

## Sounds, chords, performance FX and stutter (Ben, 21 Nov 2026, 08:41)
"Can you make the default sounds very plain? I usually don't use presets, just a plain sawtooth is fine for the bass, chords, and lead. Also I don't like having fx on by default. Also would be cool if you could step sequence chords? Not just single notes. Also would be cool if there were similar fx style interface to the mpc sample - some global fx that you can dial in and some fx that you can play on the keys. Inclusive of stutter effects / beat repeat that are so key to the Daft Punk sound. I find 1/4 stutters and 1/8 note stutter patterns more useful than triplets, or 16ths. Would be cool if there was a stutter sequencer tbh where you could set the length of the stutter pattern and then turn stutter on my step?"
His picker answers (21 Nov, morning): stutter sequencer length "Per step" (a knob sets the length for new steps: 1/4, 1/8, 1/16, 1/32; hold a step and turn to change just that one); a stutter step repeats "The whole mix"; presets "Keep presets, plain start".

## Follow-up decisions (Ben, 21 Nov 2026, about 10:00, by picker)
Held-SEQ step keys: "Remove them" (holding SEQ gives only division, swing and length). One-knob pages: "Keep, name the pages" (show page names in the strip). Song insert and delete: "Hold the entry's key" (hold the white key for an entry, then OCT+ inserts and OCT- deletes). THIRD EYE screensaver: "Keep it".
Chords track (Ben, 21 Nov, picker): "JUNO on track 2" (polyphonic saw chords; needs a check on the real FM-1).

## After playing the simulator (Ben, 21 Nov 2026, 14:30 to 14:37)
- "It looks fine. It doesn't make much sense as a thing to use. Like... it's hard to input sequences that make sense when you're writing melodic parts. I've no idea how to use half of it or what the features are anymore."
- (after the feature list and the step-record proposal) "ahhh ok, it's really hard to use because the simulator can't hold a step and turn. But that seems like a good way to do stuff. Very flowy."
- "I don't like that the stutter's minimum step is 1/8. 1/4 is preferably. 1/2 would even be nice to play with" ... "And if we're doing that we may as well go the full 1/1"
- "I don't like how the tape reel looks"
- "The filter isn't working as I'd expect a 101 or 303 to work, it doesn't growl or move in the same way" ... "Does it maybe need the ability to attach the filter to the adsr or have it on its own ADSR?" ... "I like that about the minifreak, you can fudge it to be linked to the adsr of the amp very easily or put it on its own ADSR envelope just for the filter"
- "Also having easy access to decay, filter, fx, makes for fun play on the fly"
- "Can you not change what triggers the LFO restart? Like have it free or attached to the note start or the sequence length?"
His picker answers (21 Nov, 14:40): HOME knobs "Cutoff, decay, FX, level"; and for what the FX knob does, in his own words: "Would be cool if it actually just turned on and off the stutter". Filter envelope "Own ADSR + link switch" (measure the cost and report before building). Bass engine "ACID for bass" (plain saw through the 303-style filter, a little envelope movement so it speaks). Long stutters "Replay the steps" (1/4 default; 1/2 and 1/1 loop the sequencer's last steps; shorter ones stay as audio stutter). LFO trigger "Add PATTERN". Tape reels "A strip of the flowing backdrop" (a band of the FLOW pattern moving with the four voices' levels). Step-record "Yes, add it" (press REC in the GRID, play notes or chords on the keys, each lands on the next step).
