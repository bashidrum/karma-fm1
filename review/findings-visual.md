# PHEBUS visual design review (subagent report, saved by orchestrator; branch ben, 20 Nov 2026)

Evidence PNGs and mock-ups in /home/claude/review-work/vis/ named after the finding id; sheets in vis/sheets/. Note: build/gen must be regenerated (tools/build.py generate()) before rendering or boot shots are stale. Contrast ratios against black in brackets.

Verdict: bones are good (one header band, knob row, track bar, black ground, tight palette). Biggest weakness: knob-row typography. Colour rule still broken in menus, shared pages, PUNCH, solo marker. Mute/solo not Ableton-style on the track bar; under deuteranopia purple and blue merge and teal reads as grey. Boot, mixer, KEY, record screens near finished; characters strong at full size, some poses touch their frame, snake icon reads as a duck.

Model screens: live-rec-ready / live-rec-countin; layer-key; layer-mix; chain-acid-03 (FILT); boot-1; vis-09-ORBIT, vis-10-WIRES.

F01 Knob-row values: one face, one baseline, a real 1. All knob rows (chain-acid-01, page-midi, chain-elem-05, page-step, browse-t1 vs t2, chain-acid-15 PHS "|", chain-acid-10 PAT "|"). op_knobs picks renderer by string length: 7-segment 26 px, Terminus 2x 32 px, Terminus 1x 16 px; bottoms at y 197/201/203/204; same field changes size between screens; the 1 is a 4 px bar so 11 reads "||". Change: one baseline row y 50 (screen 204); 2x when value fits 56 px else 1x same baseline; unit G3 (118,118,126) 3 px after; 1 as 12 px cell with centred 2 px stem, 2-step flag, 8 px foot. Alternatives: A Terminus Bold 2x for numbers too; B keep segments with fixed 1. Medium. F01_value_type_now.png, F01_value_type_options.png.

F02 Voice colours on shared things. menu-*/page-menu-*: K1/K2/K3 labels and bars purple/blue/teal. page-global/master/midi/system/texture: purple ACID tag and robot. layer-punch/fx-throw/locked: PUNCH title coral (ui_layers.c:714), row stripes TE_DIM of a different voice per row, FILTER/DUST/DUCK dials purple. chain-acid-11: SEND (per voice) drawn white. Change: menus active white, idle G3; shared pages tag "ALL" G4 outline, no robot; PUNCH title white, stripes G2 or none, dials G4 except TRK FLT in voice colour; SEND in voice colour. Small. F02_voice_colour_rule.png, F02_voice_colour_mock.png.

F03 Mute and solo on the track bar (chain-*, page-gen, page-dgen, browse-*, layer-mix). Mute only dims tile to TE_DIM (1.7:1), no yellow; solo a 6x6 square in TE_SOLO (60,190,255) close to alien (40,160,245); mixer S badge over alien same blue. Change: mute 10x14 badge at tile x+42,y+7, black M on (255,196,40), tile outline and number G3; solo same badge black S on ice blue (150,215,255), inverted on selected tile; same ice blue for mixer S. Alternative: letters only (M yellow, S white). Small. F03_mute_solo_now.png, F03_mute_solo_options.png.

F04 Colour-blind safety. Deuteranopia: robot (140,118,240) vs alien (40,160,245) 1.17:1 apart (1.25:1 normal); croc becomes (166,168,172) = neutral grey; snake, mute yellow, record red all olive. Missing secondary cues: mixer 16-box M/S row, HOME level bars, coloured knob labels. Change: robot (124,92,236), alien (70,186,255) (2.1:1 apart; hue change needs Ben's eye); track numbers on colour-only M/S boxes; keep REC as word/disc and mute as letter M. Small to medium. F04_colour_blind.png.

F05 Characters touch their frame; strip trims the feet. Robot UP (hands), robot STRIKE (clamp), croc still and STRIKE (tail, jaw), snake STRIKE (tongue) cut flat at 56x52 edge. Scene sprite top y 40, bottom 144, strip starts 141. Change: 2 px inner margin in every frame; sprite top y 36; cap hop at 2 px. Medium. F05_character_clipping.png.

F06 Sound-page header repeats the engine ("ACID" tag then "ACID TB ACID"); preset is dimmest text (G3); 70 px ELEMENTS tag crowds tempo. Change: line 2 preset only in white; engine tag 5 chars or fewer (ACID, JUNO, ELEM, PLTS, DRUM) fixed 46 px, page name at x 52; note readout G3. Small. F06_header_redundancy.png.

F07 Track bar reads as page tabs. Change: status strip: 18x16 head + number (1,2,3,DR) 1x; selected = 3 px underline and number in voice colour, others G3; M/S badges per F03; 1 px G1 rule above. Alternative B: G2 frames with the head inside. Small. F07_trackbar_options.png.

F08 Shared list pages (page-global, master, midi, system, texture) show the same four values twice (list and knob row) with different renderers. Change: drop knob row on list pages; four 44 px rows to y 205, value 2x right-aligned x 232. Alternative: keep knob row, drop list. Medium. F08_global_pages_duplicate.png.

F09 GRID cramped labels; held layout flips. Note names touch 28 px tile outlines; slide/tie marker jammed (stray dot after "A#2"); held step: synth strip on top/panel below, drums the reverse; panel repeats STEP/VEL. Change: note without octave centred, octave as 1 to 4 bottom ticks; marker 3x3 square 2 px in from top-right; synth order for both; remove STEP/VEL from panel. Medium. F09_grid.png.

F10 Touched value turns white on voice pages (page-env DEC, layer-mix SYN1, grid-synth), so it looks global. Change: keep voice colour; mark hot with 3 px underline and inverted label. Small. F10_hot_value_white.png.

F11 Information below 3:1: muted/silent tiles TE_DIM (1.6 to 1.9:1); grid-synth-lock unlocked values G2 (1.75:1); page-env "00/GATE" G2; HOME step dots 1 px G2; knob value-position bars 1 px MID on G1. Change: nothing informative below G3 [4.7:1]; step dots 2x2; position bars 2 px, G2 track, full voice fill. Small. F11_low_contrast.png.

F12 SONG page leaves rows 40 to 41 uncleared (header canvas 40 rows, list from 42, ui_song.c). Change: header canvas 42 rows. Small. F12_song_residue.png.

F13 Visualisers: 11 exist, not 7. Keep ORBIT (best), WIRES, BOUNCE, VU-METERS (correctly voice-coloured: each column/ball is a voice). Drop TAPE, SPECTRUM, SPECTROGRAM, CIRCLE, LISSAJOUS, LCD, OSCILLOSCOPE. F13_visualisers_keep_drop.png.

F14 TAPE screen: PHEBUS in pixel letters, 0 px gap to A tile; VU scale numbers under the needle. If kept: smooth wordmark 72 px wide at x 84; VU labels outside arc. Small. F14_tape_screen.png.

F15 Envelope page: MID ghost same weight as 1 px curve; "00/GATE" G2 unclear when idle. Change: ghost TE_DIM; curve 2 px full voice colour with 3x3 handles; hide 00/GATE until a note plays, then G4. Small. F15_envelope.png.

F16 Characters style drift: alien 1 px limbs, faintest at 36x34; croc at mixer size is speckle; snake 18x16 head reads as duck/dinosaur. Change: alien 2 px limbs + full outline; croc mixer: no speckle, 3 ridge bumps, light belly; snake head 14 px wedge, slit eye, 2 px white forked tongue, no tooth block. Medium. F16_characters_big_poses.png, F16_characters_small_sizes.png.

F17 PUNCH tiles uneven: black-key tiles vary in width, float off the grid; LOOP16/LOOP32/REWIND/LOOP8T 1 px padding. Change: black-key tiles 40 px centred on the gap; labels 5 chars (LP16, LP32, RWND, LP8T, OCT+); 3 px min padding. Small. F17_punch_tiles.png.

F18 SONG layer blank tiles (keys 9 to 12, 15) drawn as filled G1 with no label. Change: 1 px G1 outline only or omit. Small. F18_song_layer_blanks.png.

F19 FILTER page header: tag says SYN1 (others show engine), line 2 "- -", "HP11 %" stray unit. Change: engine tag and preset line as other chain pages; no unit on mode names. Small. F19_filter_page.png.

Clean: boot (boot-1 most Barbican; boot-5 blue+teal only is weakest, consider excluding that pairing); page-splash; record family; mixer apart from F03/F04/F10; KEY, STEPS, ERASE, ROLL layers; browse apart from F01/F06; toasts.

Taste options: O1 all values in Terminus 2x (calmer, more Elektron). O2 replace HOME tape reels with the four characters dancing, give the bar counter that space.
