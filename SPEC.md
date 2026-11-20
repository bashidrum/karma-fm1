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

(none yet: to be filled in from Ben's own words once he has played the simulator)

## Claude's notes

- GPL-3.0-only: if this build is shared with anyone, the full source must be shared too. A public fork covers that.
- The hardware build needs JieLi's Linux toolchain and the AC79 SDK. Neither is reachable from Claude's cloud workspace, so builds run on GitHub Actions in Ben's fork (or on his Mac with Docker).
- Newer FM-1 units have a changed chip; check Ben's unit before the first flash.
