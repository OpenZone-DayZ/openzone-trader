// OpenZone Trader Research -- the research gate of the trader (design §8.2):
// goods locked until the player's faction has completed a node of the
// research tree, and discounts while it has.
//
// It knows two mods at once, the trader and the research mod, and so lives in
// a third pbo, as any other mod's gate would: a server without research runs
// the trader without it, and every condition of kind `research` then passes.
// requiredAddons below are HARD: a blocking window before the game loads
// rather than a silent skip.

class CfgPatches
{
    class OpenZone_Trader_Research
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
            "OpenZone_Research"
        };
    };
};

class CfgMods
{
    class OpenZone_Trader_Research
    {
        dir = "OpenZone_Trader_Research";
        name = "OpenZone Trader Research";
        credits = "OpenZone";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Mission"};

        class defs
        {
            class missionScriptModule { value = ""; files[] = {"OpenZone_Trader_Research/scripts/5_Mission"}; };
        };
    };
};
