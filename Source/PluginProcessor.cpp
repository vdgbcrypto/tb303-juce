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
    juce::StringArray waveChoices;
    waveChoices.add ("Saw");
    waveChoices.add ("Square");
    waveChoices.add ("Saw+Square");
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"waveform", 1}, "Waveform", waveChoices, 0));
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
    // Fixed mono portamento (303 slide feel); per-sample glide in processBlock.
    const double portaTime = 0.006; // seconds
    mPortaCoeff = 1.0 - std::exp (-1.0 / (portaTime * sampleRate));
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
    mNoteStackSize = 0;
    mTargetFreq = 110.0;
    mPortaFreq = 110.0;
    mAccentAmount = 0.0;
    mWaveform = 0;
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
    // 10 ms time constant, applied PER SAMPLE (see processBlock). Previously the
    // coefficient was applied once per block, which stretched the effective time
    // constant to several seconds and made every knob feel laggy/unresponsive.
    mParamSmoothCoef = 1.0 - std::exp (-1.0 / (0.01 * sampleRate));
    mBypassSmoothCoef = 1.0 - std::exp (-1.0 / (0.01 * sampleRate));
}

void TB303Processor::updateFilterCoefficients ()
{
    // Chamberlin SVF coefficient: f = 2*sin(pi*fc), fc = cutoff/sr (cycles/sample).
    // mCutoffHz already folds in the ENV MOD sweep + ACCENT filter-open (set per
    // sample in processBlock); mDamping encodes RESONANCE (LOW damping = MORE
    // resonance/self-osc, so the resonance knob is INVERTED vs the old code which
    // raised q with the knob and made resonance weaker as you turned it up).
    double fc = juce::jlimit (0.0001, 0.49, mCutoffHz / mSampleRate);
    double f = 2.0 * std::sin (juce::MathConstants<double>::pi * fc);

    // Both stages share the same damping so the cascade can self-oscillate at
    // high resonance (303 resonance = negative feedback that whistles near max).
    mStage1.f = f;
    mStage1.q = mDamping;
    mStage2.f = f * 0.95;
    mStage2.q = mDamping;
}

double TB303Processor::cutoffFromKnob (double t)
{
    // TB-303 VCF: 24 dB/oct 4-pole ladder. Range ~18 Hz .. ~18 kHz with an
    // extremely low-end-weighted (non-linear) taper: most of the knob travel
    // sits in the low end, the top barely moves. pow(t,1.3) biases low end.
    const double minHz = 18.0;
    return minHz * std::pow (10.0, std::pow (t, 1.3) * 3.0); // ~18 Hz .. ~18 kHz
}

double TB303Processor::processFilterStage (double x, int stage)
{
    auto& s = (stage == 0) ? mStage1 : mStage2;
    s.lp += s.f * s.bp;
    s.hp = x - s.lp - s.q * s.bp;
    s.bp += s.f * s.hp;
    s.lp = std::clamp (s.lp, -6.0, 6.0);   // linear clamp (bounded; tanh would kill self-osc)
    s.bp = std::clamp (s.bp, -6.0, 6.0);
    return s.lp;
}

double TB303Processor::envelopeStep (int gate)
{
    if (gate)
    {
        // 303 AD envelope: fixed ~2.5 ms attack ramps to peak (no instant jump,
        // which would click the cutoff open). Accent raises the VCA peak.
        double target = 1.0 + mSmoothedAccent * 0.4;
        double a = 1.0 - std::exp (-1.0 / (0.0025 * mSampleRate));
        mEnvLevel += (target - mEnvLevel) * a;
        mEnvPeak = target;
    }
    else
    {
        // Exponential decay (no sustain).
        double decayTime = 0.05 + (1.0 - 0.05) * mSmoothedDecay; // ~50 ms .. ~1.0 s
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

    // Get parameters atomically before processing (pointers are stable; values
    // are read + smoothed PER SAMPLE inside the loop below).
    auto cutoffParam = apvts.getRawParameterValue ("cutoff");
    auto resoParam = apvts.getRawParameterValue ("resonance");
    auto envModParam = apvts.getRawParameterValue ("envmod");
    auto decayParam = apvts.getRawParameterValue ("decay");
    auto accentParam = apvts.getRawParameterValue ("accent");
    auto volumeParam = apvts.getRawParameterValue ("volume");
    auto tuneParam = apvts.getRawParameterValue ("tune");
    auto waveformParam = apvts.getRawParameterValue ("waveform");
    auto bypassParam = apvts.getRawParameterValue ("bypass");

    if (mBypass)
    {
        // Buffer was already cleared above; bypass silences output for this instrument.
        return;
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    const int numSamples = buffer.getNumSamples();

    // Sample-accurate MIDI scheduling: process note events at their real position
    // inside the block so notes trigger immediately instead of waiting for the
    // next processBlock() (removes the perceived "laggy" note response).
    juce::MidiBuffer::Iterator midiIt (midiMessages);
    juce::MidiMessage midiMsg;
    int midiPos = 0;
    bool hasEvent = midiIt.getNextEvent (midiMsg, midiPos);

    int sample = 0;
    while (sample < numSamples)
    {
        // Read + smooth parameters PER SAMPLE so the 10 ms time constant is real
        // (applying it once per block inflated it to seconds and made knobs laggy).
        float cutoffVal = cutoffParam->load();
        float resoVal = resoParam->load();
        float envModVal = envModParam->load();
        float decayVal = decayParam->load();
        float accentVal = accentParam->load();
        float volumeVal = volumeParam->load();
        float tuneVal = tuneParam->load();
        float waveformVal = waveformParam->load();
        float bypassVal = bypassParam->load();

        mSmoothedCutoff  += (cutoffVal  - mSmoothedCutoff)  * mParamSmoothCoef;
        mSmoothedReso    += (resoVal    - mSmoothedReso)    * mParamSmoothCoef;
        mSmoothedEnvMod  += (envModVal  - mSmoothedEnvMod)  * mParamSmoothCoef;
        mSmoothedDecay   += (decayVal   - mSmoothedDecay)   * mParamSmoothCoef;
        mSmoothedAccent  += (accentVal  - mSmoothedAccent)  * mParamSmoothCoef;
        mSmoothedVolume  += (volumeVal  - mSmoothedVolume)  * mParamSmoothCoef;
        mSmoothedTune    += (tuneVal    - mSmoothedTune)    * mParamSmoothCoef;
        mWaveform = (int) waveformVal;
        mSmoothedBypass  += ((bypassVal > 0.5f ? 1.0 : 0.0) - mSmoothedBypass) * mBypassSmoothCoef;
        mBypass = mSmoothedBypass > 0.5f;

        // --- Effective VCF cutoff (TB-303 behaviour) ---
        double baseCutoff = cutoffFromKnob (mSmoothedCutoff);

        // ENV MOD sweeps the cutoff using the ABSOLUTE envelope (0..~1.4), so an
        // accented note (higher envPeak) sweeps DEEPER — authentic 303 squelch.
        // Accent also deepens the sweep depth, and adds a modest static "punch"
        // open. Clamped to Nyquist so the sweep stays audible instead of pegging.
        double envDepth = mSmoothedEnvMod * (1.0 + 0.5 * mSmoothedAccent);
        double cutoffHz = baseCutoff * (1.0 + envDepth * mEnvLevel * 4.0);
        cutoffHz *= (1.0 + mSmoothedAccent * 0.3);
        mCutoffHz = juce::jlimit (18.0, 0.49 * mSampleRate, cutoffHz);

        // RESONANCE: LOW damping = MORE resonance. The Chamberlin SVF only
        // self-oscillates at damping ~0 (verified by selfosc_test), so map
        // reso=1 -> 0.0 (303 "whistle") and reso=0 -> 0.5 (clean/flat-ish).
        mDamping = 0.5 * (1.0 - mSmoothedReso);

        updateFilterCoefficients();

        while (hasEvent && midiPos == sample)
        {
            if (midiMsg.isNoteOn())
            {
                const int note = midiMsg.getNoteNumber();
                const double vel = (double) midiMsg.getVelocity() / 127.0; // 0..1
                // Mono, last-note priority on a fixed stack (no heap alloc).
                // Dedupe first so an overlapping same-note on doesn't double-push.
                int w = 0;
                for (int r = 0; r < mNoteStackSize; ++r)
                    if (mNoteStack[r] != note) mNoteStack[w++] = mNoteStack[r];
                mNoteStackSize = w;
                if (mNoteStackSize < kMaxNotes)
                    mNoteStack[mNoteStackSize++] = note;
                mTargetFreq = juce::MidiMessage::getMidiNoteInHertz (note);
                // Snap pitch (no glide) on the first note of a silent voice so we
                // don't zip from the stale 110 Hz default; legato/overlap glides.
                if (!mNoteOn && mEnvLevel < 1.0e-4)
                    mPortaFreq = mTargetFreq;
                // Accent per-note from velocity * global ACCENT knob (live 303
                // maps sequencer accent -> here velocity drives it).
                mAccentAmount = juce::jlimit (0.0, 1.0, mSmoothedAccent * vel);
                mNoteOn = true;
            }
            else if (midiMsg.isNoteOff())
            {
                // Remove the note from the stack; if others remain, glide to the
                // most recent (legato) and keep gate on. Otherwise release.
                const int note = midiMsg.getNoteNumber();
                int w = 0;
                for (int r = 0; r < mNoteStackSize; ++r)
                    if (mNoteStack[r] != note) mNoteStack[w++] = mNoteStack[r];
                mNoteStackSize = w;
                if (mNoteStackSize > 0)
                {
                    mTargetFreq = juce::MidiMessage::getMidiNoteInHertz (mNoteStack[mNoteStackSize - 1]);
                    mNoteOn = true; // legato: envelope keeps running, no retrigger
                }
                else
                {
                    mNoteOn = false;
                }
            }
            else if (midiMsg.isAllNotesOff() || midiMsg.isAllSoundOff())
            {
                mNoteStackSize = 0;
                mNoteOn = false;
            }

            hasEvent = midiIt.getNextEvent (midiMsg, midiPos);
        }

        const int noteGate = mNoteOn ? 1 : 0;

        // Mono portamento: glide the actual osc frequency toward the target
        // (per-sample, click-free; gives the 303 slide feel on legato/overlap).
        mPortaFreq += (mTargetFreq - mPortaFreq) * mPortaCoeff;

        // Oscillator — TUNE is +/- 1 octave about the played note.
        double freq = mPortaFreq * std::pow (2.0, (mSmoothedTune - 0.5) * 2.0);
        mPhase += (freq * 2.0 * juce::MathConstants<double>::pi) / mSampleRate;
        if (mPhase > 2.0 * juce::MathConstants<double>::pi) mPhase -= 2.0 * juce::MathConstants<double>::pi;

        // Waveform: 0=saw, 1=square (full +/-1), 2=saw+square mix.
        double saw = (mPhase / juce::MathConstants<double>::pi) - 1.0;       // -1..1
        double sq  = (mPhase < juce::MathConstants<double>::pi) ? 1.0 : -1.0; // +/-1
        double osc = (mWaveform == 1) ? sq : (mWaveform == 2 ? saw * 0.5 + sq * 0.5 : saw);

        // Filter (2 cascaded SVF stages)
        double filtered = processFilterStage (osc, 0);
        filtered = processFilterStage (filtered, 1);

        // Envelope is the VCA (amplitude) AND drives the cutoff sweep. This
        // removes the hard 0/1 gate click; release decays smoothly.
        double env = envelopeStep (noteGate);
        // Accent (per-note) is already folded into envPeak, so amplitude tracks
        // it; add the modest extra drive only.
        double driven = driveSignal (filtered * env * 1.2, mAccentAmount * 0.5);
        double out = driven * mSmoothedVolume * 0.8;

        left[sample] = (float) out;
        right[sample] = (float) out;

        ++sample;
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
