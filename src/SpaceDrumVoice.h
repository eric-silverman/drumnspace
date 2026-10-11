#pragma once
#include <cmath>

// Model of the EHX Space Drum (per original schematic):
// triangle VCO whose control voltage glides from START to STOP freq through an RC
// (Sweep Time), into a VCA with its own RC decay (Decay Time). Deluxe adds an LFO
// (Mod Rate / Mod Depth) on pitch. Mono, retriggers on every hit like the hardware.
struct SpaceDrumVoice
{
    struct Params
    {
        float startHz   = 600.0f;
        float stopHz    = 80.0f;
        float sweepSec  = 0.22f;  // RC time constant of the glide (stock: 2M2 * 0.1uF)
        float decaySec  = 0.6f;   // time to -60 dB
        float modRateHz = 5.0f;
        float modDepth  = 0.0f;   // 0..1 -> up to +/-1 octave
    };

    void prepare (double sampleRate) { sr = (float) sampleRate; env = 0.0f; stage = Idle; }

    void trigger (float velocity)
    {
        level = velocity;
        stage = Attack;
        glide = 1.0f;
        phase = 0.0f; // ponytail: hard phase reset like the hardware; 1 ms attack hides the click
    }

    bool isActive() const { return stage != Idle; }

    float process (const Params& p)
    {
        if (stage == Idle) return 0.0f;

        if (stage == Attack)
        {
            env += 1.0f / (0.001f * sr);
            if (env >= 1.0f) { env = 1.0f; stage = Decay; }
        }
        else
        {
            env *= std::exp (-6.9078f / (p.decaySec * sr)); // ln(1000) -> 60 dB over decaySec
            if (env < 1.0e-5f) { env = 0.0f; stage = Idle; return 0.0f; }
        }

        // RC discharge: linear-in-Hz glide, fast at first then settling on stop freq
        glide *= std::exp (-1.0f / (p.sweepSec * sr));
        float hz = p.stopHz + (p.startHz - p.stopHz) * glide;

        lfoPhase += p.modRateHz / sr;
        lfoPhase -= std::floor (lfoPhase);
        hz = std::fmin (hz * std::exp2 (p.modDepth * std::sin (6.2831853f * lfoPhase)), 0.45f * sr);
        phase += hz / sr;
        phase -= std::floor (phase);

        // triangle starting at zero crossing, gently rounded like the analog VCO
        float tri = phase < 0.25f ? 4.0f * phase
                  : phase < 0.75f ? 2.0f - 4.0f * phase
                                  : 4.0f * phase - 4.0f;
        float osc = std::tanh (1.5f * tri) / std::tanh (1.5f);

        return osc * env * level;
    }

private:
    enum Stage { Idle, Attack, Decay };
    Stage stage = Idle;
    float sr = 44100.0f, env = 0.0f, level = 1.0f, glide = 0.0f, phase = 0.0f, lfoPhase = 0.0f;
};

// Two-Note mode: glide between the lowest and highest held MIDI notes.
// Returns false (leaving start/stop untouched) unless at least two distinct notes are held.
inline bool twoNoteNotes (const bool (&held)[128], bool lowToHigh, int& startNote, int& stopNote)
{
    int lo = -1, hi = -1;
    for (int n = 0; n < 128; ++n)
        if (held[n]) { if (lo < 0) lo = n; hi = n; }
    if (lo == hi) return false;

    startNote = lowToHigh ? lo : hi;
    stopNote  = lowToHigh ? hi : lo;
    return true;
}

// Same span in Hz.
inline bool twoNoteSpan (const bool (&held)[128], bool lowToHigh, float& startHz, float& stopHz)
{
    int a, b;
    if (! twoNoteNotes (held, lowToHigh, a, b)) return false;

    auto mtof = [] (int n) { return 440.0f * std::exp2 ((float) (n - 69) / 12.0f); };
    startHz = mtof (a);
    stopHz  = mtof (b);
    return true;
}
