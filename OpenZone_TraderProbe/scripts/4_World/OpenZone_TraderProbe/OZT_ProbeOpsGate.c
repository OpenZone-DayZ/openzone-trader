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

    // A plan has a class name, and every question the engine answers needs an
    // entity. So: one local entity of that class (never announced to a client),
    // every way of asking "may this go there" side by side, the script gates
    // read directly for comparison, and the cost of asking.
    static bool AskGates(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        string type = p.ArgOf(args, "class", "Paper");
        string kind = p.ArgOf(args, "kind", "cargo");
        EntityAI into = p.ItemOf(player, p.ArgOf(args, "into", "player"));
        if (!into)
        {
            detail = "gate needs a known into= (player or a handle)";
            return false;
        }

        float t0 = GetGame().GetTickTime();
        EntityAI probe = EntityAI.Cast(GetGame().CreateObjectEx(type, player.GetPosition(), ECE_LOCAL));
        float msMake = (GetGame().GetTickTime() - t0) * 1000;
        if (!probe)
        {
            detail = "gate: no local entity of class " + type + " could be made";
            return true;
        }
        string born = "probe entity " + OZT_ProbeInv.Name(probe) + " made in " + OZT_ProbeLog.R1(msMake) + " ms";
        born = born + ", stands " + OZT_ProbeInv.Where(probe);
        p.AddLine(born);

        if (kind == "hands")
            detail = AskHands(p, player, probe);
        else if (kind == "slot")
            detail = AskSlot(p, into, probe, p.ArgOf(args, "slot", ""));
        else
            detail = AskCargo(p, into, probe, args);

        t0 = GetGame().GetTickTime();
        GetGame().ObjectDelete(probe);
        float msGone = (GetGame().GetTickTime() - t0) * 1000;
        p.AddLine("probe entity deleted, the call took " + OZT_ProbeLog.R1(msGone) + " ms");
        detail = "gate " + type + " -> " + detail;
        return true;
    }

    protected static string AskCargo(OZT_Probe p, EntityAI into, EntityAI probe, map<string, string> args)
    {
        bool flip = p.ArgOf(args, "flip", "0") == "1";
        int n = p.ArgOf(args, "n", "1000").ToInt();
        int w;
        int h;
        OZT_ProbeInv.SizeOf(probe, w, h);
        if (flip)
        {
            int turned = w;
            w = h;
            h = turned;
        }
        string cellArg = p.ArgOf(args, "cell", "free");
        int row;
        int col;
        if (!p.PickCell(into, cellArg, w, h, null, row, col))
            return "no such cell (" + cellArg + ") in " + into.GetType();

        GameInventory inv = into.GetInventory();
        InventoryLocation loc = new InventoryLocation();
        loc.SetCargo(into, probe, 0, row, col, flip);

        // the map's own word about the rectangle
        array<bool> taken;
        int gw;
        int gh;
        OZT_ProbeInv.Map(into, null, taken, gw, gh);
        bool mapFree = OZT_ProbeInv.RectFree(taken, gw, gh, row, col, w, h);

        // the engine, asked every way there is
        bool canEx = inv.CanAddEntityInCargoEx(probe, 0, row, col, flip);
        bool gatesOnly = inv.TestAddEntityInCargoEx(probe, 0, row, col, flip, false, true, false, false, true, false);
        bool occOnly = inv.TestAddEntityInCargoEx(probe, 0, row, col, flip, false, false, false, true, false, false);
        bool both = inv.TestAddEntityInCargoEx(probe, 0, row, col, flip, false, true, false, true, true, false);
        bool locCan = GameInventory.LocationCanAddEntity(loc);
        bool locGates = GameInventory.LocationTestAddEntity(loc, false, true, false, false, true, false);

        // the scripts, read directly
        bool receive = into.CanReceiveItemIntoCargo(probe);
        bool put = probe.CanPutInCargo(into);
        bool reach = inv.AreChildrenAccessible();

        float t1 = GetGame().GetTickTime();
        for (int i = 0; i < n; i++)
            inv.TestAddEntityInCargoEx(probe, 0, row, col, flip, false, true, false, false, true, false);
        float usEach = (GetGame().GetTickTime() - t1) * 1000000 / n;

        string place = into.GetType() + " r" + row.ToString() + " c" + col.ToString() + " (" + cellArg + ")";
        p.AddLine("at " + place + " at " + OZT_ProbeInv.Where(into));
        string engine = "CanAddEntityInCargoEx=" + OZT_ProbeLog.B(canEx) + " Test(gates, no occupancy)=" + OZT_ProbeLog.B(gatesOnly);
        engine = engine + " Test(occupancy alone)=" + OZT_ProbeLog.B(occOnly) + " Test(both)=" + OZT_ProbeLog.B(both);
        engine = engine + " LocationCanAddEntity=" + OZT_ProbeLog.B(locCan) + " LocationTest(gates, no occupancy)=" + OZT_ProbeLog.B(locGates);
        p.AddLine(engine);
        string scripts = "map free=" + OZT_ProbeLog.B(mapFree) + " into.CanReceiveItemIntoCargo=" + OZT_ProbeLog.B(receive);
        scripts = scripts + " item.CanPutInCargo=" + OZT_ProbeLog.B(put) + " into children accessible=" + OZT_ProbeLog.B(reach);
        p.AddLine(scripts);
        p.AddLine("Test(gates, no occupancy) costs " + OZT_ProbeLog.R1(usEach) + " microseconds a call over " + n.ToString() + " calls");

        string sum = place + ": gates=" + OZT_ProbeLog.B(gatesOnly) + " occupancy=" + OZT_ProbeLog.B(occOnly) + " map free=" + OZT_ProbeLog.B(mapFree);
        sum = sum + " receive=" + OZT_ProbeLog.B(receive) + " " + OZT_ProbeLog.R1(usEach) + " us/call";
        return sum;
    }

    protected static string AskSlot(OZT_Probe p, EntityAI into, EntityAI probe, string slotName)
    {
        int slot = InventorySlots.GetSlotIdFromString(slotName);
        if (slot == InventorySlots.INVALID)
            return "no slot is named '" + slotName + "'";
        GameInventory inv = into.GetInventory();
        EntityAI there = inv.FindAttachment(slot);
        InventoryLocation loc = new InventoryLocation();
        loc.SetAttachment(into, probe, slot);
        bool hasSlot = inv.HasAttachmentSlot(slot);
        bool canEx = inv.CanAddAttachmentEx(probe, slot);
        bool canAny = inv.CanAddAttachment(probe);
        bool locCan = GameInventory.LocationCanAddEntity(loc);
        bool locGates = GameInventory.LocationTestAddEntity(loc, false, true, false, false, true, false);
        bool receive = into.CanReceiveAttachment(probe, slot);
        bool put = probe.CanPutAsAttachment(into);
        string s = into.GetType() + " slot " + slotName + " (holds " + OZT_ProbeInv.Name(there) + "): declared=" + OZT_ProbeLog.B(hasSlot);
        s = s + " CanAddAttachmentEx=" + OZT_ProbeLog.B(canEx) + " CanAddAttachment(any slot)=" + OZT_ProbeLog.B(canAny);
        s = s + " LocationCanAddEntity=" + OZT_ProbeLog.B(locCan) + " LocationTest(gates, no occupancy)=" + OZT_ProbeLog.B(locGates);
        p.AddLine(s);
        p.AddLine("into.CanReceiveAttachment=" + OZT_ProbeLog.B(receive) + " item.CanPutAsAttachment=" + OZT_ProbeLog.B(put));
        string sum = into.GetType() + " slot " + slotName + ": CanAddAttachmentEx=" + OZT_ProbeLog.B(canEx);
        sum = sum + " gates-only=" + OZT_ProbeLog.B(locGates) + " receive=" + OZT_ProbeLog.B(receive);
        return sum;
    }

    protected static string AskHands(OZT_Probe p, PlayerBase player, EntityAI probe)
    {
        HumanInventory hands = player.GetHumanInventory();
        EntityAI held = hands.GetEntityInHands();
        bool can = hands.CanAddEntityInHands(probe);
        bool gatesOnly = hands.TestAddEntityInHands(probe, false, true, false, false, true);
        bool occOnly = hands.TestAddEntityInHands(probe, false, false, false, true, false);
        bool put = probe.CanPutIntoHands(player);
        string s = "hands (hold " + OZT_ProbeInv.Name(held) + "): CanAddEntityInHands=" + OZT_ProbeLog.B(can);
        s = s + " Test(gates, no occupancy)=" + OZT_ProbeLog.B(gatesOnly) + " Test(occupancy alone)=" + OZT_ProbeLog.B(occOnly);
        s = s + " item.CanPutIntoHands=" + OZT_ProbeLog.B(put);
        p.AddLine(s);
        return s;
    }
}
