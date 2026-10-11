#include "PluginEditor.h"
#include "BinaryData.h"

#define PNG(name) juce::ImageCache::getFromMemory (BinaryData::name##_png, BinaryData::name##_pngSize)

namespace
{
const juce::Colour cream { 0xfff6ebd7 }, navy { 0xff0b2f4a }, inset { 0xff08263a }, stripFill { 0xff072033 }, ledRed { 0xffff2d20 };
constexpr float W = 720.0f, H = 480.0f; // every position below is in this base space

// Knob centres and drawn sizes (knob image, tick scale behind it)
constexpr float bigY = 140.0f, bigX[] = { 163.0f, 263.0f, 364.0f, 469.0f }, bigSize = 64.0f, bigTicks = 84.0f;
constexpr float modY = 92.0f, modX[] = { 580.0f, 649.0f }, modSize = 46.0f, modTicks = 60.0f;
constexpr float trimY = 415.0f, trimX[] = { 462.0f, 603.0f }, trimSize = 36.0f, trimTicks = 48.0f;
// Pointer angle drawn into each knob PNG (measured, clockwise from 12 o'clock)
constexpr float bakedLarge = -38.0f, bakedMedium = 47.0f, bakedSmall = 124.5f;

constexpr juce::Point<float> presetC { 300, 415 };
constexpr juce::Point<float> modeC { 502, 272 }, dirC { 631, 278 }, padC { 259, 292 }, ledC { 79, 415 }, noteC[] = { { 472, 333 }, { 541, 333 } };
const juce::Rectangle<float> deluxeBox { 543, 39, 143, 132 }, pitchBox { 416, 201, 270, 165 }, dirBox { 588, 216, 86, 106 }, strip { 44, 392, 632, 46 };

constexpr float minDeg = -150.0f, travelDeg = 300.0f;
constexpr float modeDeg[] = { -50.0f, 0.0f, 50.0f };

void place (juce::Component& c, juce::Point<float> centre, float w, float h, float k)
{
    c.setBounds ((juce::Rectangle<float> (w, h).withCentre (centre) * k).toNearestInt());
}

juce::Point<float> polar (juce::Point<float> c, float r, float deg) // deg clockwise from 12 o'clock
{
    return c.getPointOnCircumference (r, juce::degreesToRadians (deg));
}
}

//==============================================================================
ImageKnob::ImageKnob (juce::Image img, float bakedDeg)
    : juce::Slider (RotaryVerticalDrag, NoTextBox), image (img), baked (bakedDeg) {}

void ImageKnob::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float deg = minDeg + travelDeg * (float) valueToProportionOfLength (getValue()) - baked;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (image, juce::AffineTransform::translation (-0.5f * (float) image.getWidth(), -0.5f * (float) image.getHeight())
                                       .rotated (juce::degreesToRadians (deg))
                                       .scaled (juce::jmin (b.getWidth(), b.getHeight()) / (float) image.getWidth())
                                       .translated (b.getCentre()));
}

//==============================================================================
ModeSelector::ModeSelector() : juce::Slider (RotaryVerticalDrag, NoTextBox) { setMouseDragSensitivity (120); }

void ModeSelector::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked()) // still inside the drag gesture Slider opened on mouseDown
        setValue ((double) (((int) getValue() + 1) % 3));
    juce::Slider::mouseUp (e);
}

void ModeSelector::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale ((float) getWidth() / 56.0f)); // draw in 56x56 units
    const juce::Point<float> c { 28, 28 };

    g.setColour (cream);
    for (auto d : modeDeg)
        g.drawLine ({ polar (c, 23.0f, d), polar (c, 27.0f, d) }, 1.6f);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (juce::Rectangle<float> (43, 43).withCentre (c.translated (0, 1.5f)));

    auto disc = [&] (float r, juce::Colour top, juce::Colour bottom)
    {
        g.setGradientFill ({ top, c.x, c.y - r, bottom, c.x, c.y + r, false });
        g.fillEllipse (juce::Rectangle<float> (2 * r, 2 * r).withCentre (c));
    };
    disc (20.0f, juce::Colour (0xfff4f4f4), juce::Colour (0xff5e5e5e)); // chrome ring
    disc (17.5f, juce::Colour (0xff6a6a6a), juce::Colour (0xffe6e6e6)); // inner bevel
    disc (15.5f, juce::Colour (0xff2c2c2c), juce::Colour (0xff070707)); // black knob

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (juce::Rectangle<float> (25, 25).withCentre (c), 0.8f);

    const float deg = modeDeg[juce::jlimit (0, 2, (int) getValue())];
    g.setColour (cream);
    g.drawLine ({ polar (c, 2.5f, deg), polar (c, 13.5f, deg) }, 2.2f);
    g.fillEllipse (juce::Rectangle<float> (3.5f, 3.5f).withCentre (c));
}

//==============================================================================
DirectionToggle::DirectionToggle (juce::Image up, juce::Image down)
    : juce::Button ("Direction"), upImg (up), downImg (down)
{
    setClickingTogglesState (true);
}

void DirectionToggle::paintButton (juce::Graphics& g, bool, bool)
{
    const auto& img = getToggleState() ? downImg : upImg; // toggle-on leans left -> up once turned 90 degrees
    const auto b = getLocalBounds().toFloat();
    const float s = juce::jmin (b.getHeight() / (float) img.getWidth(), b.getWidth() / (float) img.getHeight());
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (img, juce::AffineTransform::translation (-0.5f * (float) img.getWidth(), -0.5f * (float) img.getHeight())
                                     .rotated (juce::MathConstants<float>::halfPi)
                                     .scaled (s)
                                     .translated (b.getCentre()));
}

//==============================================================================
StrikePad::StrikePad (SpaceDrumProcessor& p, juce::Image idle, juce::Image pressed)
    : proc (p), idleImg (idle), pressedImg (pressed) {}

void StrikePad::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (down ? pressedImg : idleImg, getLocalBounds().toFloat());
}

static float padRadius (const juce::Component& c) { return 0.4675f * (float) c.getWidth(); } // rubber+chrome edge in the PNG

bool StrikePad::hitTest (int x, int y)
{
    return getLocalBounds().toFloat().getCentre().getDistanceFrom ({ (float) x, (float) y }) <= padRadius (*this);
}

void StrikePad::mouseDown (const juce::MouseEvent& e)
{
    const float d = e.position.getDistanceFrom (getLocalBounds().toFloat().getCentre()) / padRadius (*this);
    proc.strike (1.0f - 0.5f * juce::jmin (d, 1.0f)); // centre 1.0 -> edge 0.5
    down = true;
    repaint();
}

void StrikePad::mouseUp (const juce::MouseEvent&)
{
    down = false;
    repaint();
}

//==============================================================================
void HitLed::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (unlitImg, b);
    if (level > 0.0f)
    {
        g.setOpacity (level);
        g.drawImage (litImg, b);
    }
}

//==============================================================================
void NoteDisplay::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale ((float) getWidth() / 50.0f)); // draw in 50x30 units

    g.setGradientFill ({ juce::Colour (0xffe2e2e2), 0, 0, juce::Colour (0xff545454), 0, 30, false }); // chrome rim
    g.fillRoundedRectangle (0.5f, 0.5f, 49, 29, 4.5f);
    g.setColour (juce::Colour (0xff1c1c1c));
    g.fillRoundedRectangle (2.5f, 2.5f, 45, 25, 3);
    g.setGradientFill ({ juce::Colour (0xff1d0e0b), 0, 4, juce::Colour (0xff100807), 0, 26, false });
    g.fillRoundedRectangle (3.5f, 3.5f, 43, 23, 2);

    // segments a..g = bits 0..6
    static constexpr int digits[] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111 };
    static constexpr int letters[] = { 57, 57, 94, 94, 121, 113, 113, 61, 61, 119, 119, 124 }; // C C# d d# E F F# G G# A A# b
    static constexpr bool sharp[] = { false, true, false, true, false, false, true, false, true, false, true, false };
    constexpr int dash = 64;

    const int letterMask = note < 0 ? dash : letters[note % 12];
    const int octave = note / 12 - 1;
    const int octaveMask = note < 0 || octave < 0 ? dash : digits[octave]; // ponytail: octave -1 (notes 0-11) reads as "-"
    const bool isSharp = note >= 0 && sharp[note % 12];

    const juce::Colour ghost = ledRed.withAlpha (0.09f);
    const float t = 2.2f, gap = 0.45f, top = 7.5f, h = 15.0f;

    auto slant = [&] (juce::Path p)
    {
        const float mid = top + 0.5f * h;
        p.applyTransform (juce::AffineTransform::translation (0, -mid).sheared (-0.12f, 0).translated (0, mid));
        return p;
    };
    auto bar = [&] (juce::Point<float> a, juce::Point<float> b) // pointed segment from a to b
    {
        const auto dir = (b - a) / a.getDistanceFrom (b), n = juce::Point<float> (-dir.y, dir.x) * (0.5f * t);
        const auto in = dir * (0.5f * t);
        juce::Path p;
        p.startNewSubPath (a);
        p.lineTo (a + in + n); p.lineTo (b - in + n); p.lineTo (b); p.lineTo (b - in - n); p.lineTo (a + in - n);
        p.closeSubPath();
        return p;
    };
    auto draw = [&] (const juce::Path& p, bool lit)
    {
        if (lit)
        {
            g.setColour (ledRed.withAlpha (0.25f));
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        g.setColour (lit ? ledRed : ghost);
        g.fillPath (p);
    };
    auto cell = [&] (float x, float w, int mask)
    {
        const float m = top + 0.5f * h, b = top + h;
        const std::pair<juce::Point<float>, juce::Point<float>> segs[] = {
            { { x + gap, top }, { x + w - gap, top } },  // a
            { { x + w, top + gap }, { x + w, m - gap } }, // b
            { { x + w, m + gap }, { x + w, b - gap } },   // c
            { { x + gap, b }, { x + w - gap, b } },       // d
            { { x, m + gap }, { x, b - gap } },           // e
            { { x, top + gap }, { x, m - gap } },         // f
            { { x + gap, m }, { x + w - gap, m } },       // g
        };
        for (int i = 0; i < 7; ++i)
            draw (slant (bar (segs[i].first, segs[i].second)), (mask >> i) & 1);
    };

    cell (11.0f, 9.0f, letterMask);
    cell (32.0f, 9.0f, octaveMask);

    // narrow raised '#': two verticals, two horizontals
    juce::Path hash;
    hash.addRectangle (24.0f, top, 1.1f, 8.0f);
    hash.addRectangle (26.6f, top, 1.1f, 8.0f);
    hash.addRectangle (22.8f, top + 2.2f, 6.2f, 1.1f);
    hash.addRectangle (22.8f, top + 4.8f, 6.2f, 1.1f);
    draw (slant (hash), isSharp);
}

//==============================================================================
juce::Font SpaceDrumEditor::PopupLnF::getSliderPopupFont (juce::Slider&) { return font; }
juce::Font SpaceDrumEditor::PopupLnF::getComboBoxFont (juce::ComboBox& b) { return font.withHeight (0.6f * (float) b.getHeight()); }
juce::Font SpaceDrumEditor::PopupLnF::getPopupMenuFont() { return font.withHeight (15.0f); }

SpaceDrumEditor::SpaceDrumEditor (SpaceDrumProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      start (PNG (knoblarge), bakedLarge), stop (PNG (knoblarge), bakedLarge),
      sweep (PNG (knoblarge), bakedLarge), decay (PNG (knoblarge), bakedLarge),
      modRate (PNG (knobmedium), bakedMedium), modDepth (PNG (knobmedium), bakedMedium),
      sens (PNG (knobsmall), bakedSmall), volume (PNG (knobsmall), bakedSmall),
      direction (PNG (toggleon), PNG (toggleoff)),
      pad (p, PNG (strikepadidle), PNG (strikepadpressed)),
      led (PNG (ledunlit), PNG (ledlit)),
      directionAttachment (p.apvts, "direction", direction)
{
    font = juce::Font (juce::FontOptions (juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceGroteskBold_ttf,
                                                                                 BinaryData::SpaceGroteskBold_ttfSize)));
    lnf.font = font.withHeight (14.0f);
    lnf.setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0111111));
    lnf.setColour (juce::BubbleComponent::outlineColourId, cream.withAlpha (0.6f));
    lnf.setColour (juce::TooltipWindow::textColourId, cream);
    lnf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0a1824));
    lnf.setColour (juce::ComboBox::textColourId, cream);
    lnf.setColour (juce::ComboBox::outlineColourId, cream.withAlpha (0.5f));
    lnf.setColour (juce::ComboBox::arrowColourId, cream);
    lnf.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0a1f30));
    lnf.setColour (juce::PopupMenu::textColourId, cream);
    lnf.setColour (juce::PopupMenu::highlightedBackgroundColourId, navy.brighter (0.6f));
    lnf.setColour (juce::PopupMenu::highlightedTextColourId, cream);
    setLookAndFeel (&lnf);

    const std::pair<juce::Slider*, const char*> binds[] = {
        { &start, "start" }, { &stop, "stop" }, { &sweep, "sweepTime" }, { &decay, "decay" },
        { &modRate, "modRate" }, { &modDepth, "modDepth" }, { &sens, "sens" }, { &volume, "volume" }, { &mode, "mode" },
    };
    for (auto [slider, id] : binds)
    {
        sliderAttachments.push_back (std::make_unique<SA> (p.apvts, id, *slider));
        if (slider != &mode) // a double-click on the selector is two cycling clicks
        {
            auto* param = p.apvts.getParameter (id);
            slider->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
            slider->setPopupDisplayEnabled (true, false, this);
        }
        addAndMakeVisible (*slider);
    }
    addAndMakeVisible (direction);
    addAndMakeVisible (pad);
    addAndMakeVisible (led);
    addAndMakeVisible (startNote);
    addAndMakeVisible (stopNote);

    for (int i = 0; i < p.getNumPrograms(); ++i)
        presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setSelectedId (p.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        proc.setCurrentProgram (presetBox.getSelectedId() - 1);
        proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
    };
    addAndMakeVisible (presetBox);

    lastHits = p.hitCount.load();

    setResizable (true, true);
    setResizeLimits (540, 360, 1440, 960);
    getConstrainer()->setFixedAspectRatio (1.5);
    setSize ((int) W, (int) H);
    startTimerHz (30);
}

SpaceDrumEditor::~SpaceDrumEditor() { setLookAndFeel (nullptr); }

void SpaceDrumEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (const int h = proc.hitCount.load(); h != lastHits)
    {
        lastHits = h;
        hitTime = now;
    }
    const float level = juce::jlimit (0.0f, 1.0f, 1.0f - (float) ((now - hitTime) / 150.0));
    if (! juce::exactlyEqual (level, led.level))
    {
        led.level = level;
        led.repaint();
    }

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1) // host changed program
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);

    const bool twoNote = (int) proc.apvts.getRawParameterValue ("mode")->load() == SpaceDrumProcessor::TwoNote;
    const int notes[] = { twoNote ? proc.spanStartNote.load() : -1, twoNote ? proc.spanStopNote.load() : -1 };
    NoteDisplay* displays[] = { &startNote, &stopNote };
    for (int i = 0; i < 2; ++i)
        if (displays[i]->note != notes[i])
        {
            displays[i]->note = notes[i];
            displays[i]->repaint();
        }
}

void SpaceDrumEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff17191a));
    g.drawImage (panel, getLocalBounds().toFloat());
}

void SpaceDrumEditor::resized()
{
    const float k = (float) getWidth() / W;

    ImageKnob* big[] = { &start, &stop, &sweep, &decay };
    for (int i = 0; i < 4; ++i) place (*big[i], { bigX[i], bigY }, bigSize, bigSize, k);
    place (modRate, { modX[0], modY }, modSize, modSize, k);
    place (modDepth, { modX[1], modY }, modSize, modSize, k);
    place (sens, { trimX[0], trimY }, trimSize, trimSize, k);
    place (volume, { trimX[1], trimY }, trimSize, trimSize, k);
    place (mode, modeC, 56, 56, k);
    place (direction, dirC, 30, 44, k);
    place (pad, padC, 200, 200, k);
    place (led, ledC, 40, 40, k);
    place (startNote, noteC[0], 50, 30, k);
    place (stopNote, noteC[1], 50, 30, k);
    place (presetBox, presetC, 170, 22, k);

    // Cache the static panel at physical resolution.
    // ponytail: scale read once per resize; a window dragged to a different-DPI screen keeps the old cache until resized
    float ds = 1.0f;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()))
        ds = (float) d->scale;
    panel = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) getWidth() * ds)),
                         juce::jmax (1, juce::roundToInt ((float) getHeight() * ds)), true);
    juce::Graphics pg (panel);
    pg.addTransform (juce::AffineTransform::scale ((float) panel.getWidth() / W));
    renderPanel (pg);
}

void SpaceDrumEditor::renderPanel (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    auto image = [&] (const juce::Image& img, float x, float y, float w, float h) { g.drawImage (img, { x, y, w, h }); };

    // 1. enclosure
    image (PNG (enclosureframe), 0, 0, W, H);

    // 2. decorative space artwork
    const auto trails = PNG (cosmictrails), stars = PNG (starssparkles);
    image (trails, 20, 52, 530, 100);
    image (trails, 18, 300, 200, 112);
    image (stars, 362, 24, 170, 142);
    image (stars, 36, 186, 110, 92);
    image (PNG (planetorbit), 214, 22, 96, 96);
    g.setFont (font.withHeight (47.0f).withExtraKerningFactor (0.04f)); // wordmark
    const char* wordmark[] = { "DRUM N", "SPACE" };
    for (int line = 0; line < 2; ++line)
        for (auto [offset, colour] : { std::pair { 2.5f, juce::Colour (0xff061a29) }, std::pair { 0.0f, cream } })
        {
            g.setColour (colour);
            g.drawText (wordmark[line], juce::Rectangle<float> (200, 44).withPosition (44 + offset, 25 + 43.0f * (float) line + offset),
                        juce::Justification::centredLeft, false);
        }

    g.setColour (cream.withAlpha (0.9f)); // sunburst around the pad
    int i = 0;
    for (float deg : { -130.0f, -110.0f, -90.0f, -70.0f, -50.0f, 50.0f, 70.0f, 90.0f, 110.0f, 130.0f })
    {
        const float len = (i++ % 2) ? 14.0f : 24.0f, half = 1.6f;
        juce::Path ray;
        ray.addTriangle (polar (padC, 104.0f, deg),
                         polar (padC, 104.0f + len, deg) + juce::Point<float> (half, 0).rotatedAboutOrigin (juce::degreesToRadians (deg)),
                         polar (padC, 104.0f + len, deg) - juce::Point<float> (half, 0).rotatedAboutOrigin (juce::degreesToRadians (deg)));
        g.fillPath (ray);
    }

    // faceplate insets (drawn to the mockup's geometry)
    auto box = [&] (juce::Rectangle<float> r, float radius, juce::Colour fill, float line)
    {
        if (! fill.isTransparent())
        {
            g.setColour (fill);
            g.fillRoundedRectangle (r, radius);
        }
        g.setColour (cream);
        g.drawRoundedRectangle (r, radius, line);
    };
    box (deluxeBox, 9, inset, 1.6f);
    box (pitchBox, 9, inset, 1.6f);
    box (dirBox, 7, {}, 1.2f);
    box (strip, 7, stripFill, 1.6f);

    // 3. tick scales (gap rotated from 9 o'clock to the bottom)
    const auto tickImg = PNG (knobtickscale);
    auto ticks = [&] (float cx, float cy, float size)
    {
        g.drawImageTransformed (tickImg, juce::AffineTransform::translation (-120, -120)
                                             .rotated (-juce::MathConstants<float>::halfPi)
                                             .scaled (size / 240.0f)
                                             .translated (cx, cy));
    };
    for (auto x : bigX) ticks (x, bigY, bigTicks);
    for (auto x : modX) ticks (x, modY, modTicks);
    for (auto x : trimX) ticks (x, trimY, trimTicks);

    // 5. labels and range text
    auto text = [&] (const juce::String& s, float cx, float cy, float h, juce::Justification j = juce::Justification::centred)
    {
        g.setColour (cream);
        g.setFont (font.withHeight (h).withExtraKerningFactor (0.04f));
        auto r = juce::Rectangle<float> (200, h * 1.5f).withCentre ({ cx, cy });
        if (j == juce::Justification::centredLeft) r.setX (cx);
        g.drawText (s, r, j, false);
    };
    const auto left = juce::Justification::centredLeft;

    const char* bigNames[] = { "START FREQ", "STOP FREQ", "SWEEP TIME", "DECAY TIME" };
    const char* bigRange[][2] = { { "20", "2k" }, { "20", "2k" }, { "0.02", "1s" }, { "0.05", "4s" } };
    for (int k = 0; k < 4; ++k)
    {
        text (bigRange[k][0], bigX[k] - 28, bigY + 34, 9.5f);
        text (bigRange[k][1], bigX[k] + 28, bigY + 34, 9.5f);
        text (bigNames[k], bigX[k], bigY + 49, 11.5f);
    }

    text ("DELUXE", deluxeBox.getCentreX(), deluxeBox.getY() + 12, 12.5f);
    text ("0.1", modX[0] - 22, modY + 31, 8.5f);
    text ("30 Hz", modX[0] + 24, modY + 31, 8.5f);
    text ("0", modX[1] - 22, modY + 31, 8.5f);
    text ("100%", modX[1] + 24, modY + 31, 8.5f);
    text ("MOD RATE", modX[0], modY + 44, 10.5f);
    text ("MOD DEPTH", modX[1], modY + 44, 10.5f);

    text ("PITCH MODE", modeC.x, pitchBox.getY() + 12, 12.0f);
    text ("FIXED", modeC.x - 45, modeC.y - 22, 9.5f);
    text ("KEY", modeC.x, modeC.y - 45, 9.5f);
    text ("TRACK", modeC.x, modeC.y - 35, 9.5f);
    text ("TWO-NOTE", modeC.x + 47, modeC.y - 22, 9.5f);

    g.setColour (cream); // bracket grouping the two readouts
    g.drawLine (noteC[0].x - 26, 311, noteC[1].x + 26, 311, 1.0f);
    g.drawLine (noteC[0].x - 26, 311, noteC[0].x - 26, 315, 1.0f);
    g.drawLine (noteC[1].x + 26, 311, noteC[1].x + 26, 315, 1.0f);
    text ("START", noteC[0].x, noteC[0].y + 24, 9.5f);
    text ("STOP", noteC[1].x, noteC[1].y + 24, 9.5f);
    text ("(TWO-NOTE MODE ONLY)", 627, noteC[0].y, 7.5f);

    text ("DIRECTION", dirC.x, dirBox.getY() + 13, 10.0f);
    text (juce::String::fromUTF8 ("HIGH \xe2\x86\x92 LOW"), dirC.x, dirC.y - 27, 9.0f);
    text (juce::String::fromUTF8 ("LOW \xe2\x86\x92 HIGH"), dirC.x, dirC.y + 27, 9.0f);

    // STRIKE sits on the strip's top edge
    const auto gapR = juce::Rectangle<float> (64, 4).withCentre ({ padC.x, strip.getY() });
    g.setColour (navy);
    g.fillRect (gapR.withBottom (strip.getY()));
    g.setColour (stripFill);
    g.fillRect (gapR.withTop (strip.getY()));
    text ("STRIKE", padC.x, strip.getY(), 11.5f);

    text ("HIT", ledC.x + 19, ledC.y, 10.5f, left);
    text ("PRESET", presetC.x - 142, presetC.y, 10.0f, left);
    text ("SENSITIVITY", trimX[0] + 24, trimY - 6, 10.0f, left);
    text ("VOLUME", trimX[1] + 24, trimY - 6, 10.0f, left);
    for (auto x : trimX)
    {
        text ("0", x - 18, trimY + 15, 8.5f);
        text ("100%", x + 20, trimY + 15, 8.5f, left);
    }
}
