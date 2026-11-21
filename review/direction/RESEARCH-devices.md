# Reference devices: how they work (subagent research, 21 Nov 2026; saved by orchestrator, condensed)

## MiniFreak
- Signal order: Osc 1, Osc 2 > analogue filter > VCA (Envelope) > 3 digital FX. Around it: Cycling Envelope, 2 LFOs, mod matrix, macros, sequencer/arp.
- Oscillator: Type encoder + three macro knobs (Wave, Timbre, Shape) + Volume + tune. Changing Type remaps the three knobs and the screen names them. Examples: BasicWaves = Morph (square > saw > double saw), Sym, Sub; SuperWave = Wave, Detune, Volume; KarplusStr = Bow, Position, Decay; Waveshaper = Wave, Amount, Asym; Two Op FM = Ratio, Amount, Feedback. Types on both: BasicWaves, SuperWave, Harmo, KarplusStr, VAnalog, Waveshaper, Two Op. FM, Formant, Speech, Modal, Noise, Bass, SawX, Harm.
- Filter: LP/BP/HP 12 dB; Cutoff, Resonance, Env/Velo (bipolar amount from the main Envelope).
- Envelope: ADSR, hard-wired to the VCA. Filter follows the same envelope via the Env/Velo knob ("link" is one knob). Own filter envelope (inferred): Cycling Envelope in Env mode (ADSD) routed to Cutoff in the matrix. Retrig: Env Reset / Env Continue.
- Cycling Envelope: Rise, Fall, Hold/Sustain; modes Env, Run, Loop.
- LFO 1, 2: Sin, Tri, Saw, Sqr, SnH, SlewSnH, ExpSaw, ExpRamp, Shaper; free or synced. Trigger modes exactly: Free, Poly Kbd, Mono Kbd, Legato Kb, One, LFO, CycEnv, Seq Start.
- FX: 3 slots in series; per slot Type/Sub, on/off, and Time, Intensity, Amount (meaning changes per type). Types: Chorus, Phaser, Flanger, Super Unison, Reverb, Delay, Distortion, Bit Crusher, EQs, Multi Comp, Vocoder. Only Delay and Reverb can be send.
- Mod matrix: LED grid 7 source rows x 13 destination columns (4 fixed + 9 assignable over 3 pages). One encoder: turn to move, click to edit amount, hold 1 s to zero. Lit = active. Assign: hold Assign, turn any control.
- Macros M1, M2: up to 4 destinations each; touch strips.
- Voicing: Mono, Poly, Para, Uni; glide Time/Rate/Sync with legato variants.
- Sequencer: up to 64 steps (16 step buttons x 4 pages). Step record: stopped, press Record, play up to 6 notes per step, releasing keys advances; Hold/Tie = rest or extend. Real-time record too. Hold a step to edit its notes; tap a step to mute. 4 mod lanes: turning a knob while step-recording writes a per-step offset. Spice and Dice: non-destructive variation. 1 sequence per preset.
- Arp: Up, Down, UpDown, Random, Order, Walk, Poly, Pattern; Repeat, Ratchets, Rand Oct, Mutate.
- Screen: small; turning a knob shows the value as a bar; envelopes draw the curve; knobs never jump.
- Rules to get right: (1) each section one row of always-live knobs; (2) Type remaps the same three knobs, named on screen; (3) filter env amount one bipolar knob from the amp ADSR; (4) knob turn shows value as a picture then returns; (5) matrix a visible grid, one encoder, active points lit; (6) hold Assign + wiggle; (7) two macros; (8) step entry: play keys, release to advance, Tie; hold a step to edit; (9) knob moves in step record become per-step locks; (10) Dice; (11) no jumps.
- Do not copy: mono aftertouch, mono path, fiddly Shaper, 1 sequence per preset, hard-to-find FX subtypes.

## Analog Rytm MKII
- 12 tracks (BD SD RS CP BT LT MT HT CH OH CY CB), 8 voices (pairs share). 12 pads play; 16 TRIG keys place trigs. Track select: hold TRACK + pad; one track always selected.
- GRID RECORDING: REC, TRIG keys toggle trigs; FUNCTION + TRIG = trigless lock. LIVE RECORDING: REC + PLAY. STEP RECORDING: REC + STOP.
- Note trigs fully lit; lock trigs half-bright blinking. Parameter lock: hold trig + turn any knob; locked value shown inverted. Remove one lock: hold trig + press that knob. Sound lock: hold trig + turn LEVEL. Micro timing: hold trig + LEFT/RIGHT.
- Trig conditions: X%, FILL / NOT FILL, PRE, NEI, 1ST, A:B. Fill: hold PAGE while playing; latchable.
- Retrig: per trig (hold trig + RETRIG: on/off, rate, length, velocity curve -128 fade out to +127 fade in). Live: hold RETRIG + pad.
- Accent, swing; SCALE: per-pattern or per-track length and scale; up to 64 steps over 4 pages.
- Pad modes: MUTE (tap pads; RETRIG + pad solos), CHROMATIC (selected track across 4 octaves), SCENE, PERFORMANCE.
- Pages, 8 knobs each: TRIG (NOTE VEL LEN SYN SMP ENV LFO + condition; partly from memory); SYNTH (per machine, e.g. BD HARD: Level, Tune, Decay, Hold, Sweep Time, Sweep Depth, Waveform, Tick); SAMPLE (TUN FIN BR SMP STA END LOP LEV); FILTER (ATK DEC SUS REL FRQ RES TYP ENV); AMP (ATK HLD DEC OVR DEL REV PAN VOL); LFO (SPD MUL FAD DST WAV SPH MOD DEP).
- Machines: double-press SYNTH; each track has its own list.
- Kit = 12 Sounds + FX track; Sound = one track's pages; Pattern = trigs and locks.
- FX track: delay, reverb, distortion, compressor; takes trigless locks.
- Copy FUNCTION+REC, paste FUNCTION+STOP, clear FUNCTION+PLAY; repeat to undo.
- Rules: (1) 16 keys = 16 steps, tap to toggle; (2) hold trig + turn = lock; (3) lock trigs look distinct; (4) track select is hold + tap and never changes the steps shown; (5) same 8-knob page layout on every track; (6) conditions incl. FILL momentary; (7) retrig per trig with velocity fade; (8) micro timing; (9) per-track length; (10) live record then refine; (11) FX lockable.
- Do not copy: kit/pattern save ambiguity, shared voice chokes, deep FUNCTION combos, unnamed patterns.

## MPC Sample (March 2026) and MPC feel
- 16 velocity pads with poly aftertouch, 2.4 inch colour LCD, 3 knobs, a fader. Sampling, chopping, 25 s Recall buffer, patterns to 128 bars, step edit, automation, song mode, resample, swing. So PHEBUS need not sample, arrange or record.
- Pad FX: press PAD FX, each pad one master effect, pressure = depth, up to 4 at once (oldest drops), K1 to K3 tweak the held effect, B1 latches. The 16: Half Speed, Chorus, Flanger, Phaser, Comb, LP, HP, BP, Ring Mod, LoFi, Color, Granulator, Beat Repeat, Rev Stepper, Delay, Reverb. Beat Repeat divisions 1/4, 1/4t, 1/8, 1/8t, 1/16, 1/16t, 1/32, 1/64. Pad FX cannot be recorded into patterns or played while resampling (MusicRadar): a sequenced stutter from PHEBUS fills that gap (inferred).
- Flex Beat: tempo chops/scratches/gates, latchable and recordable.
- Note Repeat: toggle or hold; B1 to B3 pick division; swing follows Time Correct. Mute: pads mute/unmute; hold = momentary. Full Level. 16 Levels.
- Melodee (community firmware) adds tracks, synths, p-locks etc. "SampQOL" not found: unverified.
- Rules: (1) effects momentary while held; (2) up to 4 stacked; (3) pressure = depth; (4) a latch; (5) tempo-synced repeat divisions chosen before the hold; (6) mutes momentary or toggled, unmute-all; (7) Note Repeat hold or toggle with quick rate buttons; (8) effects act on the whole master.
- Do not copy: effects that cannot be recorded; opaque effect menus.

## Mapping to FM-1 (4 knobs, 2 encoders, 27 keys, 240x240)
- Nearly every MiniFreak section has 4 or fewer continuous controls: Osc (Wave, Timbre, Shape, Volume; Type on an encoder); Filter (Cutoff, Res, Env amount, Type); Envelope (A D S R); Cycling Env (Rise, Fall, Hold; Mode on encoder); LFO (Rate, Wave, sync, trigger); FX (Time, Intensity, Amount; Type on encoder). "Section = 4 knobs" is faithful. Matrix grid fits the screen. Assign by hold + turn works.
- Rytm 8-knob pages become two banks of 4. 12 pads become keys in a track layer (hold a button + key selects a track). Macros on the two extra encoders or shift + knob. MPC Pad FX = hold FX + a key, momentary, stack limit, depth from a knob while held (no pressure), latch by double tap. OP-Z precedent: keys as punch-in FX recorded to a performance track.

## Sources
Arturia MiniFreak manual https://www.adorama.com/col/productManuals/AR571001.pdf ; SOS MiniFreak review https://www.soundonsound.com/reviews/arturia-minifreak ; MusicTech MiniFreak https://www.musictech.net/reviews/hardware-instruments/the-big-review-arturia-minifreak/ ; Elektron Analog Rytm MKII manual OS 1.71 https://elektron.se/wp-content/uploads/2024/10/analog_rytm_user_manual_eng_os1.71_241016.pdf ; MusicRadar Rytm MKII https://www.musicradar.com/reviews/elektron-analog-rytm-mkii ; Akai MPC Sample guide Pad FX https://www.akaipro.com/guides/mpc-sample/pad_fx.htm , Flex Beat https://www.akaipro.com/guides/mpc-sample/flex_beat.htm , Features https://www.akaipro.com/guides/mpc-sample/features.htm ; MusicRadar MPC Sample review https://www.musicradar.com/music-tech/samplers/the-closest-thing-akai-has-released-to-a-vintage-mpc-in-decades-akai-mpc-sample-review ; MusicTech MPC Sample https://musictech.com/reviews/hardware-instruments/akai-mpc-sample-review/ ; Loopop MPC Sample https://loopopmusic.com/akai-mpc-sample-does-it-beat-the-competition-5-power-tips-review-tutorial ; Matrixsynth Melodee https://www.matrixsynth.com/2026/11/melodee-for-mpc-sample-16-tracks.html ; Teenage Engineering OP-Z punch-in effects https://teenage.engineering/guides/op-z/punch-in-effects
