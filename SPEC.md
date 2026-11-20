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
