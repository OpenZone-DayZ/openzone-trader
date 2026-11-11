// Server-side glue between the trader and the MCP bridge: the `oz_trader` verb
// for world_exec -- scripted deals, sent through the same service handler a
// client's request reaches. A stand tool, never published. Requires
// @DZMCP_Bridge; this pbo is listed in mods.server_only of dayz-mcp.local.toml
// and never reaches a client.

class CfgPatches
{
    class OpenZone_Trader_Bridge
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "DZMCP_Bridge",
            "OpenZone_Trader"
        };
    };
};

class CfgMods
{
    class OpenZone_Trader_Bridge
    {
        dir = "OpenZone_Trader_Bridge";
        name = "OpenZone Trader Bridge";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class missionScriptModule { value = ""; files[] = {"OpenZone_Trader_Bridge/scripts/5_Mission"}; };
        };
    };
};
