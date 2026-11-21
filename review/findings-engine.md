# PHEBUS review: engine, code, consistency, spec (subagent report, saved by orchestrator)
Head 63f20d1 vs base ba0e39d. Harnesses in /home/claude/review-work/eng/h/.

Tests: 29 suites pass at head and base. ui_pages_test, UBSan, ASan stress, soak, node web tests pass. speak_test FAILS at head (boot went 124 to 180 frames, test waits 160; passes with 200). dsyn_test will not compile at base either (stale test). hostsim TRACKS heap error at base too (test code, tests/hostsim.c:405, :502). Regress goldens: 26 of 117 differ at base and head identically (platform float); all 117 bit-identical base vs head, so no sound changed.

ENG-1 stack overflow: sg_lines (ui_grid.c:526-548) writes up to 49 chars into ln[4][31]; ASan stops at :542 via sg_draw (h/ovf_h.c).
ENG-2 no button reaches FAM_FLT with a step held; filter cannot be locked (h/flt_h.c).
ENG-3 step key released after leaving the GRID clears that step on the newly selected track (h/grid_h.c case C). Fix: remember track at key-down or reset sg_held/sg_pend on leaving.
ENG-4 boot cost about 4.2M host instructions per frame vs 0.31M for an average visualiser; BOOT_END counts frames; comment says 2x2 but code draws st=1.
ENG-7 undo after clearing a locked step restores note but not locks, nudge, condition (h/undo_h.c).
ENG-8 tapping a TIE step creates a note that takes the old note's tail (h/grid_h.c case D).
ENG-9 lock table (24 per track) fills with several steps held: only first steps locked, "NO LOCK LEFT".
Budget (CI log): image 553,240 of 581,564 B (28,324 left); today +10,524 B (characters +12,680); pool 8,208 B spare against an 8,192 B rule (16 B margin, already so at base).
Colour leftovers: ui_layers.c:726 punch tile tops by key row; ui_menu.c:128-129 menu rows by knob; editor.html:156-158 knob columns; cheatsheet legend; GUIDE "in the knobs' colours", "FX wizard".
SPEC-1 backdrops as screensavers not built. SPEC-2 note choice blind. SPEC-3 STRIKE never fires in UNISON (velocity scaled to 45 or 63, threshold 116) nor on legato ACC+SLIDE. SPEC-4 SPEC.md lacked the later wordmark quotes (now added).
Save/load: no new saved state; old patterns load; new ones load on stock KARMA.
CODE-1: on_drum_page/drum_screen_*/SC_DRUM also mean the synth GRID; KARMA_ROW holds PHEBUS; KARMA_COL unused; sloop_splash/sloop_logo_draw/fade unused by firmware; gen_logo KARMA_CELLS unused; OP_PUR is a grey; stale comments ui_op.c:274, ui_draw.c:108, main.c:161, ui_vis.c:724; held-step editing duplicated between ui_grid.c and ui_layers.c, PG[12] copied.
CON-2: ABOUT "KARMA" (ui_menu.c:90), "KARMA USB RESCUE", installer "install KARMA"; installer and simulator pages send counts to upstream's goatcounter; drum GRID ENV/LFO do nothing while GUIDE says drum ENV opens HEX AEG.
SAFE-1: installer, package maker, build.py, loader, hal, app.ld untouched; identity FM-1_926; no chip-revision check anywhere in the install path.
Unverified: boot seed randomness on hardware; real boot frame time; STRIKE missing very short notes; PEN display stale when a MIDI keyboard changes a chord with the same root.
