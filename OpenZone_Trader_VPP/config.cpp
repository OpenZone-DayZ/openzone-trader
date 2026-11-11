// OpenZone Trader VPP -- the pane TRADERS of the OpenZone admin window, its own
// pbo (design §9.5).
//
// The glue knows two mods at once, the trader and the core's VPP window, and so
// lives in a third pbo: a server without VPP Admin Tools runs the trader
// without it. requiredAddons below are HARD: a blocking window before the game
// loads rather than a silent skip, for a pane that has nothing to hang on
// without the window.
//
// DZM_VPPAdminToolsScripts is the CfgPatches class of VPP's script pbo. The
// code guards on AVPPAdminTools and OpenZone_VPP: the engine defines the names
// of CfgMods classes, not of CfgPatches ones.

class CfgPatches
{
    class OpenZone_Trader_VPP
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
            "OpenZone_VPP",
            "DZM_VPPAdminToolsScripts"
        };
    };
};

class CfgMods
{
    class OpenZone_Trader_VPP
    {
        dir = "OpenZone_Trader_VPP";
        name = "OpenZone Trader VPP Pane";
        credits = "OpenZone";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Mission"};

        class defs
        {
            class missionScriptModule { value = ""; files[] = {"OpenZone_Trader_VPP/scripts/5_Mission"}; };
        };
    };
};
