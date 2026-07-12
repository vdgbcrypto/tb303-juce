# Permanent Error Log — tb303-juce (Hermes VST Architect protocol)

Maintained by the Lead Audio Architect loop. Every bug found during development
gets a "Lesson Learned" entry so it is never repeated. Format:
  Error: <what broke>  |  Prevention: <rule to avoid it>

---

## Lessons Learned

### L1 — Parameter smoothing applied per-block, not per-sample
Error: The 10ms exponential smoothing coefficient was computed for per-sample
application but applied once per block, stretching the effective time constant
to several seconds and making every knob feel laggy/unresponsive.
Prevention: compute the coefficient for per-sample use and apply it inside the
per-sample loop (mParamSmoothCoef = 1 - exp(-1/(0.01*sr))), not once per block.

  ---

### L1 — Parameter smoothing applied per-block, not per-sample
Error: The 10ms exponential smoothing coefficient was computed for per-sample
use but applied ONCE per block (before the sample loop). At a 256-sample buffer
that inflated the time constant to ~8s, so every knob (cutoff, volume, etc.)
felt laggy / "does nothing" because it crawled to target over seconds.
Prevention: Either run the one-pole smoother PER SAMPLE inside the loop, OR
recompute the coefficient for block rate using numSamples. Per-sample is preferred.

### L2 — Wrong Chamberlin SVF cutoff coefficient
Error: `f = cutoff / (2*sr)` (and missing the sin term) pitched the filter
~10x (3+ octaves) too low; most of the CUT OFF knob range was effectively dead.
Verified by freq_resp test: -3dB point was 20Hz for a 200Hz knob setting.
Prevention: Chamberlin SVF coefficient is `f = 2*sin(pi*fc)` with
`fc = cutoff / sampleRate` (NOT /2sr). Clamp fc to [0.0001, 0.49].

### L3 — Resonance inverted + self-osc killed by tanh
Error: Damping `q` was raised WITH the resonance knob, so turning resonance UP
reduced resonance. Also `tanh()` on the lp/bp integrator STATE prevented the
filter from ever self-oscillating (the 303's signature whistle).
Prevention: For Chamberlin SVF, LOW damping = MORE resonance. The cascade only
self-oscillates at damping ~0 (verified: reso=1 -> damping 0 -> sustained tone).
Use a bounded LINEAR clamp (e.g. ±6) on the state, never tanh, when self-osc
is desired. Map reso=0 -> ~0.5 (clean), reso=1 -> 0.0 (whistle).

### L4 — Env-Mod normalization cancelled accent; note-on click
Error: `envNorm = mEnvLevel/mEnvPeak` made the cutoff sweep independent of
accent (accent's envPeak scaling cancelled out), and the instant env jump at
note-on slammed the cutoff open (click). Release was actually fine (normalised
env decays smoothly).
Prevention: Use the ABSOLUTE envelope for the cutoff sweep so accented notes
sweep deeper. Add a fixed ~2.5ms attack ramp to the envelope (no instant jump).
Clamp the effective cutoff to Nyquist so the sweep stays audible.

### L5 — Cutoff taper overshoot
Error: `cutoffFromKnob` exponent 3.7 pushed the top of the knob to ~90kHz,
clamping at Nyquist and leaving the top ~12% of travel dead.
Prevention: exponent 3.0 gives ~18Hz..18kHz with the low-end-weighted taper.

### L6 — Hard 0/1 amplitude gate causes note-on/off clicks
Error: amplitude was `osc * activeLevel` where activeLevel was a hard 0/1 switch
toggled at note-on/off. That injects a discontinuity (click) at every note edge.
Prevention: the envelope (VCA) is the amplitude — `out = drive(filtered * env) * vol`.
Gate transitions only start/stop the envelope (attack/decay ramp), so edges are
smooth. Do NOT multiply by a hard gate.

### L7 — Mono voice: no heap alloc, fixed note stack, legato
Error (avoided): allocating a note list inside processBlock violates the
no-alloc-on-audio-thread rule. Also retriggering the envelope on every
overlapping note kills authentic legato.
Prevention: fixed-size `int mNoteStack[8]` (last-note priority, no heap). Gate
stays 1 while >=1 note held, so the shared AD envelope does NOT retrigger on
legato note changes (only on 0->1 gate). Per-note portamento glides pitch
(mPortaFreq += (target-porta)*coeff) per sample. Accent is per-note = knob*velocity.

### L9 — Sequencer: lock-free copy, slides = omitted note-off, sample-accurate clock
Error: a step sequencer must drive the voice WITHOUT allocating on the audio
thread, and SLIDE must be legato (glide + no envelope retrigger), not a retriggered
note. Sub-block step boundaries must land at the exact sample (no block-quantised drift).
Prevention: (1) Pattern (16 Steps, fixed-size, no heap). TRUE LOCK-FREE
DOUBLE BUFFER: UI thread edits mEditPattern then commitPattern() copies it into
mCommittedPattern and swaps the atomic mActivePattern pointer. The audio
thread reads mActivePattern.load() ONCE per block (no lock, no copy of
contents) -> race-free + allocation-free (L7). Do NOT copy Pattern into a
stack local under a lock (that was the wrong first attempt). (2) SLIDE: if a step has slide=true, OMIT its note-off so the
next step's note-on is legato (voice glides pitch, shared AD envelope does not
retrigger). (3) Clock: samplesPerStep = (60/bpm)/4 * sr (16th note); advance
mSeqSamplePos per sub-block slice; place note-on/off at the exact blockPos
sample. Verified: step spacing error = 0.00 samples. (4) REST (on=false) = no
note-on (silence). (5) On STOP, do NOT leave a stuck note — next processBlock
with seq off stops emitting; ensure any held note is released (the voice's own
note-off / all-notes-off path handles it).


### L8 — BLEP: verify aliasing with FFT, not peak-delta; use canonical formula
Error: when adding polyBlep band-limiting to the square/saw, a naive
"peak sample-to-sample jump" test gives a FALSE NEGATIVE — polyBlep correctly
concentrates the correction in 1-2 samples, so the peak delta stays ~2.0 even
when aliasing is fixed. Also, hand-rewriting Finke's polyBlep algebra produced
a wrong-shaped bump (square peaked at 3.0 instead of rounding).
Prevention: (1) Use the CANONICAL polyBlep verbatim (Tale/Finke):
  if(t<dt){u=t/dt; return u+u-u*u-1.0;} else if(t>1-dt){u=(t-1)/dt; return u*u+u+u+1.0;}
  square = sq + polyBlep(t,dt) - polyBlep(fmod(t+0.5,1),dt)  (coefficient 1).
(2) Verify with a REAL FFT measuring energy in the alias band of a HIGH note
(e.g. 9kHz): naïve square vs BLEP should show ~95% alias-energy reduction.
Peak-delta is NOT a valid band-limiting metric.

### L10 — Lock-free pattern handoff MUST be a true SPSC double-buffer
Error: a "double-buffer" that points mActivePattern at ONE shared
mCommittedPattern and only does mCommitted = mPatterns[ab]; store(ptr) is a
NO-OP swap (the pointer value never changes) -> the atomic establishes NO
happens-before on the data, so the audio thread can read a torn/half-written
Step mid-block. That is a real data race / UB.
Prevention: TWO committed slots mCommitted[2] + std::atomic<int>
mActiveCommitted. commitPattern() copies into the IDLE slot (1 - active) then
flips the index (release). Audio reads mCommitted[mActiveCommitted.load()]
(acquire). Producer and consumer NEVER touch the same slot -> no torn read,
no data race (L9). Do NOT point an atomic at a single shared buffer and call
it a swap.

### L11 — Release the note you actually SCHEDULED, not the live pattern note
Error: emitting noteOff(pat->steps[playing].note) re-reads the pattern at
release time. If the user edits the playing step's note while running, the
note sounding (e.g. 48) diverges from the now-committed note (e.g. 36) ->
noteOff(36) is sent and 48 is left STUCK. (Phase 2 regression once the
committed buffer became mutable.)
Prevention: snapshot mSeqCurrentNote = s.note at note-on; emit
noteOff(mSeqCurrentNote) at release. Reset mSeqCurrentNote=-1 on STOP/START.

### L12 — MSVC rejects `const` nested-array aggregate init of a struct with an array member
Error: a `const PPattern kPatterns[3][20]` (PPattern { PStep steps[16]; double bpm; })
initialized with brace lists fails under MSVC 19.5x with C2078 "too many
initializers" / C2440 "cannot convert from initializer list to double" — even
though the SAME shape compiles for a single non-array variable. Raw
`PStep steps[16]` member + multi-dim `const` array = MSVC brace-elision bug.
It ALSO fails for a flat 1D `const PPattern[60]` and for `std::array<PStep,16>`
member with brace init, and a `const int[16]` constructor arg cannot take a
braced list.
Prevention (what compiles cleanly): give PPattern a constructor taking
`const std::array<int,16>&` for each of note/on/slide/accent + a double bpm,
and store the bank as a FLAT `PPattern kPatterns[60]` (index = genre*20+step),
each entry a constructor call `PPattern(std::array<int,16>{...}, ...)`. Skip
const (use a plain global array) and avoid brace-eliding the whole array. This
sidesteps every MSVC aggregate-init foot-gun. See Source/patterns_data.h.

### L13 — DAW state recall must re-apply the preset (ComboBoxAttachment is silent)
Error: on setStateInformation the ComboBoxAttachment restores the "preset"
choice with dontSendNotification, so the dropdown's onChange (which calls
loadPreset) NEVER fires. The UI shows preset N but mPatterns/audio still hold
the default pattern -> UI/sound desync until the user manually re-selects.
Prevention: after apvts.replaceState(...) in setStateInformation, call
loadPreset((int)apvts.getParameter("preset")->getValue()) so the restored
index is re-applied to the grid + audio thread (SPSC, L9/L10).

---
---

## Standing environment facts (persist across sessions)
- JUCE 8.0.14 canonical at C:\JUCE. Build via CMake "Visual Studio 18 2026" -A x64.
- MSVC default toolset 14.51; Win SDK 10.0.26100.0.
- Install VST3 to C:\Users\verho\AppData\Local\Programs\Common\VST3.
- Repo: github.com/vdgbcrypto/tb303-juce (public, master).
- Verification = real MSVC compile + standalone numerical DSP tests in dsp_test/.
