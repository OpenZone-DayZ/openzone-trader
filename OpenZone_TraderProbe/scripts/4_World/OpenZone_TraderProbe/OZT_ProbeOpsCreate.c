// Creation ops of the trader probe: one item made at a named place by a named
// engine call, with the place asked for and the place it ended in side by side.
class OZT_ProbeOpsCreate
{
    static bool MakeOne(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        string type = p.ArgOf(args, "class", "");
        string intoName = p.ArgOf(args, "into", "player");
        string kind = p.ArgOf(args, "kind", "cargo");
        string via = p.ArgOf(args, "via", "loc");
        bool flip = p.ArgOf(args, "flip", "0") == "1";
        EntityAI into = p.ItemOf(player, intoName);
        if (type == "" || !into)
        {
            detail = "create needs class= and a known into= (player or a handle); got class='" + type + "' into='" + intoName + "'";
            return false;
        }

        int cw;
        int ch;
        bool sized = OZT_ProbeInv.ConfigSize(type, cw, ch);
        p.AddLine("config size of " + type + ": " + cw.ToString() + "x" + ch.ToString() + " found=" + OZT_ProbeLog.B(sized));
        if (!sized)
        {
            // no itemSize under the three roots: a cell is picked as for 1x1
            cw = 1;
            ch = 1;
        }

        EntityAI made;
        string asked = "";
        float t0 = GetGame().GetTickTime();
        if (kind == "hands")
            made = MakeInHands(player, type, via, asked);
        else if (kind == "slot")
            made = MakeInSlot(p, into, type, p.ArgOf(args, "slot", ""), via, asked);
        else
            made = MakeInCargo(p, into, type, p.ArgOf(args, "cell", "free"), flip, via, cw, ch, asked);
        float ms = (GetGame().GetTickTime() - t0) * 1000;

        if (!made)
        {
            detail = "create " + type + " asked " + asked + " -> the engine returned nothing";
            return true;
        }

        string qty = p.ArgOf(args, "qty", "");
        ItemBase ib = ItemBase.Cast(made);
        if (qty != "" && ib)
            ib.SetQuantity(qty.ToFloat());
        string ammo = p.ArgOf(args, "ammo", "");
        Magazine mag = Magazine.Cast(made);
        if (ammo != "" && mag)
            mag.ServerSetAmmoCount(ammo.ToInt());

        int ew;
        int eh;
        OZT_ProbeInv.SizeOf(made, ew, eh);
        p.RememberAs(p.ArgOf(args, "as", ""), made);
        p.WatchItem(made, "created " + type);
        detail = "create " + type + " asked " + asked + " -> got " + OZT_ProbeInv.Where(made);
        detail = detail + " | " + OZT_ProbeInv.Amount(made) + " | entity " + ew.ToString() + "x" + eh.ToString();
        detail = detail + " | " + OZT_ProbeLog.R1(ms) + " ms";
        return true;
    }

    protected static EntityAI MakeInCargo(OZT_Probe p, EntityAI into, string type, string cellArg, bool flip, string via, int cw, int ch, out string asked)
    {
        int w = cw;
        int h = ch;
        if (flip)
        {
            w = ch;
            h = cw;
        }
        int row;
        int col;
        if (!p.PickCell(into, cellArg, w, h, null, row, col))
        {
            asked = "cell=" + cellArg + " in " + into.GetType() + " (no such cell: no cargo, or nothing free by the map)";
            return null;
        }
        asked = "CARGO " + into.GetType() + " r" + row.ToString() + " c" + col.ToString();
        asked = asked + " flip=" + OZT_ProbeLog.B(flip) + " via=" + via;
        if (via == "ex")
            return into.GetInventory().CreateEntityInCargoEx(type, 0, row, col, flip);
        if (via == "find")
            return into.GetInventory().CreateEntityInCargo(type);
        InventoryLocation loc = new InventoryLocation();
        loc.SetCargo(into, null, 0, row, col, flip);
        return GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT);
    }

    protected static EntityAI MakeInSlot(OZT_Probe p, EntityAI into, string type, string slotName, string via, out string asked)
    {
        if (via == "mag")
        {
            asked = "a magazine through Weapon_Base.SpawnAttachedMagazine on " + into.GetType();
            Weapon_Base wpn = Weapon_Base.Cast(into);
            if (!wpn)
                return null;
            return wpn.SpawnAttachedMagazine(type, WeaponWithAmmoFlags.NONE);
        }
        if (slotName == "")
        {
            asked = "ATT " + into.GetType() + " slot=(the engine's choice) via=CreateAttachment";
            return into.GetInventory().CreateAttachment(type);
        }
        asked = "ATT " + into.GetType() + " slot=" + slotName + " via=" + via;
        int slot = InventorySlots.GetSlotIdFromString(slotName);
        if (slot == InventorySlots.INVALID)
        {
            asked = asked + " (no slot has that name)";
            return null;
        }
        EntityAI there = into.GetInventory().FindAttachment(slot);
        if (there)
            p.AddLine("the slot " + slotName + " already holds " + OZT_ProbeInv.Name(there));
        if (via == "att")
            return into.GetInventory().CreateAttachmentEx(type, slot);
        InventoryLocation loc = new InventoryLocation();
        loc.SetAttachment(into, null, slot);
        return GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT);
    }

    protected static EntityAI MakeInHands(PlayerBase player, string type, string via, out string asked)
    {
        EntityAI held = player.GetHumanInventory().GetEntityInHands();
        asked = "HANDS via=" + via + " (the hands hold " + OZT_ProbeInv.Name(held) + ")";
        if (via == "hands")
            return player.GetHumanInventory().CreateInHands(type);
        InventoryLocation loc = new InventoryLocation();
        loc.SetHands(player, null);
        return GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT);
    }

    // fill into=all|<handle> [class=Paper]: one item of the class on every cell
    // the occupancy map calls free, so that a fit rule can be driven to its
    // edge. A holder whose gate refuses the class keeps its room, which is the
    // point for a note wallet.
    static bool FillCargo(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        string type = p.ArgOf(args, "class", "Paper");
        string intoName = p.ArgOf(args, "into", "all");
        array<EntityAI> holders = new array<EntityAI>();
        if (intoName == "all")
        {
            array<EntityAI> all = new array<EntityAI>();
            player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
            for (int a = 0; a < all.Count(); a++)
            {
                EntityAI h = all[a];
                if (h && h != player && h.GetInventory().GetCargo())
                    holders.Insert(h);
            }
        }
        else
        {
            EntityAI one = p.ItemOf(player, intoName);
            if (!one)
            {
                detail = "fill needs into=all or a known handle";
                return false;
            }
            holders.Insert(one);
        }

        int made = 0;
        int refused = 0;
        for (int i = 0; i < holders.Count(); i++)
        {
            EntityAI holder = holders[i];
            array<bool> taken;
            int gw;
            int gh;
            if (!OZT_ProbeInv.Map(holder, null, taken, gw, gh))
                continue;
            int here = 0;
            for (int r = 0; r < gh; r++)
            {
                for (int c = 0; c < gw; c++)
                {
                    if (taken[r * gw + c])
                        continue;
                    InventoryLocation loc = new InventoryLocation();
                    loc.SetCargo(holder, null, 0, r, c, false);
                    if (GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT))
                        here++;
                    else
                        refused++;
                }
            }
            made = made + here;
            p.AddLine("filled " + here.ToString() + " cell(s) of " + OZT_ProbeInv.Name(holder));
        }
        detail = "fill: " + made.ToString() + " " + type + " made in " + holders.Count().ToString() + " holder(s), " + refused.ToString() + " cell(s) refused";
        return true;
    }

    // A stack grown or shrunk by script, or a magazine's rounds set.
    static bool ChangeAmount(OZT_Probe p, PlayerBase player, map<string, string> args, out string detail)
    {
        EntityAI e = p.ItemOf(player, p.ArgOf(args, "item", ""));
        if (!e)
        {
            detail = "qty needs item=<handle of something that still exists>";
            return false;
        }
        string type = e.GetType();
        string before = OZT_ProbeInv.Amount(e);
        string setTo = p.ArgOf(args, "set", "");
        string add = p.ArgOf(args, "add", "");
        string ammo = p.ArgOf(args, "ammo", "");
        ItemBase ib = ItemBase.Cast(e);
        Magazine mag = Magazine.Cast(e);
        string call = "nothing: give set=, add= or ammo=";
        if (mag && ammo != "")
        {
            mag.ServerSetAmmoCount(ammo.ToInt());
            call = "ServerSetAmmoCount(" + ammo + ")";
        }
        else if (ib && setTo != "")
        {
            bool setOk = ib.SetQuantity(setTo.ToFloat());
            call = "SetQuantity(" + setTo + ") answered " + OZT_ProbeLog.B(setOk);
        }
        else if (ib && add != "")
        {
            bool addOk = ib.AddQuantity(add.ToFloat());
            call = "AddQuantity(" + add + ") answered " + OZT_ProbeLog.B(addOk);
        }
        string after = "GONE in the same call";
        if (e)
            after = OZT_ProbeInv.Amount(e);
        if (e && ib)
        {
            string caps = "GetQuantityMax=" + ib.GetQuantityMax().ToString() + " GetTargetQuantityMax=" + ib.GetTargetQuantityMax(-1).ToString();
            caps = caps + " varStackMax=" + ib.ConfigGetInt("varStackMax").ToString() + " CanBeSplit=" + OZT_ProbeLog.B(ib.CanBeSplit());
            caps = caps + " IsFullQuantity=" + OZT_ProbeLog.B(ib.IsFullQuantity());
            p.AddLine(caps);
        }
        // a stack set to zero may already read null here; WatchItem takes that
        p.WatchItem(e, "amount of " + type);
        detail = type + ": " + before + " -> " + call + " -> " + after;
        return true;
    }
}
