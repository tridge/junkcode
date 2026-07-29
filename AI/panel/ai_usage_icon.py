"""Render the two usage meters as one small SVG icon.

The text form ("C ▰▰▱▱▱ ~14%5h  X ▰▱▱▱▱ 4%wk") was around eight times the width
of the battery icon next to it, which is far too much panel real estate for two
numbers. This draws the same information as two side-by-side gauges, filled
bottom-up like a battery, in roughly the width of one icon. The numbers
themselves live in the tooltip.

SVG rather than PNG because gdk-pixbuf renders it (librsvg), so it stays sharp
at whatever height the panel scales it to, and generating it needs nothing but
string formatting.
"""

# Geometry in user units; the panel scales the whole thing to its height, so
# the width:height ratio here decides how much panel width it eats. Kept
# narrow deliberately - at 16:24 it lands around 30px wide on a 44px panel,
# comparable to the battery icon rather than the ~370px the text form used.
W, H = 16.0, 24.0
PAD_X, PAD_TOP, PAD_BOT = 1.3, 1.2, 6.4      # bottom band holds the labels
GAP = 1.9
BAR_W = (W - 2 * PAD_X - GAP) / 2.0
BAR_TOP = PAD_TOP
BAR_BOT = H - PAD_BOT
BAR_H = BAR_BOT - BAR_TOP
RADIUS = 0.9


def _bar(x, fill_pct, fill, empty, outline):
    """One gauge: track, proportional fill from the bottom, outline.

    fill_pct is how full the gauge should look, NOT how much quota is used.
    Callers pass the REMAINING headroom, so it drains like a battery: full
    means plenty left, empty means you are at the limit. Filling it with the
    used fraction instead reads exactly backwards - a nearly empty bar looks
    like trouble when it actually means barely touched.
    """
    fill_pct = max(0.0, min(float(fill_pct), 100.0))
    fh = BAR_H * (fill_pct / 100.0)
    # keep a sliver visible while anything remains, so "almost out" is still
    # distinguishable from "completely out"
    if fill_pct > 0:
        fh = max(fh, 1.4)
    fy = BAR_BOT - fh
    return (
        f'<rect x="{x:.2f}" y="{BAR_TOP:.2f}" width="{BAR_W:.2f}" '
        f'height="{BAR_H:.2f}" rx="{RADIUS}" fill="{empty}"/>'
        f'<rect x="{x:.2f}" y="{fy:.2f}" width="{BAR_W:.2f}" '
        f'height="{fh:.2f}" rx="{RADIUS}" fill="{fill}"/>'
        f'<rect x="{x:.2f}" y="{BAR_TOP:.2f}" width="{BAR_W:.2f}" '
        f'height="{BAR_H:.2f}" rx="{RADIUS}" fill="none" '
        f'stroke="{outline}" stroke-width="0.7"/>'
    )


def _label(x, text, colour):
    # sits in the bottom band, clear of BAR_BOT so the glyph never touches the
    # gauge outline
    return (f'<text x="{x + BAR_W / 2.0:.2f}" y="{H - 1.1:.2f}" '
            f'font-family="sans-serif" font-size="5.0" font-weight="bold" '
            f'text-anchor="middle" fill="{colour}">{text}</text>')


def render(left_fill, right_fill, left_col, right_col, empty, outline,
           label_col, left_label="C", right_label="X", px_height=40):
    """Two gauges side by side -> SVG document text.

    width/height are emitted in PIXELS, not viewBox units: gdk-pixbuf
    rasterises an SVG at its declared intrinsic size and genmon does not scale
    it up, so declaring 16x24 gave a 16px-tall icon that was unreadable. The
    viewBox keeps the drawing coordinates unchanged.
    """
    px_h = float(px_height)
    px_w = px_h * W / H
    x1 = PAD_X
    x2 = PAD_X + BAR_W + GAP
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'width="{px_w:.1f}" height="{px_h:.1f}" '
        f'viewBox="0 0 {W} {H}">',
        _bar(x1, left_fill if left_fill is not None else 0, left_col, empty, outline),
        _bar(x2, right_fill if right_fill is not None else 0, right_col, empty, outline),
        _label(x1, left_label, label_col),
        _label(x2, right_label, label_col),
        '</svg>',
    ]
    return "".join(parts)
