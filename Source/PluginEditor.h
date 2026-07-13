#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TB303Editor : public juce::AudioProcessorEditor
{
public:
    TB303Editor (TB303Processor&);
    ~TB303Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshGrid();   // load active pattern (A/B) into the 16-step widgets

private:
    TB303Processor& processor;

    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider envModSlider;
    juce::Slider decaySlider;
    juce::Slider accentSlider;
    juce::Slider volumeSlider;
    juce::Slider tuneSlider;
    juce::ComboBox waveformCombo;
    juce::Label syncLabel;          // "SYNC" caption for the sync dropdown
    juce::ToggleButton seqRunButton;
    juce::Slider seqTempoSlider;
    juce::Label seqTempoLabel;       // "TEMPO" caption for the seq tempo knob
    juce::ComboBox presetCombo;        // built-in 303 pattern presets (60)
    juce::ComboBox syncCombo;          // Sync: Off / MIDI Clock / DAW
    juce::Slider distortionSlider;     // distortion amount 0..1
    juce::Slider swingSlider;          // swing amount 0..0.6
    juce::ToggleButton abButton;        // A/B pattern select
    juce::TextButton randomButton;
    juce::TextButton clearButton;

    // 16-step grid (Phase 2): per step a note slider + On/Slide/Accent toggles.
    static constexpr int kSteps = 16;
    juce::Slider noteSlider[kSteps];
    juce::ToggleButton onButton[kSteps];
    juce::ToggleButton slideButton[kSteps];
    juce::ToggleButton accentButton[kSteps];

    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label envModLabel;
    juce::Label decayLabel;
    juce::Label accentLabel;
    juce::Label volumeLabel;
    juce::Label tuneLabel;
    juce::Label waveformLabel;
    juce::Label distortionLabel;
    juce::Label swingLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envModAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> accentAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqRunAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seqTempoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distortionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> swingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TB303Editor)
};
