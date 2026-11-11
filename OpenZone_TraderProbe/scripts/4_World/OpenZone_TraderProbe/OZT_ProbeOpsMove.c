// Move ops of the trader probe. A part the player keeps comes off the sold
// item and goes to a planned cell: by which server call, when does it land,
// what do the script gates say and does the call obey them, and what happens
// when the item it sat on is deleted right after.
class OZT_ProbeOpsMove
{
    static bool MoveOne(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        EntityAI e = p.ItemOf(player, p.ArgOf(args, "item", ""));
        if (!e || e == player)
        {
            detail = "move needs item=<handle of something that still exists>";
            return false;
        }
        InventoryLocation src = new InventoryLocation();
        e.GetInventory().GetCurrentInventoryLocation(src);
        EntityAI host = src.GetParent();
        string from = OZT_ProbeInv.Where(e);
        string follow = p.ArgOf(args, "follow", "none");
        string via = p.ArgOf(args, "via", "dst");

        if (p.ArgOf(args, "kind", "cargo") == "hands")
        {
            p.AddLine("from " + from + "; source gates: " + SourceGates(e, host, src.GetType()));
            p.WatchItem(e, "to the hands " + e.GetType());
            if (via != "sync")
            {
                player.ServerTakeEntityToHands(e);
                detail = "move " + e.GetType() + " to the hands by ServerTakeEntityToHands; same call: " + OZT_ProbeInv.Where(e);
                return true;
            }
            // The synchronous way: the engine's own move with a hands location,
            // then the inventory command for the clients. It goes around the
            // hand state machine, and what that costs is the question.
            InventoryLocation handsDst = new InventoryLocation();
            handsDst.SetHands(player, e);
            bool handsOk = GameInventory.LocationCanAddEntity(handsDst);
            bool handsMoved = false;
            if (handsOk)
            {
                handsMoved = GameInventory.LocationSyncMoveEntity(src, handsDst);
                if (handsMoved)
                    InventoryInputUserData.SendServerMove(null, InventoryCommandType.SYNC_MOVE, src, handsDst);
            }
            detail = "move " + e.GetType() + " to the hands by LocationSyncMoveEntity: LocationCanAddEntity=" + OZT_ProbeLog.B(handsOk);
            detail = detail + " moved=" + OZT_ProbeLog.B(handsMoved) + "; same call: " + OZT_ProbeInv.Where(e);
            return true;
        }

        EntityAI into = p.ItemOf(player, p.ArgOf(args, "into", ""));
        if (!into)
        {
            detail = "move needs into=<handle of a container>";
            return false;
        }
        bool flip = p.ArgOf(args, "flip", "0") == "1";
        int w;
        int h;
        OZT_ProbeInv.SizeOf(e, w, h);
        if (flip)
        {
            int turned = w;
            w = h;
            h = turned;
        }
        int row;
        int col;
        if (!p.PickCell(into, p.ArgOf(args, "cell", "free"), w, h, e, row, col))
        {
            detail = "move: no such cell in " + into.GetType() + " for " + w.ToString() + "x" + h.ToString();
            return true;
        }
        InventoryLocation dst = new InventoryLocation();
        dst.SetCargo(into, e, 0, row, col, flip);

        // what the scripts and the engine say before anything moves
        string gates = SourceGates(e, host, src.GetType());
        gates = gates + " | item.CanPutInCargo=" + OZT_ProbeLog.B(e.CanPutInCargo(into));
        gates = gates + " into.CanReceiveItemIntoCargo=" + OZT_ProbeLog.B(into.CanReceiveItemIntoCargo(e));
        gates = gates + " into children accessible=" + OZT_ProbeLog.B(into.GetInventory().AreChildrenAccessible());
        bool engineOk = GameInventory.LocationCanAddEntity(dst);
        string target = "CARGO " + into.GetType() + " r" + row.ToString() + " c" + col.ToString() + " flip=" + OZT_ProbeLog.B(flip);
        p.AddLine("from " + from + " to " + target + " via=" + via);
        p.AddLine("gates: " + gates);
        p.AddLine("LocationCanAddEntity(destination)=" + OZT_ProbeLog.B(engineOk));

        // Never hand the engine a destination it refused: that has ended the
        // server process without a log.
        if (!engineOk)
        {
            detail = "move NOT attempted: LocationCanAddEntity refused " + target + " | " + gates;
            return true;
        }

        p.WatchItem(e, "moved " + e.GetType());
        bool answered;
        float t0 = GetGame().GetTickTime();
        if (via == "cargoex")
        {
            answered = player.ServerTakeEntityToTargetCargoEx(into.GetInventory().GetCargo(), e, row, col);
        }
        else if (via == "sync")
        {
            // What GameInventory.TakeToDst(SERVER) does for an owner that is not
            // a living player: the engine's synchronous move, then the inventory
            // command that tells the clients. A living player's own TakeToDst
            // sends a sync juncture instead and lands a tenth of a second later.
            answered = GameInventory.LocationSyncMoveEntity(src, dst);
            if (answered)
                InventoryInputUserData.SendServerMove(null, InventoryCommandType.SYNC_MOVE, src, dst);
        }
        else if (via == "syncraw")
        {
            // The same move with nobody told.
            answered = GameInventory.LocationSyncMoveEntity(src, dst);
        }
        else
        {
            answered = player.ServerTakeToDst(src, dst);
        }
        float ms = (GetGame().GetTickTime() - t0) * 1000;
        string sameCall = OZT_ProbeInv.Where(e);
        p.AddLine("the call answered " + OZT_ProbeLog.B(answered) + " in " + OZT_ProbeLog.R1(ms) + " ms; same call the item reads: " + sameCall);

        Weapon_Base wpn = Weapon_Base.Cast(host);
        if (wpn)
        {
            Magazine still = wpn.GetMagazine(wpn.GetCurrentMuzzle());
            p.AddLine("the weapon in the same call: GetMagazine=" + OZT_ProbeInv.Name(still) + " IsIdle=" + OZT_ProbeLog.B(wpn.IsIdle()));
        }

        detail = "move " + e.GetType() + " " + via + " answered " + OZT_ProbeLog.B(answered) + " | same call: " + sameCall;
        detail = detail + " | " + gates;

        // The order of a deal's commit: kept parts move, then what was sold is
        // deleted. Does the part outlive the item it sat on?
        if (follow == "delhost" && host && host != player)
        {
            p.WatchItem(host, "host of the kept part " + host.GetType());
            GetGame().ObjectDelete(host);
            p.AddLine("the host " + host.GetType() + " deleted in the same call; the part now reads: " + OZT_ProbeInv.Where(e));
        }
        if (follow == "delhost_next" && host && host != player)
        {
            OZT_ProbeLater job = new OZT_ProbeLater();
            job.m_DueFrame = p.FrameNo() + 1;
            job.m_Kind = "delete";
            job.m_Target = host;
            p.QueueLater(job);
            p.WatchItem(host, "host of the kept part " + host.GetType());
            p.AddLine("the host " + host.GetType() + " queued for deletion on frame " + job.m_DueFrame.ToString());
        }
        return true;
    }

    // What the item's present holder says about letting it go.
    protected static string SourceGates(EntityAI e, EntityAI host, int srcType)
    {
        if (!host)
            return "no host";
        string s = "host " + host.GetType() + " children accessible=" + OZT_ProbeLog.B(host.GetInventory().AreChildrenAccessible());
        if (srcType == InventoryLocationType.ATTACHMENT)
            s = s + " host.CanReleaseAttachment=" + OZT_ProbeLog.B(host.CanReleaseAttachment(e));
        if (srcType == InventoryLocationType.CARGO)
        {
            s = s + " host.CanReleaseCargo=" + OZT_ProbeLog.B(host.CanReleaseCargo(e));
            s = s + " item.CanRemoveFromCargo=" + OZT_ProbeLog.B(e.CanRemoveFromCargo(host));
        }
        return s;
    }

    // The fallback for a part that will not move: delete it and create the same
    // class at the planned cell with its amount and health. What another mod
    // stored on the item is lost this way, which is why it is the fallback.
    static bool RecreateOne(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        EntityAI e = p.ItemOf(player, p.ArgOf(args, "item", ""));
        EntityAI into = p.ItemOf(player, p.ArgOf(args, "into", ""));
        if (!e || e == player || !into)
        {
            detail = "recreate needs item=<handle> and into=<handle of a container>";
            return false;
        }
        bool flip = p.ArgOf(args, "flip", "0") == "1";
        string type = e.GetType();
        int w;
        int h;
        OZT_ProbeInv.SizeOf(e, w, h);
        if (flip)
        {
            int turned = w;
            w = h;
            h = turned;
        }
        int row;
        int col;
        if (!p.PickCell(into, p.ArgOf(args, "cell", "free"), w, h, e, row, col))
        {
            detail = "recreate: no such cell in " + into.GetType();
            return true;
        }

        string from = OZT_ProbeInv.Where(e);
        string amount = OZT_ProbeInv.Amount(e);
        float health = e.GetHealth01("", "");
        int rounds = -1;
        float quantity = -1;
        Magazine oldMag = Magazine.Cast(e);
        ItemBase oldItem = ItemBase.Cast(e);
        if (oldMag)
            rounds = oldMag.GetAmmoCount();
        else if (oldItem && oldItem.HasQuantity())
            quantity = oldItem.GetQuantity();

        p.WatchItem(e, "old " + type);
        GetGame().ObjectDelete(e);

        InventoryLocation loc = new InventoryLocation();
        loc.SetCargo(into, null, 0, row, col, flip);
        EntityAI made = GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT);
        if (!made)
        {
            detail = "recreate " + type + ": the old one is deleted and the engine created nothing at the cell";
            return true;
        }
        made.SetHealth01("", "", health);
        Magazine newMag = Magazine.Cast(made);
        ItemBase newItem = ItemBase.Cast(made);
        if (newMag && rounds >= 0)
            newMag.ServerSetAmmoCount(rounds);
        else if (newItem && quantity >= 0)
            newItem.SetQuantity(quantity);
        p.RememberAs(p.ArgOf(args, "as", ""), made);
        p.WatchItem(made, "new " + type);
        detail = "recreate " + type + " from " + from + " (" + amount + ") -> " + OZT_ProbeInv.Where(made);
        detail = detail + " (" + OZT_ProbeInv.Amount(made) + ", health " + OZT_ProbeLog.R1(made.GetHealth01("", "") * 100) + "%)";
        return true;
    }
}
