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
    juce::ToggleButton bypassButton;
    juce::ToggleButton seqRunButton;
    juce::Slider seqTempoSlider;
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

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envModAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> accentAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqRunAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seqTempoAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TB303Editor)
};
