#!/usr/bin/env python3
"""Turn a flag colour into a badge colour: muted, but still saturated, with
white text at a contrast of at least 4.5:1.

    tools/mute-color.py '#FFCC00' '#0039A6'

Lightness is capped at OKLCH L 0.56 and chroma at 0.17, then lightness is
lowered until the contrast with white is high enough. Colours that already
fit, such as the Serbian flag's blue #0c4076, come out unchanged. The three
original badges (EN #4a4a4a, SR #0c4076, СР #c6363c) are kept as they are.
"""
import math
import sys


def to_linear(c):
    c /= 255
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def to_srgb(c):
    c = 12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055
    return c * 255


def parse(hex_color):
    h = hex_color.lstrip("#")
    return [int(h[i:i + 2], 16) for i in (0, 2, 4)]


def to_hex(rgb):
    return "#%02x%02x%02x" % tuple(max(0, min(255, round(x))) for x in rgb)


def to_oklch(hex_color):
    r, g, b = (to_linear(x) for x in parse(hex_color))
    l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
    m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
    s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
    L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s
    a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s
    bb = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s
    return L, math.hypot(a, bb), math.degrees(math.atan2(bb, a)) % 360


def from_oklch(L, C, H):
    a, b = C * math.cos(math.radians(H)), C * math.sin(math.radians(H))
    l = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
    return [to_srgb(x) for x in (
        4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s)]


def in_gamut(L, C, H):
    return all(-0.5 <= x <= 255.5 for x in from_oklch(L, C, H))


def contrast_with_white(hex_color):
    r, g, b = (to_linear(x) for x in parse(hex_color))
    return 1.05 / (0.2126 * r + 0.7152 * g + 0.0722 * b + 0.05)


def mute(hex_color, min_contrast=4.6, max_l=0.56, min_l=0.32, max_c=0.17):
    L, C, H = to_oklch(hex_color)
    C, L = min(C, max_c), max(min(L, max_l), min_l)
    while not in_gamut(L, C, H) and C > 0.01:
        C -= 0.005
    while contrast_with_white(to_hex(from_oklch(L, C, H))) < min_contrast and L > 0.2:
        L -= 0.005
        while not in_gamut(L, C, H) and C > 0.01:
            C -= 0.005
    return to_hex(from_oklch(L, C, H))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for color in sys.argv[1:]:
        muted = mute(color)
        print(f"{color} -> {muted}  (contrast with white {contrast_with_white(muted):.2f})")
