// Build & run: c++ -std=c++20 -I../src test_voice.cpp -o /tmp/tv && /tmp/tv
#include "SpaceDrumVoice.h"
#include <cassert>
#include <cstdio>
#include <vector>

static int zeroCrossings (const std::vector<float>& x, size_t from, size_t to)
{
    int n = 0;
    for (size_t i = from + 1; i < to; ++i) n += (x[i - 1] < 0) != (x[i] < 0);
    return n;
}

int main()
{
    const double sr = 48000;
    SpaceDrumVoice v;
    SpaceDrumVoice::Params p;
    p.decaySec = 0.5f;
    v.prepare (sr);

    assert (v.process (p) == 0.0f); // silent before a hit

    v.trigger (1.0f);
    std::vector<float> x (sr);
    for (auto& s : x) s = v.process (p);

    float peak = 0;
    for (auto s : x) peak = std::fmax (peak, std::fabs (s));
    assert (peak > 0.5f && peak <= 1.0f);
    assert (! v.isActive()); // 0.5 s decay is done within 1 s

    // start > stop falls: more crossings in the first 50 ms than the next 50 ms
    const size_t w = sr / 20;
    assert (zeroCrossings (x, 0, w) > zeroCrossings (x, w, 2 * w));

    // start < stop rises
    p.startHz = 80.0f; p.stopHz = 600.0f;
    v.trigger (1.0f);
    for (auto& s : x) s = v.process (p);
    assert (zeroCrossings (x, 0, w) < zeroCrossings (x, w, 2 * w));

    // Two-Note span: needs two distinct held notes; direction picks the order
    bool held[128] = {};
    float a = 0, b = 0;
    held[60] = true;
    assert (! twoNoteSpan (held, false, a, b) && a == 0);
    held[72] = true;
    assert (twoNoteSpan (held, false, a, b) && a > b && std::fabs (b - 261.63f) < 0.1f); // high -> low, ends on C4
    assert (twoNoteSpan (held, true, a, b) && a < b && std::fabs (b - 523.25f) < 0.1f);  // low -> high, ends on C5
    held[66] = true; // a middle note doesn't change the span
    assert (twoNoteSpan (held, true, a, b) && std::fabs (a - 261.63f) < 0.1f);

    // note-number sibling: same span, same untouched-on-failure contract
    int s = -1, e = -1;
    assert (twoNoteNotes (held, false, s, e) && s == 72 && e == 60);
    assert (twoNoteNotes (held, true, s, e) && s == 60 && e == 72);
    held[60] = held[66] = false; // one note left
    s = e = -1;
    assert (! twoNoteNotes (held, true, s, e) && s == -1 && e == -1);

    std::puts ("ok");
}
