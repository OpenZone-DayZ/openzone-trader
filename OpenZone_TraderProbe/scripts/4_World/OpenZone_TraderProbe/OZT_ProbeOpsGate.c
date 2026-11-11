// What the engine and the scripts answer BEFORE anything is created or moved.
// Task 4 needs the state of the hands; Task 7 adds the gates asked with a probe
// entity.
class OZT_ProbeOpsGate
{
    // Everything that decides whether the hands can take an item now, and
    // whether a deal may touch the inventory at all. PlayerBase.CanDeleteItems
    // is vanilla's own "the player is doing nothing with their things".
    static bool HandsState(OZT_Probe p, PlayerBase player, out string detail)
    {
        EntityAI held = player.GetHumanInventory().GetEntityInHands();
        bool acting = false;
        if (player.GetActionManager() && player.GetActionManager().GetRunningAction())
            acting = true;
        detail = "hands hold " + OZT_ProbeInv.Name(held);
        detail = detail + " | inventory processing=" + OZT_ProbeLog.B(player.GetDayZPlayerInventory().IsProcessing());
        detail = detail + " | CanDeleteItems=" + OZT_ProbeLog.B(player.CanDeleteItems());
        detail = detail + " | action running=" + OZT_ProbeLog.B(acting);
        detail = detail + " | in vehicle=" + OZT_ProbeLog.B(player.IsInVehicle());
        // The hand state machine's own picture (HumanInventoryWithFSM): a move
        // that went around it leaves this where it was.
        detail = detail + " | hand state=" + player.GetDayZPlayerInventory().GetCurrentStateID().ToString();
        detail = detail + " idle=" + OZT_ProbeLog.B(player.GetDayZPlayerInventory().IsIdle());
        string body = "alive=" + OZT_ProbeLog.B(player.IsAlive()) + " unconscious=" + OZT_ProbeLog.B(player.IsUnconscious());
        body = body + " restrained=" + OZT_ProbeLog.B(player.IsRestrained());
        p.AddLine(body);
        if (held)
            p.AddLine("juncture on the held item=" + OZT_ProbeLog.B(GetGame().HasInventoryJunctureItem(held)));
        return true;
    }
}
