// The character's inventory as a deal's plan sees it.
//
// A deal places several new things at once, and no engine query can do that:
// every query answers about the inventory as it is NOW. So the plan keeps a
// picture of its own -- every cargo the character carries as a grid of taken
// cells, the character's slots, the hands -- leaves out of it what the deal
// takes away, and marks every place it gives. The engine is asked two things
// only, with a local entity of the class that no client ever sees: how big the
// thing is, and whether a holder's script gate takes it.
//
// A part the player keeps of a sold thing is the one exception: it exists, and
// it moves before anything is deleted. It is placed in a picture of the
// inventory as it is, and the engine is asked about the thing itself at the
// very cell (PlaceMoved).
//
// What this rests on was measured in phase 0
// (docs/measurements/2026-11-11/answers.md): a picture built from each item's
// own inventory location agrees with the engine on every cell; a creation at a
// place the picture calls free succeeds in the very call in which the things
// that stood there were deleted with ObjectDelete; a gate's answer does not
// depend on the cell.
//
// The reading of a cargo is the storage mod's (OZS_Ops.c): the cell from the
// item's own location, never CargoBase.GetItemRowCol; the size from
// CargoBase.GetItemSize by cargo index, whose return value is not to be trusted
// while its numbers are.

// A place a plan gives to a thing that does not exist yet.
class OZT_Place
{
    int Kind;
    EntityAI Holder;
    int Row;
    int Col;
    bool Turned;
    int SlotId;

    string Text()
    {
        if (Kind == InventoryLocationType.HANDS)
            return "HANDS";
        if (Kind == InventoryLocationType.ATTACHMENT)
            return "SLOT " + InventorySlots.GetSlotName(SlotId);
        string s = "CARGO " + Holder.GetType() + " r" + Row.ToString() + " c" + Col.ToString();
        if (Turned)
            s = s + " turned";
        return s;
    }
}

// The cargo of one holder as the plan sees it.
class OZT_Grid
{
    EntityAI Holder;
    int W;
    int H;
    ref array<bool> Taken;
    bool ForNotes;

    bool IsFree(int row, int col, int w, int h)
    {
        if (row < 0 || col < 0 || w <= 0 || h <= 0)
            return false;
        if (row + h > H || col + w > W)
            return false;
        for (int r = row; r < row + h; r++)
        {
            for (int c = col; c < col + w; c++)
            {
                if (Taken[r * W + c])
                    return false;
            }
        }
        return true;
    }

    void Mark(int row, int col, int w, int h)
    {
        for (int r = row; r < row + h; r++)
        {
            for (int c = col; c < col + w; c++)
            {
                if (r >= 0 && r < H && c >= 0 && c < W)
                    Taken.Set(r * W + c, true);
            }
        }
    }

    // The first free place for a w x h thing, row by row: every unturned place
    // before any turned one.
    bool Seek(int w, int h, out int row, out int col, out bool turned)
    {
        turned = false;
        if (SeekAs(w, h, row, col))
            return true;
        if (w == h)
            return false;
        turned = true;
        return SeekAs(h, w, row, col);
    }

    protected bool SeekAs(int w, int h, out int row, out int col)
    {
        row = -1;
        col = -1;
        for (int r = 0; r + h <= H; r++)
        {
            for (int c = 0; c + w <= W; c++)
            {
                if (IsFree(r, c, w, h))
                {
                    row = r;
                    col = c;
                    return true;
                }
            }
        }
        return false;
    }
}

class OZT_InvModel
{
    protected PlayerBase m_Player;
    protected ref array<EntityAI> m_Gone;
    protected ref array<EntityAI> m_Shut;
    protected ref array<ref OZT_Grid> m_Grids;
    protected ref array<int> m_SlotsTaken;
    protected bool m_HandsFree;
    protected ref map<string, EntityAI> m_Probes;
    protected ref map<string, int> m_Gates;

    // The countable things the player keeps, and how many units each will hold
    // when the deal is done.
    protected ref array<EntityAI> m_Stacks;
    protected ref array<int> m_StackUnits;

    // `gone` is what leaves in this deal: the things sold whole and the note
    // stacks a payment spends whole. Their cells, slots and hands count as free.
    // `shut` is what stays where it is in this picture and still takes nothing:
    // a part that moves in this deal, or, in the picture as it is, a thing that
    // is about to be sold. Neither list has to name what hangs on its things or
    // lies in them: all of that takes nothing either.
    void OZT_InvModel(PlayerBase player, array<EntityAI> gone, array<EntityAI> shut)
    {
        m_Player = player;
        m_Gone = new array<EntityAI>();
        if (gone)
        {
            for (int i = 0; i < gone.Count(); i++)
                m_Gone.Insert(gone[i]);
        }
        m_Shut = new array<EntityAI>();
        if (shut)
        {
            for (int k = 0; k < shut.Count(); k++)
                m_Shut.Insert(shut[k]);
        }
        m_Grids = new array<ref OZT_Grid>();
        m_SlotsTaken = new array<int>();
        m_Probes = new map<string, EntityAI>();
        m_Gates = new map<string, int>();
        m_Stacks = new array<EntityAI>();
        m_StackUnits = new array<int>();
        Read();
    }

    // ---- reading -------------------------------------------------------------

    protected void Read()
    {
        // The engine's own order: what the character wears, then what hangs on
        // that, then what lies inside. It is the order new things try holders in.
        array<EntityAI> all = new array<EntityAI>();
        m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI e = all[i];
            if (!e || e == m_Player)
                continue;
            if (Leaves(e))
                continue;
            if (OZT_Stack.Countable(e) && !e.IsRuined())
            {
                int units = OZT_Stack.Units(e);
                m_Stacks.Insert(e);
                m_StackUnits.Insert(units);
            }
            CargoBase cargo = e.GetInventory().GetCargo();
            if (cargo && cargo.GetWidth() > 0 && cargo.GetHeight() > 0)
                m_Grids.Insert(GridOf(e, cargo));
        }

        int worn = m_Player.GetInventory().AttachmentCount();
        for (int a = 0; a < worn; a++)
        {
            EntityAI w = m_Player.GetInventory().GetAttachmentFromIndex(a);
            if (!w || m_Gone.Find(w) > -1)
                continue;
            InventoryLocation il = new InventoryLocation();
            if (w.GetInventory().GetCurrentInventoryLocation(il))
                m_SlotsTaken.Insert(il.GetSlot());
        }

        EntityAI held = m_Player.GetHumanInventory().GetEntityInHands();
        bool free = true;
        if (held && m_Gone.Find(held) < 0)
            free = false;
        m_HandsFree = free;
    }

    // Does the thing leave or move in this deal, or hang on or lie in one that
    // does? Such a thing is no holder and no stack of the plan's.
    protected bool Leaves(EntityAI e)
    {
        EntityAI at = e;
        while (at && at != m_Player)
        {
            if (m_Gone.Find(at) > -1 || m_Shut.Find(at) > -1)
                return true;
            at = at.GetHierarchyParent();
        }
        return false;
    }

    protected OZT_Grid GridOf(EntityAI holder, CargoBase cargo)
    {
        OZT_Grid g = new OZT_Grid();
        g.Holder = holder;
        g.W = cargo.GetWidth();
        g.H = cargo.GetHeight();
        g.Taken = new array<bool>();
        int cells = g.W * g.H;
        for (int k = 0; k < cells; k++)
            g.Taken.Insert(false);
        g.ForNotes = IsNoteHolder(holder);

        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI it = cargo.GetItem(i);
            if (!it || m_Gone.Find(it) > -1)
                continue;
            InventoryLocation il = new InventoryLocation();
            if (!it.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            int w;
            int h;
            cargo.GetItemSize(i, w, h);
            if (w <= 0 || h <= 0)
                continue;
            if (il.GetFlip())
            {
                int turned = w;
                w = h;
                h = turned;
            }
            g.Mark(il.GetRow(), il.GetCol(), w, h);
        }
        return g;
    }

    static bool IsNoteHolder(EntityAI e)
    {
        array<string> holders = OZT_Config.Get().Settings().Holders;
        for (int i = 0; i < holders.Count(); i++)
        {
            if (e.IsKindOf(holders[i]))
                return true;
        }
        return false;
    }

    // ---- asking the engine -----------------------------------------------------

    // One local entity per class, on the ground at the character's feet, never
    // announced to a client and never saved; Dispose() deletes them all in the
    // same call. Null: the game makes no such thing.
    EntityAI ProbeOf(string type)
    {
        EntityAI probe;
        if (m_Probes.Find(type, probe))
            return probe;
        vector feet = m_Player.GetPosition();
        probe = EntityAI.Cast(GetGame().CreateObjectEx(type, feet, ECE_LOCAL | ECE_NOPERSISTENCY_WORLD));
        m_Probes.Set(type, probe);
        return probe;
    }

    protected bool SizeOfClass(string type, out int w, out int h)
    {
        w = 0;
        h = 0;
        EntityAI probe = ProbeOf(type);
        if (!probe)
            return false;
        GetGame().GetInventoryItemSize(InventoryItem.Cast(probe), w, h);
        return w > 0 && h > 0;
    }

    // The cells one thing of the class takes: what a deal sorts its lines by.
    // 0: the game makes no such thing.
    int AreaOf(string type)
    {
        int w;
        int h;
        if (!SizeOfClass(type, w, h))
            return 0;
        return w * h;
    }

    // Does this holder's cargo take the class: the item check and the script
    // check with the occupancy test off (the flags phase 0 measured). Asked once
    // per holder and class.
    protected bool Accepts(OZT_Grid g, string type)
    {
        string key = g.Holder.GetNetworkIDString() + "|" + type;
        int known;
        if (m_Gates.Find(key, known))
            return known == 1;

        bool yes = false;
        EntityAI probe = ProbeOf(type);
        if (probe)
            yes = g.Holder.GetInventory().TestAddEntityInCargoEx(probe, 0, 0, 0, false, false, true, false, false, true, false);
        int mark = 0;
        if (yes)
            mark = 1;
        m_Gates.Set(key, mark);
        return yes;
    }

    // The slot names a class may sit on, from its own config.
    protected void SlotNamesOf(string type, array<string> names)
    {
        OZT_Fit.PartSlots(type, names);
    }

    // ---- placing -----------------------------------------------------------------

    // A free slot of the character that the class may sit on. The slot may hold
    // a thing this deal sells, so occupancy is the plan's to know and the engine
    // is asked for its gates alone. Marks the slot.
    bool PlaceOnSlot(string type, out OZT_Place place)
    {
        place = null;
        EntityAI probe = ProbeOf(type);
        if (!probe)
            return false;
        array<string> names = new array<string>();
        SlotNamesOf(type, names);
        for (int i = 0; i < names.Count(); i++)
        {
            int slot = InventorySlots.GetSlotIdFromString(names[i]);
            if (slot == InventorySlots.INVALID)
                continue;
            if (!m_Player.GetInventory().HasAttachmentSlot(slot))
                continue;
            if (m_SlotsTaken.Find(slot) > -1)
                continue;
            InventoryLocation loc = new InventoryLocation();
            loc.SetAttachment(m_Player, probe, slot);
            if (!GameInventory.LocationTestAddEntity(loc, false, true, false, false, true, false))
                continue;
            m_SlotsTaken.Insert(slot);
            place = new OZT_Place();
            place.Kind = InventoryLocationType.ATTACHMENT;
            place.Holder = m_Player;
            place.Row = -1;
            place.Col = -1;
            place.Turned = false;
            place.SlotId = slot;
            return true;
        }
        return false;
    }

    // The first cargo place for a new thing of the class: holders in the
    // engine's order (the note holders first when `notesFirst`), each asked for
    // its gate once. Marks the cells.
    bool PlaceInCargo(string type, bool notesFirst, out OZT_Place place)
    {
        place = null;
        int w;
        int h;
        if (!SizeOfClass(type, w, h))
            return false;

        array<OZT_Grid> order = new array<OZT_Grid>();
        if (notesFirst)
        {
            for (int a = 0; a < m_Grids.Count(); a++)
            {
                if (m_Grids[a].ForNotes)
                    order.Insert(m_Grids[a]);
            }
            for (int b = 0; b < m_Grids.Count(); b++)
            {
                if (!m_Grids[b].ForNotes)
                    order.Insert(m_Grids[b]);
            }
        }
        else
        {
            for (int c = 0; c < m_Grids.Count(); c++)
                order.Insert(m_Grids[c]);
        }

        for (int i = 0; i < order.Count(); i++)
        {
            OZT_Grid g = order[i];
            int row;
            int col;
            bool turned;
            if (!g.Seek(w, h, row, col, turned))
                continue;
            if (!Accepts(g, type))
                continue;
            if (turned)
                g.Mark(row, col, h, w);
            else
                g.Mark(row, col, w, h);
            place = new OZT_Place();
            place.Kind = InventoryLocationType.CARGO;
            place.Holder = g.Holder;
            place.Row = row;
            place.Col = col;
            place.Turned = turned;
            place.SlotId = InventorySlots.INVALID;
            return true;
        }
        return false;
    }

    // The hands, for the one thing that found no other place. In a vehicle the
    // hands count as busy (a rule of the design, not a measurement).
    bool PlaceInHands(string type, out OZT_Place place)
    {
        place = null;
        if (!m_HandsFree || m_Player.IsInVehicle())
            return false;
        EntityAI probe = ProbeOf(type);
        if (!probe)
            return false;

        HumanInventory hands = m_Player.GetHumanInventory();
        bool can;
        if (hands.GetEntityInHands())
            can = hands.TestAddEntityInHands(probe, false, true, false, false, true);
        else
            can = hands.CanAddEntityInHands(probe);
        if (!can)
            return false;

        m_HandsFree = false;
        place = new OZT_Place();
        place.Kind = InventoryLocationType.HANDS;
        place.Holder = m_Player;
        place.Row = -1;
        place.Col = -1;
        place.Turned = false;
        place.SlotId = InventorySlots.INVALID;
        return true;
    }

    // ---- a thing that exists and moves ------------------------------------------------

    static void SizeOf(EntityAI e, out int w, out int h)
    {
        w = 0;
        h = 0;
        GetGame().GetInventoryItemSize(InventoryItem.Cast(e), w, h);
    }

    // A cell for a part the player keeps, in this picture: holders in the
    // engine's order, the first free place of each, and the engine's own word
    // about the thing itself at that very cell (occupancy and every gate, so a
    // pouch with something in it is not laid where the game would not let it
    // be). Marks the cells.
    bool PlaceMoved(EntityAI e, out OZT_Place place)
    {
        place = null;
        int w;
        int h;
        SizeOf(e, w, h);
        if (w <= 0 || h <= 0)
            return false;
        for (int i = 0; i < m_Grids.Count(); i++)
        {
            OZT_Grid g = m_Grids[i];
            int row;
            int col;
            bool turned;
            if (!g.Seek(w, h, row, col, turned))
                continue;
            InventoryLocation dst = new InventoryLocation();
            dst.SetCargo(g.Holder, e, 0, row, col, turned);
            if (!GameInventory.LocationCanAddEntity(dst))
                continue;
            if (turned)
                g.Mark(row, col, h, w);
            else
                g.Mark(row, col, w, h);
            place = new OZT_Place();
            place.Kind = InventoryLocationType.CARGO;
            place.Holder = g.Holder;
            place.Row = row;
            place.Col = col;
            place.Turned = turned;
            place.SlotId = InventorySlots.INVALID;
            return true;
        }
        return false;
    }

    // The cells of a place another picture gave to a part that moves there:
    // taken in this one. `w` and `h` are the part's own, unturned.
    void Reserve(OZT_Place place, int w, int h)
    {
        if (!place)
            return;
        if (place.Kind == InventoryLocationType.HANDS)
        {
            m_HandsFree = false;
            return;
        }
        if (place.Kind != InventoryLocationType.CARGO)
            return;
        for (int i = 0; i < m_Grids.Count(); i++)
        {
            OZT_Grid g = m_Grids[i];
            if (g.Holder != place.Holder)
                continue;
            if (place.Turned)
                g.Mark(place.Row, place.Col, h, w);
            else
                g.Mark(place.Row, place.Col, w, h);
            return;
        }
    }

    // ---- stacks the player keeps -------------------------------------------------

    // How many units a kept stack will hold after the deal, as planned so far.
    // -1: the model does not count the thing.
    int PlannedUnits(EntityAI stack)
    {
        int at = m_Stacks.Find(stack);
        if (at < 0)
            return -1;
        return m_StackUnits[at];
    }

    void PlanUnits(EntityAI stack, int units)
    {
        int at = m_Stacks.Find(stack);
        if (at > -1)
            m_StackUnits.Set(at, units);
    }

    // The stacks of a class that lie in a cargo, are in the condition of new
    // goods and have room, with the room of each as planned so far; with
    // `notesFirst` the ones inside note holders come first. A stack in another
    // condition gets nothing: what differs in condition is neither stacked nor
    // averaged (the owner, 2026-11-11).
    void StacksWithRoom(string type, bool notesFirst, out array<EntityAI> stacks, out array<int> room)
    {
        stacks = new array<EntityAI>();
        room = new array<int>();
        for (int pass = 0; pass < 2; pass++)
        {
            for (int i = 0; i < m_Stacks.Count(); i++)
            {
                EntityAI s = m_Stacks[i];
                if (!s || s.GetType() != type)
                    continue;
                if (!OZT_Stack.LikeNew(s))
                    continue;
                InventoryLocation il = new InventoryLocation();
                if (!s.GetInventory().GetCurrentInventoryLocation(il))
                    continue;
                if (il.GetType() != InventoryLocationType.CARGO)
                    continue;
                bool inHolder = IsNoteHolder(il.GetParent());
                bool first = inHolder && notesFirst;
                if (pass == 0 && !first)
                    continue;
                if (pass == 1 && first)
                    continue;
                int free = OZT_Stack.Max(s) - m_StackUnits[i];
                if (free <= 0)
                    continue;
                stacks.Insert(s);
                room.Insert(free);
            }
        }
    }

    // ---- making what was planned ----------------------------------------------------

    // Creates a thing at its planned place, the way phase 0 measured. Null: the
    // engine refused, and nothing was created anywhere.
    static EntityAI Make(PlayerBase player, OZT_Place place, string type)
    {
        InventoryLocation loc = new InventoryLocation();
        if (place.Kind == InventoryLocationType.HANDS)
            loc.SetHands(player, null);
        else if (place.Kind == InventoryLocationType.ATTACHMENT)
            loc.SetAttachment(place.Holder, null, place.SlotId);
        else
            loc.SetCargo(place.Holder, null, 0, place.Row, place.Col, place.Turned);
        return GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT);
    }

    // Does the thing stand where the plan put it?
    static bool Stands(EntityAI e, OZT_Place place)
    {
        if (!e)
            return false;
        InventoryLocation il = new InventoryLocation();
        if (!e.GetInventory().GetCurrentInventoryLocation(il))
            return false;
        if (il.GetType() != place.Kind)
            return false;
        if (place.Kind == InventoryLocationType.ATTACHMENT)
            return il.GetSlot() == place.SlotId;
        if (place.Kind != InventoryLocationType.CARGO)
            return true;
        if (il.GetParent() != place.Holder)
            return false;
        if (il.GetRow() != place.Row || il.GetCol() != place.Col)
            return false;
        return il.GetFlip() == place.Turned;
    }

    void Dispose()
    {
        for (int i = 0; i < m_Probes.Count(); i++)
        {
            EntityAI probe = m_Probes.GetElement(i);
            if (probe)
                GetGame().ObjectDelete(probe);
        }
        m_Probes.Clear();
    }
}
