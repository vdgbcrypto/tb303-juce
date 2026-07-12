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
    // Two editable patterns (A + B) and a live committed copy the audio
    // thread points at. UI edits mPatterns[ab], then commitPattern() copies
    // it into the IDLE committed slot and flips the atomic index (true SPSC
    // double-buffer: the audio thread only ever reads the slot the producer
    // is NOT writing -> no concurrent read/write, no data race, L9).
    Pattern mPatterns[2];       // A (index 0) and B (index 1), UI-editable
    Pattern mCommitted[2];       // double-buffer slots
    std::atomic<int> mActiveCommitted {0};
    int  mActiveAB {0};        // which pattern the grid currently edits (0=A,1=B)
    // Pre-sized member MidiBuffers: clear()'d each block, never re-alloc'd
    // in processBlock (L7).
    juce::MidiBuffer mSeqMidi, mMergeMidi;
    bool mSeqRunning {false};
    int  mSeqStepIndex {0};
    long long mSeqSamplePos {0};   // samples since current step started
    double mSeqSamplesPerStep {0.0};
    int  mSeqCurrentStepPlaying {-1};
    int  mSeqCurrentNote {-1};    // note actually sent at note-on (for release)

    // UI calls these to edit + publish the pattern (grid editor, Phase 2).
    int  getActiveAB() const { return mActiveAB; }
    void setActiveAB (int ab) { mActiveAB = (ab == 0) ? 0 : 1; }
    Pattern& editPattern() { return mPatterns[mActiveAB]; }
    // SPSC double-buffer: copy into the IDLE slot, then flip the atomic index
    // (release). The audio thread reads mActiveCommitted.load() (acquire) and
    // never touches the slot being written -> no torn read, no data race.
    void commitPattern()
    {
        int other = 1 - mActiveCommitted.load();
        mCommitted[other] = mPatterns[mActiveAB];
        mActiveCommitted.store (other);
    }
    // Load a built-in preset (global index g*20+i) into the active editable
    // pattern, publish it, and set the sequencer tempo to the preset BPM.
    // Runs on the UI thread; commitPattern() makes it visible to the audio thread.
    void loadPreset (int globalIndex);
    // Build the ComboBox item list for the preset dropdown: "Genre - Name".
    juce::StringArray presetChoices() const;

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

    double mSmoothedCutoff {0.5};
    double mSmoothedReso {0.0};
    double mSmoothedEnvMod {0.5};
    double mSmoothedDecay {0.3};
    double mSmoothedAccent {0.0};
    double mSmoothedVolume {0.5};
    double mSmoothedTune {0.5};
    double mSmoothedDist {0.0}; // smoothed distortion amount (0..1)
    double mEnvLevel {0.0};
    bool mNoteOn {false};
    SVF mStage1;
    SVF mStage2;
    double mPhase {0.0};
    double mParamSmoothCoef {0.0};
    double mCutoffHz {1000.0};   // effective VCF cutoff (folds in env-mod + accent)
    double mDamping {0.707};     // SVF damping (LOW = more resonance / self-osc)
    double mEnvPeak {1.0};       // envelope peak this note (for env-mod normalisation)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TB303Processor)
};
