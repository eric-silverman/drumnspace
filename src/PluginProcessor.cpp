#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using P = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;
    using A = juce::AudioParameterFloatAttributes;
    auto skewed = [] (float lo, float hi, float centre) { R r (lo, hi); r.setSkewForCentre (centre); return r; };
    const auto hz  = A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, v < 100.0f ? 1 : 0) + " Hz"; });
    const auto sec = A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " s"; });
    const auto pct = A().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; })
                        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    return {
        // ponytail: freq ranges guessed from the 10k log pots; calibrate against a real unit
        std::make_unique<P> (juce::ParameterID { "start", 1 },    "Start Freq",  skewed (20.0f, 2000.0f, 200.0f), 600.0f, hz),
        std::make_unique<P> (juce::ParameterID { "stop", 1 },     "Stop Freq",   skewed (20.0f, 2000.0f, 200.0f), 80.0f, hz),
        std::make_unique<P> (juce::ParameterID { "sweepTime", 1 },"Sweep Time",  skewed (0.02f, 1.0f, 0.22f), 0.22f, sec),
        std::make_unique<P> (juce::ParameterID { "decay", 1 },    "Decay Time",  skewed (0.05f, 4.0f, 0.6f), 0.6f, sec),
        std::make_unique<P> (juce::ParameterID { "modRate", 1 },  "Mod Rate",    skewed (0.1f, 30.0f, 5.0f), 5.0f, hz),
        std::make_unique<P> (juce::ParameterID { "modDepth", 1 }, "Mod Depth",   R (0.0f, 1.0f), 0.0f, pct),
        std::make_unique<C> (juce::ParameterID { "mode", 1 },     "Mode",        juce::StringArray { "Fixed", "Key Track", "Two-Note" }, 0),
        std::make_unique<C> (juce::ParameterID { "direction", 1 },"Direction",   juce::StringArray { "High > Low", "Low > High" }, 0),
        std::make_unique<P> (juce::ParameterID { "sens", 1 },     "Sensitivity", R (0.0f, 1.0f), 0.7f, pct),
        std::make_unique<P> (juce::ParameterID { "volume", 1 },   "Volume",      R (0.0f, 1.0f), 0.7f, pct),
    };
}

SpaceDrumProcessor::SpaceDrumProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

void SpaceDrumProcessor::hit (float velocity)
{
    const float sens = apvts.getRawParameterValue ("sens")->load();
    // sens 0 = every hit full level, 1 = fully velocity-scaled
    voice.trigger (1.0f - sens + sens * velocity);
    ++hitCount;
}

void SpaceDrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    SpaceDrumVoice::Params p;
    p.sweepSec  = v ("sweepTime");
    p.decaySec  = v ("decay");
    p.modRateHz = v ("modRate");
    p.modDepth  = v ("modDepth");
    const auto mode       = (Mode) (int) v ("mode");
    const bool lowToHigh  = v ("direction") > 0.5f;
    const float volume    = v ("volume");

    auto* out = buffer.getWritePointer (0);
    int pos = 0;

    auto render = [&] (int end)
    {
        if (spanStartHz > 0.0f)
        {
            p.startHz = spanStartHz;
            p.stopHz  = spanStopHz;
        }
        else
        {
            // DIRECTION picks which end of the START/STOP pair the glide begins from
            const float key = std::exp2 (keyOffset);
            const float a = v ("start") * key, b = v ("stop") * key;
            p.startHz = lowToHigh ? std::fmin (a, b) : std::fmax (a, b);
            p.stopHz  = lowToHigh ? std::fmax (a, b) : std::fmin (a, b);
        }
        for (; pos < end; ++pos)
            out[pos] = std::tanh (1.5f * voice.process (p)) * volume; // ponytail: tanh stands in for the analog output stage
    };

    if (const float s = pendingStrike.exchange (-1.0f); s >= 0.0f)
        hit (s); // ponytail: pad reuses the last pitch state; snaps to block start (<= one buffer late)

    for (const auto meta : midi)
    {
        render (meta.samplePosition);
        auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            held[note] = true;
            keyOffset = mode == Fixed ? 0.0f : (float) (note - 60) / 12.0f;
            spanStartHz = 0.0f;
            int startNote = -1, stopNote = -1;
            if (mode == TwoNote && twoNoteNotes (held, lowToHigh, startNote, stopNote)) // one key held -> falls back to Key Track
                twoNoteSpan (held, lowToHigh, spanStartHz, spanStopHz);
            spanStartNote = startNote;
            spanStopNote  = stopNote;
            hit (msg.getFloatVelocity());
        }
        else if (msg.isNoteOff())
            held[msg.getNoteNumber()] = false;
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            std::fill (std::begin (held), std::end (held), false);
    }
    render (buffer.getNumSamples());

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* SpaceDrumProcessor::createEditor() { return new SpaceDrumEditor (*this); }

namespace
{
struct Preset
{
    const char* name;
    float start, stop, sweepTime, decay, modRate, modDepth, mode, direction, sens, volume;
};

// mode: 0 Fixed, 1 Key Track, 2 Two-Note. direction: 0 High > Low, 1 Low > High
constexpr Preset presets[] = {
    //  name               start   stop  sweep  decay  mRate mDepth mode dir  sens  vol
    { "Classic Pew",       600,     80, 0.22f, 0.60f,  5.0f, 0.00f, 0, 0, 0.7f, 0.7f },
    { "Disco Tom",         220,     90, 0.15f, 0.45f,  5.0f, 0.00f, 0, 0, 0.8f, 0.7f },
    { "Laser Zap",        2000,    120, 0.08f, 0.30f,  5.0f, 0.00f, 0, 0, 0.6f, 0.6f },
    { "Deep Space Kick",   140,     40, 0.06f, 0.80f,  5.0f, 0.00f, 0, 0, 0.5f, 0.8f },
    { "Blip",             1500,   1300, 0.02f, 0.08f,  5.0f, 0.00f, 0, 0, 0.7f, 0.6f },
    { "Rising UFO",         80,    900, 0.50f, 1.20f,  7.0f, 0.10f, 0, 1, 0.7f, 0.6f },
    { "Wobble Drone",      300,    150, 0.80f, 3.50f,  6.0f, 0.35f, 0, 0, 0.4f, 0.6f },
    { "Red Alert",         700,    350, 0.40f, 2.50f,  2.0f, 0.50f, 0, 0, 0.5f, 0.55f },
    { "Melodic Tom",       300,    100, 0.20f, 0.50f,  5.0f, 0.00f, 1, 0, 0.8f, 0.7f },
    { "Two-Note Glide",    600,     80, 0.40f, 1.50f,  5.0f, 0.00f, 2, 0, 0.7f, 0.65f },
    { "Syndrum Snap",      900,    300, 0.05f, 0.18f,  5.0f, 0.00f, 0, 0, 0.8f, 0.7f },
    { "Asteroid Thud",      90,     30, 0.04f, 0.35f,  5.0f, 0.00f, 0, 0, 0.6f, 0.85f },
    { "Cosmic Cowbell",    820,    780, 0.10f, 0.25f,  5.0f, 0.00f, 0, 0, 0.7f, 0.55f },
    { "Falling Star",     2000,     40, 0.90f, 2.00f,  5.0f, 0.00f, 0, 0, 0.5f, 0.6f },
    { "Warp Drive",       1800,     60, 1.00f, 4.00f, 12.0f, 0.20f, 0, 0, 0.4f, 0.55f },
    { "Moon Bounce",       400,    120, 0.30f, 0.90f,  3.0f, 0.15f, 0, 0, 0.7f, 0.65f },
    { "Robot Chirp",      1200,    400, 0.03f, 0.12f, 25.0f, 0.30f, 0, 0, 0.7f, 0.6f },
    { "Pulsar",            500,    500, 0.10f, 3.00f, 18.0f, 0.25f, 0, 0, 0.5f, 0.55f },
    { "Sonar Ping",       1000,    950, 0.30f, 2.50f,  0.5f, 0.05f, 0, 0, 0.6f, 0.55f },
    { "Tractor Beam",       60,    600, 0.90f, 3.00f,  4.0f, 0.15f, 0, 1, 0.5f, 0.6f },
    { "Bubble Pop",        200,   1200, 0.04f, 0.10f,  5.0f, 0.00f, 0, 1, 0.8f, 0.6f },
    { "Arcade Coin",       900,   1800, 0.02f, 0.20f,  5.0f, 0.00f, 0, 1, 0.7f, 0.5f },
    { "Key Track Bass",    120,     50, 0.10f, 0.60f,  5.0f, 0.00f, 1, 0, 0.8f, 0.8f },
    { "Key Track Zap",    1500,    150, 0.08f, 0.25f,  5.0f, 0.00f, 1, 0, 0.8f, 0.6f },
    { "Two-Note Rise",     300,    300, 0.30f, 1.20f,  5.0f, 0.00f, 2, 1, 0.7f, 0.65f },
};
}

int SpaceDrumProcessor::getNumPrograms() { return (int) std::size (presets); }

const juce::String SpaceDrumProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, getNumPrograms()) ? presets[i].name : "";
}

void SpaceDrumProcessor::setCurrentProgram (int i)
{
    if (! juce::isPositiveAndBelow (i, getNumPrograms())) return;
    currentProgram = i;
    const auto& pr = presets[i];
    const std::pair<const char*, float> values[] = {
        { "start", pr.start }, { "stop", pr.stop }, { "sweepTime", pr.sweepTime }, { "decay", pr.decay },
        { "modRate", pr.modRate }, { "modDepth", pr.modDepth }, { "mode", pr.mode }, { "direction", pr.direction },
        { "sens", pr.sens }, { "volume", pr.volume },
    };
    for (auto [id, v] : values)
    {
        auto* param = apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (v));
    }
}

void SpaceDrumProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void SpaceDrumProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpaceDrumProcessor(); }
