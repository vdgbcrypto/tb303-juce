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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // DSP state
    double envelopeStep (int gate);
    double driveSignal (double x, double drive) const;
    double processFilterStage (double x, int stage);
    void updateFilterCoefficients ();
    double cutoffFromKnob (double t);
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
    float noteFrequency {0.0f};
    float activeLevel {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TB303Processor)
};
