// OpenZone Trader -- a trader for DayZ on the OpenZone core.
//
// WHAT THIS PBO IS. Goods and prices, deals, the money service with the item
// provider, trader points. Every trader, category, price and banknote is the
// administrator's JSON in $profile:OpenZone.
//
// WHAT IT STANDS ON. OpenZone_Core, hard: the config loader, the service
// registry and the log. CF through the core. Nothing else: with the core alone
// the trader runs, with item money and no gates.
//
// WHAT IT IS NOT YET. Phase 1 is the server half for plain items; the trade
// screen, items with parts, the NPC and the gates come in their own phases
// (docs/2026-11-11-phase-1-plan.md).

class CfgPatches
{
    class OpenZone_Trader
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        // Hard dependencies: a blocking window before the game loads rather
        // than a silent skip.
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "JM_CF_Scripts",
            "OpenZone_Core"
        };
    };
};

class CfgMods
{
    class OpenZone_Trader
    {
        dir        = "OpenZone_Trader";
        name       = "OpenZone Trader";
        credits    = "OpenZone";
        author     = "OpenZone";
        version    = "0.1.0";
        type       = "mod";

        // No mission module yet: the trade screen brings it in phase 2.
        dependencies[] = {"Game", "World"};

        class defs
        {
            class gameScriptModule  { value = ""; files[] = {"OpenZone_Trader/scripts/3_Game"}; };
            class worldScriptModule { value = ""; files[] = {"OpenZone_Trader/scripts/4_World"}; };
        };
    };
};
