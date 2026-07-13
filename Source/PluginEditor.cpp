#include "PluginEditor.h"

TB303Editor::TB303Editor (TB303Processor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (600, 560);
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

    waveformCombo.addItem ("Saw", 1);
    waveformCombo.addItem ("Square", 2);
    waveformCombo.addItem ("Saw+Square", 3);
    waveformCombo.setSelectedId (1);
    addAndMakeVisible (waveformCombo);
    waveformLabel.setText ("WAVE", juce::dontSendNotification);
    waveformLabel.setJustificationType (juce::Justification::centred);
    waveformLabel.setColour (juce::Label::textColourId, green);
    waveformLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (waveformLabel);

    // Distortion knob (manual, placed in the sequencer row, not the 7-knob row).
    distortionSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    distortionSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 16);
    distortionSlider.setRange (0.0, 1.0, 0.01);
    distortionSlider.setColour (juce::Slider::rotarySliderFillColourId, green);
    addAndMakeVisible (distortionSlider);
    distortionLabel.setText ("DRIVE", juce::dontSendNotification);
    distortionLabel.setJustificationType (juce::Justification::centred);
    distortionLabel.setColour (juce::Label::textColourId, green);
    distortionLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (distortionLabel);

    // Swing knob (Phase 3): 0 = straight, up to 0.6 (~triplet feel).
    swingSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    swingSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 16);
    swingSlider.setRange (0.0, 0.6, 0.01);
    swingSlider.setColour (juce::Slider::rotarySliderFillColourId, green);
    addAndMakeVisible (swingSlider);
    swingLabel.setText ("SWING", juce::dontSendNotification);
    swingLabel.setJustificationType (juce::Justification::centred);
    swingLabel.setColour (juce::Label::textColourId, green);
    swingLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (swingLabel);

    // Sequencer (Phase 1): Run toggle + Tempo knob.
    addAndMakeVisible (seqRunButton);
    seqRunButton.setButtonText ("SEQ Run");
    seqTempoSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    seqTempoSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 16);
    seqTempoSlider.setRange (40.0, 240.0, 1.0);
    seqTempoSlider.setColour (juce::Slider::rotarySliderFillColourId, green);
    addAndMakeVisible (seqTempoSlider);
    seqTempoLabel.setText ("TEMPO", juce::dontSendNotification);
    seqTempoLabel.setJustificationType (juce::Justification::centred);
    seqTempoLabel.setColour (juce::Label::textColourId, green);
    seqTempoLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (seqTempoLabel);

    // ---- Sequencer grid (Phase 2) ----
    const juce::Colour noteCol (0xff66ccff);
    for (int i = 0; i < kSteps; ++i)
    {
        auto& ns = noteSlider[i];
        ns.setSliderStyle (juce::Slider::LinearVertical);
        ns.setRange (24.0, 72.0, 1.0);     // C1..C5 (~3 octaves)
        ns.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        ns.setColour (juce::Slider::thumbColourId, noteCol);
        ns.setColour (juce::Slider::rotarySliderFillColourId, noteCol);
        ns.onValueChange = [this, i] { auto& p = processor.editPattern(); p.steps[i].note = (int) noteSlider[i].getValue(); processor.commitPattern(); };
        addAndMakeVisible (ns);

        auto& ob = onButton[i];
        ob.setButtonText ("On");
        ob.setColour (juce::ToggleButton::textColourId, green);
        ob.onClick = [this, i] { auto& p = processor.editPattern(); p.steps[i].on = onButton[i].getToggleState(); processor.commitPattern(); };
        addAndMakeVisible (ob);

        auto& sb = slideButton[i];
        sb.setButtonText ("Sl");
        sb.setColour (juce::ToggleButton::textColourId, green);
        sb.onClick = [this, i] { auto& p = processor.editPattern(); p.steps[i].slide = slideButton[i].getToggleState(); processor.commitPattern(); };
        addAndMakeVisible (sb);

        auto& ab = accentButton[i];
        ab.setButtonText ("Ac");
        ab.setColour (juce::ToggleButton::textColourId, green);
        ab.onClick = [this, i] { auto& p = processor.editPattern(); p.steps[i].accent = accentButton[i].getToggleState(); processor.commitPattern(); };
        addAndMakeVisible (ab);
    }
    abButton.setButtonText ("A / B");
    abButton.setColour (juce::ToggleButton::textColourId, green);
    abButton.onClick = [this] { processor.setActiveAB (abButton.getToggleState() ? 1 : 0); refreshGrid(); };
    addAndMakeVisible (abButton);

    randomButton.setButtonText ("Random");
    randomButton.setColour (juce::TextButton::textColourOffId, green);
    randomButton.onClick = [this] {
        auto& p = processor.editPattern();
        static const int scale[8] = { 0,3,5,7,10,12,15,17 }; // minor pentatonic-ish
        for (int i = 0; i < kSteps; ++i)
        {
            p.steps[i].on = (std::rand() % 100) < 80;
            p.steps[i].note = 36 + scale[std::rand() % 8] + (std::rand() % 3) * 12;
            p.steps[i].slide  = (std::rand() % 100) < 20;
            p.steps[i].accent = (std::rand() % 100) < 25;
        }
        processor.commitPattern(); refreshGrid();
    };
    addAndMakeVisible (randomButton);

    clearButton.setButtonText ("Clear");
    clearButton.setColour (juce::TextButton::textColourOffId, green);
    clearButton.onClick = [this] {
        auto& p = processor.editPattern();
        for (int i = 0; i < kSteps; ++i) { p.steps[i].on = false; p.steps[i].slide = false; p.steps[i].accent = false; p.steps[i].note = 36; }
        processor.commitPattern(); refreshGrid();
    };
    addAndMakeVisible (clearButton);

    refreshGrid();

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
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "waveform", waveformCombo);
    // Built-in 303 pattern presets dropdown (60: 20 Acid / 20 Techno / 20 Hardcore).
    presetCombo.addItemList (processor.presetChoices(), 1);
    presetCombo.setSelectedItemIndex (0, juce::dontSendNotification);
    presetCombo.onChange = [this] {
        processor.loadPreset (presetCombo.getSelectedItemIndex());
        refreshGrid();   // show the loaded pattern in the grid
    };
    addAndMakeVisible (presetCombo);
    // Distortion knob.
    distortionSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    distortionSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 16);
    distortionSlider.setRange (0.0, 1.0, 0.01);
    distortionSlider.setColour (juce::Slider::rotarySliderFillColourId, green);
    addAndMakeVisible (distortionSlider);
    seqRunAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "seqrun", seqRunButton);
    seqTempoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "seqtempo", seqTempoSlider);
    presetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "preset", presetCombo);
    distortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "distortion", distortionSlider);
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "swing", swingSlider);
}

TB303Editor::~TB303Editor() {}

void TB303Editor::refreshGrid()
{
    const auto& p = processor.editPattern();   // active A or B
    for (int i = 0; i < kSteps; ++i)
    {
        noteSlider[i].setValue  (p.steps[i].note,   juce::dontSendNotification);
        onButton[i].setToggleState  (p.steps[i].on,     juce::dontSendNotification);
        slideButton[i].setToggleState (p.steps[i].slide,  juce::dontSendNotification);
        accentButton[i].setToggleState(p.steps[i].accent, juce::dontSendNotification);
    }
}

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

    // Second row: waveform selector + DRIVE / Run / SWING / Tempo, spaced out.
    const int rowY = knobY + knobSize + labelH + 14;
    const int comboW = 120, comboH = 24;
    waveformCombo.setBounds (bounds.getX(), rowY, comboW, comboH);
    waveformLabel.setBounds (bounds.getX(), rowY + comboH, comboW, labelH);
    // Spread the small controls with comfortable horizontal padding.
    const int c0 = bounds.getX() + comboW + 30;   // start of the control cluster
    const int cGap = 86;                            // horizontal padding between controls
    distortionSlider.setBounds (c0,               rowY - 6, 56, 56);
    distortionLabel.setBounds (c0,               rowY + 52, 56, labelH);
    seqRunButton.setBounds    (c0 + cGap,        rowY,     70, comboH);
    swingSlider.setBounds     (c0 + cGap + 86,   rowY - 6, 56, 56);
    swingLabel.setBounds      (c0 + cGap + 86,   rowY + 52, 56, labelH);
    seqTempoSlider.setBounds  (c0 + cGap + 172,  rowY - 6, 56, 56);
    seqTempoLabel.setBounds   (c0 + cGap + 172,  rowY + 52, 56, labelH);

    // A/B + Random + Clear (sequencer action row). Pushed well below the knob
    // band so the buttons never overlap the DRIVE/SWING/TEMPO readouts.
    const int seqY = rowY + 78;
    abButton.setBounds (bounds.getX(),       seqY, 70, comboH);
    randomButton.setBounds (bounds.getX() + 80,  seqY, 70, comboH);
    clearButton.setBounds  (bounds.getX() + 160, seqY, 70, comboH);

    // Preset dropdown: directly ABOVE the 16-step grid (where the arrow points).
    const int presetY = seqY + comboH + 12;
    presetCombo.setBounds (bounds.getX(), presetY, bounds.getWidth(), comboH);

    // 16-step grid: a note slider + On/Slide/Accent toggles per step.
    const int gridY = presetY + comboH + 12;
    const int stepW = 34, stepGap = 2, stepStride = stepW + stepGap;
    const int gridX = bounds.getX();
    const int noteH = 70, togH = 16, togGap = 2;
    const int stepTop = gridY + comboH + 8;
    for (int i = 0; i < kSteps; ++i)
    {
        int sx = gridX + i * stepStride;
        noteSlider[i].setBounds (sx, stepTop, stepW, noteH);
        onButton[i].setBounds    (sx, stepTop + noteH + togGap, stepW, togH);
        slideButton[i].setBounds (sx, stepTop + noteH + togGap + (togH + togGap), stepW, togH);
        accentButton[i].setBounds(sx, stepTop + noteH + togGap + 2 * (togH + togGap), stepW, togH);
    }
}
