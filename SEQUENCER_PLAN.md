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
- `Step { int note; bool on, slide, accent; }`, `Pattern { Step[16]; int length; double bpm; }`.
- UI edits write to `mPatterns[ab]` (on the message thread — allowed).
- TRUE SPSC double-buffer: `mCommitted[2]` + `std::atomic<int> mActiveCommitted`.
  `commitPattern()` copies into the IDLE slot then flips the index; the audio
  thread reads `mCommitted[mActiveCommitted.load()]` (never the slot being
  written) -> no data race, no torn read (ERROR_LOG L10). processBlock never
  allocates (member MidiBuffers pre-sized in prepareToPlay, clear()'d/block).

### Driving the voice (reuse, don't rewrite)
- When SEQ is running, `processBlock` builds `mSeqMidi` = sequencer note events
  at sample-accurate positions, merges with host MIDI into `mMergeMidi`, then
  runs the EXISTING note handler over it.
- Step clock: `mSeqSamplePos` advances by `samplesPerStep` each step; events are
  placed at the exact sample index within the block.
- Slide: if step i has `slide`, omit the note-off at i's end so step i+1's
  note-on is legato (glide + no retrigger). Rest (`on==false`) = no note-on
  (silence for that step).
- Release uses `mSeqCurrentNote` (snapshot at note-on), NOT the live pattern
  note, so editing the playing step can't leave a stuck note (ERROR_LOG L11).

### UI (phased)
- Phase 1: RUN/STOP toggle + TEMPO knob + a default acid pattern so it's audible. ✅
- Phase 2: 16-step grid editor (note slider + on/slide/accent toggles per step),
  A/B select, clear/randomize. ✅
- Phase 3 (optional): swing, pattern chain A→B, MIDI clock sync.

## Phases
1. **Data model + clock + note emission + Run/Tempo + default pattern.** ✅ DONE
   (real MSVC build + DSP Reviewer — fixed slide→rest stuck note, STOP
   allNotesOff, heap-alloc-in-processBlock).
2. **16-step grid editor UI** (note/slide/accent/on), A/B, lock-free commit. ✅ DONE
   (real MSVC build + DSP Reviewer — fixed SPSC double-buffer data race, live-edit
   stuck-note via mSeqCurrentNote snapshot).
3. **Polish:** swing, pattern chaining, host MIDI-clock sync, preset save/load.

## Verification
- Numerical tests (dsp_test/): step spacing error = 0.00 samples (sample-accurate
  clock); slide→rest emits note-off (no stuck note); STOP emits allNotesOff;
  live-edit STOP releases the sounding note (not the re-edited pattern note).
- {Compiler} real MSVC build on every phase; {DSP Reviewer} subagent audits
  timing precision + thread-safety of the pattern swap on every phase.
