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

### Sound design layout

> "The menu diving is intense and doesn't yield significantly better results. I would want a sound design layout that matches the flow and ease of the Minifreak. Oscillator engine > Filter > ADSR > FX + MOD matrix. We could leave the mod matrix to the end. But their concepts for small builds on synths are definitely the best and most fluid sound design."

## Claude's notes

- GPL-3.0-only: if this build is shared with anyone, the full source must be shared too. A public fork covers that.
- The hardware build needs JieLi's Linux toolchain and the AC79 SDK. Neither is reachable from Claude's cloud workspace, so builds run on GitHub Actions in Ben's fork (or on his Mac with Docker).
- Newer FM-1 units have a changed chip; check Ben's unit before the first flash.
