# Sequencer Implementation Plan — TB-303 VSTi

Forked from `master` on branch `feature/sequencer`. Research + design below;
implementation is phased so each phase passes {Compiler} + {DSP Reviewer} before
the next begins.

## Authentic TB-303 sequencer behaviour (from Roland docs / olney.ai)
- **16 steps** per pattern (canonical; length can be shortened, 16 is the norm).
- **Two-pass programming model:** enter PITCH per step, then RHYTHM (which steps
  are gated vs rest), then per-step SLIDE and ACCENT. We expose all four per step.
- **Per-step fields:** note (or rest), `on` (gate/rhythm), `slide`, `accent`.
- **Slide** = legato into the NEXT step: pitch glides (portamento) and the
  envelope does NOT retrigger. Our voice ALREADY supports legato glide
  (fixed note stack + shared AD envelope, gate stays 1 while held) — so slide
  just means "don't send note-off before the next note-on." (See ERROR_LOG L7.)
- **Accent** = per-step level boost + filter open. Maps to our existing
  velocity→accent path: emit accent steps at velocity 127, non-accent at ~90.
- **Tempo**: 16th-note clock. step duration = (60/BPM)/4 seconds.
- **Patterns A and B** + loop. Original 303 has no swing; clones add it
  (optional Phase 3).
- **Range**: ~3 octaves of chromatic notes.

## Design
### Thread-safety (no alloc in processBlock — L7/L1)
- `SeqStep { int note; bool on, slide, accent; }`, `SeqPattern { SeqStep[16];
  int length; double bpm; }`.
- UI edits write to an `mEditPattern` (heap alloc on UI thread only — allowed).
- Audio thread reads via `std::atomic<SeqPattern*> mActivePattern`, swapped on
  commit (lock-free). processBlock never allocates.

### Driving the voice (reuse, don't rewrite)
- When SEQ is running, `processBlock` builds a local `MidiBuffer` = host events
  merged with sequencer-generated note-on/off events at sample-accurate
  positions, then runs the EXISTING note handler over it.
- Step clock: `mSeqSamplePos` advances by `samplesPerStep` each step; events are
  placed at the exact sample index within the block.
- Slide: if step i has `slide`, omit the note-off at i's end so step i+1's
  note-on is legato (glide + no retrigger). Rest (`on==false`) = no note-on
  (silence for that step).

### UI (phased)
- Phase 1: RUN/STOP toggle + TEMPO knob + a default acid pattern so it's audible.
- Phase 2: 16-step grid editor (note, on, slide, accent), Pattern A/B select,
  clear/randomize.
- Phase 3 (optional): swing, pattern chain A→B, MIDI clock sync.

## Phases
1. **Data model + clock + note emission + Run/Tempo + default pattern.** Build +
   {Compiler} + {DSP Reviewer}. ← WE ARE HERE
2. **16-step grid editor UI** (note/slide/accent/on), A/B, lock-free commit.
3. **Polish:** swing, pattern chaining, host MIDI-clock sync, preset save/load.

## Verification
- Unit-style numerical test: render N blocks at a BPM, confirm note-on/off
  events land at correct sample offsets (step = k*samplesPerStep) and that a
  slide step produces a legato (no retrigger) transition.
- {Compiler} real MSVC build; {DSP Reviewer} audits timing precision +
  thread-safety of the pattern swap.
