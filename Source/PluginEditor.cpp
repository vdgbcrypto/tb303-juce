#include "PluginEditor.h"

TB303Editor::TB303Editor (TB303Processor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (520, 280);
    setResizable (false, false);

    // TB-303 green on dark panel styling
    auto makeSlider = [this] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff00ff41));
        slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00ff41));
        slider.setColour (juce::Slider::trackColourId, juce::Colour (0xff445544));
        addAndMakeVisible (slider);
    };

    makeSlider (cutoffSlider);
    makeSlider (resonanceSlider);
    makeSlider (envModSlider);
    makeSlider (decaySlider);
    makeSlider (accentSlider);
    makeSlider (volumeSlider);
    makeSlider (tuneSlider);

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

    int sliderWidth = bounds.getWidth() / 7;
    int x = bounds.getX();

    cutoffSlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);
    x += sliderWidth;
    resonanceSlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);
    x += sliderWidth;
    envModSlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);
    x += sliderWidth;
    decaySlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);
    x += sliderWidth;
    accentSlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);
    x += sliderWidth;
    volumeSlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);
    x += sliderWidth;
    tuneSlider.setBounds (x, bounds.getY(), sliderWidth, bounds.getHeight() - 20);

    bypassButton.setBounds (bounds.withWidth (80).withHeight (24).translated (0, bounds.getHeight() - 20));
}
