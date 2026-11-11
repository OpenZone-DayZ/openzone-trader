// The deletion op of the trader probe. A deal deletes what is sold and then
// creates what is bought; the question is whether the second may use the room
// of the first within one script call, and how each way of deleting behaves.
class OZT_ProbeOpsDelete
{
    static bool DeleteOne(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        EntityAI e = p.ItemOf(player, p.ArgOf(args, "item", ""));
        if (!e || e == player)
        {
            detail = "del needs item=<handle of something that still exists>";
            return false;
        }
        string how = p.ArgOf(args, "how", "object");
        string follow = p.ArgOf(args, "follow", "none");
        string type = p.ArgOf(args, "class", e.GetType());
        string handle = p.ArgOf(args, "as", "");

        // the place that is about to be freed
        InventoryLocation was = new InventoryLocation();
        e.GetInventory().GetCurrentInventoryLocation(was);
        EntityAI holder = was.GetParent();
        int lt = was.GetType();
        int row = was.GetRow();
        int col = was.GetCol();
        int slot = was.GetSlot();
        bool flip = was.GetFlip();
        string place = OZT_ProbeInv.Where(e);
        int before = ItemsIn(holder);

        // what hangs on it goes with it; watch when
        int kids = e.GetInventory().AttachmentCount();
        for (int k = 0; k < kids; k++)
            p.WatchItem(e.GetInventory().GetAttachmentFromIndex(k), "part of the doomed " + e.GetType());
        p.WatchItem(e, "doomed " + e.GetType());

        if (how == "delete")
            e.Delete();
        else if (how == "safe")
            e.DeleteSafe();
        else
            GetGame().ObjectDelete(e);

        // the same script call, right after
        string state = "the reference reads null";
        if (e)
        {
            state = "still there: pending=" + OZT_ProbeLog.B(e.IsPendingDeletion()) + " toDelete=" + OZT_ProbeLog.B(e.ToDelete());
            state = state + " setForDeletion=" + OZT_ProbeLog.B(e.IsSetForDeletion()) + " at " + OZT_ProbeInv.Where(e);
        }
        p.AddLine("deleted by " + how + " from " + place + "; same call: " + state);
        p.AddLine("items in the holder's cargo: " + before.ToString() + " before, " + ItemsIn(holder).ToString() + " in the same call");
        detail = "del " + how + " " + place + " | same call: " + state;

        if (follow == "none")
            return true;

        if (follow == "can")
        {
            if (lt != InventoryLocationType.CARGO || !holder)
            {
                p.AddLine("follow=can only asks about a cargo cell");
                return true;
            }
            EntityAI probe = EntityAI.Cast(GetGame().CreateObjectEx(type, player.GetPosition(), ECE_LOCAL));
            if (!probe)
            {
                p.AddLine("no local probe entity of class " + type);
                return true;
            }
            InventoryLocation loc = new InventoryLocation();
            loc.SetCargo(holder, probe, 0, row, col, flip);
            bool canAdd = GameInventory.LocationCanAddEntity(loc);
            bool occFree = holder.GetInventory().TestAddEntityInCargoEx(probe, 0, row, col, flip, false, false, false, true, false, false);
            GetGame().ObjectDelete(probe);
            string asked = "the freed cell in the same call: LocationCanAddEntity=" + OZT_ProbeLog.B(canAdd);
            asked = asked + " occupancy test alone=" + OZT_ProbeLog.B(occFree);
            p.AddLine(asked);
            detail = detail + " | " + asked;
            return true;
        }

        if (follow == "next")
        {
            OZT_ProbeLater job = new OZT_ProbeLater();
            job.m_DueFrame = p.FrameNo() + p.ArgOf(args, "frames", "1").ToInt();
            job.m_Kind = "create";
            job.m_Class = type;
            job.m_Handle = handle;
            job.m_Target = holder;
            job.m_LocType = lt;
            job.m_Row = row;
            job.m_Col = col;
            job.m_Slot = slot;
            job.m_Flip = flip;
            p.QueueLater(job);
            detail = detail + " | " + type + " queued for frame " + job.m_DueFrame.ToString() + " (now " + p.FrameNo().ToString() + ")";
            return true;
        }

        // follow=create: the same call
        EntityAI made = p.MakeAt(player, lt, holder, type, row, col, flip, slot);
        string got = "the engine returned nothing";
        if (made)
        {
            got = OZT_ProbeInv.Where(made);
            p.RememberAs(handle, made);
            p.WatchItem(made, "made in the freed place " + type);
        }
        p.AddLine("same call: " + type + " created at the freed place -> " + got);
        detail = detail + " | same-call create -> " + got;
        return true;
    }

    protected static int ItemsIn(EntityAI holder)
    {
        if (!holder || !holder.GetInventory())
            return -1;
        CargoBase cargo = holder.GetInventory().GetCargo();
        if (!cargo)
            return -1;
        return cargo.GetItemCount();
    }
}
