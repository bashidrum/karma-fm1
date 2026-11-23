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
- (of the ENV page, with a screenshot of the dim character behind the envelope, 21 Nov 15:05) "I don't like how the characters jump and grow here... I'd rather something more chill there tbh. Maybe not even the character"

## New direction for PHEBUS (Ben, 21 Nov 2026, 15:32). This governs everything above it where they clash.

"I'd really love the melodic parts of PHEBUS to feel as similar to the minifreak as possible. With the drums feeling as similar to the Analog Rytm mk2 as possible"

"Then I want the MPC's feel for pad like fx play and stuttering. The end goal is to record into an MPC sample anyway, and I'll always use them together. So I want to remove any features that don't serve exactly the device vibe I'm trying to build."

"So PHEBUS is three things and nothing else:
1. Melodic voices: the MiniFreak's swift sound design. Oscillator type plus a few macro knobs, filter, envelopes, FX slots, mod matrix last. The same layout on every voice.
2. Drums: the Analog Rytm MKII's easy drum programming. Pick a sound, 16 trig keys, hold a trig and turn to lock, sound pages in the Rytm's order (SRC, FLTR, AMP, LFO), mute, fill, retrig, chromatic.
3. Performance: the MPC's feel for playing effects and stutters on the keys like pads.

The MPC Sample does the sampling, arranging and recording. PHEBUS never needs to do those jobs."

"Do not build anything yet. I want a review first:
A. List every current feature, page, layer and gesture (use GUIDE.md section 19 as the checklist). Mark each one KEEP (already feels like its reference device), CHANGE (right job, wrong feel: say what the reference device does instead) or CUT (serves none of the three). For anything the MPC Sample already does, default to CUT. Say plainly which items you are unsure about.
B. Say where the FM-1 cannot match the reference devices (four knobs, no pads, no analogue filter) and the closest honest substitute for each.
C. Tell me whether it is simpler to keep editing the current interface or to rebuild the interface from a blank page on top of the existing engines, drivers, simulator and build pipeline. Base it on the code, with a rough size for each route. Say what would be lost either way.
D. Include a visual review from a graphic design point of view: each kept screen should look like it belongs to its reference device's way of working.
Give me the result as a short grouped list, then put every CUT and every real choice to me as multiple-choice pickers. Known open choices: fold ACID and JUNO into one oscillator Type list or keep separate engines; 16 drum sounds or the Rytm's 12; keep or drop the snake view; whether song mode, the generators, CLOUD and the screensavers stay.
Things I have already decided today stay decided unless they clash with this direction. If one clashes, show me the clash and ask.
Ask me before starting any long test run."

## Direction decisions (Ben, 21 Nov 2026, evening, by picker; the questions and context are in REVIEW.md)
- Route: "Hybrid" (keep drawing code, menus and rescue screens; replace the page layout, step editor and screens one module at a time).
- Save format: "Accept" (new format; PHEBUS reads old projects; stock KARMA cannot read new ones).
- Drum model: "Per-sound trigs, one track" (each sound's hits get their own velocity, condition, nudge, retrig and locks; all sounds share one pattern length).
- Drum sounds: "16 sounds".
- Oscillator types: "Same layout, widen in stages" (every voice gets the same pages now, with ACID and JUNO on every track; the big multi-model list on every track only after a processor check on the real FM-1).
- Filter envelope: "As decided: ADSR + link" (a dedicated filter envelope page with a LINK AMP/OWN switch).
- Mod matrix: "A: grid".
- Locks: "Rytm way" (hold a step and turn: whatever is on the knobs gets locked; page buttons still change page while the step is held; locked values shown inverted).
- Song and sections: "Keep A-D as patterns only" (four patterns you switch between, queued to the bar; cut chains, the SONG screen, song record and the songs bank).
- Generators, in his words: "Cut all but give me the spice & dice from minifreak".
- Stutter lane: "Keep, and record played FX into it" (it becomes PHEBUS's FX track).
- CLOUD, TEXTURE, SLICER, in his words: "Slicer sounds fun. Cut the others".
- Characters: "Heads in the strip only".
- Snake view: "Drop it".
- Screensavers and HOME band: "Keep both".
- Old extras to cut (all four ticked): "Sample kits", "Free take", "Screen reader", "Web editor (keep backup + install)".
- Drum pages: "B: four dials over the knobs".
- Drum kits: "Make every kit editable" (keep the 606 start; turning a knob on any kit makes your own editable copy).
- Performance: "A: pad map, trimmed set" (about 18 effects: straight stutters 1/1 to 1/32, reverse, half, tape stop, sweeps, gate, crush, echo, kills, throws; latch; knobs tweak the last effect pressed).
- HOME: "B: three zones" (SYNTH, DRUM and PERFORM panels, one per pillar), keeping the knobs cutoff, decay, stutter, level.
- OSC page: "A: Type + three macros" (knob 1 picks the Type, knobs 2 to 4 are its three macros, renamed per type; volume on HOME and the mixer).
- Held build (step-record, HOME knobs, ACID bass, stutter lengths, LFO PATT, HOME band): "Merge it".
- Order: "Drums first", after a first wave of cuts and the new save format.
- "Start building".
- After wave 1 (Ben, by picker): web tests "Run it now"; stutter headroom "Keep 12 dB"; scene backgrounds left behind sound-page graphs "Cut them". Asked how to pace the rebuild, he wrote: "What even are the waves that you're building?"

### 22 Nov 2026, 00:49

Asked whether to pause after wave 4, Ben said: "Noo build the thing we dreamed of. If you're confident". All seven waves are to be built.

### 22 Nov 2026, 00:50

On the real-device CPU caveat, Ben said: "Let's worry about that once it's built. Make the perfect version. Then we'll compromise".

How this is applied: no feature is trimmed or staged in advance out of worry about CPU headroom. That includes the big multi-model oscillator list on every track, which was to wait for a device check and is now built in wave 4. Anything that turns out not to fit is reported to Ben as a compromise to choose, never dropped quietly.

### 22 Nov 2026, 23:20: the layout, in Ben's words

"Does the way that you interact with the device feel consistently implemented and obvious? Like do the knobs do the same job on every screen wherever possible? Is it obvious on the screen what the knobs control? Could it be more obvious what the knobs do? What the keyboard does? The quickest synths to learn are the ones that have a really clear logic - like the mpc sample - everything is either written on the knob or written below the knob and access by a shift press. The most hidden things get is a long press on a button."

"Are the pages unique? Or do they exist in lots of places at once ( I think this is more confusing). Like fx? I feel each voice's fx sends should be on their own edit page and the fx only does the master fx."

He asked for an audit, not a build (review/LAYOUT-AUDIT-2026-11-22.md). His answers by picker afterwards:

- FX button: "FX = whole mix only". Each voice gets three inserts plus a SENDS page under EDIT. FX tap is the master delay and reverb pages, FX hold is PERFORM. GLO is settings and the mixer.
- Key legend on every screen: "Leave as built".
- Hidden depth: "One rule everywhere". One latch gesture, one keep-open gesture, one confirm gesture, no double presses. Nothing deeper than a hold.
- Same words: same filter page on drums and synths (CUTOFF, RESO, ENV, TYPE; one spelling everywhere); one repeat word (RATCH becomes RETRIG; ROLL stays for held keys; SLICER decided separately); cut the spare filters (the hidden CC 74 filter and the GLO DJ filter; PERFORM LP and HP do that job); tempo in one place.

### 23 Nov 2026: after the layout wave, by picker

- SLICER on each voice: "Keep as it is".
- Tap tempo: "Leave it out".
- Reaching a voice's own effects in two presses: Ben asked "What is actually on the ENV button?" (open).

### 23 Nov 2026, 08:25: how to move around, in Ben's words

"I don't understand how you'd ever find something that was a combination of button presses like ENV then EDIT. That's a bad button logic. I'd rather the voice effect sends were just another page in FX."

"As I see it. The FM1 has 3 ways we can navigate fairly effectively:

1. Press a labelled button and obviously associated features pop up on screen with knobs 1-4 controlling parameters. Continuing to press the same labelled button walks the pages of that category. I like this one as it costs not very much in knobs or long presses.

2. Press a labelled button and then use a rotary controller to walk through the pages of the category. Maybe the select knob. Maybe is faster but we lose the select knob. Plus I feel the select knob would be better acting as a knob 5 effectively changing parameters within the category that have a type - oscillator type or type of distortion engine for example.

3. Long presses: good for a quick hidden shortcut only for the things that users want a lot for performance flow. But to be thought of as an additional quick short cut rather than a main way to move around or select things.

4. Button combo presses are the least pleasant of all. Maybe ok if one of the buttons is always the same and then the shortcut you're triggering is an obvious linked feature to the button you're pressing. Like -oct and sequence could halve the sequence. Or +oct and sequence could double it as a clone"

His answers by picker, 23 Nov 2026 (these replace the 22 Nov answer "FX = whole mix only" where they differ):

- FX button: "Voice FX, then mix FX". Pressing FX walks this voice's inserts, sends and slicer (voice colour), then master delay, reverb, master (white).
- Filter home: "EDIT: osc, filter, alternating". EDIT press 1 oscillator, press 2 filter, then the deeper oscillator and filter pages.
- SELECT knob: "Yes, SELECT = type". SELECT chooses the type on the current page (oscillator type, insert type, filter type, LFO wave). Pages are walked only by pressing the labelled button again.
- When: "Build it now".

### 23 Nov 2026, 08:42: a page button, in Ben's words

"I think it would be cleaner for moving through pages if we used a button like SEL or SAVE to start walking pages. So the standard navigation would be something like:

Hit the edit key, which brings Osc and Filter categories. And if the Osc category has 4 pages, you cycle through those using the Sel or save button, if you press edit again it moves to the next category of Filter?"

This revises the 08:25 answer "EDIT: osc, filter, alternating": a labelled button steps through its categories, and one page button walks the pages inside the current category.

His answers by picker, 23 Nov 2026, 08:50:

- PAGE button: "SEL". SEL becomes PAGE. Its old jobs (scale, drum mutes, drum sound pick) move elsewhere.
- Drum sound pick: "SELECT knob". On the drum screen, turning SELECT steps through the 16 sounds. The sound's wave or model goes on a normal knob. (On synth pages SELECT is the type, as answered at 08:25.)
- Key, scale and chord: "Under ARP". ARP steps through: arpeggiator, then key/scale/chord.
- Drum mutes: "On the mixer". Hold GLO: with the drums selected, the keys mute the 16 sounds; voice mutes stay on keys 1 to 4 otherwise.

### 23 Nov 2026, 09:08: what each control is for, in Ben's words

"I don't mind the octave button working as + and - that's actually pretty logical and maybe we should double down on that. Same with arp working as a more general notes / scales key. Or a performance button that controls notey or scaley perfomance controls. Same with Seq being the equivalent for rhythmic controls.

Global should be the place for any and all global settings where there isn't a more obvious place; such as global fx living under the fx tab.

Algorithm is fine selecting voice.

I feel the master knob is underused as I won't be changing master volume very often. Would make sense if it was bound to the other controls as a button press and twist master and it changes a broad / master like parameter that's related to the button you're pressing. Like it could be a global fx dry wet? If the fx button is held and master pot is turned.

Couldn't you free up a lot of the oct + and - bindings by making the actully oct +- keys require a double tap to change octave. Then have the + and - function according to whatever is on screen?

I don't like the record button taking things away. It's ok to have it as a confirm button. Though wouldn't save be the logical confirm button. Or select?

The sequence button should be in charge of patterns somehow.

Holding - and edit should clear the whole voice. Edit shouldn't be the clear or erase button. Oct - should be. Transpose should be something like arp and octave + -.

Global should have midi learn. But not fill. Fill should be arp, seq. Maybe hold arp or seq and press play/stop

How can we make the black keys un hidden?"

Also, at 08:47: "To my mind, the select button had no business having control of the mutes."

His answers by picker, 23 Nov 2026, 09:20:

- Octave: "Double tap for octave". One exception to the no-double-press rule. A single tap of OCT- or OCT+ is always the screen's minus/plus.
- Undo and redo: "SAVE with -/+". Hold SAVE: OCT- steps back, OCT+ forward.
- Black keys: "Only live when drawn". A black key does something only if the screen draws it, in the same strip PERFORM uses. Otherwise it is a note or nothing.
- MASTER pairs: "Yes, those pairs". Hold a button and turn MASTER: FX = whole-mix effect dry/wet; SEQ = swing; ARP = transpose; EDIT = this voice's level; ENV = envelope length (all stages); LFO = modulation depth; GLO = tempo.

Proposed by Claude from his 09:08 words and not objected to (to be built, for him to confirm): fill is hold SEQ and press PLAY; patterns A to D are a SEQ category, storing a pattern stays with SAVE; the confirm is hold SAVE with the ring; REC only records; clear a voice is OCT- with EDIT then confirm; erase is OCT- with the thing held; MASTER acts relatively so volume never jumps; the settings menu behind hold HOME moves into GLO.

### 23 Nov 2026, 10:16: the filter page on drums and synths

Asked whether the fourth filter knob should match (DRIVE on synths with TYPE on SELECT; TYPE on drums), Ben said: "Keep it as is, drums will always feel a little different and that's ok".

### 23 Nov 2026, 10:33: the sequence staying on screen, in Ben's words

With two simulator screenshots (the GRID still showing after pressing EDIT, with only the OSC knobs under it): "Kind of annoying that the sequence stays on screen even when you go to the edit page? Means you're editing with none of the edit view controls"

How this is applied (Claude's reading, for Ben to confirm): the steps show only on SEQ. Pressing EDIT, ENV, LFO or FX with nothing held always opens that button's real page, with its picture and the keys playing notes. Only while a step is held do those buttons bring their knobs under the steps, for a lock; letting go of the step returns to the SEQ screen.

### 23 Nov 2026, 10:41: hold to map, in Ben's words

"Would be great if anything that's a mappable thing... like the LFO usually wants things mapped to it. It would be cool if that happened by holding the LFO button and then it jumps back into the page you were just on, gives you the same controls and you simply wiggle what you want mapped to the lfo. same for the ENV button?

So if you wanted to map cut off to the LFO, you'd navigate to the synth's filter page, then hold LFO it would probably flash up the LFO page until it realises you're doing a hold to map, jumps back to the synth's filter page and gives you the mappable controls back on the knobs etc to map to the LFO...

Would be awesome if the same logic were applied to all the nice and obviously pleasantly mappable parameters"

His answers by picker, 23 Nov 2026, 10:50:

- Which LFO: "The one last shown". Its name shows while you hold, and SEL flips to the other.
- Which ENV: "The one last shown". Its name shows while you hold, and SEL flips to the other.
- Velocity and key: "Matrix page only".

To be built as its own wave after wave 10.

### 23 Nov 2026, 10:45: the lights, in Ben's words

"I can't see how the buttons illuminate but I'm sure it would be helpful if they aren't doing it already, that all buttons that are related combinations illuminate to make it obvious that there are "hidden" controls under those buttons?"

To be built with the hold-to-map wave: while a button (or a step) is held, every button and key that does something with it lights, and those that do nothing go dark.

### 23 Nov 2026, 10:46: how much of a disco, in Ben's words

"I also see that the white key buttons can do all kind of mad dances. We should have settings for how much of a disco you can put on in the global settings. For example, it might be a dick move to have it flashing for 11hrs on a flight to Mexico. But I'd want it flashing if I was ever shooting some content with it"

To be built with the lights work: one setting under GLO for how much the lights move, from none to a full show.

### 23 Nov 2026, 11:02: changing one drum's sound, in Ben's words

With a simulator screenshot (drum screen, RIM selected, kit HEX BOOM, the bar "HOLD SAVE: REPLACE 606*"): "Kind of confusing how to change the sound of individual drums. I could do sound design on a preset snare but couldn't see how to load in like an 808 snare with a 606 kit preset. Or just choose the snare engine and then do sound design. Bit confusing. Also it seemed like it wanted me to hold save to change away from loading a 606 snare or something?"

His answers by picker, 23 Nov 2026, 11:10:

- PRESETS knob on the drums: "Kits on HOME, sounds on the drum screen". PRESETS loads whole kits when you are on HOME, and single sounds when you are on a drum sound page.
- Sound list: "Same kind first, then all". On the snare slot you get every kit's snares first, then everything else.
- Engine: "Yes, an INIT per engine". The sound list includes a plain starting sound for each drum engine.

Proposed by Claude and not objected to (for him to confirm when built): the kit is 16 slots in the project, each holding any sound from any kit; no separate edited copy and no star; loading a whole kit replaces all 16 and is the only thing that asks for a confirm. To be built as its own wave after wave 11, with the memory cost worked out first.

### 23 Nov 2026, 11:33: the colour code, in Ben's words

"I've actually been a bit silly... I use a colour code for my tracks. That teal is meant to be drums. The others are for synths. My normal colour code is:

Blue for bass
Surf green for drums
Purple for synths
Coral for vocals

Happy to keep it with just swapping the surf green to drums"

Applied as: the drum voice is teal (20,196,170) and voice 3 is coral (255,128,108). Voice 1 stays purple and voice 2 blue. This replaces the earlier colour rule wherever it named coral for the drums and teal for voice 3.

### 23 Nov 2026, 13:35: five open questions, answered by picker in Ben's words

- Coral beside record red: "I think it's fine tbh. The red for recording is very different. I would prefer the crocodile to stay as the green character and the snake as the coral one though, which would make the crocodile the new drummer please."
- The drum screen's step row: "I don't need to see the sequence on screen, it's shown on my keys. It would be nicer to be able to change the sound of the individual drums though? I thought we changed that to the preset knob when we had a specific sound in view?"
- Drum erase against ghost hits: "Move ghost hits elsewhere". OCT- with a key is only ever erase. Ghost and hard hits come from velocity on the TRIG page.
- "QUIET also dims the screen".
- "Hold LFO maps on drums too". On a drum sound, hold LFO and wiggle a knob to make that knob the sound's LFO destination (one per sound).

Claude's reading of the second answer, for him to confirm: the row of steps comes off the drum screen; the keys stay the steps and their lights show the pattern; the freed room on screen goes to the selected sound. PRESETS choosing the sound in view is the wave 12 work already decided at 11:10.

### 23 Nov 2026, 14:03: stutter keys and the iso EQ pads, in Ben's words

"The black keys on the stutter sequencer should just do stutters / beat repeat fx. There should definitely be 3 iso EQ fx on the pad FX play mode too, iso lows, iso mids and iso highs - set to Daft punk values of 220hz bass 1.1khz mids, high 1.1khz +"

Claude's reading, for him to confirm: on the STUTTER lane screen the black keys play stutters by hand (the lengths), drawn in the strip. PERFORM gains three pads, ISO LOW (below 220 Hz), ISO MID (220 Hz to 1.1 kHz) and ISO HIGH (above 1.1 kHz); holding one lets only that band through, holding two lets both through. To be built as the wave after wave 12.

### 23 Nov 2026, 14:13: the PERFORM set, in Ben's words

"Just the 3 iso, meaning isolate, as in - I want to hear this band. And having just those 3 gives me all the filtering I'd need for sure. Maybe a motion filter could be cool if there's room. Most of the fx I'll use are broad stutters. Subtle bit crusher that just makes things feel a little thin and papery / gritty. Phasers, jets, anything vowely. I like 1/2 time. A delay throw. A reverb throw. And three iso EQ bands. Some fast performable mutes would be amazing:

Mute kicks and low tom
Mute all drums but tops - snare, clap and hats, shaker
Mute all drums but shaker and clap.
Mute all drums but hats and snare.
Mute voice 1
Mute voice 2
Mute voice 3

If you could play all those mutes and fx in one place that would be a power house for keeping beats varied in daft punk style drops"

His answers by picker, 23 Nov 2026, 14:25:

- The four spare black keys in PERFORM: "Reverse, tape stop, and 2 filter sweeps - one that moves up, one that moves down and you can set the length/duration, resonance, range and slope"
- Mutes: "Hold to mute, latch with OCT+".
- MOTION: "Tempo-synced sweep". A band-pass that sweeps up and down in time with the beat; knobs set rate and depth.

### 23 Nov 2026, 14:19: the mixer screen, in Ben's words

With two simulator screenshots of the mixer (hold GLO) with the drums selected, showing "KEYS: SOLO" and "KEYS: MUTE": "This global screen is kind of hard to figure out. It also doesn't seem to work - like it says keys solo. What voice is the keys? It didn't solo anything either way."

### 23 Nov 2026, 14:22: the screensaver's lines, in Ben's words

With four simulator screenshots of the FLOW visual crossed by horizontal glitch lines, one of them a full-width white line: "Also these horizontal lines that scroll vertically across the screen come over very harsh. They should be super infrequent. And I don't like the white one at all. Also, don't know where to find the other screensavers / graphics now? Would be cool if it loaded a different one at random every time behind the Phebus boot and for the screensaver on the home button"

### 23 Nov 2026, 15:44: three things from the simulator, in Ben's words

"Ohh the decay that is mapped to the home of each device is a filter decay! haha... it should be the amp envelope decay.. sorry, my lack of clarity there.

Knob one in the screenshot doesn't pick a type. Select knob does.

Not sure where the master stutter sequence has gone?"

His answers by picker, 23 Nov 2026, 15:55:

- A home for PERFORM and the stutter sequence: "Both on FX as you've recommended but don't walk the PERFORM page. The perform page is only in the long press for FX button"
- What FX opens on: "Option 2 - but obvi without the Perform page." (Option 2 was: this voice's inserts, then sends, slicer, then the mix group.)

So tapping FX steps: inserts, sends, slicer, then MIX, then STUTTER. PERFORM is reached only by holding FX. The stutter sequence leaves the ALGORITHM knob.

### 23 Nov 2026, 20:56: mixer keys, the artwork, drum sends, in Ben's words

With a screenshot of the new mixer and one of the THIRD EYE visual:

"At the moment the mixer page is pretty good. But would work better if the keyboard keys were grouped by track rather than by row.

So there are 16 white keys:

1-4 voice 1
5-8 voice 2
9-12 voice 3
13-16 Drums

Can we slightly edit the artwork too. I think this about the big EYE over the desert pic;
It would be cleaner if the eye had no white outline. And if the eye was slightly bigger. And remove the 2 black outlines to the eye shape also. White also removing the outlines of the coloured rolling sand dunes too. And clearing the thin black line on the sand dunes also. So a little less noisy and minimalist all over. The colours within the eye are phenomenal. The rolling dunes are great. The background is great and the movement of the eye is great.

Please match the dunes in the other desert view with the sunset on a starry night. Just the dunes need an edit in that one. The starry night and sunset are fantastic. The sun itself is majestic and could be a tiny bit bigger.

If you still have the dancing characters, could you make one more artwork that is all 4 of them dancing together? Maybe on a tropical beach? With a trippy ocean lapping the shore line?

The drums having one global slicer and fx sends is problematic. Delay for example, great on the tops, horrendous on the click and low tom. If it isn't too much, individual sends for every drum voice? OR we could simply have a routing option on the sends for the drum voices that lets you exclude voices from the drum voice's sends? That's arguably the leaner, cleaner build?

Can you maybe just explain the K1 lights and colour section in global for me?"

His answers by picker, 23 Nov 2026, 21:10:

- The mixer's keys: "OHHHH I'm so sorry I've just seen the screen, totally my misread, that's where the kit's 16 mutes and solos are and then the top black note keys are the other voices and drum group mute. OK, that's fine to keep them as is, but please rearrange the way the white keys with the 16 drum pad grid. Can you make notes 1-4 mute the drum voices in the left column, 5-8 mute drum voices in column 2, 9-12 mute the drum voices in column 3 and 13-16 mute the pads in the right hand column. My mistake"
- Drum sound mutes moving: "forget about it".
- Drum effects (per-sound sends exist on AMP page 2; whole-kit sends, slicer, insert and filter on the drum FX tab): "leave it".

Applied as: each key still mutes the same drum sound as before; the mixer draws the 16 tiles running down the columns, so keys 1 to 4 are the left column.

### 23 Nov 2026, 21:07: the drum grid like the Rytm's, in Ben's words

"I would however prefer the drum voices were depicted in their 16 note grid in a format that more closely followed the analog rytm's layout.

Cymbals near the top. Toms on the second row. Kick in the bottom left with snares in the right hand corner and hi hats in the top left corner."

By picker, 21:15: the first sketch (HAT, OP HAT, CRASH, RIDE / LO TOM, HI TOM, CONGA, C-BELL / PEDAL, SHAKER, RIM, CLAP / KICK, KICK 2, SNARE 2, SNARE) was "Close, I'll describe changes" (his changes awaited). Keys within a column run "Top to bottom". Use it "Everywhere the 16 are drawn".

### 23 Nov 2026, 21:14: the drum grid, as Ben typed it

"Col 1	Col 2	Col 3	Col 4
HAT	OP HAT	RIDE CRASH
PEDAL	CONGA	C-BELL SHAKER
KICK 2 LO TOM	HI TOM CLAP
KICK RIM SNARE 2	SNARE"

Read as four rows, top to bottom:
HAT, OP HAT, RIDE, CRASH
PEDAL, CONGA, C-BELL, SHAKER
KICK 2, LO TOM, HI TOM, CLAP
KICK, RIM, SNARE 2, SNARE

With keys running top to bottom in each column: keys 1 to 4 are HAT, PEDAL, KICK 2, KICK; keys 5 to 8 OP HAT, CONGA, LO TOM, RIM; keys 9 to 12 RIDE, C-BELL, HI TOM, SNARE 2; keys 13 to 16 CRASH, SHAKER, CLAP, SNARE.

### 23 Nov 2026, 21:30 and 21:35: drum effects belong under FX, in Ben's words

"I'm fairly certain that the drum's sends and stutter are all working globally and changing one voice's settings is changing them as global settings for the kit"

"I don't like that the FX are in env. ENV should be for dry sound design / modulation. Edit should be for the same. FX should be for FX. Why is it different for the kit, I vaguely remember you suggesting it would be a good compromise?"
