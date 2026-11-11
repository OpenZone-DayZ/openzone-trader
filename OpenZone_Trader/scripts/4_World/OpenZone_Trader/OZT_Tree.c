// A sold thing as a tree (design §6.1, decisions 17-20 of the phase 3a plan).
//
// The root is the thing a sell line names. Its nodes are what hangs on it, at
// any depth, each in one of four states:
//
//   sold        the trader pays for it by its own line of the book; it is walked
//               in turn, so its parts and its rounds are nodes and lots too
//   kept        the request names it: it comes off and stays with the player
//   not taken   the trader pays nothing for it: it comes off and stays, and the
//               player cannot change that
//   rides       it hangs on a kept part and goes wherever that part goes
//
// What lies in a thing's cargo is not in its tree: it is a thing of its own.
// A sold node with a cargo can go only when everything inside goes with it in
// the same deal; `Inside` counts what would be left.
//
// Rounds are not entities. Every sold magazine and every sold weapon is read
// round by round and the rounds are gathered into lots by pile class and by
// condition. A lot is paid exactly as a pile of those rounds is: the pile's
// line, its rule, the factor of the rounds' own condition. So loading worn
// rounds into a magazine does not sell them as new (the owner, 2026-11-11: no
// money out of nothing). A lot the player keeps, or the trader does not take,
// comes back as piles in the condition the rounds were in.

class OZT_Node
{
    EntityAI Item;
    OZT_Node Up;
    int State;
    int Price;
}

class OZT_RoundLot
{
    // The magazine or the weapon the rounds are loaded in.
    EntityAI In;
    string Pile;
    int Level;
    int Count;

    // The damage of the lot's rounds, summed: a round's damage is 0 when new.
    float Damage;

    int Price;
    bool Kept;

    // The health a pile of these rounds has: the mean of what they had.
    float Health()
    {
        if (Count <= 0)
            return 1;
        float mean = Damage / Count;
        return Math.Clamp(1 - mean, 0, 1);
    }
}

class OZT_Tree
{
    static const int SOLD      = 0;
    static const int KEPT      = 1;
    static const int NOT_TAKEN = 2;
    static const int RIDES     = 3;

    // The root first, then every node under it, a part after what it hangs on.
    ref array<ref OZT_Node> Nodes;
    ref array<ref OZT_RoundLot> Lots;

    // Every sold node: what the deal deletes.
    ref array<EntityAI> Gone;

    // Every part that comes off: the topmost node of each kept branch.
    ref array<EntityAI> Moves;

    // What the trader pays for the tree.
    int Total;

    // What stands in the way: things lying in the cargo of sold nodes that the
    // deal does not sell whole, and loaded rounds the game has no pile for.
    int Inside;

    protected OZT_Point m_Trader;
    protected ref array<EntityAI> m_Keep;
    protected ref array<EntityAI> m_KeepRounds;
    protected ref array<EntityAI> m_Whole;
    protected ref map<string, string> m_Piles;

    void OZT_Tree()
    {
        Nodes = new array<ref OZT_Node>();
        Lots = new array<ref OZT_RoundLot>();
        Gone = new array<EntityAI>();
        Moves = new array<EntityAI>();
        m_Keep = new array<EntityAI>();
        m_KeepRounds = new array<EntityAI>();
        m_Piles = new map<string, string>();
    }

    // `keep` is what the request keeps (null: nothing); `whole` is every thing
    // the same deal sells whole (null: nothing else is sold).
    static OZT_Tree Read(OZT_Point trader, EntityAI root, array<ref OZT_WireKeep> keep, array<EntityAI> whole)
    {
        OZT_Tree t = new OZT_Tree();
        t.m_Trader = trader;
        t.m_Whole = whole;
        if (keep)
        {
            for (int k = 0; k < keep.Count(); k++)
            {
                OZT_WireKeep line = keep[k];
                if (!line)
                    continue;
                EntityAI named = EntityAI.Cast(GetGame().GetObjectByNetworkId(line.Low, line.High));
                if (!named)
                    continue;
                if (line.Rounds == 1)
                    t.m_KeepRounds.Insert(named);
                else
                    t.m_Keep.Insert(named);
            }
        }

        OZT_Node top = t.AddNode(root, null, NOT_TAKEN);
        int price = OZT_Book.SellPrice(trader, OZT_Book.EntryOf(root.GetType()), root, 1);
        if (price <= 0)
            return t;
        top.State = SOLD;
        top.Price = price;
        t.Total = price;
        t.Gone.Insert(root);
        t.Walk(top);
        return t;
    }

    // Does the trader take the root at all?
    bool Taken()
    {
        return Nodes.Count() > 0 && Nodes[0].State == SOLD;
    }

    protected OZT_Node AddNode(EntityAI item, OZT_Node up, int state)
    {
        OZT_Node n = new OZT_Node();
        n.Item = item;
        n.Up = up;
        n.State = state;
        Nodes.Insert(n);
        return n;
    }

    // A sold node: its rounds, what lies in it, what hangs on it.
    protected void Walk(OZT_Node node)
    {
        EntityAI e = node.Item;
        ReadRounds(e);

        CargoBase cargo = e.GetInventory().GetCargo();
        if (cargo)
        {
            int lying = cargo.GetItemCount();
            for (int c = 0; c < lying; c++)
            {
                EntityAI inside = cargo.GetItem(c);
                if (!inside)
                    continue;
                if (!m_Whole || m_Whole.Find(inside) < 0)
                    Inside++;
            }
        }

        int n = e.GetInventory().AttachmentCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = e.GetInventory().GetAttachmentFromIndex(i);
            if (!part)
                continue;
            if (m_Keep.Find(part) > -1)
            {
                OZT_Node kept = AddNode(part, node, KEPT);
                Moves.Insert(part);
                Riders(kept);
                continue;
            }
            int price = OZT_Book.SellPrice(m_Trader, OZT_Book.EntryOf(part.GetType()), part, 1);
            if (price <= 0)
            {
                OZT_Node stays = AddNode(part, node, NOT_TAKEN);
                Moves.Insert(part);
                Riders(stays);
                continue;
            }
            OZT_Node sold = AddNode(part, node, SOLD);
            sold.Price = price;
            Total = Total + price;
            Gone.Insert(part);
            Walk(sold);
        }
    }

    // Everything on a part that stays: listed, never priced, never touched.
    protected void Riders(OZT_Node node)
    {
        int n = node.Item.GetInventory().AttachmentCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = node.Item.GetInventory().GetAttachmentFromIndex(i);
            if (!part)
                continue;
            OZT_Node rider = AddNode(part, node, RIDES);
            Riders(rider);
        }
    }

    // ---- rounds ------------------------------------------------------------------

    // The rounds of one sold node: a magazine's cartridges; a weapon's chambers
    // and internal magazines. A fired case in a chamber is not a round.
    protected void ReadRounds(EntityAI e)
    {
        int first = Lots.Count();
        float dmg;
        string type;

        Magazine mag = Magazine.Cast(e);
        if (mag && !mag.IsAmmoPile())
        {
            int held = mag.GetAmmoCount();
            for (int i = 0; i < held; i++)
            {
                if (mag.GetCartridgeAtIndex(i, dmg, type))
                    AddRound(e, type, dmg);
            }
        }

        Weapon wpn = Weapon.Cast(e);
        if (wpn)
        {
            int muzzles = wpn.GetMuzzleCount();
            for (int m = 0; m < muzzles; m++)
            {
                bool live = !wpn.IsChamberEmpty(m) && !wpn.IsChamberFiredOut(m);
                if (live && wpn.GetCartridgeInfo(m, dmg, type))
                    AddRound(e, type, dmg);
                int stored = wpn.GetInternalMagazineCartridgeCount(m);
                for (int k = 0; k < stored; k++)
                {
                    if (wpn.GetInternalMagazineCartridgeInfo(m, k, dmg, type))
                        AddRound(e, type, dmg);
                }
            }
        }

        bool keep = m_KeepRounds.Find(e) > -1;
        for (int l = first; l < Lots.Count(); l++)
            PriceLot(Lots[l], keep);
    }

    protected void AddRound(EntityAI e, string type, float dmg)
    {
        // A round the game has no pile for could neither be paid nor given
        // back: it stands in the way, like a thing lying inside.
        string pile = PileOf(type);
        if (pile == "")
        {
            Inside++;
            return;
        }
        int level = LevelOf(dmg);
        OZT_RoundLot lot;
        for (int i = 0; i < Lots.Count(); i++)
        {
            OZT_RoundLot have = Lots[i];
            if (have.In == e && have.Pile == pile && have.Level == level)
                lot = have;
        }
        if (!lot)
        {
            lot = new OZT_RoundLot();
            lot.In = e;
            lot.Pile = pile;
            lot.Level = level;
            Lots.Insert(lot);
        }
        lot.Count = lot.Count + 1;
        lot.Damage = lot.Damage + dmg;
    }

    // The pile class loose rounds of a cartridge type lie in.
    protected string PileOf(string type)
    {
        string pile;
        if (m_Piles.Find(type, pile))
            return pile;
        GetGame().ConfigGetText("CfgAmmo " + type + " spawnPileType", pile);
        m_Piles.Set(type, pile);
        return pile;
    }

    // The condition of a round by its damage, on the game's own steps: a thing
    // is worn at 0.7 of its health, damaged at 0.5, badly damaged at 0.3.
    static int LevelOf(float dmg)
    {
        float health = 1 - dmg;
        if (health > GameConstants.DAMAGE_WORN_VALUE)
            return GameConstants.STATE_PRISTINE;
        if (health > GameConstants.DAMAGE_DAMAGED_VALUE)
            return GameConstants.STATE_WORN;
        if (health > GameConstants.DAMAGE_BADLY_DAMAGED_VALUE)
            return GameConstants.STATE_DAMAGED;
        if (health > GameConstants.DAMAGE_RUINED_VALUE)
            return GameConstants.STATE_BADLY_DAMAGED;
        return GameConstants.STATE_RUINED;
    }

    // How many rounds a full pile of the class holds.
    static int PileMax(string pile)
    {
        int most = GetGame().ConfigGetInt("CfgMagazines " + pile + " count");
        if (most < 1)
            most = 1;
        return most;
    }

    // What the trader pays for a lot: what he pays for a pile of those rounds
    // (OZT_Book.SellPrice for a countable thing, with the lot's count and
    // condition). A lot he pays nothing for stays with the player.
    protected void PriceLot(OZT_RoundLot lot, bool keep)
    {
        lot.Kept = true;
        lot.Price = 0;
        if (keep)
            return;
        OZT_BookEntry entry = OZT_Book.EntryOf(lot.Pile);
        if (!OZT_Book.Deals(m_Trader, entry) || entry.Sell <= 0)
            return;
        if (lot.Level >= OZT_Const.LEVEL_RUINED)
            return;

        float price = entry.Sell * m_Trader.SellFactor * OZT_Config.Get().Settings().Condition.Of(lot.Level);
        if (entry.Rule == OZT_Const.RULE_UNIT)
        {
            price = price * lot.Count;
        }
        else
        {
            float most = PileMax(lot.Pile);
            if (most < lot.Count)
                most = lot.Count;
            price = price * lot.Count / most;
        }
        int paid = OZT_Config.RoundDownTo(price, OZT_Config.Get().Unit());
        if (paid <= 0)
            return;
        lot.Kept = false;
        lot.Price = paid;
        Total = Total + paid;
    }

    // The rounds loaded in one thing: how many, and what they bring.
    void LoadedIn(EntityAI e, out int count, out int price)
    {
        count = 0;
        price = 0;
        for (int i = 0; i < Lots.Count(); i++)
        {
            OZT_RoundLot lot = Lots[i];
            if (lot.In != e)
                continue;
            count = count + lot.Count;
            price = price + lot.Price;
        }
    }

    // ---- for the stand -------------------------------------------------------------

    // One line: every node with its state, every lot, the sums.
    string Text()
    {
        string s = "";
        for (int i = 0; i < Nodes.Count(); i++)
        {
            OZT_Node n = Nodes[i];
            string word = n.Item.GetType();
            if (n.State == SOLD)
                word = word + "=" + n.Price.ToString();
            else if (n.State == KEPT)
                word = word + ":kept";
            else if (n.State == NOT_TAKEN)
                word = word + ":no";
            else
                word = word + ":rides";
            if (s != "")
                s = s + " ";
            s = s + word;
        }
        for (int l = 0; l < Lots.Count(); l++)
        {
            OZT_RoundLot lot = Lots[l];
            string r = " " + lot.Pile + "x" + lot.Count.ToString() + "L" + lot.Level.ToString();
            if (lot.Kept)
                r = r + ":kept";
            else
                r = r + "=" + lot.Price.ToString();
            s = s + r;
        }
        s = s + " | total=" + Total.ToString() + " moves=" + Moves.Count().ToString();
        s = s + " inside=" + Inside.ToString();
        return s;
    }
}
