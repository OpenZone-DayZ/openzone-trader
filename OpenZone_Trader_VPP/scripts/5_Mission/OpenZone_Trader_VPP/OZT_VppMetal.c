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
    static const int RAISED = 0xFF1E1E1D;   // vppRaised  0.118 0.118 0.114
    static const int CELL   = 0xFF070707;   // vppCell    0.027
    static const int TEXT   = 0xFFE0DFDA;   // vppText    0.88  0.875 0.855
    static const int MUTED  = 0xFFA09F99;   // vppMuted   0.627 0.624 0.6
    static const int FAINT  = 0xFF5C5A58;   // vppFaint   0.36  0.353 0.345
    static const int ACCENT = 0xFFE8782B;   // vppAccent  0.91  0.47  0.17
    static const int ALERT  = 0xFFF5C740;   // vppAlert   0.96  0.78  0.25
    static const int OK     = 0xFF66C75C;   // vppOk      0.4   0.78  0.36
    static const int BAD    = 0xFFE0524A;   // vppBad     0.88  0.32  0.29
}

#endif
#endif
#endif
