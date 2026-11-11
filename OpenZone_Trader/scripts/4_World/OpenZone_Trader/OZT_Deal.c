// A deal: one cart of sells and buys, settled in one step or not at all.
//
// The server is the judge. It resolves every sold entity itself, computes every
// price itself, and plans every place before anything changes (design §4.3):
//
//   1. the player and the trader      who, where, doing nothing else
//   2. the sells                      yours, taken here, plain
//   3. the buys and the net           compared with what the client showed
//   4. the money                      enough of it; which notes; what comes back
//   5. the places                     everything bought and every note given
//   6. the commit                     ONE script call
//
// The commit's order is phase 0's: what leaves is deleted with ObjectDelete,
// which frees its place at once; stacks shrink; new things are created at their
// planned places; stacks grow. No frame passes in between.
//
// Phase 1 sells and buys plain things. An item that has something hanging on
// it, lying in it or loaded into it is refused as "not empty"; phase 3 sells
// such an item as a tree.

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
}

class OZT_Deal
{
    // The most one line may ask for: pieces, and units under the unit rule.
    static const int MAX_PIECES = 50;
    static const int MAX_UNITS = 5000;

    // When each player last made a deal: uid -> the game's milliseconds.
    protected static ref map<string, int> s_Last = new map<string, int>();

    protected PlayerBase m_Player;
    protected OZT_Point m_Trader;
    protected OZT_MoneyProvider m_Money;
    protected ref OZT_InvModel m_Model;
    protected ref OZT_DealRes m_Res;

    protected ref array<EntityAI> m_Gone;
    protected ref array<ref OZT_Grow> m_Grows;
    protected ref array<ref OZT_Make> m_Makes;
    protected ref array<string> m_BuyClasses;
    protected ref array<int> m_BuyCounts;
    protected ref array<string> m_BuyRules;
    protected ref array<string> m_Homeless;
    protected ref array<int> m_HomelessUnits;

    // True while a plan is laid with the first thing that needs cells set aside
    // for the hands: the second laying of PlanPlaces.
    protected bool m_Aside;

    protected int m_Net;
    protected int m_Give;
    protected string m_Lines;

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
        m_Homeless = new array<string>();
        m_HomelessUnits = new array<int>();
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
        why = ReadSells(sells);
        if (why != "")
            return why;
        why = ReadBuys(buys);
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
            m_Res.Balance = m_Money.Balance(m_Player) - m_Net;
            return "";
        }
        return Commit();
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
        return "";
    }

    // ---- 2. the sells ----------------------------------------------------------

    protected string ReadSells(array<ref OZT_WireSell> sells)
    {
        int total = 0;
        for (int i = 0; i < sells.Count(); i++)
        {
            OZT_WireSell line = sells[i];
            EntityAI e = EntityAI.Cast(GetGame().GetObjectByNetworkId(line.Low, line.High));
            if (!e || e.GetHierarchyRootPlayer() != m_Player)
                return OZT_Const.NO_NOT_YOURS;
            if (m_Gone.Find(e) > -1 || Shrinking(e))
                return OZT_Const.NO_INTERNAL;

            int units = 1;
            bool whole = true;
            if (OZT_Stack.Countable(e))
            {
                units = OZT_Stack.Units(e);
                if (line.Count > 0 && line.Count < units)
                {
                    units = line.Count;
                    whole = false;
                }
            }

            // "Not taken" before "not empty": if the trader does not want the
            // thing, emptying it would not help. A ruined thing prices at zero.
            OZT_BookEntry entry = OZT_Book.EntryOf(e.GetType());
            int price = OZT_Book.SellPrice(m_Trader, entry, e, units);
            if (price <= 0)
                return OZT_Const.NO_NOT_TAKEN;

            int inside = InTheWay(e);
            if (inside > 0)
            {
                m_Res.Left = inside;
                return OZT_Const.NO_NOT_EMPTY;
            }

            total = total + price;
            if (whole)
                m_Gone.Insert(e);
            else
                AddGrow(e, -units);
            m_Lines = m_Lines + " -" + e.GetType() + "x" + units.ToString() + "=" + price.ToString();
        }
        m_Net = m_Net - total;
        return "";
    }

    protected bool Shrinking(EntityAI e)
    {
        for (int i = 0; i < m_Grows.Count(); i++)
        {
            if (m_Grows[i].Stack == e)
                return true;
        }
        return false;
    }

    // What hangs on a thing, lies in it or is loaded into it.
    protected int InTheWay(EntityAI e)
    {
        int n = e.GetInventory().AttachmentCount();
        CargoBase cargo = e.GetInventory().GetCargo();
        if (cargo)
            n = n + cargo.GetItemCount();

        Magazine mag = Magazine.Cast(e);
        if (mag && !mag.IsAmmoPile())
            n = n + mag.GetAmmoCount();

        Weapon wpn = Weapon.Cast(e);
        if (wpn)
        {
            for (int m = 0; m < wpn.GetMuzzleCount(); m++)
            {
                if (!wpn.IsChamberEmpty(m))
                    n++;
                n = n + wpn.GetInternalMagazineCartridgeCount(m);
            }
        }
        return n;
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
            int price = OZT_Book.BuyPrice(m_Trader, entry, line.Count);
            if (price <= 0)
                return OZT_Const.NO_CHANGED;

            int most = MAX_PIECES;
            if (entry.Rule == OZT_Const.RULE_UNIT)
                most = MAX_UNITS;
            if (line.Count > most)
                return OZT_Const.NO_INTERNAL;

            total = total + price;
            m_BuyClasses.Insert(entry.ClassName);
            m_BuyCounts.Insert(line.Count);
            m_BuyRules.Insert(entry.Rule);
            m_Lines = m_Lines + " +" + entry.ClassName + "x" + line.Count.ToString() + "=" + price.ToString();
        }
        m_Net = m_Net + total;
        return "";
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
        int change = OZT_ItemMoney.Pay(stacks, m_Net);
        if (change < 0)
            return OZT_Const.NO_MONEY;
        for (int i = 0; i < stacks.Count(); i++)
        {
            OZT_NoteStack s = stacks[i];
            if (s.Spend <= 0)
                continue;
            if (s.Spend >= s.Units)
                m_Gone.Insert(s.Item);
            else
                AddGrow(s.Item, -s.Spend);
        }
        m_Give = change;
        return "";
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
    protected string PlanPlaces()
    {
        int kept = m_Grows.Count();
        string why = LayThings(false);
        if (why != "")
            return why;
        int left = m_Homeless.Count();
        if (left == 0)
            return "";

        if (left == 1)
        {
            OZT_Place hands;
            if (m_Model.PlaceInHands(m_Homeless[0], hands))
            {
                AddMake(m_Homeless[0], hands, m_HomelessUnits[0]);
                m_Res.InHands = m_Homeless[0];
                m_Homeless.Clear();
                m_HomelessUnits.Clear();
                return "";
            }
        }
        else
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

    // Lays everything the deal brings into a fresh picture of the inventory: the
    // bought lines largest first, then the change or the payout. With `aside`
    // the first thing that needs cells goes to the hands instead. What finds no
    // place is left in m_Homeless. Returns "" or the key of an internal refusal.
    protected string LayThings(bool aside)
    {
        m_Model = new OZT_InvModel(m_Player, m_Gone);
        m_Aside = aside;

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
                PlacePieces(type, m_BuyCounts[b], wear);
        }

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
        m_Homeless.Clear();
        m_HomelessUnits.Clear();
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
                AddMake(type, place, n);
            else
            {
                m_Homeless.Insert(type);
                m_HomelessUnits.Insert(n);
            }
            left = left - n;
        }
    }

    // `count` pieces of a class: a free slot of the character first when bought
    // gear is worn (decision 4), then a cargo. A thing that goes onto a slot
    // needs no cells and is never the one set aside.
    protected void PlacePieces(string type, int count, bool wear)
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
                AddMake(type, place, 0);
                continue;
            }
            m_Homeless.Insert(type);
            m_HomelessUnits.Insert(0);
        }
    }

    protected void AddGrow(EntityAI stack, int units)
    {
        OZT_Grow g = new OZT_Grow();
        g.Stack = stack;
        g.Units = units;
        m_Grows.Insert(g);
    }

    protected void AddMake(string type, OZT_Place place, int units)
    {
        OZT_Make m = new OZT_Make();
        m.ClassName = type;
        m.Place = place;
        m.Units = units;
        m_Makes.Insert(m);
    }

    // ---- 6. the commit --------------------------------------------------------------------

    protected string Commit()
    {
        // Money that is not items leaves first: if it cannot, nothing has
        // changed yet.
        if (!m_Money.Physical() && m_Net > 0)
        {
            if (!m_Money.Take(m_Player, m_Net, "deal"))
                return OZT_Const.NO_MONEY;
        }

        // One script call from here on.
        for (int d = 0; d < m_Gone.Count(); d++)
        {
            if (m_Gone[d])
                GetGame().ObjectDelete(m_Gone[d]);
        }
        for (int s = 0; s < m_Grows.Count(); s++)
        {
            if (m_Grows[s].Units < 0)
                OZT_Stack.Add(m_Grows[s].Stack, m_Grows[s].Units);
        }
        for (int m = 0; m < m_Makes.Count(); m++)
            CreateOne(m_Makes[m]);
        for (int g = 0; g < m_Grows.Count(); g++)
        {
            if (m_Grows[g].Units > 0)
                Merge(m_Grows[g].Stack, m_Grows[g].Units);
        }

        if (!m_Money.Physical() && m_Net < 0)
            m_Money.Give(m_Player, -m_Net, "deal");

        string uid = Uid();
        int now = GetGame().GetTime();
        s_Last.Set(uid, now);
        m_Res.Balance = m_Money.Balance(m_Player);

        string line = "deal: " + uid + " at '" + m_Trader.Id + "' net " + m_Net.ToString();
        line = line + " (" + m_Money.Id() + "):" + m_Lines;
        OZT_Log.Info(line);
        return "";
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
    }

    // The game makes a magazine full, and its rounds are goods of their own:
    // a bought magazine holds none (decision 16 of the plan). Phase 3 sells a
    // magazine with rounds, each paid for.
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
