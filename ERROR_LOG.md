# Permanent Error Log — tb303-juce (Hermes VST Architect protocol)

Maintained by the Lead Audio Architect loop. Every bug found during development
gets a "Lesson Learned" entry so it is never repeated. Format:
  Error: <what broke>  |  Prevention: <rule to avoid it>

---

## Lessons Learned

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

---

## Standing environment facts (persist across sessions)
- JUCE 8.0.14 canonical at C:\JUCE. Build via CMake "Visual Studio 18 2026" -A x64.
- MSVC default toolset 14.51; Win SDK 10.0.26100.0.
- Install VST3 to C:\Users\verho\AppData\Local\Programs\Common\VST3.
- Repo: github.com/vdgbcrypto/tb303-juce (public, master).
- Verification = real MSVC compile + standalone numerical DSP tests in dsp_test/.
