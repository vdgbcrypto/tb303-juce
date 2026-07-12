#pragma once
#include <JuceHeader.h>

class TB303Processor : public juce::AudioProcessor
{
public:
    TB303Processor();
    ~TB303Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Mono note stack (last-note priority, legato). Fixed-size to avoid any
    // heap allocation on the audio thread (processBlock must not allocate).
    static constexpr int kMaxNotes = 8;
    int mNoteStack [kMaxNotes] {};
    int mNoteStackSize {0};
    double mTargetFreq {110.0};   // MIDI-target frequency (Hz)
    double mPortaFreq {110.0};    // glided frequency actually used by the osc
    double mPortaCoeff {0.0};     // per-sample portamento coeff (set in prepareToPlay)
    double mAccentAmount {0.0};   // per-note accent = knob * velocity (0..1)
    int mWaveform {0};            // 0=saw, 1=square, 2=saw+square

    // ---- Sequencer (branch feature/sequencer) ----
    struct Step
    {
        int  note   {36};   // MIDI note (C2 default); -1 = rest
        bool on     {true}; // rhythm gate (false = rest for that step)
        bool slide  {false};// glide into next step (legato, no retrigger)
        bool accent {false};// per-step accent (velocity boost + filter open)
    };
    struct Pattern
    {
        Step  steps[16];
        int   length {16};
        double bpm  {120.0};
    };
    // Lock-free DOUBLE BUFFER: UI thread edits mEditPattern, then commitPattern()
    // swaps the atomic pointer. Audio thread reads the pointer ONCE per block
    // (no lock, no copy of contents) -> no heap, no race (L7/L9).
    Pattern mEditPattern;        // UI-owned, edited then committed
    Pattern mCommittedPattern;    // the live copy the pointer points at
    std::atomic<Pattern*> mActivePattern {nullptr};
    // Pre-sized member MidiBuffers: clear()'d each block, never re-alloc'd
    // in processBlock (L7).
    juce::MidiBuffer mSeqMidi, mMergeMidi;
    bool mSeqRunning {false};
    int  mSeqStepIndex {0};
    long long mSeqSamplePos {0};   // samples since current step started
    double mSeqSamplesPerStep {0.0};
    // Per-step note scheduling: track which step is currently sounding so we
    // can emit note-off at the right sample (unless slide -> carry to next).
    int  mSeqCurrentStepPlaying {-1};

    // UI calls these to edit + publish the pattern (Phase 2 grid editor).
    void commitPattern() { mCommittedPattern = mEditPattern; mActivePattern.store (&mCommittedPattern); }
    const Pattern& getPattern() const { return mEditPattern; }
    Pattern& editPattern() { return mEditPattern; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // DSP state
    double envelopeStep (int gate);
    double driveSignal (double x, double drive) const;
    double processFilterStage (double x, int stage);
    void updateFilterCoefficients ();
    double cutoffFromKnob (double t);
    // PolyBLEP band-limiting: corrects oscillator step discontinuities (square
    // edges, saw ramp) to kill aliasing. t in [0,1), dt = freq/sampleRate.
    static double polyBlep (double t, double dt);
    void reset ();
    void updateSmoothing (double sampleRate);
    
    struct SVF { double lp = 0.0, bp = 0.0, hp = 0.0, f = 0.0, q = 0.707; };
    
    double mSampleRate {44100.0};
    double mCutoff {0.5};
    double mResonance {0.0};
    double mEnvMod {0.5};
    double mDecay {0.3};
    double mAccent {0.0};
    double mVolume {0.5};
    double mTune {0.5};
    bool mBypass {false};

    double mSmoothedCutoff {0.5};
    double mSmoothedReso {0.0};
    double mSmoothedEnvMod {0.5};
    double mSmoothedDecay {0.3};
    double mSmoothedAccent {0.0};
    double mSmoothedVolume {0.5};
    double mSmoothedTune {0.5};
    double mEnvLevel {0.0};
    bool mNoteOn {false};
    SVF mStage1;
    SVF mStage2;
    double mPhase {0.0};
    double mParamSmoothCoef {0.0};
    double mBypassSmoothCoef {0.0};
    double mSmoothedBypass {0.0};
    double mCutoffHz {1000.0};   // effective VCF cutoff (folds in env-mod + accent)
    double mDamping {0.707};     // SVF damping (LOW = more resonance / self-osc)
    double mEnvPeak {1.0};       // envelope peak this note (for env-mod normalisation)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TB303Processor)
};
