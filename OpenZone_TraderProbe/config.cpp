// OpenZone Trader Probe -- measurement tooling for the trader's design.
//
// Not a gameplay mod and never published. It asks the engine the questions a
// deal depends on (docs/2026-11-11-phase-0-plan.md): creating an item at a
// chosen cell, slot or in the hands; reusing the cell of a deleted item; moving
// a part off an item; banknote stacks and the things that hold them.
//
// Every op is driven through the MCP bridge verb `ozt_probe`
// (OpenZone_TraderProbe_Bridge) and reports into
// $profile:OpenZone_TraderProbe/results.log.

class CfgPatches
{
    class OpenZone_TraderProbe
    {
        units[] = {"OZT_ProbeWallet", "OZT_ProbeBadge", "OZT_ProbeSlotWallet", "OZT_ProbeNote"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            // The survivor classes gain a wear slot below, so the characters
            // must be defined before this addon loads.
            "DZ_Characters",
            // For \dz\gear\consumables\9v_battery.p3d, the borrowed model.
            "DZ_Gear_Consumables"
        };
    };
};

class CfgMods
{
    class OpenZone_TraderProbe
    {
        dir = "OpenZone_TraderProbe";
        name = "OpenZone Trader Probe";
        author = "OpenZone";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class gameScriptModule    { value = ""; files[] = {"OpenZone_TraderProbe/scripts/3_Game"}; };
            class worldScriptModule   { value = ""; files[] = {"OpenZone_TraderProbe/scripts/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"OpenZone_TraderProbe/scripts/5_Mission"}; };
        };
    };
};

class CfgSlots
{
    // A slot a mod adds to the character, to see that an item can be created
    // straight in it the way it can in a vanilla slot.
    class Slot_OZT_ProbeWear
    {
        name = "OZT_ProbeWear";
        displayName = "Probe wear slot";
        ghostIcon = "set:dayz_inventory image:walkietalkie";
    };

    // A slot for a banknote stack on a holder that has no cargo.
    class Slot_OZT_ProbeNote
    {
        name = "OZT_ProbeNote";
        displayName = "Probe note slot";
    };
};

class CfgVehicles
{
    class Man;
    class SurvivorBase : Man
    {
        attachments[] += {"OZT_ProbeWear"};

        // The equipment grid is built from this list, not from attachments[].
        class InventoryEquipment
        {
            playerSlots[] += {"Slot_OZT_ProbeWear"};
        };
    };

    class Inventory_Base;

    // A stand-in banknote with the numbers of the reference server's notes:
    // two cells wide, a stack of up to 500, destroyed when it reaches zero.
    class OZT_ProbeNote : Inventory_Base
    {
        scope = 2;
        displayName = "Probe note";
        descriptionShort = "A stand-in banknote of the trader probe. Not for play.";
        model = "\dz\gear\consumables\9v_battery.p3d";
        itemSize[] = {2, 1};
        weight = 1;
        rotationFlags = 16;
        quantityBar = 0;
        canBeSplit = 1;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 500;
        varQuantityDestroyOnMin = 1;
        destroyOnEmpty = 1;
        inventorySlot[] = {"OZT_ProbeNote"};
    };

    // A stand-in wallet shaped like the reference server's: an Inventory_Base,
    // not a Container_Base, with a cargo and a receive gate in script
    // (OZT_ProbeWallet.c).
    class OZT_ProbeWallet : Inventory_Base
    {
        scope = 2;
        displayName = "Probe wallet";
        descriptionShort = "A stand-in wallet of the trader probe: its cargo takes notes only. Not for play.";
        model = "\dz\gear\consumables\9v_battery.p3d";
        itemSize[] = {2, 2};
        itemsCargoSize[] = {6, 4};
        weight = 130;
        allowOwnedCargoManipulation = 1;
    };

    // The same wallet worn on the character, in the slot this pbo adds.
    class OZT_ProbeBadge : OZT_ProbeWallet
    {
        displayName = "Probe worn wallet";
        inventorySlot[] = {"OZT_ProbeWear"};
    };

    // A holder with a slot for one note stack and no cargo.
    class OZT_ProbeSlotWallet : Inventory_Base
    {
        scope = 2;
        displayName = "Probe slot wallet";
        descriptionShort = "A stand-in note holder of the trader probe with one slot. Not for play.";
        model = "\dz\gear\consumables\9v_battery.p3d";
        itemSize[] = {2, 2};
        weight = 130;
        attachments[] = {"OZT_ProbeNote"};
    };
};
