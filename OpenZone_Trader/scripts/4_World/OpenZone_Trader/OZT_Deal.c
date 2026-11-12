// A deal: one cart of sells and buys, settled in one step or not at all.
//
// The server is the judge. It resolves every sold entity itself, computes every
// price itself, and plans every place before anything changes (design §4.3):
//
//   1. the player and the trader      who, where, doing nothing else
//   2. the sells                      yours, taken here; a stack, or a tree
//   3. the buys and the net           compared with what the client showed
//   4. the money                      enough of it; which notes; what comes back
//   5. the places                     kept parts first, then everything bought,
//                                     every round that comes back, every note given
//   6. the commit                     ONE script call
//
// The commit's order is phase 0's: the parts the player keeps move off what is
// sold, while it still stands; what leaves is deleted with ObjectDelete, which
// frees its place at once; stacks shrink; new things are created at their
// planned places; stacks grow. No frame passes in between.
//
// A thing that is not a stack is sold as a tree (OZT_Tree): itself, what hangs
// on it and what is loaded in it, each part by its own line of the book. What
// lies inside a thing is no part of it: a container is sold when everything in
// it is sold by the same deal.
//
// A bought thing may arrive with parts on it: what the goods come with at this
// trader, and what the request fits to it. A part may also be bought straight
// onto a thing the player carries. Every part is goods of the trader, paid at
// its own price; it needs a slot, not cells (OZT_Fit), and is attached in the
// commit, a parent before what hangs on it.

// A stack the player keeps, grown (a merge) or shrunk (a part sold or paid).
class OZT_Grow
{
    EntityAI Stack;
    int Units;
}

// A thing the commit creates.
class OZT_Make
{
    string ClassName;
    ref OZT_Place Place;

    // Above zero: a stack of this many units. Zero: as the game makes it, a
    // magazine with no rounds in it.
    int Units;

    // Below zero: new. Otherwise the health of rounds that come back, 0..1.
    float Health;

    // What arrives attached to it; null when nothing does.
    OZT_PartList Parts;
}

// The parts of one bought thing, in the order they are attached.
class OZT_PartList
{
    ref array<ref OZT_PartMake> Items;

    void OZT_PartList()
    {
        Items = new array<ref OZT_PartMake>();
    }
}

// A part the commit attaches: to the thing its list belongs to (On below
// zero), or to an earlier part of the same list.
class OZT_PartMake
{
    string ClassName;
    string SlotName;
    int SlotId;
    int On;
}

// A part bought onto a thing the player carries: the first entry of Parts is
// that part, the rest hang on it.
class OZT_Onto
{
    EntityAI Host;
    ref OZT_PartList Parts;
}

// A part the player keeps: it comes off the sold thing and goes to a place the
// plan chose in the inventory as it was. No place: none was found.
class OZT_Move
{
    EntityAI Item;
    ref OZT_Place Place;
    int W;
    int H;
}

class OZT_Deal
{
    // The most one line may ask for: pieces, and units under the unit rule;
    // and parts on one bought thing.
    static const int MAX_PIECES = 50;
    static const int MAX_UNITS = 5000;
    static const int MAX_PARTS = 16;

    // A quote changes nothing, but it lays a whole plan: one client may ask
    // for one only so often.
    static const int QUOTE_GAP_MS = 100;

    // When each player last made a deal: uid -> the game's milliseconds.
    protected static ref map<string, int> s_Last = new map<string, int>();
    protected static ref map<string, int> s_LastQuote = new map<string, int>();

    // The stand's switch, for one commit: 1 treats the planned place of every
    // kept part as refused, 2 the engine's own choice too, 3 the ground as
    // well. It is how the three ways out of a wrong plan are seen working;
    // nothing but the stand verb sets it.
    protected static int s_StandRefuse = 0;

    protected PlayerBase m_Player;
    protected OZT_Point m_Trader;
    protected OZT_MoneyProvider m_Money;
    protected ref OZT_InvModel m_Model;
    protected ref OZT_DealRes m_Res;

    // Everything that is deleted: every sold node of every tree, the stacks
    // sold whole, the note stacks spent whole.
    protected ref array<EntityAI> m_Gone;
    protected ref array<ref OZT_Grow> m_Grows;
    protected ref array<ref OZT_Make> m_Makes;
    protected ref array<string> m_BuyClasses;
    protected ref array<int> m_BuyCounts;
    protected ref array<string> m_BuyRules;
    // The condition of the stock line each bought line comes from (design
    // §11.1): 0 for everything that is not stocked.
    protected ref array<int> m_BuyLevels;
    // What the deal takes from the trader's stock and brings into it.
    protected ref OZT_StockCursor m_Cursor;
    // The licences the deal sells (design §11.2): granted at the commit.
    protected ref array<string> m_Licences;

    // The parts of each bought line (an empty list for a plain one), and the
    // parts bought onto things the player carries.
    protected ref array<ref OZT_PartList> m_BuyParts;
    protected ref array<ref OZT_Onto> m_Ontos;
    protected ref array<string> m_Homeless;
    protected ref array<int> m_HomelessUnits;
    protected ref array<float> m_HomelessHealth;
    protected ref array<OZT_PartList> m_HomelessParts;

    // The sold trees; the parts that come off them; the same parts as a list
    // for the pictures; the lots of rounds that come back.
    protected ref array<ref OZT_Tree> m_Trees;
    protected ref array<ref OZT_Move> m_Moves;
    protected ref array<EntityAI> m_Shut;
    protected ref array<OZT_RoundLot> m_KeptLots;

    // The things the sell lines name, sold whole: what a sold part leaves.
    protected ref array<EntityAI> m_SoldRoots;

    // True while a plan is laid with the first thing that needs cells set aside
    // for the hands: the second laying of PlanPlaces.
    protected bool m_Aside;

    protected int m_Net;
    protected int m_Give;
    protected string m_Lines;

    // The notes the player gives and takes at the change tab, by face value.
    // They are money already: the money after the deal counts them, the net
    // does not know them from goods.
    protected int m_NotesOut;
    protected int m_NotesIn;

    // The whole of it, as the JSON of an OZT_DealRes. `commit` false is a quote:
    // every check and the whole plan, nothing changed, the net not compared.
    static string Run(PlayerBase player, string json, bool commit, out bool ok, out string error)
    {
        OZT_Deal deal = new OZT_Deal(player);
        error = deal.Go(json, commit);
        ok = error == "";
        string body = deal.Body();
        deal.Close();
        return body;
    }

    // 4_World classes held by a static: let go at mission end.
    static void Reset()
    {
        s_Last.Clear();
        s_LastQuote.Clear();
        s_StandRefuse = 0;
    }

    static void StandRefuse(int ways)
    {
        s_StandRefuse = ways;
    }

    void OZT_Deal(PlayerBase player)
    {
        m_Player = player;
        m_Res = new OZT_DealRes();
        m_Gone = new array<EntityAI>();
        m_Grows = new array<ref OZT_Grow>();
        m_Makes = new array<ref OZT_Make>();
        m_BuyClasses = new array<string>();
        m_BuyCounts = new array<int>();
        m_BuyRules = new array<string>();
        m_BuyLevels = new array<int>();
        m_Licences = new array<string>();
        m_BuyParts = new array<ref OZT_PartList>();
        m_Ontos = new array<ref OZT_Onto>();
        m_Homeless = new array<string>();
        m_HomelessUnits = new array<int>();
        m_HomelessHealth = new array<float>();
        m_HomelessParts = new array<OZT_PartList>();
        m_Trees = new array<ref OZT_Tree>();
        m_Moves = new array<ref OZT_Move>();
        m_Shut = new array<EntityAI>();
        m_KeptLots = new array<OZT_RoundLot>();
        m_SoldRoots = new array<EntityAI>();
        m_Lines = "";
    }

    string Body()
    {
        string body;
        string err;
        JsonFileLoader<OZT_DealRes>.MakeData(m_Res, body, err, false);
        return body;
    }

    void Close()
    {
        if (m_Model)
            m_Model.Dispose();
    }

    protected string Uid()
    {
        PlayerIdentity id = m_Player.GetIdentity();
        if (!id)
            return "";
        return id.GetPlainId();
    }

    // Returns "" when the deal stands, or the key of the refusal.
    string Go(string json, bool commit)
    {
        OZT_DealReq req = new OZT_DealReq();
        string err;
        if (!JsonFileLoader<OZT_DealReq>.LoadData(json, req, err))
        {
            OZT_Log.Dbg("deal refused: unreadable body (" + err + ")");
            return OZT_Const.NO_INTERNAL;
        }

        // What arrived was allocated by the loader: read once, now, into our own.
        array<ref OZT_WireSell> sells = new array<ref OZT_WireSell>();
        if (req.Sells)
        {
            for (int s = 0; s < req.Sells.Count(); s++)
            {
                if (req.Sells[s])
                    sells.Insert(req.Sells[s].Copy());
            }
        }
        array<ref OZT_WireBuy> buys = new array<ref OZT_WireBuy>();
        if (req.Buys)
        {
            for (int b = 0; b < req.Buys.Count(); b++)
            {
                if (req.Buys[b])
                    buys.Insert(req.Buys[b].Copy());
            }
        }
        string traderId = req.Trader;
        int shown = req.Net;

        string why = CheckPlayer(traderId, commit);
        if (why != "")
            return why;
        m_Cursor = new OZT_StockCursor(m_Trader.StockId());
        why = ReadSells(sells);
        if (why != "")
            return why;
        why = ReadBuys(buys);
        if (why != "")
            return why;
        why = CheckOffs();
        if (why != "")
            return why;

        m_Res.Net = m_Net;
        if (commit && shown != m_Net)
            return OZT_Const.NO_CHANGED;

        why = PlanMoney();
        if (why != "")
            return why;
        why = PlanPlaces();
        if (why != "")
            return why;

        if (m_Net > 0)
            m_Res.Paid = m_Net;
        else
            m_Res.Received = -m_Net;

        if (!commit)
        {
            m_Res.Balance = m_Money.Balance(m_Player) - m_Net - m_NotesOut + m_NotesIn;
            return "";
        }

        // A commit that stops has paid nothing and given nothing.
        why = Commit();
        if (why != "")
        {
            m_Res.Paid = 0;
            m_Res.Received = 0;
        }
        return why;
    }

    // ---- 1. the player and the trader ------------------------------------------

    protected string CheckPlayer(string traderId, bool commit)
    {
        if (!m_Player.IsAlive() || m_Player.IsUnconscious() || m_Player.IsRestrained())
            return OZT_Const.NO_BUSY;

        // "The player is doing nothing with their things": vanilla's own check,
        // read in phase 0.
        bool acting = false;
        if (m_Player.GetActionManager() && m_Player.GetActionManager().GetRunningAction())
            acting = true;
        if (acting || !m_Player.CanDeleteItems())
            return OZT_Const.NO_BUSY;
        if (m_Player.GetDayZPlayerInventory().IsProcessing())
            return OZT_Const.NO_BUSY;

        m_Trader = OZT_Config.Get().FindTrader(traderId);
        float away = -1;
        if (m_Trader)
            away = OZT_Points.Away(m_Trader.Id, m_Player);
        if (away < 0)
            return OZT_Const.NO_CLOSED;
        if (away > m_Trader.Radius)
            return OZT_Const.NO_TOO_FAR;
        if (!OZT_Gates.Admits(m_Player, m_Trader))
            return OZT_Const.NO_ACCESS;

        m_Money = OZT_Money.Of(m_Trader);
        if (!m_Money)
            return OZT_Const.NO_INTERNAL;

        if (commit)
        {
            int last;
            if (s_Last.Find(Uid(), last))
            {
                int since = GetGame().GetTime() - last;
                if (since < OZT_Config.Get().Settings().CooldownMs)
                    return OZT_Const.NO_BUSY;
            }
        }
        else
        {
            int asked;
            int now = GetGame().GetTime();
            if (s_LastQuote.Find(Uid(), asked) && now - asked < QUOTE_GAP_MS)
                return OZT_Const.NO_BUSY;
            s_LastQuote.Set(Uid(), now);
        }
        return "";
    }

    // ---- 2. the sells ----------------------------------------------------------

    protected string ReadSells(array<ref OZT_WireSell> sells)
    {
        // Every line's thing first: a container goes when everything that
        // lies in it goes in the same deal, so each line has to know them all.
        array<EntityAI> roots = new array<EntityAI>();
        array<EntityAI> whole = new array<EntityAI>();
        for (int r = 0; r < sells.Count(); r++)
        {
            OZT_WireSell asked = sells[r];
            EntityAI thing = EntityAI.Cast(GetGame().GetObjectByNetworkId(asked.Low, asked.High));
            if (!thing || thing.GetHierarchyRootPlayer() != m_Player)
                return OZT_Const.NO_NOT_YOURS;
            if (roots.Find(thing) > -1)
                return OZT_Const.NO_INTERNAL;
            roots.Insert(thing);
            bool part = false;
            if (OZT_Stack.Countable(thing) && asked.Count > 0 && asked.Count < OZT_Stack.Units(thing))
                part = true;
            if (!part)
                whole.Insert(thing);
        }

        // A thing that hangs on another line's thing is that line's part
        // already, sold or kept: no screen sends it twice.
        for (int h = 0; h < roots.Count(); h++)
        {
            if (HangsOn(roots[h], roots))
                return OZT_Const.NO_INTERNAL;
        }

        int total = 0;
        for (int i = 0; i < sells.Count(); i++)
        {
            OZT_WireSell line = sells[i];
            EntityAI e = roots[i];
            int price = 0;
            if (OZT_Stack.Countable(e))
            {
                int units = OZT_Stack.Units(e);
                bool entire = true;
                if (line.Count > 0 && line.Count < units)
                {
                    units = line.Count;
                    entire = false;
                }
                // A ruined thing prices at zero.
                OZT_BookEntry entry = OZT_Book.EntryOf(e.GetType());
                price = OZT_Book.SellPriceAt(m_Trader, entry, e, units, m_Cursor);
                if (price <= 0)
                    return OZT_Const.NO_NOT_TAKEN;
                if (entry.Category == OZT_Const.CATEGORY_CHANGE)
                    m_NotesOut = m_NotesOut + price;
                if (entire)
                {
                    m_Gone.Insert(e);
                    m_SoldRoots.Insert(e);
                }
                else
                {
                    AddGrow(e, -units);
                }
                m_Lines = m_Lines + " -" + e.GetType() + "x" + units.ToString() + "=" + price.ToString();
            }
            else
            {
                // "Not taken" before "not empty": if the trader does not want
                // the thing, emptying it would not help.
                OZT_Tree tree = OZT_Tree.Read(m_Trader, e, line.Keep, whole, m_Cursor);
                if (!tree.Taken())
                    return OZT_Const.NO_NOT_TAKEN;
                if (tree.Inside > 0)
                {
                    m_Res.Left = tree.Inside;
                    return OZT_Const.NO_NOT_EMPTY;
                }
                price = tree.Total;
                int back = m_Res.KeptRounds;
                TakeTree(tree);
                m_SoldRoots.Insert(e);
                back = m_Res.KeptRounds - back;
                string word = " -" + e.GetType() + "=" + price.ToString();
                word = word + "(" + tree.Gone.Count().ToString() + " sold, " + tree.Moves.Count().ToString() + " kept";
                if (back > 0)
                    word = word + ", " + back.ToString() + " rounds back";
                m_Lines = m_Lines + word + ")";
            }

            total = total + price;
            m_Res.SellPrices.Insert(price);
            m_Res.Sold = total;
        }
        m_Net = m_Net - total;
        return "";
    }

    // Does the thing hang, by attachments alone, on one of the others?
    protected bool HangsOn(EntityAI e, array<EntityAI> roots)
    {
        EntityAI at = e;
        while (at)
        {
            InventoryLocation il = new InventoryLocation();
            if (!at.GetInventory().GetCurrentInventoryLocation(il))
                return false;
            if (il.GetType() != InventoryLocationType.ATTACHMENT)
                return false;
            EntityAI up = il.GetParent();
            if (!up || up == m_Player)
                return false;
            if (roots.Find(up) > -1)
                return true;
            at = up;
        }
        return false;
    }

    // What a sold tree means for the deal: its sold nodes are deleted, the
    // parts that come off it move, the rounds that are not sold come back.
    protected void TakeTree(OZT_Tree tree)
    {
        m_Trees.Insert(tree);
        for (int g = 0; g < tree.Gone.Count(); g++)
            m_Gone.Insert(tree.Gone[g]);
        for (int m = 0; m < tree.Moves.Count(); m++)
        {
            EntityAI part = tree.Moves[m];
            int w;
            int h;
            OZT_InvModel.SizeOf(part, w, h);
            OZT_Move move = new OZT_Move();
            move.Item = part;
            move.W = w;
            move.H = h;
            m_Moves.Insert(move);
            m_Shut.Insert(part);
        }
        int rounds = m_Res.KeptRounds;
        for (int l = 0; l < tree.Lots.Count(); l++)
        {
            OZT_RoundLot lot = tree.Lots[l];
            if (!lot.Kept)
                continue;
            m_KeptLots.Insert(lot);
            rounds = rounds + lot.Count;
        }
        m_Res.KeptRounds = rounds;
        m_Res.Kept = m_Moves.Count();
    }

    // ---- 3. the buys -------------------------------------------------------------

    protected string ReadBuys(array<ref OZT_WireBuy> buys)
    {
        int total = 0;
        for (int i = 0; i < buys.Count(); i++)
        {
            OZT_WireBuy line = buys[i];
            OZT_BookEntry entry = OZT_Book.EntryOf(line.ClassName);

            // Goods this trader does not sell: the catalogue the client holds
            // is stale.
            // Locked for this player: said so; not sold here at all: the
            // catalogue the client holds is stale.
            if (OZT_Book.Locked(m_Player, m_Trader, entry))
                return OZT_Const.NO_LOCKED;
            // A licence (design §11.2): one, not held yet, nothing to make.
            string lid = "";
            if (entry)
                lid = OZT_Licences.IdOf(entry.ClassName);
            if (lid != "")
            {
                if (line.Count != 1 || line.Fit.Count() > 0 || line.OnLow != 0 || line.OnHigh != 0)
                    return OZT_Const.NO_INTERNAL;
                if (OZT_Licences.Has(OZT_Licences.UidOf(m_Player), lid) || m_Licences.Find(lid) > -1)
                    return OZT_Const.NO_HAS_LICENCE;
                int fee = OZT_Book.BuyFor(m_Player, m_Trader, entry, 1);
                if (fee <= 0)
                    return OZT_Const.NO_CHANGED;
                m_Licences.Insert(lid);
                total = total + fee;
                m_Res.BuyPrices.Insert(fee);
                m_Res.Bought = total;
                m_Lines = m_Lines + " +" + entry.ClassName + "=" + fee.ToString();
                continue;
            }
            // A stock line (design §11.1): its condition, and enough in it.
            int level = line.Level;
            bool stocked = false;
            if (entry)
                stocked = entry.Stocked;
            if (level < 0 || level >= OZT_Const.LEVEL_RUINED)
                return OZT_Const.NO_CHANGED;
            if (level > 0 && !stocked)
                return OZT_Const.NO_CHANGED;
            if (stocked && line.Count > 0 && m_Cursor.Count(entry.ClassName, level) < line.Count)
                return OZT_Const.NO_STOCK;
            int price = OZT_Book.BuyForAt(m_Player, m_Trader, entry, level, line.Count, m_Cursor);
            if (price <= 0)
                return OZT_Const.NO_CHANGED;

            int most = MAX_PIECES;
            if (entry.Rule == OZT_Const.RULE_UNIT)
                most = MAX_UNITS;
            if (line.Count > most)
                return OZT_Const.NO_INTERNAL;

            // A thing of the player's the line's goods go onto, as a part.
            EntityAI host = null;
            bool onto = line.OnLow != 0 || line.OnHigh != 0;
            if (onto)
            {
                host = EntityAI.Cast(GetGame().GetObjectByNetworkId(line.OnLow, line.OnHigh));
                if (!host || host.GetHierarchyRootPlayer() != m_Player)
                    return OZT_Const.NO_NOT_YOURS;
                if (m_Gone.Find(host) > -1 || UnderGone(host))
                    return OZT_Const.NO_CHANGED;
            }
            bool fitted = line.Fit.Count() > 0;
            if ((onto || fitted) && line.Count != 1)
                return OZT_Const.NO_INTERNAL;

            // The line's parts: what the goods come with here, then what the
            // request fits; each priced, each given a slot.
            OZT_PartList list = new OZT_PartList();
            array<ref OZT_PartMake> parts = list.Items;
            int extra = 0;
            string why = ReadParts(line, entry, host, parts, extra);
            if (why != "")
                return why;
            price = price + extra;
            if (entry.Category == OZT_Const.CATEGORY_CHANGE)
                m_NotesIn = m_NotesIn + price;

            total = total + price;
            m_Res.BuyPrices.Insert(price);
            m_Res.Bought = total;
            string word = " +" + entry.ClassName + "x" + line.Count.ToString() + "=" + price.ToString();
            if (parts.Count() > 0)
                word = word + "(" + parts.Count().ToString() + " parts)";
            if (onto)
            {
                OZT_Onto o = new OZT_Onto();
                o.Host = host;
                o.Parts = list;
                m_Ontos.Insert(o);
                word = word + " onto " + host.GetType();
            }
            else
            {
                m_BuyClasses.Insert(entry.ClassName);
                m_BuyCounts.Insert(line.Count);
                m_BuyRules.Insert(entry.Rule);
                m_BuyLevels.Insert(level);
                m_BuyParts.Insert(list);
            }
            m_Lines = m_Lines + word;
        }
        m_Net = m_Net + total;
        return "";
    }

    // One bought line's parts into `parts`, and what they add to the line's
    // price into `extra`. For a line that goes onto a carried thing the first
    // entry is the line's own goods on that thing.
    protected string ReadParts(OZT_WireBuy line, OZT_BookEntry entry, EntityAI host, array<ref OZT_PartMake> parts, out int extra)
    {
        extra = 0;
        string why;
        int root = -1;
        if (host)
        {
            // The slot may hold a thing this deal sells: then it is free. A
            // part it does not sell comes off into the bags, as a kept part
            // of a sold thing does -- a cell planned, moved before anything is
            // deleted -- and the bought part takes its slot (the owner,
            // 2026-11-12: replace a part, sell the old one or keep it).
            why = AddPart(parts, entry.ClassName, line.SlotName, -1, host.GetType());
            if (why != "")
                return why;
            EntityAI there = host.GetInventory().FindAttachment(parts[0].SlotId);
            if (there && m_Gone.Find(there) < 0)
            {
                if (!Displace(there))
                    return OZT_Const.NO_CHANGED;
            }
            if (TakenOnto(host, parts[0].SlotId))
                return OZT_Const.NO_CHANGED;
            root = 0;
        }

        // What the goods come with at this trader, each for every piece: a
        // stocked part the stock cannot give stays off; a worn line comes bare
        // (design §11.1).
        array<string> comes;
        if (line.Level == 0)
            OZT_Book.ComesAt(m_Player, m_Trader, entry, line.Count, m_Cursor, comes);
        else
            comes = new array<string>();
        for (int c = 0; c < comes.Count(); c++)
        {
            why = AddPart(parts, comes[c], "", root, entry.ClassName);
            if (why != "")
            {
                OZT_Log.Error("deal: '" + entry.ClassName + "' comes with '" + comes[c] + "', which found no slot on it");
                return OZT_Const.NO_INTERNAL;
            }
            extra = extra + OZT_Book.BuyForAt(m_Player, m_Trader, OZT_Book.EntryOf(comes[c]), 0, line.Count, m_Cursor);
        }

        // What the request fits: goods of this trader, an entry hanging on the
        // bought thing or on an earlier entry of its own list.
        int first = parts.Count();
        if (first + line.Fit.Count() > MAX_PARTS)
            return OZT_Const.NO_INTERNAL;
        for (int f = 0; f < line.Fit.Count(); f++)
        {
            OZT_WireFit fit = line.Fit[f];
            OZT_BookEntry sold = OZT_Book.EntryOf(fit.ClassName);
            if (OZT_Book.Locked(m_Player, m_Trader, sold))
                return OZT_Const.NO_LOCKED;
            if (sold && sold.Stocked && m_Cursor.Count(sold.ClassName, 0) < 1)
                return OZT_Const.NO_STOCK;
            int cost = OZT_Book.BuyForAt(m_Player, m_Trader, sold, 0, 1, m_Cursor);
            if (cost <= 0)
                return OZT_Const.NO_CHANGED;
            if (fit.On >= f)
                return OZT_Const.NO_INTERNAL;
            int on = root;
            string onClass = entry.ClassName;
            if (fit.On >= 0)
            {
                on = first + fit.On;
                onClass = parts[on].ClassName;
            }
            why = AddPart(parts, sold.ClassName, fit.SlotName, on, onClass);
            if (why != "")
                return why;
            extra = extra + cost;
        }
        return "";
    }

    // One more part of a list: its slot on what it hangs on, by the request's
    // word or by config, and not one an earlier part of the list has taken.
    protected string AddPart(array<ref OZT_PartMake> parts, string type, string slot, int on, string onClass)
    {
        array<string> taken = new array<string>();
        for (int i = 0; i < parts.Count(); i++)
        {
            if (parts[i].On == on)
                taken.Insert(parts[i].SlotName);
        }
        string name = slot;
        if (name == "")
            name = OZT_Fit.SlotFor(onClass, type, taken);
        else if (!OZT_Fit.SlotOf(onClass, type, name) || InNames(taken, name))
            name = "";
        if (name == "")
            return OZT_Const.NO_CHANGED;
        int id = InventorySlots.GetSlotIdFromString(name);
        if (id == InventorySlots.INVALID)
            return OZT_Const.NO_CHANGED;

        OZT_PartMake p = new OZT_PartMake();
        p.ClassName = type;
        p.SlotName = name;
        p.SlotId = id;
        p.On = on;
        parts.Insert(p);
        return "";
    }

    protected bool InNames(array<string> names, string name)
    {
        string want = name;
        want.ToLower();
        for (int i = 0; i < names.Count(); i++)
        {
            string have = names[i];
            have.ToLower();
            if (have == want)
                return true;
        }
        return false;
    }

    // A part of the player's that a bought part pushes out of its slot: kept,
    // like a part that comes off a sold thing.
    // `first`: it moves before the parts already planned -- it holds one of
    // them on.
    protected bool Displace(EntityAI part, bool first = false)
    {
        if (m_Shut.Find(part) > -1)
            return false;
        int w;
        int h;
        OZT_InvModel.SizeOf(part, w, h);
        if (w <= 0 || h <= 0)
            return false;
        OZT_Move move = new OZT_Move();
        move.Item = part;
        move.W = w;
        move.H = h;
        if (first)
            m_Moves.InsertAt(move, 0);
        else
            m_Moves.Insert(move);
        m_Shut.Insert(part);
        m_Res.Kept = m_Moves.Count();
        return true;
    }

    // Has an earlier line of this deal bought a part into that slot of that
    // thing already?
    protected bool TakenOnto(EntityAI host, int slotId)
    {
        for (int i = 0; i < m_Ontos.Count(); i++)
        {
            OZT_Onto o = m_Ontos[i];
            if (o.Host == host && o.Parts.Items[0].SlotId == slotId)
                return true;
        }
        return false;
    }

    // Does every part the deal takes off a thing come off, as the game lets
    // it (OZT_Offs)? A part sold off a thing the player keeps, a part a bought
    // one pushes out, a part kept off a sold thing or not taken with it --
    // thing by thing, the sold parts of a sold thing taken off first where
    // they hold the rest. The answer does not depend on where anything goes.
    protected string CheckOffs()
    {
        array<EntityAI> hosts = new array<EntityAI>();
        array<ref array<EntityAI>> leaves = new array<ref array<EntityAI>>();
        for (int m = 0; m < m_Moves.Count(); m++)
            OZT_Offs.AddLeave(hosts, leaves, m_Moves[m].Item);
        for (int r = 0; r < m_SoldRoots.Count(); r++)
            OZT_Offs.AddLeave(hosts, leaves, m_SoldRoots[r]);
        for (int h = 0; h < hosts.Count(); h++)
        {
            EntityAI host = hosts[h];
            array<EntityAI> may = new array<EntityAI>();
            SoldPartsOf(host, may);
            array<EntityAI> stuck = new array<EntityAI>();
            OZT_Offs.Stuck(host, leaves[h], may, stuck);
            if (stuck.Count() == 0)
                continue;
            // On a thing the player keeps, what holds the part on comes off
            // with it, into the bags, before it (a light on a rifle whose rail
            // handguard is replaced or sold); the screen says so.
            bool freed = false;
            array<EntityAI> holders = new array<EntityAI>();
            if (m_Gone.Find(host) < 0)
                freed = OZT_Offs.Holders(host, leaves[h], holders);
            for (int k = 0; k < holders.Count() && freed; k++)
            {
                if (m_Shut.Find(holders[k]) > -1)
                    continue;
                if (!Displace(holders[k], true))
                    freed = false;
            }
            if (freed)
            {
                OZT_Log.Dbg("deal: " + holders.Count().ToString() + " part(s) come off " + host.GetType() + " with " + stuck[0].GetType());
                continue;
            }
            OZT_Log.Dbg("deal: " + stuck[0].GetType() + " does not come off " + host.GetType());
            return OZT_Const.NO_PARTS;
        }
        return "";
    }

    // The sold parts in the slots of a thing a sold tree holds.
    protected void SoldPartsOf(EntityAI host, array<EntityAI> into)
    {
        for (int t = 0; t < m_Trees.Count(); t++)
        {
            OZT_Tree tree = m_Trees[t];
            for (int n = 0; n < tree.Nodes.Count(); n++)
            {
                OZT_Node node = tree.Nodes[n];
                if (node.State != OZT_Tree.SOLD || !node.Up)
                    continue;
                if (node.Up.Item == host)
                    into.Insert(node.Item);
            }
        }
    }

    // ---- 4. the money --------------------------------------------------------------

    protected string PlanMoney()
    {
        m_Give = 0;
        if (!m_Money.Physical())
        {
            if (m_Net > 0 && m_Money.Balance(m_Player) < m_Net)
                return OZT_Const.NO_MONEY;
            return "";
        }

        if (m_Net <= 0)
        {
            m_Give = -m_Net;
            return "";
        }

        array<ref OZT_NoteStack> stacks;
        OZT_ItemMoney.Stacks(m_Player, m_Gone, stacks);
        // Notes the player gives at the change tab leave first: a stack that
        // gives some pays with the rest of it, never with them again.
        for (int g = 0; g < m_Grows.Count(); g++)
        {
            if (m_Grows[g].Units < 0)
                OZT_ItemMoney.Spent(stacks, m_Grows[g].Stack, -m_Grows[g].Units);
        }
        int change = OZT_ItemMoney.Pay(stacks, m_Net);
        if (change < 0)
            return OZT_Const.NO_MONEY;
        for (int i = 0; i < stacks.Count(); i++)
        {
            OZT_NoteStack s = stacks[i];
            if (s.Spend <= 0)
                continue;
            if (s.Spend >= s.Units)
            {
                // What the giving left of the stack pays: it goes whole.
                DropGrows(s.Item);
                m_Gone.Insert(s.Item);
            }
            else
            {
                AddGrow(s.Item, -s.Spend);
            }
        }
        m_Give = change;
        return "";
    }

    protected void DropGrows(EntityAI stack)
    {
        for (int i = m_Grows.Count() - 1; i >= 0; i--)
        {
            if (m_Grows[i].Stack == stack)
                m_Grows.RemoveOrdered(i);
        }
    }

    // ---- 5. the places ----------------------------------------------------------------

    // The fit rule: everything has a place; or the one thing that has none goes
    // to free hands; or the deal is refused with the count.
    //
    // A deal lays several things at once and each takes the first place that
    // fits, so the order decides what fits. Two rules (decision 11 of the plan):
    // things are laid largest first, so that small ones fill what large ones
    // leave; and when more than one thing is left without a place, everything is
    // laid once more with the first thing that needs cells -- the largest -- set
    // aside for the hands, because its cells free the most room.
    //
    // The parts the player keeps are laid before all of that and in another
    // picture, the inventory as it is: they move while what they hang on still
    // stands, so the room a sale frees is not theirs (decisions 22 and 23 of
    // the phase 3a plan). What has no place is counted together.
    protected string PlanPlaces()
    {
        int adrift = PlanMoves();
        int kept = m_Grows.Count();
        string why = LayThings(false);
        if (why != "")
            return why;
        int left = m_Homeless.Count() + adrift;
        if (left == 0)
            return "";

        if (left == 1 && adrift == 1)
        {
            if (MoveToHands())
                return "";
        }
        else if (left == 1)
        {
            OZT_Place hands;
            if (m_Model.PlaceInHands(m_Homeless[0], hands))
            {
                AddMake(m_Homeless[0], hands, m_HomelessUnits[0], m_HomelessHealth[0]);
                OZT_PartList rode = m_HomelessParts[0];
                if (rode && rode.Items.Count() > 0)
                    m_Makes[m_Makes.Count() - 1].Parts = rode;
                m_Res.InHands = m_Homeless[0];
                ForgetHomeless();
                return "";
            }
        }
        else if (adrift == 0)
        {
            UnlayThings(kept);
            why = LayThings(true);
            if (why != "")
                return why;
            if (m_Homeless.Count() == 0)
                return "";
            m_Res.InHands = "";
        }

        // The count a refusal carries is the first laying's: how many things had
        // no place when the bags were used best.
        m_Res.Left = left;
        return OZT_Const.NO_SPACE;
    }

    // A cell for every part the player keeps, the largest first, in the
    // inventory as it is with everything that leaves or moves in this deal
    // shut. Returns how many found none.
    protected int PlanMoves()
    {
        int count = m_Moves.Count();
        if (count == 0)
            return 0;

        array<EntityAI> shut = new array<EntityAI>();
        for (int g = 0; g < m_Gone.Count(); g++)
            shut.Insert(m_Gone[g]);
        for (int s = 0; s < m_Shut.Count(); s++)
            shut.Insert(m_Shut[s]);
        OZT_InvModel now = new OZT_InvModel(m_Player, null, shut);

        array<int> order = new array<int>();
        for (int n = 0; n < count; n++)
        {
            int best = -1;
            for (int i = 0; i < count; i++)
            {
                if (order.Find(i) > -1)
                    continue;
                if (best < 0)
                {
                    best = i;
                    continue;
                }
                if (m_Moves[i].W * m_Moves[i].H > m_Moves[best].W * m_Moves[best].H)
                    best = i;
            }
            order.Insert(best);
        }

        int adrift = 0;
        for (int o = 0; o < order.Count(); o++)
        {
            OZT_Move move = m_Moves[order[o]];
            OZT_Place place;
            if (now.PlaceMoved(move.Item, place))
            {
                move.Place = place;
                continue;
            }
            move.Place = null;
            adrift++;
        }
        now.Dispose();
        return adrift;
    }

    // The one kept part that found no cell: to the hands as they are now, for
    // it moves before anything is deleted.
    protected bool MoveToHands()
    {
        if (m_Player.GetHumanInventory().GetEntityInHands() || m_Player.IsInVehicle())
            return false;
        for (int i = 0; i < m_Moves.Count(); i++)
        {
            OZT_Move move = m_Moves[i];
            if (move.Place)
                continue;
            InventoryLocation dst = new InventoryLocation();
            dst.SetHands(m_Player, move.Item);
            if (!GameInventory.LocationCanAddEntity(dst))
                return false;
            OZT_Place hands = new OZT_Place();
            hands.Kind = InventoryLocationType.HANDS;
            hands.Holder = m_Player;
            hands.Row = -1;
            hands.Col = -1;
            hands.Turned = false;
            hands.SlotId = InventorySlots.INVALID;
            move.Place = hands;
            m_Res.InHands = move.Item.GetType();
            return true;
        }
        return false;
    }

    protected void ForgetHomeless()
    {
        m_Homeless.Clear();
        m_HomelessUnits.Clear();
        m_HomelessHealth.Clear();
        m_HomelessParts.Clear();
    }

    // Lays everything the deal brings into a fresh picture of the inventory as
    // it will be: the bought lines largest first, then the rounds that come
    // back, then the change or the payout. With `aside` the first thing that
    // needs cells goes to the hands instead. What finds no place is left in
    // m_Homeless. Returns "" or the key of an internal refusal.
    protected string LayThings(bool aside)
    {
        m_Model = new OZT_InvModel(m_Player, m_Gone, m_Shut);
        m_Aside = aside;

        // The cells the kept parts were given are taken.
        for (int k = 0; k < m_Moves.Count(); k++)
        {
            OZT_Move move = m_Moves[k];
            m_Model.Reserve(move.Place, move.W, move.H);
        }

        // Do the bought parts fit, as the engine's own gates see it? Asked
        // once: the answer does not depend on the laying.
        if (!aside)
        {
            string fits = CheckFits();
            if (fits != "")
                return fits;
        }

        // Parts sold and parts paid shrink their stacks before anything is
        // merged into them.
        for (int g = 0; g < m_Grows.Count(); g++)
        {
            EntityAI shrunk = m_Grows[g].Stack;
            int held = m_Model.PlannedUnits(shrunk);
            m_Model.PlanUnits(shrunk, held + m_Grows[g].Units);
        }

        bool wear = OZT_Config.Get().Settings().WearBought == 1;
        array<int> order;
        LayingOrder(order);
        for (int o = 0; o < order.Count(); o++)
        {
            int b = order[o];
            string type = m_BuyClasses[b];
            EntityAI probe = m_Model.ProbeOf(type);
            if (!probe)
            {
                OZT_Log.Error("deal: the game makes nothing of class " + type);
                return OZT_Const.NO_INTERNAL;
            }
            if (m_BuyRules[b] == OZT_Const.RULE_UNIT && OZT_Stack.Countable(probe))
                PlaceUnits(type, m_BuyCounts[b], false);
            else
                PlacePieces(type, m_BuyCounts[b], wear, m_BuyParts[b], OZT_Book.LevelHealth(m_BuyLevels[b]));
        }

        // The rounds that come back: stacks of their own, as they were.
        for (int l = 0; l < m_KeptLots.Count(); l++)
            PlaceLot(m_KeptLots[l]);

        // The change or the payout, in the largest notes that make the sum.
        if (m_Give > 0)
        {
            array<int> counts;
            if (!OZT_ItemMoney.Change(m_Give, counts))
            {
                OZT_Log.Error("deal: the configured notes cannot make " + m_Give.ToString());
                return OZT_Const.NO_INTERNAL;
            }
            array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
            for (int n = notes.Count() - 1; n >= 0; n--)
            {
                if (counts[n] > 0)
                    PlaceUnits(notes[n].ClassName, counts[n], true);
            }
        }
        return "";
    }

    // Forgets what LayThings did, so that it can be done again: the picture and
    // its probe entities, the things to make, and the merges it added. The first
    // `kept` entries of m_Grows are the deal's own: parts sold and notes paid.
    protected void UnlayThings(int kept)
    {
        m_Model.Dispose();
        m_Makes.Clear();
        ForgetHomeless();
        m_Res.InHands = "";
        while (m_Grows.Count() > kept)
        {
            int last = m_Grows.Count() - 1;
            m_Grows.RemoveOrdered(last);
        }
    }

    // The bought lines by the cells one thing of each takes, largest first; lines
    // of one size stay in the order they were asked in.
    protected void LayingOrder(out array<int> order)
    {
        order = new array<int>();
        array<int> areas = new array<int>();
        int count = m_BuyClasses.Count();
        for (int b = 0; b < count; b++)
        {
            int area = m_Model.AreaOf(m_BuyClasses[b]);
            areas.Insert(area);
        }
        for (int n = 0; n < count; n++)
        {
            int best = -1;
            for (int i = 0; i < count; i++)
            {
                if (order.Find(i) > -1)
                    continue;
                if (best < 0)
                {
                    best = i;
                    continue;
                }
                if (areas[i] > areas[best])
                    best = i;
            }
            order.Insert(best);
        }
    }

    // The hands for the thing set aside, once per laying.
    protected bool AsideToHands(string type, out OZT_Place place)
    {
        place = null;
        if (!m_Aside)
            return false;
        m_Aside = false;
        if (!m_Model.PlaceInHands(type, place))
            return false;
        m_Res.InHands = type;
        return true;
    }

    // `units` of a countable class: onto the stacks of the class the player
    // keeps, then as new stacks of at most the class's maximum.
    protected void PlaceUnits(string type, int units, bool notes)
    {
        int left = units;
        array<EntityAI> stacks;
        array<int> room;
        m_Model.StacksWithRoom(type, notes, stacks, room);
        for (int i = 0; i < stacks.Count() && left > 0; i++)
        {
            int take = room[i];
            if (take > left)
                take = left;
            EntityAI onto = stacks[i];
            AddGrow(onto, take);
            int held = m_Model.PlannedUnits(onto);
            m_Model.PlanUnits(onto, held + take);
            left = left - take;
        }
        if (left <= 0)
            return;

        EntityAI probe = m_Model.ProbeOf(type);
        int most = OZT_Stack.Max(probe);
        if (most < 1)
            most = 1;
        while (left > 0)
        {
            int n = left;
            if (n > most)
                n = most;
            OZT_Place place;
            bool placed = AsideToHands(type, place);
            if (!placed)
                placed = m_Model.PlaceInCargo(type, notes, place);
            if (placed)
                AddMake(type, place, n, -1);
            else
                AddHomeless(type, n, -1);
            left = left - n;
        }
    }

    // Rounds that come back: new stacks of the pile's class in the condition
    // the rounds were in, each no larger than a full pile, never merged into a
    // stack the player has (decision 24: what differs in condition is not
    // stacked, and these need not be new).
    protected void PlaceLot(OZT_RoundLot lot)
    {
        int most = OZT_Tree.PileMax(lot.Pile);
        float health = lot.Health();
        int left = lot.Count;
        while (left > 0)
        {
            int n = left;
            if (n > most)
                n = most;
            OZT_Place place;
            bool placed = AsideToHands(lot.Pile, place);
            if (!placed)
                placed = m_Model.PlaceInCargo(lot.Pile, false, place);
            if (placed)
                AddMake(lot.Pile, place, n, health);
            else
                AddHomeless(lot.Pile, n, health);
            left = left - n;
        }
    }

    // `count` pieces of a class: a free slot of the character first when bought
    // gear is worn (decision 4), then a cargo. A thing that goes onto a slot
    // needs no cells and is never the one set aside. `parts` arrive on every
    // piece; they need no place of their own. `health` below zero: new;
    // otherwise the health a worn stock line's things are made with.
    protected void PlacePieces(string type, int count, bool wear, OZT_PartList parts, float health = -1)
    {
        for (int i = 0; i < count; i++)
        {
            OZT_Place place;
            bool placed = false;
            if (wear)
                placed = m_Model.PlaceOnSlot(type, place);
            if (!placed)
                placed = AsideToHands(type, place);
            if (!placed)
                placed = m_Model.PlaceInCargo(type, false, place);
            if (placed)
            {
                AddMake(type, place, 0, health);
                if (parts.Items.Count() > 0)
                    m_Makes[m_Makes.Count() - 1].Parts = parts;
                continue;
            }
            AddHomeless(type, 0, health);
            m_HomelessParts.Set(m_HomelessParts.Count() - 1, parts);
        }
    }

    // The engine's gates for every bought part, with stand-ins: a local entity
    // of the class for a thing that does not exist yet, the carried thing
    // itself for a part that goes onto it. The item check and the script check,
    // no occupancy: which slot is free is the plan's own to know.
    // The parts go on one after another, each asked of a thing that already
    // holds the ones before it: a thing's own script may refuse a part beside
    // another (an AK takes a bayonet or a suppressor, never both), which a bare
    // probe of the class never shows (2026-11-12, the owner's run).
    protected string CheckFits()
    {
        for (int b = 0; b < m_BuyParts.Count(); b++)
        {
            array<ref OZT_PartMake> parts = m_BuyParts[b].Items;
            if (parts.Count() == 0)
                continue;
            EntityAI fresh = m_Model.FreshOf(m_BuyClasses[b]);
            bool takes = GatesTake(fresh, parts);
            if (fresh)
                GetGame().ObjectDelete(fresh);
            if (!takes)
                return OZT_Const.NO_PARTS;
        }

        // Parts bought onto one thing of the player's, whatever lines they came
        // in, go onto one twin of it, in order: each onto line's first part on
        // the thing, the rest on that part.
        array<EntityAI> hosts = new array<EntityAI>();
        for (int o = 0; o < m_Ontos.Count(); o++)
        {
            EntityAI host = m_Ontos[o].Host;
            if (hosts.Find(host) > -1)
                continue;
            hosts.Insert(host);
            array<ref OZT_PartMake> all = new array<ref OZT_PartMake>();
            for (int k = o; k < m_Ontos.Count(); k++)
            {
                if (m_Ontos[k].Host != host)
                    continue;
                int offset = all.Count();
                array<ref OZT_PartMake> its = m_Ontos[k].Parts.Items;
                for (int j = 0; j < its.Count(); j++)
                {
                    OZT_PartMake src = its[j];
                    OZT_PartMake cp = new OZT_PartMake();
                    cp.ClassName = src.ClassName;
                    cp.SlotName = src.SlotName;
                    cp.SlotId = src.SlotId;
                    cp.On = src.On;
                    if (cp.On >= 0)
                        cp.On = cp.On + offset;
                    all.Insert(cp);
                }
            }
            EntityAI twin = m_Model.TwinOf(host);
            bool fine = GatesTake(twin, all);
            if (twin)
                GetGame().ObjectDelete(twin);
            if (!fine)
                return OZT_Const.NO_PARTS;
        }
        return "";
    }

    // `host` is a local thing (FreshOf, TwinOf) that this fills: each part is
    // asked of the engine where it would go, then made there, so the next one
    // is asked beside it. A part's `On` is the index of the part it hangs on.
    protected bool GatesTake(EntityAI host, array<ref OZT_PartMake> parts)
    {
        if (!host)
            return false;
        array<EntityAI> made = new array<EntityAI>();
        for (int i = 0; i < parts.Count(); i++)
        {
            OZT_PartMake p = parts[i];
            EntityAI on = host;
            if (p.On >= 0)
            {
                on = null;
                if (p.On < made.Count())
                    on = made[p.On];
            }
            EntityAI part = m_Model.ProbeOf(p.ClassName);
            if (!on || !part)
                return false;
            InventoryLocation loc = new InventoryLocation();
            loc.SetAttachment(on, part, p.SlotId);
            if (!GameInventory.LocationTestAddEntity(loc, false, true, false, false, true, false))
            {
                OZT_Log.Dbg("deal: the gates refuse " + p.ClassName + " in the slot " + p.SlotName + " of " + on.GetType());
                return false;
            }
            EntityAI put = on.GetInventory().CreateAttachmentEx(p.ClassName, p.SlotId);
            if (!put)
                put = on.GetInventory().CreateAttachment(p.ClassName);
            made.Insert(put);
        }
        return true;
    }

    // What found no place. PlacePieces sets the parts of its own thing right
    // after; everything else has none.
    protected void AddHomeless(string type, int units, float health)
    {
        m_Homeless.Insert(type);
        m_HomelessUnits.Insert(units);
        m_HomelessHealth.Insert(health);
        m_HomelessParts.Insert(null);
    }

    protected void AddGrow(EntityAI stack, int units)
    {
        OZT_Grow g = new OZT_Grow();
        g.Stack = stack;
        g.Units = units;
        m_Grows.Insert(g);
    }

    protected void AddMake(string type, OZT_Place place, int units, float health)
    {
        OZT_Make m = new OZT_Make();
        m.ClassName = type;
        m.Place = place;
        m.Units = units;
        m.Health = health;
        m_Makes.Insert(m);
    }

    // ---- 6. the commit --------------------------------------------------------------------

    protected string Commit()
    {
        // One script call from here on. The parts the player keeps come off
        // first, while what they hang on still stands: nothing is paid and
        // nothing is deleted before every one of them stands somewhere else.
        string why = MoveParts();
        if (why != "")
            return why;

        // Money that is not items leaves before anything is deleted: if it
        // cannot, nothing is sold (parts that came off stay off).
        if (!m_Money.Physical() && m_Net > 0)
        {
            if (!m_Money.Take(m_Player, m_Net, "deal"))
                return OZT_Const.NO_MONEY;
        }

        // A sold part goes with what it hangs on: only the topmost of what
        // leaves is deleted by name.
        for (int d = 0; d < m_Gone.Count(); d++)
        {
            EntityAI leaving = m_Gone[d];
            if (!leaving || UnderGone(leaving))
                continue;
            GetGame().ObjectDelete(leaving);
        }
        for (int s = 0; s < m_Grows.Count(); s++)
        {
            if (m_Grows[s].Units < 0)
                OZT_Stack.Add(m_Grows[s].Stack, m_Grows[s].Units);
        }
        for (int m = 0; m < m_Makes.Count(); m++)
            CreateOne(m_Makes[m]);

        // Parts bought onto things the player carries: after the deletions,
        // so that a slot this deal frees is free.
        for (int o = 0; o < m_Ontos.Count(); o++)
        {
            OZT_Onto onto = m_Ontos[o];
            AttachParts(onto.Host, onto.Parts.Items);
        }
        for (int g = 0; g < m_Grows.Count(); g++)
        {
            if (m_Grows[g].Units > 0)
                Merge(m_Grows[g].Stack, m_Grows[g].Units);
        }

        if (!m_Money.Physical() && m_Net < 0)
            m_Money.Give(m_Player, -m_Net, "deal");

        // The stock moves of the deal onto the shelf and into its file.
        m_Cursor.Commit();
        // The licences bought, onto the character.
        for (int lc = 0; lc < m_Licences.Count(); lc++)
            OZT_Licences.Grant(OZT_Licences.UidOf(m_Player), m_Licences[lc]);

        string uid = Uid();
        int now = GetGame().GetTime();
        s_Last.Set(uid, now);
        m_Res.Balance = m_Money.Balance(m_Player);

        string line = "deal: " + uid + " at '" + m_Trader.Id + "' net " + m_Net.ToString();
        line = line + " (" + m_Money.Id() + "):" + m_Lines;
        OZT_Log.Info(line);
        return "";
    }

    // Does the thing hang on, or lie in, something else that leaves?
    protected bool UnderGone(EntityAI e)
    {
        EntityAI at = e.GetHierarchyParent();
        while (at && at != m_Player)
        {
            if (m_Gone.Find(at) > -1)
                return true;
            at = at.GetHierarchyParent();
        }
        return false;
    }

    // Is the holder one that leaves or moves in this deal, or inside one?
    protected bool Leaving(EntityAI holder)
    {
        EntityAI at = holder;
        while (at && at != m_Player)
        {
            if (m_Gone.Find(at) > -1 || m_Shut.Find(at) > -1)
                return true;
            at = at.GetHierarchyParent();
        }
        return false;
    }

    // Every kept part to its planned place. The plan asked the engine about
    // each part at its very cell, so a refusal here is a defect of the plan;
    // the part then goes to the engine's own choice, then to the ground at the
    // player's feet, and the log says so. A part that can go nowhere stops the
    // deal before anything is paid or deleted: a part is never lost.
    protected string MoveParts()
    {
        int refuse = s_StandRefuse;
        s_StandRefuse = 0;
        for (int i = 0; i < m_Moves.Count(); i++)
        {
            OZT_Move move = m_Moves[i];
            if (MoveOne(move, refuse))
                continue;
            string lost = "deal defect: the kept part ";
            if (move.Item)
                lost = lost + move.Item.GetType();
            lost = lost + " could be moved nowhere; the deal is refused, nothing was sold";
            OZT_Log.Error(lost);
            return OZT_Const.NO_INTERNAL;
        }
        return "";
    }

    protected bool MoveOne(OZT_Move move, int refuse)
    {
        EntityAI e = move.Item;
        if (!e)
            return false;
        InventoryLocation src = new InventoryLocation();
        if (!e.GetInventory().GetCurrentInventoryLocation(src))
            return false;

        string was = "no place";
        if (move.Place)
            was = move.Place.Text();
        if (move.Place && refuse < 1)
        {
            InventoryLocation dst = new InventoryLocation();
            if (move.Place.Kind == InventoryLocationType.HANDS)
                dst.SetHands(m_Player, e);
            else
                dst.SetCargo(move.Place.Holder, e, 0, move.Place.Row, move.Place.Col, move.Place.Turned);
            if (SyncMove(src, dst, true))
                return true;
        }

        // The engine's own choice among the cargos the player keeps.
        string head = "deal defect: the engine refused the kept part " + e.GetType() + " at " + was;
        InventoryLocation own = new InventoryLocation();
        bool found = false;
        if (refuse < 2)
            found = m_Player.GetInventory().FindFreeLocationFor(e, FindInventoryLocationType.CARGO, own);
        if (found && !Leaving(own.GetParent()))
        {
            if (SyncMove(src, own, true))
            {
                OZT_Log.Error(head + "; it went to the engine's own choice");
                return true;
            }
        }

        // The ground at the player's feet, as the game itself drops a thing
        // off a holder that is no living player: its own place for it, then
        // the same move.
        if (refuse >= 3)
            return false;
        InventoryLocation ground = new InventoryLocation();
        bool spot = GameInventory.SetGroundPosByOwner(m_Player, e, ground);
        if (!spot)
            return false;
        if (!SyncMove(src, ground, false))
            return false;
        OZT_Log.Error(head + "; it lies at the player's feet");
        return true;
    }

    // The engine's synchronous move and the word to the clients, as phase 0
    // measured them. A place in an inventory is asked first: a destination the
    // engine refuses is never handed to it.
    protected bool SyncMove(InventoryLocation src, InventoryLocation dst, bool ask)
    {
        if (ask && !GameInventory.LocationCanAddEntity(dst))
            return false;
        if (!GameInventory.LocationSyncMoveEntity(src, dst))
            return false;
        InventoryInputUserData.SendServerMove(null, InventoryCommandType.SYNC_MOVE, src, dst);
        return true;
    }

    protected void CreateOne(OZT_Make make)
    {
        EntityAI made = OZT_InvModel.Make(m_Player, make.Place, make.ClassName);
        if (!made)
            made = Rescue(make);
        else if (!OZT_InvModel.Stands(made, make.Place))
            OZT_Log.Error("deal defect: " + make.ClassName + " was planned at " + make.Place.Text() + " and stands elsewhere");

        if (made && make.Units > 0)
            OZT_Stack.Set(made, make.Units);
        else if (made)
            Unload(made);
        if (made && make.Health >= 0)
            Wear(made, make.Health);
        if (made && make.Parts)
            AttachParts(made, make.Parts.Items);
    }

    // The parts of one thing, a parent before what hangs on it. A part the
    // engine refuses is still made, loose: a paid thing is never lost, and
    // what hangs on it is attached to it where it lies.
    protected void AttachParts(EntityAI host, array<ref OZT_PartMake> parts)
    {
        array<EntityAI> made = new array<EntityAI>();
        for (int i = 0; i < parts.Count(); i++)
        {
            OZT_PartMake p = parts[i];
            EntityAI on = host;
            if (p.On >= 0)
                on = made[p.On];
            EntityAI part = null;
            if (on)
                part = AttachOne(on, p);
            if (!part)
            {
                part = m_Player.GetHumanInventory().CreateInInventory(p.ClassName);
                string how = "by the engine's own search";
                if (!part)
                {
                    vector feet = m_Player.GetPosition();
                    part = EntityAI.Cast(GetGame().CreateObjectEx(p.ClassName, feet, ECE_PLACE_ON_SURFACE));
                    how = "at the player's feet";
                }
                string msg = "deal defect: the engine refused the part " + p.ClassName + " in the slot " + p.SlotName;
                msg = msg + "; it was made " + how;
                OZT_Log.Error(msg);
                if (part)
                    Unload(part);
            }
            made.Insert(part);
        }
    }

    // A magazine goes onto a weapon through the weapon, so that its state
    // machine knows (phase 0: a bare attachment call leaves it without one);
    // anything else straight into its slot. A bought magazine holds no rounds.
    protected EntityAI AttachOne(EntityAI on, OZT_PartMake p)
    {
        EntityAI part = null;
        Weapon_Base wpn = Weapon_Base.Cast(on);
        if (wpn && OZT_Fit.IsMagazine(p.ClassName))
            part = wpn.SpawnAttachedMagazine(p.ClassName, WeaponWithAmmoFlags.NONE);
        else
            part = on.GetInventory().CreateAttachmentEx(p.ClassName, p.SlotId);
        if (part)
            Unload(part);
        return part;
    }

    // Rounds that come back are as they were: the pile takes their health,
    // then every round in it their damage -- in that order, the game's own
    // (Ammunition_Base.SetFromProjectile): a change of health level rewrites
    // the damage of every round.
    protected void Wear(EntityAI made, float health)
    {
        made.SetHealth01("", "", health);
        Magazine pile = Magazine.Cast(made);
        if (!pile)
            return;
        float dmg = 1 - health;
        int n = pile.GetAmmoCount();
        for (int i = 0; i < n; i++)
            pile.SetCartridgeDamageAtIndex(i, dmg);
    }

    // The game makes a magazine full, and its rounds are goods of their own:
    // a bought magazine holds none (decision 16 of the phase 1 plan). A sold
    // magazine's rounds are paid for one by one (OZT_Tree).
    protected void Unload(EntityAI made)
    {
        Magazine mag = Magazine.Cast(made);
        if (mag && !mag.IsAmmoPile())
            mag.ServerSetAmmoCount(0);
    }

    // The engine refused a planned place. A paid thing is never lost: the
    // engine's own search, then the ground at the player's feet; and the log
    // says where the plan was wrong.
    protected EntityAI Rescue(OZT_Make make)
    {
        EntityAI made = m_Player.GetHumanInventory().CreateInInventory(make.ClassName);
        string how = "by the engine's own search";
        if (!made)
        {
            vector feet = m_Player.GetPosition();
            made = EntityAI.Cast(GetGame().CreateObjectEx(make.ClassName, feet, ECE_PLACE_ON_SURFACE));
            how = "at the player's feet";
        }
        string msg = "deal defect: the engine refused " + make.ClassName + " at " + make.Place.Text();
        msg = msg + "; it was made " + how;
        OZT_Log.Error(msg);
        return made;
    }

    // More units onto a stack the player keeps. The plan gives new units only
    // to a stack in the condition of new goods (OZT_InvModel.StacksWithRoom),
    // so the condition never changes here; inside that condition the stack
    // takes the weighted health of what it held and the new units it gets, as
    // the game's own combine does for a stack and for a pile of rounds.
    protected void Merge(EntityAI stack, int units)
    {
        if (!stack || units <= 0)
            return;
        float held = OZT_Stack.Units(stack);
        float health = stack.GetHealth01("", "");
        float mixed = (health * held + units) / (held + units);
        OZT_Stack.Add(stack, units);
        stack.SetHealth01("", "", mixed);
    }
}
