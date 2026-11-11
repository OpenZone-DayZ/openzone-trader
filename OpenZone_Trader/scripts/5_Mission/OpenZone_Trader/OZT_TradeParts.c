// The parts strip (design §3.4): under the character, the selected thing's
// slots and what stands in them, to any depth, a part's own slots right after
// it. What each part is to the deal is said by its colour and its price:
// sold with the thing, kept by the player, not taken by the trader, riding on
// a kept part, coming with the goods, fitted in this deal.
//
// The strip shows; the menu acts (OZT_TradeMenu: a click on an empty slot
// filters the shelf to what fits it, a double click or a drag on a part keeps
// it, sells it, takes it off).

class OZT_PartsEntry
{
    // What a cell is.
    static const int PART   = 1;
    static const int EMPTY  = 2;
    static const int ROUNDS = 3;

    // What a part is to the deal.
    static const int OWN       = 1;   // the player's, outside the deal
    static const int SOLD      = 2;   // sold with the thing
    static const int KEPT      = 3;   // kept by the player
    static const int NOT_TAKEN = 4;   // the trader does not take it: it stays
    static const int RIDES     = 5;   // on a part that stays: goes with it
    static const int INCLUDED  = 6;   // comes with the goods
    static const int FITTED    = 7;   // fitted to bought goods in this deal
    static const int ONTO      = 8;   // bought onto the player's thing
    static const int ONTO_SOLD = 9;   // a part of the player's sold on its own

    int What;
    int State;
    string Type;
    EntityAI Item;
    string SlotName;
    int SlotId;
    int Price;
    int Depth;

    // The thing the slot belongs to: a real entity, or for goods the fit it
    // is (-1: the line's own goods) and its class.
    EntityAI Host;
    int HostFit = -1;
    string HostType;

    // A real part, by the server's name for it.
    int Low;
    int High;

    // A fitted part's index in its line; a line bought onto a thing.
    int Fit = -1;
    int Line = -1;

    // Rounds: how many.
    int Count;
}

// What the strip shows: the selected thing.
class OZT_PartsAt
{
    int Kind;        // OZT_Tile.SHELF, BUY, OWN or SELL
    int Line = -1;   // the cart line of BUY and SELL
    EntityAI Item;   // the thing: real, or the shelf's stand-in
    string Type;
    int Low;
    int High;
}

// Reads the entries of a strip from what the screen knows.
class OZT_PartsReader
{
    static const int MAX_CELLS = 40;

    static void Read(OZT_PartsAt at, OZT_OpenRes data, OZT_Cart cart, OZT_Shelf shelf, array<ref OZT_PartsEntry> into)
    {
        into.Clear();
        if (!at || !at.Item)
            return;
        if (at.Kind == OZT_Tile.SHELF || at.Kind == OZT_Tile.BUY)
            ReadGoods(at, -1, at.Type, at.Item, 0, data, cart, shelf, into);
        else
            ReadReal(at, at.Item, false, 0, data, cart, shelf, into);
    }

    // ---- goods ------------------------------------------------------------------------------

    // The slots of a class a player sees, by config; a weapon's magazine slot
    // too. `standIn` (may be null) answers which ones are shown.
    static void GoodsSlots(string type, EntityAI standIn, array<string> names)
    {
        array<string> all = new array<string>();
        OZT_Fit.HostSlots(type, all);
        if (OZT_Fit.IsWeapon(type))
        {
            array<string> mags = new array<string>();
            GetGame().ConfigGetTextArray("CfgWeapons " + type + " magazines", mags);
            if (mags.Count() > 0)
                all.Insert(OZT_Fit.MAGAZINE_SLOT);
        }
        for (int i = 0; i < all.Count(); i++)
        {
            int id = InventorySlots.GetSlotIdFromString(all[i]);
            if (id == InventorySlots.INVALID)
                continue;
            if (standIn && !standIn.CanDisplayAttachmentSlot(id))
                continue;
            if (Holds(names, all[i]))
                continue;
            names.Insert(all[i]);
        }
    }

    protected static void ReadGoods(OZT_PartsAt at, int hostFit, string hostType, EntityAI standIn, int depth, OZT_OpenRes data, OZT_Cart cart, OZT_Shelf shelf, array<ref OZT_PartsEntry> into)
    {
        if (depth > 4)
            return;
        array<string> slots = new array<string>();
        GoodsSlots(hostType, standIn, slots);

        // What the goods come with here, in the slots the server gives them.
        array<string> inc = new array<string>();
        array<string> incSlot = new array<string>();
        if (hostFit < 0)
        {
            OZT_WireGoods g = GoodsIn(data, hostType);
            if (g && g.With)
            {
                array<string> taken = new array<string>();
                for (int w = 0; w < g.With.Count(); w++)
                {
                    string s = OZT_Fit.SlotFor(hostType, g.With[w], taken);
                    if (s == "")
                        continue;
                    taken.Insert(s);
                    inc.Insert(g.With[w]);
                    incSlot.Insert(s);
                }
            }
        }

        OZT_CartLine line = null;
        if (at.Kind == OZT_Tile.BUY && at.Line >= 0 && at.Line < cart.Buys.Count())
            line = cart.Buys[at.Line];

        for (int i = 0; i < slots.Count() && into.Count() < MAX_CELLS; i++)
        {
            OZT_PartsEntry e = new OZT_PartsEntry();
            e.SlotName = slots[i];
            e.SlotId = InventorySlots.GetSlotIdFromString(slots[i]);
            e.Depth = depth;
            e.HostFit = hostFit;
            e.HostType = hostType;
            e.What = OZT_PartsEntry.EMPTY;

            int k = IndexOfName(incSlot, slots[i]);
            if (k > -1)
            {
                e.What = OZT_PartsEntry.PART;
                e.State = OZT_PartsEntry.INCLUDED;
                e.Type = inc[k];
                e.Item = shelf.Of(inc[k]);
                e.Price = BuyOf(data, inc[k]);
                into.Insert(e);
                continue;
            }
            int f = -1;
            if (line)
                f = FitIn(line, hostFit, slots[i]);
            if (f > -1)
            {
                e.What = OZT_PartsEntry.PART;
                e.State = OZT_PartsEntry.FITTED;
                e.Type = line.Fits[f].Type;
                e.Item = shelf.Of(e.Type);
                e.Price = line.Fits[f].Each;
                e.Fit = f;
                into.Insert(e);
                ReadGoods(at, f, e.Type, e.Item, depth + 1, data, cart, shelf, into);
                continue;
            }
            into.Insert(e);
        }
    }

    protected static int FitIn(OZT_CartLine line, int on, string slot)
    {
        for (int i = 0; i < line.Fits.Count(); i++)
        {
            OZT_CartFit fit = line.Fits[i];
            if (fit.On == on && Same(fit.SlotName, slot))
                return i;
        }
        return -1;
    }

    // ---- the player's own things ---------------------------------------------------------------

    protected static void ReadReal(OZT_PartsAt at, EntityAI host, bool riding, int depth, OZT_OpenRes data, OZT_Cart cart, OZT_Shelf shelf, array<ref OZT_PartsEntry> into)
    {
        if (!host || depth > 4)
            return;
        OZT_CartLine sell = null;
        if (at.Kind == OZT_Tile.SELL && at.Line >= 0 && at.Line < cart.Sells.Count())
            sell = cart.Sells[at.Line];
        int hostLow;
        int hostHigh;
        host.GetNetworkID(hostLow, hostHigh);

        array<int> ids = new array<int>();
        int n = host.GetInventory().GetAttachmentSlotsCount();
        for (int i = 0; i < n; i++)
            ids.Insert(host.GetInventory().GetAttachmentSlotId(i));
        Weapon_Base wpn = Weapon_Base.Cast(host);
        if (wpn && !wpn.HasInternalMagazine(-1))
        {
            int magSlot = InventorySlots.GetSlotIdFromString(OZT_Fit.MAGAZINE_SLOT);
            if (magSlot != InventorySlots.INVALID && ids.Find(magSlot) < 0)
                ids.Insert(magSlot);
        }

        for (int s = 0; s < ids.Count() && into.Count() < MAX_CELLS; s++)
        {
            int id = ids[s];
            if (!host.CanDisplayAttachmentSlot(id))
                continue;
            OZT_PartsEntry e = new OZT_PartsEntry();
            e.SlotId = id;
            e.SlotName = InventorySlots.GetSlotName(id);
            e.Depth = depth;
            e.Host = host;
            e.HostType = host.GetType();

            EntityAI part = host.GetInventory().FindAttachment(id);
            if (!part)
            {
                // A part bought onto this slot in this deal.
                int onto = OntoLine(cart, hostLow, hostHigh, e.SlotName);
                if (onto > -1)
                {
                    e.What = OZT_PartsEntry.PART;
                    e.State = OZT_PartsEntry.ONTO;
                    e.Type = cart.Buys[onto].Type;
                    e.Item = shelf.Of(e.Type);
                    e.Price = cart.Buys[onto].Each;
                    e.Line = onto;
                    into.Insert(e);
                    continue;
                }
                e.What = OZT_PartsEntry.EMPTY;
                into.Insert(e);
                continue;
            }

            int low;
            int high;
            part.GetNetworkID(low, high);
            e.What = OZT_PartsEntry.PART;
            e.Type = part.GetType();
            e.Item = part;
            e.Low = low;
            e.High = high;
            OZT_WireOwn own = OwnIn(data, low, high);
            if (own)
                e.Price = own.Price;
            bool stays = false;
            if (riding)
            {
                e.State = OZT_PartsEntry.RIDES;
                stays = true;
            }
            else if (!own)
            {
                e.State = OZT_PartsEntry.NOT_TAKEN;
                stays = true;
            }
            else if (sell && sell.Keeps(low, high, 0))
            {
                e.State = OZT_PartsEntry.KEPT;
                stays = true;
            }
            else if (sell)
            {
                e.State = OZT_PartsEntry.SOLD;
            }
            else if (SoldAlone(cart, low, high))
            {
                // Outside a sold thing, a part the cart sells on its own.
                e.State = OZT_PartsEntry.ONTO_SOLD;
            }
            else
            {
                e.State = OZT_PartsEntry.OWN;
            }
            if (!sell)
                stays = false;
            into.Insert(e);
            AddRounds(at, part, stays, depth + 1, data, cart, shelf, into);
            ReadReal(at, part, stays, depth + 1, data, cart, shelf, into);
        }
        if (depth == 0)
            AddRounds(at, host, riding, depth, data, cart, shelf, into);
    }

    // The rounds loaded in a thing, as one cell: what `open` said of them.
    protected static void AddRounds(OZT_PartsAt at, EntityAI thing, bool riding, int depth, OZT_OpenRes data, OZT_Cart cart, OZT_Shelf shelf, array<ref OZT_PartsEntry> into)
    {
        int low;
        int high;
        thing.GetNetworkID(low, high);
        OZT_WireOwn own = OwnIn(data, low, high);
        if (!own || own.Loaded <= 0 || into.Count() >= MAX_CELLS)
            return;
        OZT_CartLine sell = null;
        if (at.Kind == OZT_Tile.SELL && at.Line >= 0 && at.Line < cart.Sells.Count())
            sell = cart.Sells[at.Line];

        OZT_PartsEntry e = new OZT_PartsEntry();
        e.What = OZT_PartsEntry.ROUNDS;
        e.Depth = depth;
        e.Host = thing;
        e.HostType = thing.GetType();
        e.Low = low;
        e.High = high;
        e.Count = own.Loaded;
        e.Price = own.LoadedPrice;
        e.Type = PileOf(thing);
        if (e.Type != "")
            e.Item = shelf.Of(e.Type);
        if (riding)
            e.State = OZT_PartsEntry.RIDES;
        else if (own.LoadedPrice <= 0)
            e.State = OZT_PartsEntry.NOT_TAKEN;
        else if (sell && sell.Keeps(low, high, 1))
            e.State = OZT_PartsEntry.KEPT;
        else if (sell)
            e.State = OZT_PartsEntry.SOLD;
        else
            e.State = OZT_PartsEntry.OWN;
        into.Insert(e);
    }

    // The pile the first round of a magazine lies in when loose.
    protected static string PileOf(EntityAI thing)
    {
        Magazine mag = Magazine.Cast(thing);
        if (!mag || mag.GetAmmoCount() <= 0)
            return "";
        float dmg;
        string type;
        if (!mag.GetCartridgeAtIndex(0, dmg, type))
            return "";
        string pile;
        GetGame().ConfigGetText("CfgAmmo " + type + " spawnPileType", pile);
        return pile;
    }

    protected static bool SoldAlone(OZT_Cart cart, int low, int high)
    {
        for (int i = 0; i < cart.Sells.Count(); i++)
        {
            if (cart.Sells[i].Low == low && cart.Sells[i].High == high)
                return true;
        }
        return false;
    }

    protected static int OntoLine(OZT_Cart cart, int low, int high, string slot)
    {
        for (int i = 0; i < cart.Buys.Count(); i++)
        {
            OZT_CartLine line = cart.Buys[i];
            if (line.OnLow == low && line.OnHigh == high && Same(line.OnSlot, slot))
                return i;
        }
        return -1;
    }

    // ---- what they share --------------------------------------------------------------------------

    static OZT_WireGoods GoodsIn(OZT_OpenRes data, string type)
    {
        if (!data)
            return null;
        for (int i = 0; i < data.Goods.Count(); i++)
        {
            if (data.Goods[i].ClassName == type)
                return data.Goods[i];
        }
        return null;
    }

    static int BuyOf(OZT_OpenRes data, string type)
    {
        OZT_WireGoods g = GoodsIn(data, type);
        if (!g)
            return 0;
        return g.Buy;
    }

    static OZT_WireOwn OwnIn(OZT_OpenRes data, int low, int high)
    {
        if (!data)
            return null;
        for (int i = 0; i < data.Yours.Count(); i++)
        {
            OZT_WireOwn own = data.Yours[i];
            if (own.Low == low && own.High == high)
                return own;
        }
        return null;
    }

    static bool Same(string a, string b)
    {
        string x = a;
        x.ToLower();
        string y = b;
        y.ToLower();
        return x == y;
    }

    protected static bool Holds(array<string> names, string name)
    {
        return IndexOfName(names, name) > -1;
    }

    protected static int IndexOfName(array<string> names, string name)
    {
        for (int i = 0; i < names.Count(); i++)
        {
            if (Same(names[i], name))
                return i;
        }
        return -1;
    }
}

// The strip's cells: one ozt_trade_slot each, a tile inside where a part
// stands, laid in rows that shrink until every cell fits the strip.
class OZT_PartsStrip
{
    static const string SLOT_LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_slot.layout";
    static const string SLOT_ROOT   = "OZT_TradeSlot";

    // The strip in layout units, and a cell at its largest.
    static const float AREA_W = 440;
    static const float AREA_H = 104;
    static const float CELL_MAX = 52;
    static const float CELL_MIN = 22;
    static const float GAP = 3;

    protected Widget m_Host;
    protected ref array<Widget> m_Cells;
    protected ref array<ref OZT_PartsEntry> m_Entries;
    protected ref array<ref OZT_Tile> m_Tiles;
    protected ref array<int> m_TileEntry;
    protected int m_Lit = -1;

    void OZT_PartsStrip(Widget host)
    {
        m_Host = host;
        m_Cells = new array<Widget>();
        m_Entries = new array<ref OZT_PartsEntry>();
        m_Tiles = new array<ref OZT_Tile>();
        m_TileEntry = new array<int>();
    }

    void Clear()
    {
        for (int t = 0; t < m_Tiles.Count(); t++)
            m_Tiles[t].Unbuild();
        m_Tiles.Clear();
        m_TileEntry.Clear();
        for (int c = 0; c < m_Cells.Count(); c++)
        {
            if (m_Cells[c])
                m_Cells[c].Unlink();
        }
        m_Cells.Clear();
        m_Entries.Clear();
        m_Lit = -1;
    }

    int Count()
    {
        return m_Entries.Count();
    }

    OZT_PartsEntry Entry(int i)
    {
        if (i < 0 || i >= m_Entries.Count())
            return null;
        return m_Entries[i];
    }

    // Lays the entries out; `labels` are the slots' names as the game shows
    // them.
    void Lay(array<ref OZT_PartsEntry> entries)
    {
        Clear();
        if (!m_Host)
            return;
        for (int e = 0; e < entries.Count(); e++)
            m_Entries.Insert(entries[e]);
        int n = m_Entries.Count();
        if (n == 0)
            return;

        // The largest cell with which every cell fits.
        float side = CELL_MAX;
        int perRow = 1;
        while (side > CELL_MIN)
        {
            perRow = Math.Floor((AREA_W + GAP) / (side + GAP));
            if (perRow < 1)
                perRow = 1;
            int rows = (n + perRow - 1) / perRow;
            if (rows * side + (rows - 1) * GAP <= AREA_H)
                break;
            side = side - 2;
        }
        perRow = Math.Floor((AREA_W + GAP) / (side + GAP));
        if (perRow < 1)
            perRow = 1;

        float px = OZT_Ui.Px();
        float w = Math.Round(side * px);
        float h = Math.Round(side * px);
        float hold = Math.Round((side - 12) * px);
        bool small = side < 40;
        for (int i = 0; i < n; i++)
        {
            OZT_PartsEntry it = m_Entries[i];
            Widget cell = OZT_Ui.Make(SLOT_LAYOUT, m_Host, SLOT_ROOT);
            if (!cell)
                continue;
            m_Cells.Insert(cell);
            cell.SetUserID(1000 + i);
            int row = i / perRow;
            int col = i - row * perRow;
            cell.SetPos(Math.Round(col * (side + GAP) * px), Math.Round(row * (side + GAP) * px));
            cell.SetSize(w, h);
            Widget bg = cell.FindAnyWidget("SlotBg");
            if (bg)
            {
                bg.SetPos(1, 1);
                bg.SetSize(w - 2, h - 2);
            }
            // The slot's name is the card's to say: the cells are too small
            // for it, and the ghost icon tells an empty slot.
            OZT_Ui.Say(cell, "SlotName", "");
            ImageWidget img = ImageWidget.Cast(cell.FindAnyWidget("SlotIcon"));
            if (img)
            {
                float icon = Math.Round((side - 18) * px);
                if (icon < 8)
                    icon = 8;
                img.SetPos(Math.Round((w - icon) / 2), Math.Round(2 * px));
                img.SetSize(icon, icon);
                string ghost = GhostOf(it.SlotName);
                if (it.What == OZT_PartsEntry.ROUNDS)
                    ghost = "";
                if (ghost != "")
                    img.LoadImageFile(0, StaticGUIUtils.VerifyIconImageString(StaticGUIUtils.IMAGESETGROUP_INVENTORY, ghost));
                img.Show(ghost != "" && it.What == OZT_PartsEntry.EMPTY);
            }

            if (it.What == OZT_PartsEntry.EMPTY || !it.Item)
                continue;
            Widget into = cell.FindAnyWidget("SlotHold");
            if (!into)
                continue;
            OZT_Tile t = new OZT_Tile(OZT_Tile.PART, it.Type, it.Item);
            t.Low = it.Low;
            t.High = it.High;
            t.Price = it.Price;
            t.Index = i;
            if (it.What == OZT_PartsEntry.ROUNDS)
                t.Count = it.Count;
            // A compound condition goes through a local: assigned straight
            // into a field it corrupts the native heap (the series' rule).
            bool paid = it.State == OZT_PartsEntry.INCLUDED || it.State == OZT_PartsEntry.FITTED || it.State == OZT_PartsEntry.ONTO;
            t.Pays = paid;
            t.Why = WhyOf(it.State);
            if (!t.Build(into))
                continue;
            t.Fill(w, h - 2);
            // A cell is narrow: the count goes to the top, clear of the price.
            t.W = 1;
            t.PaintMarks();
            m_Tiles.Insert(t);
            m_TileEntry.Insert(i);
        }
    }

    // A tile's look for what the part is to the deal: 1 red (stays, not
    // taken), 2 amber (kept), 3 accent (bought in this deal), 4 dim (rides).
    static int WhyOf(int state)
    {
        if (state == OZT_PartsEntry.NOT_TAKEN)
            return 1;
        if (state == OZT_PartsEntry.KEPT)
            return 2;
        if (state == OZT_PartsEntry.FITTED || state == OZT_PartsEntry.ONTO || state == OZT_PartsEntry.ONTO_SOLD)
            return 3;
        if (state == OZT_PartsEntry.RIDES)
            return 4;
        return 0;
    }

    protected string SlotLabel(OZT_PartsEntry it)
    {
        string label;
        if (it.What == OZT_PartsEntry.ROUNDS)
            label = OZT_Ui.Words("STR_OZT_UI_PARTS_ROUNDS");
        else
            label = InventorySlots.GetSlotDisplayName(it.SlotId);
        if (label.IndexOf("#") == 0)
            label = Widget.TranslateString(label);
        if (label == "")
            label = it.SlotName;
        if (it.Depth > 0)
            label = "> " + label;
        return label;
    }

    static string GhostOf(string slot)
    {
        string icon;
        string path = "CfgSlots Slot_" + slot + " ghostIcon";
        if (GetGame().ConfigIsExisting(path))
            GetGame().ConfigGetText(path, icon);
        return icon;
    }

    // The entry under a widget: a cell or anything inside it. -1: none.
    int EntryAt(Widget w)
    {
        while (w)
        {
            for (int i = 0; i < m_Cells.Count(); i++)
            {
                if (m_Cells[i] == w)
                    return w.GetUserID() - 1000;
            }
            if (w == m_Host)
                return -1;
            w = w.GetParent();
        }
        return -1;
    }

    OZT_Tile TileAt(Widget w)
    {
        int at = EntryAt(w);
        if (at < 0)
            return null;
        for (int i = 0; i < m_Tiles.Count(); i++)
        {
            if (m_TileEntry[i] == at)
                return m_Tiles[i];
        }
        return null;
    }

    OZT_PartsEntry EntryOfTile(OZT_Tile t)
    {
        for (int i = 0; i < m_Tiles.Count(); i++)
        {
            if (m_Tiles[i] == t)
                return Entry(m_TileEntry[i]);
        }
        return null;
    }

    bool HasTile(OZT_Tile t)
    {
        return m_Tiles.Find(t) > -1;
    }

    int TileCount()
    {
        return m_Tiles.Count();
    }

    OZT_Tile TileOfIndex(int i)
    {
        return m_Tiles[i];
    }

    // The empty slot the shelf is filtered for wears the accent.
    void Light(int at)
    {
        m_Lit = at;
        for (int i = 0; i < m_Cells.Count(); i++)
        {
            Widget bg = m_Cells[i].FindAnyWidget("SlotBg");
            if (!bg)
                continue;
            if (i == at)
                bg.SetColor(OZT_Look.BedLit());
            else
                bg.SetColor(OZT_Look.Bed());
        }
    }
}
