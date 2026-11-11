// The reading side of the trader probe: how an item is named in a report, where
// it stands, and the occupancy of a cargo.
//
// Occupancy is built from each item's OWN inventory location. CargoBase.
// GetItemRowCol is not used: on 1.29 it answered cells that were not the item's.
class OZT_ProbeInv
{
    // "<netid> <Type>". The network id is the same number on the server and on
    // every client, which is what lets two dumps be compared line by line.
    static string Name(EntityAI e)
    {
        if (!e)
            return "null";
        return e.GetNetworkIDString() + " " + e.GetType();
    }

    // The unturned size of an item, as the engine holds it for the entity.
    static void SizeOf(EntityAI e, out int w, out int h)
    {
        w = 0;
        h = 0;
        if (!e)
            return;
        GetGame().GetInventoryItemSize(InventoryItem.Cast(e), w, h);
    }

    // The size a CLASS declares in config, looked up in the three roots an item
    // can live in. This is what a plan has before the item exists.
    static bool ConfigSize(string type, out int w, out int h)
    {
        w = 0;
        h = 0;
        array<string> roots = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
        for (int i = 0; i < roots.Count(); i++)
        {
            string path = roots.Get(i) + " " + type + " itemSize";
            if (!GetGame().ConfigIsExisting(path))
                continue;
            TIntArray size = new TIntArray();
            GetGame().ConfigGetIntArray(path, size);
            if (size.Count() == 2)
            {
                w = size.Get(0);
                h = size.Get(1);
                return true;
            }
        }
        return false;
    }

    // Where an item stands, as text that reads the same on both sides.
    static string Where(EntityAI e)
    {
        if (!e)
            return "GONE";
        if (!e.GetInventory())
            return "NOINV";
        InventoryLocation il = new InventoryLocation();
        if (!e.GetInventory().GetCurrentInventoryLocation(il))
            return "NOLOC";
        int lt = il.GetType();
        string parent = "none";
        if (il.GetParent())
            parent = il.GetParent().GetType() + "#" + il.GetParent().GetNetworkIDString();
        if (lt == InventoryLocationType.CARGO || lt == InventoryLocationType.PROXYCARGO)
        {
            string s = "CARGO " + parent + " r" + il.GetRow().ToString() + " c" + il.GetCol().ToString();
            s = s + " flip=" + OZT_ProbeLog.B(il.GetFlip());
            return s;
        }
        if (lt == InventoryLocationType.ATTACHMENT)
            return "ATT " + parent + " slot=" + InventorySlots.GetSlotName(il.GetSlot());
        if (lt == InventoryLocationType.HANDS)
            return "HANDS " + parent;
        if (lt == InventoryLocationType.GROUND)
            return "GROUND";
        return "LOCTYPE" + lt.ToString();
    }

    // How much there is of it, the way the inventory screen counts: rounds for
    // anything that is a magazine (weapon magazines and ammunition piles alike),
    // the item's own quantity otherwise.
    static string Amount(EntityAI e)
    {
        Magazine mag = Magazine.Cast(e);
        if (mag)
            return "ammo " + mag.GetAmmoCount().ToString() + "/" + mag.GetAmmoMax().ToString();
        ItemBase ib = ItemBase.Cast(e);
        if (ib && ib.HasQuantity())
            return "qty " + OZT_ProbeLog.R1(ib.GetQuantity()) + "/" + ib.GetQuantityMax().ToString();
        return "-";
    }

    // One line of a dump.
    static string Line(EntityAI e)
    {
        int w;
        int h;
        SizeOf(e, w, h);
        string s = Name(e) + " | " + Where(e) + " | " + w.ToString() + "x" + h.ToString();
        s = s + " | " + Amount(e);
        return s;
    }

    // Everything the character carries, one line each, sorted, so that the
    // server's file and a client's file can be compared with a plain diff.
    static void Tree(PlayerBase player, array<string> lines)
    {
        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        for (int i = 0; i < items.Count(); i++)
        {
            EntityAI e = items.Get(i);
            if (!e || e == player)
                continue;
            lines.Insert(Line(e));
        }
        lines.Sort();
    }

    // The cargo of `holder` as a grid of taken cells, row by row. False when the
    // holder has no cargo. `skip` is left out of the map: an item that is about
    // to leave does not stand in its own way.
    static bool Map(EntityAI holder, EntityAI skip, out array<bool> taken, out int gw, out int gh)
    {
        gw = 0;
        gh = 0;
        taken = new array<bool>();
        if (!holder || !holder.GetInventory())
            return false;
        CargoBase cargo = holder.GetInventory().GetCargo();
        if (!cargo)
            return false;
        gw = cargo.GetWidth();
        gh = cargo.GetHeight();
        int cells = gw * gh;
        for (int k = 0; k < cells; k++)
            taken.Insert(false);
        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI it = cargo.GetItem(i);
            if (!it || it == skip)
                continue;
            InventoryLocation il = new InventoryLocation();
            if (!it.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            int w;
            int h;
            SizeOf(it, w, h);
            if (il.GetFlip())
            {
                int turned = w;
                w = h;
                h = turned;
            }
            for (int r = il.GetRow(); r < il.GetRow() + h; r++)
            {
                for (int c = il.GetCol(); c < il.GetCol() + w; c++)
                {
                    if (r >= 0 && r < gh && c >= 0 && c < gw)
                        taken.Set(r * gw + c, true);
                }
            }
        }
        return cells > 0;
    }

    // Is the w x h rectangle at row, col inside the grid and free?
    static bool RectFree(array<bool> taken, int gw, int gh, int row, int col, int w, int h)
    {
        if (row < 0 || col < 0 || w <= 0 || h <= 0)
            return false;
        if (row + h > gh || col + w > gw)
            return false;
        for (int r = row; r < row + h; r++)
        {
            for (int c = col; c < col + w; c++)
            {
                if (taken.Get(r * gw + c))
                    return false;
            }
        }
        return true;
    }
}
