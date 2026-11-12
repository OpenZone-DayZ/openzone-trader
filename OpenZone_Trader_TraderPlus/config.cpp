// OpenZone Trader TraderPlus -- the TraderPlus bank as a trader's money (design
// §7.3): what a player has on their TraderPlus account pays and is paid.
//
// It knows two mods at once, the trader and TraderPlus's banking pbo, and so
// lives in a third pbo, as any other bank's provider would: a server without
// TraderPlus runs the trader on item money. requiredAddons below are HARD: a
// blocking window before the game loads rather than a silent skip.
//
// TraderPlusBanking_Script is the CfgPatches class of TraderPlus's banking pbo
// (TraderPlusBanking.pbo of Workshop item 2458896948).

class CfgPatches
{
    class OpenZone_Trader_TraderPlus
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "OpenZone_Core",
            "OpenZone_Trader",
            "TraderPlusBanking_Script"
        };
    };
};

class CfgMods
{
    class OpenZone_Trader_TraderPlus
    {
        dir = "OpenZone_Trader_TraderPlus";
        name = "OpenZone Trader TraderPlus";
        credits = "OpenZone";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Mission"};

        class defs
        {
            class missionScriptModule { value = ""; files[] = {"OpenZone_Trader_TraderPlus/scripts/5_Mission"}; };
        };
    };
};
