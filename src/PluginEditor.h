#pragma once
#include "PluginProcessor.h"

// Knob that turns by rotating its whole PNG. bakedDeg = pointer angle drawn into the image
// (clockwise from 12 o'clock), subtracted so the parameter minimum points to ~7 o'clock.
struct ImageKnob : juce::Slider
{
    ImageKnob (juce::Image img, float bakedDeg);
    void paint (juce::Graphics&) override;

    juce::Image image;
    float baked;
};

// PITCH MODE: drawn in code (the kit PNG has its ticks baked in). Drag snaps, click cycles.
struct ModeSelector : juce::Slider
{
    ModeSelector();
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
};

// DIRECTION: toggle PNG turned 90 degrees. Off (lever up) = High > Low.
struct DirectionToggle : juce::Button
{
    DirectionToggle (juce::Image up, juce::Image down);
    void paintButton (juce::Graphics&, bool, bool) override;

    juce::Image upImg, downImg;
};

struct StrikePad : juce::Component
{
    StrikePad (SpaceDrumProcessor&, juce::Image idle, juce::Image pressed);
    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    SpaceDrumProcessor& proc;
    juce::Image idleImg, pressedImg;
    bool down = false;
};

struct HitLed : juce::Component
{
    HitLed (juce::Image unlit, juce::Image lit) : unlitImg (unlit), litImg (lit) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;

    juce::Image unlitImg, litImg;
    float level = 0.0f; // lit-image alpha
};

// Red 7-segment note readout: letter, sharp, octave. note < 0 shows "--".
struct NoteDisplay : juce::Component
{
    NoteDisplay() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;

    int note = -1;
};

class SpaceDrumEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpaceDrumEditor (SpaceDrumProcessor&);
    ~SpaceDrumEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderPanel (juce::Graphics&);

    struct PopupLnF : juce::LookAndFeel_V4
    {
        juce::Font getSliderPopupFont (juce::Slider&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        juce::Font font { juce::FontOptions() };
    };

    SpaceDrumProcessor& proc;
    PopupLnF lnf;
    juce::Font font { juce::FontOptions() };
    juce::Image panel;

    ImageKnob start, stop, sweep, decay, modRate, modDepth, sens, volume;
    ModeSelector mode;
    DirectionToggle direction;
    StrikePad pad;
    HitLed led;
    NoteDisplay startNote, stopNote;
    juce::ComboBox presetBox;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<SA>> sliderAttachments;
    juce::AudioProcessorValueTreeState::ButtonAttachment directionAttachment;

    int lastHits = 0;
    double hitTime = -1.0e9;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceDrumEditor)
};
