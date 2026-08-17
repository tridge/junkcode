"""Render the usage meters as one small SVG icon.

The text form ("C ▰▰▱▱▱ ~14%5h  X ▰▱▱▱▱ 4%wk") was around eight times the width
of the battery icon next to it, which is far too much panel real estate for a
couple of numbers. This draws the same information as side-by-side gauges,
filled bottom-up like a battery. The numbers themselves live in the tooltip.

One gauge per window, not one per provider: Claude has three windows that run
out independently (per-model weekly, all-model weekly, 5h), and collapsing them
to their worst hides which one you are actually up against. Each gauge carries
a one-glyph label - F/W/5 for Claude's, X for Codex.

SVG rather than PNG because gdk-pixbuf renders it (librsvg), so it stays sharp
at whatever height the panel scales it to, and generating it needs nothing but
string formatting.
"""

# Geometry in user units. Height is fixed; width grows with the number of
# gauges, so the panel footprint scales with what there is to show rather than
# being a fixed guess.
H = 24.0
PAD_X, PAD_TOP, PAD_BOT = 1.3, 1.2, 6.4      # bottom band holds the labels
BAR_W = 5.4
GAP = 1.6          # between gauges of the same provider
GROUP_GAP = 3.0    # between providers, so the grouping is readable at a glance
BAR_TOP = PAD_TOP
BAR_BOT = H - PAD_BOT
BAR_H = BAR_BOT - BAR_TOP
RADIUS = 0.9
FONT_PX = 5.0


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
    # A fully drained gauge has no coloured pixels left, so "out of quota"
    # would look identical to "no data". Outline it in its own colour instead,
    # thicker than usual - a 0.7-wide stroke all but vanishes at panel size.
    stroke_w = 0.7
    if fill_pct <= 0:
        outline, stroke_w = fill, 1.3
    return (
        f'<rect x="{x:.2f}" y="{BAR_TOP:.2f}" width="{BAR_W:.2f}" '
        f'height="{BAR_H:.2f}" rx="{RADIUS}" fill="{empty}"/>'
        f'<rect x="{x:.2f}" y="{fy:.2f}" width="{BAR_W:.2f}" '
        f'height="{fh:.2f}" rx="{RADIUS}" fill="{fill}"/>'
        f'<rect x="{x:.2f}" y="{BAR_TOP:.2f}" width="{BAR_W:.2f}" '
        f'height="{BAR_H:.2f}" rx="{RADIUS}" fill="none" '
        f'stroke="{outline}" stroke-width="{stroke_w}"/>'
    )


def _label(x, text, colour):
    # sits in the bottom band, clear of BAR_BOT so the glyph never touches the
    # gauge outline
    return (f'<text x="{x + BAR_W / 2.0:.2f}" y="{H - 1.1:.2f}" '
            f'font-family="sans-serif" font-size="{FONT_PX}" '
            f'font-weight="bold" text-anchor="middle" '
            f'fill="{colour}">{text}</text>')


def layout(bars):
    """x offset for each gauge, plus the total width.

    A change of `group` widens the gap, which is what keeps three Claude
    gauges reading as one cluster rather than four unrelated bars.
    """
    xs = []
    x = PAD_X
    prev = None
    for b in bars:
        if prev is not None:
            x += GAP if b.get("group") == prev else GROUP_GAP
            x += BAR_W
        xs.append(x)
        prev = b.get("group")
    return xs, (xs[-1] + BAR_W + PAD_X if xs else 2 * PAD_X)


def render(bars, empty, outline, label_col, px_height=40):
    """bars: [{'fill', 'colour', 'label', 'group'}] -> SVG document text.

    'fill' is headroom remaining (see _bar); None reads as empty, so missing
    data never looks like plenty of quota.

    width/height are emitted in PIXELS, not viewBox units: gdk-pixbuf
    rasterises an SVG at its declared intrinsic size and genmon does not scale
    it up, so declaring 16x24 gave a 16px-tall icon that was unreadable. The
    viewBox keeps the drawing coordinates unchanged.
    """
    xs, w = layout(bars)
    px_h = float(px_height)
    px_w = px_h * w / H
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" '
             f'width="{px_w:.1f}" height="{px_h:.1f}" '
             f'viewBox="0 0 {w:.2f} {H}">']
    for x, b in zip(xs, bars):
        fill = b.get("fill")
        parts.append(_bar(x, fill if fill is not None else 0,
                          b["colour"], empty, outline))
    for x, b in zip(xs, bars):
        parts.append(_label(x, b["label"], label_col))
    parts.append('</svg>')
    return "".join(parts)
