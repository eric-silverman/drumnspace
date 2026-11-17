# Space Drum UI Asset Pack

Reusable visual assets for rebuilding the Space Drum skeuomorphic audio-plugin mockup. These are standalone, transparent-background SVG assets, with 2× PNG exports when CairoSVG is available. No audio or plugin code is included.

## Files
- `svg/enclosure-frame.svg`: 720×480 metallic enclosure and navy faceplate.
- `svg/faceplate-lines.svg`: transparent panel outlines/dividers for Deluxe, Pitch Mode, and utility strip.
- `svg/strike-pad-idle.svg`, `svg/strike-pad-pressed.svg`: 200×200 rubber strike pad states.
- `svg/knob-large.svg`, `knob-medium.svg`, `knob-small.svg`, `knob-mini.svg`: standalone black knurled knobs with pointers and tick marks.
- `svg/knob-tick-scale.svg`: independent tick arc to use behind a knob if desired.
- `svg/toggle-on.svg`, `toggle-off.svg`: chrome toggle states.
- `svg/mode-selector.svg`: 3-position rotary selector.
- `svg/led-lit.svg`, `led-unlit.svg`: red indicator states.
- `svg/note-c2.svg`, `note-g3.svg`: lit note readout assets.
- `svg/corner-screw.svg`: standalone screw.
- `svg/space-drum-wordmark.svg`: editable-text wordmark (use an installed heavy rounded sans font or convert text to outlines in your build pipeline).
- `svg/planet-orbit.svg`, `cosmic-trails.svg`, `stars-sparkles.svg`: separate retro sci-fi graphic elements.
- `svg/control-label-template.svg`: example of the typography and range label layout.

## Palette
- Navy: `#0B2F4A`
- Orange: `#F77A00`
- Yellow: `#F2C94C`
- Cream / silkscreen: `#F6EBD7`
- Silver metal: `#B7B7B7`
- Black: `#111111`
- LED red: `#FF2D20`

## Suggested typography
Use a heavy rounded 1970s display sans for the wordmark (e.g. ITC Bauhaus, Space Grotesk Black, or a similar geometric display face). For controls, use a compact bold sans in uppercase. Use a monospaced or seven-segment font for note displays.

## Approximate UI dimensions (720×480 base)
- Outer enclosure: 720×480 px.
- Main knob visual diameter: 76–100 px; center-to-center spacing around 105–120 px.
- Small utility knob diameter: 38–52 px.
- Strike pad: 200×200 px asset; displayed at roughly 180–205 px diameter.
- Deluxe inset: roughly 154×128 px.
- Pitch Mode inset: roughly 254×117 px.
- Utility strip: roughly 44 px tall at the bottom.
- Keep 20–26 px inner margin for enclosure and screw clearance.

## Layering recommendation
1. Enclosure/frame
2. Faceplate lines and decorative space artwork
3. Tick scales and range text
4. Knobs / pad / switches / LED / readouts
5. Control labels and range labels

## CSS usage
Inline SVGs or reference them as image assets. For state changes, swap `strike-pad-idle.svg` ↔ `strike-pad-pressed.svg` and `led-unlit.svg` ↔ `led-lit.svg`. Keep labels as HTML text when you want accessibility and localization. SVG dimensions are intrinsic and assets have transparent backgrounds outside their shapes.

## Notes
The enclosure and faceplate are visual starting points; build text labels and layout in your UI framework for crisp resizing. Knob positions in the SVGs are intentionally varied to avoid a uniform all-at-noon appearance.
