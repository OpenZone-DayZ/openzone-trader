// The trader pages' script colours: the palette of the OpenZone admin window as its
// mockup boards have it (docs/specs/2026-11-13-openzone-vpp-page-guide.md, "Palette").
// A converted page carries its colours in its layout; the script needs them only where
// it repaints a part: a picked row, a lit segment, a state's text.
//
// Hex literals on purpose: a constant made by a call (ARGB(...)) reads 0 in a class of
// this module (measured 2026-11-12).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

class OZT_Metal
{
    static const int PANEL  = 0xFF0C0C0C;   // the page's ground
    static const int HEAD   = 0xFF131312;   // a box, a band, a plain row
    static const int RAISED = 0xFF1E1E1D;   // a button, a chip
    static const int CELL   = 0xFF070707;   // a field, an inner row
    static const int EDGE   = 0xFF3C3C3A;   // a frame, a rule
    static const int TEXT   = 0xFFE0DFDA;
    static const int MUTED  = 0xFFA09F99;
    static const int FAINT  = 0xFF5C5A58;
    static const int ACCENT = 0xFFE8782B;
    static const int ALERT  = 0xFFF5C740;
    static const int OK     = 0xFF66C75C;
    static const int BAD    = 0xFFE0524A;

    // The accent's soft ground (16 % of the accent) as it comes out over the page's
    // ground and over a box's: what a picked row and a lit segment lie on.
    static const int PICK      = 0xFF2F1D11;
    static const int PICK_HEAD = 0xFF352316;

    // The grounds of a condition's chip on the stock page: 16 % of the level's colour
    // over the page's ground (pristine, worn, damaged, badly damaged).
    static const int LEVEL_0 = 0xFF1A2A19;
    static const int LEVEL_1 = 0xFF242423;
    static const int LEVEL_2 = 0xFF312A14;
    static const int LEVEL_3 = 0xFF2E1716;
}

#endif
#endif
#endif
