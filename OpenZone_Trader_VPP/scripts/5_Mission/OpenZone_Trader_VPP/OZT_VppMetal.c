// The trader pages' script colours: the OpenZone admin window's metal (VPP rework
// 2026-11-12, design §2.3), the values of the vpp* tokens of the core's
// ui/tokens.json, which this repository's ui/tokens.json repeats for its own page
// descriptions. The engine does not read the tokens file: a vpp* token changed
// there is changed here too.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

class OZT_Metal
{
    static const int RAISED = ARGB(255,  30,  30,  29);   // vppRaised  0.118 0.118 0.114
    static const int CELL   = ARGB(255,   7,   7,   7);   // vppCell    0.027
    static const int TEXT   = ARGB(255, 224, 223, 218);   // vppText    0.88  0.875 0.855
    static const int MUTED  = ARGB(255, 160, 159, 153);   // vppMuted   0.627 0.624 0.6
    static const int FAINT  = ARGB(255,  92,  90,  88);   // vppFaint   0.36  0.353 0.345
    static const int ACCENT = ARGB(255, 232, 120,  43);   // vppAccent  0.91  0.47  0.17
    static const int ALERT  = ARGB(255, 245, 199,  64);   // vppAlert   0.96  0.78  0.25
    static const int OK     = ARGB(255, 102, 199,  92);   // vppOk      0.4   0.78  0.36
    static const int BAD    = ARGB(255, 224,  82,  74);   // vppBad     0.88  0.32  0.29
}

#endif
#endif
#endif
