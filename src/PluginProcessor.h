#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpaceDrumVoice.h"

class SpaceDrumProcessor : public juce::AudioProcessor
{
public:
    SpaceDrumProcessor();

    void prepareToPlay (double sampleRate, int) override { voice.prepare (sampleRate); }
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DrumNSpace"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Factory presets, exposed as host programs
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Pad hit from the UI (any thread). Fires at the start of the next audio block.
    void strike (float velocity) { pendingStrike = velocity; }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> hitCount { 0 }; // bumps on every hit; UI polls it for the LED
    std::atomic<int> spanStartNote { -1 }, spanStopNote { -1 }; // active Two-Note span, -1 when none

    enum Mode { Fixed, KeyTrack, TwoNote };

private:
    void hit (float velocity);

    SpaceDrumVoice voice;
    int currentProgram = 0;
    std::atomic<float> pendingStrike { -1.0f };
    bool held[128] = {};
    float keyOffset = 0.0f;                    // octaves, Key Track
    float spanStartHz = 0.0f, spanStopHz = 0.0f; // > 0 while a Two-Note glide is active

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceDrumProcessor)
};
