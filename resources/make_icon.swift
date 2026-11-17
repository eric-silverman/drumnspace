// Renders resources/icon.png (1024x1024) — the app/plugin icon, built from the panel artwork.
// Run from the repo root:  swift resources/make_icon.swift
import CoreGraphics
import Foundation
import ImageIO
import UniformTypeIdentifiers

let size = 1024
let cs = CGColorSpace(name: CGColorSpace.sRGB)!
let ctx = CGContext(data: nil, width: size, height: size, bitsPerComponent: 8, bytesPerRow: 0,
                    space: cs, bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
ctx.translateBy(x: 0, y: CGFloat(size))  // draw y-down like the JUCE editor
ctx.scaleBy(x: 1, y: -1)

func rgb(_ hex: UInt32, _ a: CGFloat = 1) -> CGColor {
    CGColor(srgbRed: CGFloat((hex >> 16) & 0xFF) / 255, green: CGFloat((hex >> 8) & 0xFF) / 255,
            blue: CGFloat(hex & 0xFF) / 255, alpha: a)
}
func linear(_ colors: [CGColor], from: CGPoint, to: CGPoint) {
    let g = CGGradient(colorsSpace: cs, colors: colors as CFArray, locations: nil)!
    ctx.drawLinearGradient(g, start: from, end: to, options: [])
}
func roundedRect(_ r: CGRect, _ radius: CGFloat) -> CGPath {
    CGPath(roundedRect: r, cornerWidth: radius, cornerHeight: radius, transform: nil)
}
func png(_ name: String) -> CGImage {
    let url = URL(fileURLWithPath: "space_drum_assets/png/\(name).png")
    let src = CGImageSourceCreateWithURL(url as CFURL, nil)!
    return CGImageSourceCreateImageAtIndex(src, 0, nil)!
}
func draw(_ img: CGImage, _ r: CGRect) {  // un-flip so images land upright in the y-down context
    ctx.saveGState()
    ctx.translateBy(x: r.minX, y: r.maxY)
    ctx.scaleBy(x: 1, y: -1)
    ctx.draw(img, in: CGRect(origin: .zero, size: r.size))
    ctx.restoreGState()
}

// ── Tile (macOS icon grid: 824pt body, ~185pt corners, soft drop shadow) ──
let tile = CGRect(x: 100, y: 100, width: 824, height: 824)
let tilePath = roundedRect(tile, 185)
ctx.saveGState()
ctx.setShadow(offset: CGSize(width: 0, height: 18), blur: 36, color: rgb(0x000000, 0.45))
ctx.addPath(tilePath); ctx.setFillColor(rgb(0x8A8A8A)); ctx.fillPath()
ctx.restoreGState()

// chrome bezel, like the enclosure frame
ctx.saveGState()
ctx.addPath(tilePath); ctx.clip()
linear([rgb(0xF0F0F0), rgb(0xA8A8A8), rgb(0xD6D6D6), rgb(0x6E6E6E)], from: CGPoint(x: 0, y: 100), to: CGPoint(x: 0, y: 924))
ctx.restoreGState()

// navy faceplate
let face = tile.insetBy(dx: 30, dy: 30)
let facePath = roundedRect(face, 158)
ctx.saveGState()
ctx.addPath(facePath); ctx.clip()
linear([rgb(0x134670), rgb(0x0B2F4A), rgb(0x061C2E)], from: CGPoint(x: 0, y: face.minY), to: CGPoint(x: 0, y: face.maxY))

ctx.addPath(roundedRect(face.insetBy(dx: 14, dy: 14), 146))  // cream silkscreen line, under the art
ctx.setStrokeColor(rgb(0xF6EBD7, 0.55)); ctx.setLineWidth(5); ctx.strokePath()

draw(png("stars-sparkles"), CGRect(x: 130, y: 140, width: 400, height: 333))
draw(png("cosmic-trails"), CGRect(x: 90, y: 470, width: 800, height: 457))
draw(png("planet-orbit"), CGRect(x: 340, y: 170, width: 560, height: 560))
ctx.restoreGState()


// ── Write ──
let out = URL(fileURLWithPath: "resources/icon.png")
let dest = CGImageDestinationCreateWithURL(out as CFURL, UTType.png.identifier as CFString, 1, nil)!
CGImageDestinationAddImage(dest, ctx.makeImage()!, nil)
CGImageDestinationFinalize(dest)
print("wrote \(out.path)")
