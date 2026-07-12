#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout TB303Processor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"cutoff", 1}, "Cutoff", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"resonance", 1}, "Resonance", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"envmod", 1}, "Env Mod", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"decay", 1}, "Decay", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"accent", 1}, "Accent", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"volume", 1}, "Volume", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"tune", 1}, "Tune", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID {"bypass", 1}, "Bypass", false));
    return { params.begin(), params.end() };
}

TB303Processor::TB303Processor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    reset();
}

TB303Processor::~TB303Processor() {}

void TB303Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    updateSmoothing (sampleRate);
    reset();
}

void TB303Processor::releaseResources() {}

void TB303Processor::reset()
{
    mStage1 = {};
    mStage2 = {};
    mPhase = 0.0;
    mEnvLevel = 0.0;
    mNoteOn = false;
    mSmoothedBypass = 0.0;
    mSmoothedCutoff = mCutoff;
    mSmoothedReso = mResonance;
    mSmoothedEnvMod = mEnvMod;
    mSmoothedDecay = mDecay;
    mSmoothedAccent = mAccent;
    mSmoothedVolume = mVolume;
    mSmoothedTune = mTune;
}

void TB303Processor::updateSmoothing (double sampleRate)
{
    mParamSmoothCoef = 1.0 - std::exp (-1.0 / (0.03 * sampleRate));
    mBypassSmoothCoef = 1.0 - std::exp (-1.0 / (0.01 * sampleRate));
}

void TB303Processor::updateFilterCoefficients ()
{
    double cutoff = 20.0 * std::pow (10.0, mSmoothedCutoff * 4.0);
    double reso = mSmoothedReso * 3.5;
    double q = 0.05 + 0.707 + reso * (1.0 - 0.707 - 0.08 * (cutoff / mSampleRate));
    q = std::max (q, 0.05);
    double fc = cutoff / (mSampleRate * 2.0);
    if (fc > 0.49) fc = 0.49;
    mStage1.f = fc;
    mStage1.q = q;
    mStage2.f = fc * 0.95;
    mStage2.q = q * 1.03;
}

double TB303Processor::processFilterStage (double x, int stage)
{
    auto& s = (stage == 0) ? mStage1 : mStage2;
    s.lp += s.f * s.bp;
    s.hp = x - s.lp - s.q * s.bp;
    s.bp += s.f * s.hp;
    s.lp = std::clamp (s.lp, -1.5, 1.5);
    s.bp = std::clamp (s.bp, -1.5, 1.5);
    return s.lp;
}

double TB303Processor::envelopeStep (int gate)
{
    if (gate)
    {
        // Fast attack to peak on note-on
        mEnvLevel = 1.0 + mSmoothedAccent * 0.4;
    }
    else
    {
        // Exponential decay driven by Decay
        double decayTime = 0.05 + (1.0 - 0.05) * mSmoothedDecay;
        double coeff = std::exp (-1.0 / (decayTime * mSampleRate));
        mEnvLevel *= coeff;
        if (mEnvLevel < 1e-6) mEnvLevel = 0.0;
    }
    return mEnvLevel;
}

double TB303Processor::driveSignal (double x, double drive) const
{
    double gain = 1.0 + drive * 6.0;
    double y = x * gain;
    if (y > 1.0) y = 1.0;
    if (y < -1.0) y = -1.0;
    return y * (1.5 - 0.5 * y * y);
}

void TB303Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get parameters atomically before processing
    auto cutoffParam = apvts.getRawParameterValue ("cutoff");
    auto resoParam = apvts.getRawParameterValue ("resonance");
    auto envModParam = apvts.getRawParameterValue ("envmod");
    auto decayParam = apvts.getRawParameterValue ("decay");
    auto accentParam = apvts.getRawParameterValue ("accent");
    auto volumeParam = apvts.getRawParameterValue ("volume");
    auto tuneParam = apvts.getRawParameterValue ("tune");
    auto bypassParam = apvts.getRawParameterValue ("bypass");

    float cutoffVal = cutoffParam->load();
    float resoVal = resoParam->load();
    float envModVal = envModParam->load();
    float decayVal = decayParam->load();
    float accentVal = accentParam->load();
    float volumeVal = volumeParam->load();
    float tuneVal = tuneParam->load();
    float bypassVal = bypassParam->load();

    // Smooth parameters
    mSmoothedCutoff += (cutoffVal - mSmoothedCutoff) * mParamSmoothCoef;
    mSmoothedReso += (resoVal - mSmoothedReso) * mParamSmoothCoef;
    mSmoothedEnvMod += (envModVal - mSmoothedEnvMod) * mParamSmoothCoef;
    mSmoothedDecay += (decayVal - mSmoothedDecay) * mParamSmoothCoef;
    mSmoothedAccent += (accentVal - mSmoothedAccent) * mParamSmoothCoef;
    mSmoothedVolume += (volumeVal - mSmoothedVolume) * mParamSmoothCoef;
    mSmoothedTune += (tuneVal - mSmoothedTune) * mParamSmoothCoef;
    mSmoothedBypass += ((bypassVal > 0.5f ? 1.0 : 0.0) - mSmoothedBypass) * mBypassSmoothCoef;
    mBypass = mSmoothedBypass > 0.5f;

    updateFilterCoefficients();

    int noteGate = 0;
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            noteFrequency = juce::MidiMessage::getMidiNoteInHertz (message.getNoteNumber());
            activeLevel = 1.0f;
            noteGate = 1;
        }
        else if (message.isNoteOff())
        {
            activeLevel = 0.0f;
        }
    }

    if (mBypass)
    {
        // Buffer was already cleared above; bypass silences output for this instrument.
        return;
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    double tune = 0.5 + mSmoothedTune;
    float currentVolume = mSmoothedVolume;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Simple oscillator
        double freq = noteFrequency * std::pow (2.0, tune - 0.5);
        mPhase += (freq * 2.0 * juce::MathConstants<double>::pi) / mSampleRate;
        if (mPhase > 2.0 * juce::MathConstants<double>::pi) mPhase -= 2.0 * juce::MathConstants<double>::pi;
        
        double saw = (mPhase / juce::MathConstants<double>::pi) - 1.0;
        double sq = (mPhase < juce::MathConstants<double>::pi) ? 0.7 : -0.7;
        double osc = (saw * 0.6 + sq * 0.4) * activeLevel;

        // Filter
        double filtered = processFilterStage (osc, 0);
        filtered = processFilterStage (filtered, 1);

        // Envelope
        double env = envelopeStep (noteGate);
        double accented = env + mSmoothedAccent * 0.3;
        double driven = driveSignal (filtered * accented * 1.2, mSmoothedAccent * 0.5);
        double out = driven * currentVolume * 0.8;

        left[sample] = (float) out;
        right[sample] = (float) out;
    }
}

juce::AudioProcessorEditor* TB303Processor::createEditor()
{
    return new TB303Editor (*this);
}

void TB303Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TB303Processor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TB303Processor();
}
