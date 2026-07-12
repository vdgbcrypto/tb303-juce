#include "PluginEditor.h"

TB303Editor::TB303Editor (TB303Processor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (560, 320);
    setResizable (false, false);

    const juce::Colour green (0xff00ff41);
    const juce::Colour track (0xff445544);

    // TB-303 green on dark panel styling — knobs instead of faders
    auto makeKnob = [this, &green, &track] (juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f, true);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        slider.setColour (juce::Slider::thumbColourId, green);
        slider.setColour (juce::Slider::rotarySliderFillColourId, green);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, track);
        slider.setColour (juce::Slider::trackColourId, track);
        slider.setTextValueSuffix ("");
        addAndMakeVisible (slider);

        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, green);
        label.setFont (juce::Font (12.0f, juce::Font::bold));
        addAndMakeVisible (label);
    };

    makeKnob (cutoffSlider,    cutoffLabel,    "CUT OFF");
    makeKnob (resonanceSlider, resonanceLabel, "RESONANCE");
    makeKnob (envModSlider,    envModLabel,    "ENV MOD");
    makeKnob (decaySlider,     decayLabel,     "DECAY");
    makeKnob (accentSlider,    accentLabel,    "ACCENT");
    makeKnob (volumeSlider,    volumeLabel,    "VOLUME");
    makeKnob (tuneSlider,      tuneLabel,      "TUNE");

    addAndMakeVisible (bypassButton);
    bypassButton.setButtonText ("Bypass");

    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "cutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "resonance", resonanceSlider);
    envModAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "envmod", envModSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "decay", decaySlider);
    accentAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "accent", accentSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "volume", volumeSlider);
    tuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "tune", tuneSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "bypass", bypassButton);
}

TB303Editor::~TB303Editor() {}

void TB303Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));

    g.setColour (juce::Colour (0xff00ff41));
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("TB-303 VSTi", getLocalBounds().removeFromTop (24), juce::Justification::centred, true);

    g.setColour (juce::Colour (0xff557755));
    g.drawRect (getLocalBounds().reduced (4).removeFromTop (24), 1);
}

void TB303Editor::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    bounds.removeFromTop (32);

    const int knobSize = 64;
    const int labelH = 18;
    const int gap = 6;
    const int stride = knobSize + gap;
    const int totalW = 7 * stride - gap;
    int x = bounds.getX() + (bounds.getWidth() - totalW) / 2;
    const int knobY = bounds.getY() + 6;

    cutoffSlider.setBounds (x, knobY, knobSize, knobSize);
    cutoffLabel.setBounds (x, knobY + knobSize, knobSize, labelH);
    x += stride;
    resonanceSlider.setBounds (x, knobY, knobSize, knobSize);
    resonanceLabel.setBounds (x, knobY + knobSize, knobSize, labelH);
    x += stride;
    envModSlider.setBounds (x, knobY, knobSize, knobSize);
    envModLabel.setBounds (x, knobY + knobSize, knobSize, labelH);
    x += stride;
    decaySlider.setBounds (x, knobY, knobSize, knobSize);
    decayLabel.setBounds (x, knobY + knobSize, knobSize, labelH);
    x += stride;
    accentSlider.setBounds (x, knobY, knobSize, knobSize);
    accentLabel.setBounds (x, knobY + knobSize, knobSize, labelH);
    x += stride;
    volumeSlider.setBounds (x, knobY, knobSize, knobSize);
    volumeLabel.setBounds (x, knobY + knobSize, knobSize, labelH);
    x += stride;
    tuneSlider.setBounds (x, knobY, knobSize, knobSize);
    tuneLabel.setBounds (x, knobY + knobSize, knobSize, labelH);

    const int btnY = knobY + knobSize + labelH + 16;
    bypassButton.setBounds (bounds.withWidth (80).withHeight (24).translated ((bounds.getWidth() - 80) / 2, 0).withY (btnY));
}
