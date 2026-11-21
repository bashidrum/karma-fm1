# Visual review against the three reference devices (subagent, 21 Nov 2026; saved by orchestrator, condensed)
Files in /home/claude/review2/: sheet-01..09 (all families), ev-01..19 (evidence), mock-*.png (240x240 at 3x). Note: chain-*/snd-* test shots show "LEARN" in the header because the test leaves MIDI LEARN on (LEARN should be a small badge, ev-04). Note: reviewed branch ben, which does not have f-play (HOME knobs, FLOW strip, stutter 1/4 default) merged.

## Reference grammar
MiniFreak: panel does the layout; small OLED shows what you just touched (name, value, small picture; test-tube meter for osc/FX knobs); sections in signal order, one row of always-live knobs each; Type remaps the same three knobs and the screen names them; filter type is a button; envelope screen draws the curve with values beneath; FX three slots each Type/Time/Intensity/Amount; mod matrix a lit grid, screen shows the routing's amount, inverts in edit; step screen shows index, length, notes, velocity; browser number/name/category, asterisk when edited.
Rytm: monochrome high contrast; top line kit name turning into the full parameter name while turning, tempo, track, pattern always shown; each parameter's screen position matches its encoder, three-letter labels; five fixed pages in fixed order (SRC SMPL FLTR AMP LFO) on lit keys; locked parameter drawn inverted; trig keys red = note trig, yellow = lock trig; page LEDs; mute mode lit/unlit; chromatic mode; retrig side menu and per-trig pop-up; fill held/latched; plain name-list browsers.
MPC: 4x4 pads are the interface, screen supports; Pad FX one effect per pad, held = on, pressure = depth, up to four, latch, three knobs set the held effect's parameters; Beat Repeat divisions; Note Repeat divisions on buttons; mutes on pads (red muted, yellow unmuted on MPC Sample); MPC Sample screen thinly documented.

## Findings
Melodic sound pages (fits: strip in signal order, wave/filter/env/LFO graphs, one voice colour):
- S1 values collide and change size in one row (snd-lfodest-b "-34%-34%-34%33%", snd-filter-juno-a "100%0%", snd-voice2-a "+21%OFF"; ev-01). Fix: fixed 59 px columns at x 4/63/122/181; number always 2x condensed max 4 glyphs at y 180; unit 1x grey after; abbreviate long values.
- S2 the Type is the smallest thing on the OSC page (ev-11). Fix: Type name 2x white in the body with "TYPE n/12"; waveform right. mock-synth-osc-A.
- S3 abstract bar boxes on VOICE, VOICE 2, ENV DEST, LFO DEST mean nothing (ev-10). Fix: body shows the touched knob: name, value 3x, test-tube meter; falls back after 1 s. mock-synth-osc-B.
- S4 FX page draws sends as four boxes, not slots; CHORUS, INSERT, SLICER separate pages (ev-12). Fix: three slot boxes; K1 TYPE K2 TIME K3 INTENS K4 AMOUNT. mock-synth-fx-A.
- S5 no mod matrix page. Fix: 5x6 grid (sources LFO1 LFO2 FENV VEL KEY x destinations PIT CUT RES SHP AMP FX), dot size = amount, ring = negative; K1 SOURCE K2 DEST K3 AMOUNT K4 CLEAR. mock-synth-mod-A (grid), -B (list).
- S6 filter page header "-" (ev-16); needs a visible tag for which envelope drives it ("AMP ENV"/"FILT ENV"); K4 TYPE; ENV gets AMP | FILT selector. mock-synth-filt-A, mock-synth-env-A.
- S7 page name inside the lit strip box stretches it. Fix: fixed-width stage boxes, page name in the header title, "1/2" at right.
- S8 the voice is shown three times (tag, corner character, strip): taste.
Melodic sequencer (fits: held-step panel, note tiles):
- Q1 locked values full colour, unlocked dim (grid-synth-lock, ev-13): reverse of Elektron. Fix: locked value inverted; a step carrying locks gets a notch.
- Q2 page-pattern empty and duplicates the STEPS layer (ev-18): cut or fold in. Q3 show a mod-lane view (16 values) as the lock view.
Drums (fits: grid-drum-full, live-kit 4x4 tiles, held pop-up, coral):
- D1 HEX pages show "A HEX KIT ONLY" placeholder with values under it on non-HEX kits (ev-02); edited sound's name nowhere but as a value.
- D2 drum strip says OSC FILT ENV FX MOD (ev-03); Rytm order is SRC FLTR AMP LFO.
- D3 snake view: large character, 16 tiny steps, sound name only as a knob value (ev-08).
- Redesign: header DRUM tag, kit, tempo; selected sound name large; tabs SRC FLTR AMP LFO; 16 trigs 13x22 px on 15 px pitch (note trig filled coral; trig with locks a notch; lock-only trig outline + dot; retrig three ticks; held white frame; playhead bar; quarter dots); page bars; locked value inverted. mock-drum-A (8 dials, two rows of four, SELECT swaps rows), mock-drum-B (four dials over the four knobs, paged 1/2, trig row in the body, 4x4 sound map with mute marks).
- D4 mute mode hidden (EDIT + black key), shows red "OFF". Fix: mock-drum-mute (16 tiles, yellow M, ice-blue S).
- D5 rename HEX OSC/MOD/CLICK/MORE to SRC 1/2 2/2, HEX FILT to FLTR, HEX AEG to AMP (with sends); LFO a real page or dimmed tab.
- D6 ROLL layer becomes the retrig menu (RATE LEN VEL curve), "RTRG 1/16" on the held trig; FILL shown inverted in header when active. D7 chromatic mode needs a screen (keys as a semitone strip).
Performance (fits: PUNCH white/grey, held key inverted):
- P1 PUNCH is 27 labels in a pseudo-keyboard, no grouping, no latch state (ev-07). Fix mock-perf-A: title FX: PADS, latch count; 11 black keys as two rows of chips; 16 white keys as a 4x4 pad grid (row 1: 1/1 1/2 1/4 1/8; row 2: 1/16 1/32 REVRS HALF; row 3: LP HP CRUSH ECHO; row 4: GATE STOP DUST WOW); held pad solid white, latched outlined with dot; caption names the last pad; K1 to K3 that pad's settings, K4 AMOUNT. mock-perf-B keyboard-true.
- P2 stutter lane lengths as numbers in tiles: draw spans across the steps they cover (ev-17).
- P3 mixer is a good mute page; keep; home of PAN.
HOME: H1 (ben branch) still tape reels and old knobs; H2 stutter knob must be grey. mock-home-A (recommended): bar counter, names, A to D, 8 px FLOW band, four voice lanes + white STUTTER lane with playhead, knobs CUTOFF DECAY STUTR LEVEL. mock-home-B: three zones SYNTH / DRUM / PERFORM.
Shared: F1 GLO list pages show the same four values twice (ev-05): drop the knob row. Fine: menus, about, browse (add "*" when edited), SEL scale, rec screens, boot.

## Screens that would no longer belong
Arranging: page-song, page-song-held, page-songs, layer-song, layer-song-chain. Generators: page-gen, gen2, dgen, dgen2, ACID GEN / HARMONY / PLAY. page-texture. page-glo-cloud (open). page-pattern. SLICER pages (it is a stutter: belongs on the pads). Folded: chorus > FX slot type; VOICE 2 (PAN, MUTE to mixer; ALLOC, DTUNE to VOICE); FX sends > slot page; HEX placeholders > SRC/FLTR/AMP/LFO. Tape reels. Snake view as a working view (open). Unsure: live-free-take; screensavers (identity, not a pillar).

## Visual choices (reviewer's recommendation in brackets)
1 OSC page: A K1 TYPE + three macros, volume on HOME/mixer; B TYPE on encoder, K4 VOL, touched knob fills body. [A + B's touched-knob view where no graph]
2 Mod matrix: A grid; B slot list. [A]
3 Drum page: A 8 dials two rows; B 4 dials over the 4 knobs, paged. [B]
4 Lock display: inverted (Rytm) or current. [Inverted]
5 Performance: A 4x4 pad map; B keyboard-true. [A]
6 HOME: A lanes + stutter lane; B three pillar zones. [A]
7 Characters: A heads in strip + full figures only in mixer (gentle lean); B also a still figure on sound pages; C as now. [A; Ben's call]
8 Snake view: cut as working view (keep as idle/screensaver pose) or keep behind SELECT. [Cut as working view]
9 Backdrops: A boot + screensavers only; B also the thin HOME band. [ask]
10 Value type: always 2x number + 1x unit. [Always 2x]
11 Stutter lane: numbers or spans. [Spans]

Could not determine: MiniFreak OLED resolution/timeouts; whether the Rytm draws dial icons; MPC Sample screen layout; exact FM-1 black key positions for mock-perf-B; device speed/readability.
Sources: MiniFreak manual https://www.adorama.com/col/productManuals/AR571001.pdf ; MicroFreak manual https://www.adorama.com/col/productManuals/AR570101.pdf ; Analog Rytm MKII manual https://elektron.se/wp-content/uploads/2024/09/AnalogRytmMKIIUserManual_ENG_OS1.70_231122.pdf ; SOS MkII review https://www.soundonsound.com/reviews/elektron-mkii-analog-four-analog-rytm-octatrack ; MPC Sample guide https://www.akaipro.com/guides/mpc-sample/pad_fx.htm ; SOS MPC Live III https://www.soundonsound.com/reviews/akai-mpc-live-iii
