// Server-side glue between the trader probe and the MCP bridge: adds the
// `ozt_probe` verb to DZMCP_BridgeCore so world_exec can drive the probe.
// Requires @DZMCP_Bridge; this pbo is listed in mods.server_only of
// dayz-mcp.local.toml and never reaches a client.

class CfgPatches
{
    class OpenZone_TraderProbe_Bridge
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "DZMCP_Bridge",
            "OpenZone_TraderProbe"
        };
    };
};

class CfgMods
{
    class OpenZone_TraderProbe_Bridge
    {
        dir = "OpenZone_TraderProbe_Bridge";
        name = "OpenZone Trader Probe Bridge";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class missionScriptModule { value = ""; files[] = {"OpenZone_TraderProbe_Bridge/scripts/5_Mission"}; };
        };
    };
};
