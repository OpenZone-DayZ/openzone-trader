// The three files of the trader, live: loaded with the core's loader, then
// checked against the game's config and against each other.
//
// The cross-checks change the objects IN MEMORY only. The loader writes a
// Validate fix back to the admin's file, and "a mod was not loaded today" or
// "the notes changed" must not delete or rewrite a line of it.
//
// Every finding of a load is a WARNING line and counts in `problems=` of the
// ready line, whichever of the two it is: a Validate fix (a value out of range,
// which the loader then writes back) or a cross-check.
class OZT_Config
{
    private static ref OZT_Config s_Instance;

    private ref OZT_Settings    m_Settings;
    private ref OZT_GoodsFile   m_Goods;
    private ref OZT_TradersFile m_Traders;
    private int m_Revision;
    private int m_Problems;
    private int m_GoodsCount;

    static OZT_Config Get()
    {
        if (!s_Instance)
            s_Instance = new OZT_Config();
        return s_Instance;
    }

    static void ServerLoad()
    {
        Get().LoadAll();
    }

    // Every file again and everything that stands on them: what the stand's
    // reload, an admin's edit in the pane and the core's file editor all do.
    // The points are made again, and their respawn sends the clients the sync
    // packet again.
    static void ReloadLive()
    {
        ServerLoad();
        OZT_Book.Rebuild();
        OZT_Points.Respawn();
    }

    OZT_Settings Settings()
    {
        return m_Settings;
    }

    OZT_GoodsFile Goods()
    {
        return m_Goods;
    }

    OZT_TradersFile Traders()
    {
        return m_Traders;
    }

    // Grows with every load: an answer of `open` carries it, so that an
    // unchanged catalogue can later be skipped.
    int Revision()
    {
        return m_Revision;
    }

    // The smallest amount that can change hands: the smallest note's value with
    // item money, 1 with any other provider.
    int Unit()
    {
        if (m_Settings.Provider != OZT_Const.PROVIDER_ITEMS)
            return 1;
        if (m_Settings.Notes.Count() == 0)
            return 1;
        return m_Settings.Notes[0].Value;
    }

    OZT_Category FindCategory(string id)
    {
        for (int i = 0; i < m_Goods.Categories.Count(); i++)
        {
            if (m_Goods.Categories[i].Id == id)
                return m_Goods.Categories[i];
        }
        return null;
    }

    OZT_Point FindTrader(string id)
    {
        for (int i = 0; i < m_Traders.Traders.Count(); i++)
        {
            if (m_Traders.Traders[i].Id == id)
                return m_Traders.Traders[i];
        }
        return null;
    }

    string Counters()
    {
        string s = "traders=" + m_Traders.Traders.Count().ToString() + " goods=" + m_GoodsCount.ToString();
        s = s + " money=" + m_Settings.Provider + " gates=" + OZT_Gates.Families();
        s = s + " problems=" + m_Problems.ToString();
        return s;
    }

    static bool ClassKnown(string type)
    {
        if (type == "")
            return false;
        if (GetGame().ConfigIsExisting("CfgVehicles " + type))
            return true;
        if (GetGame().ConfigIsExisting("CfgWeapons " + type))
            return true;
        return GetGame().ConfigIsExisting("CfgMagazines " + type);
    }

    // A price becomes a multiple of `unit` always in the trader's favour (the
    // owner, 2026-11-11: no money dupes): what the player pays goes up to the
    // next multiple, what the trader pays goes down to the last one. Never to
    // the nearest, which would now and then hand out money.
    //
    // A product of floats carries noise in its last digits, so an amount that
    // is a multiple to within a thousandth of a unit (more for a large amount)
    // counts as that multiple: 120 x 1.0 must not become 125.
    static int RoundUpTo(float amount, int unit)
    {
        if (unit < 1)
            unit = 1;
        float parts = amount / unit;
        float slack = 0.001 + parts * 0.000001;
        float whole = Math.Floor(parts);
        int n = whole;
        if (parts - whole > slack)
            n = n + 1;
        return n * unit;
    }

    static int RoundDownTo(float amount, int unit)
    {
        if (unit < 1)
            unit = 1;
        float parts = amount / unit;
        float slack = 0.001 + parts * 0.000001;
        float whole = Math.Floor(parts);
        int n = whole;
        if (whole + 1 - parts < slack)
            n = n + 1;
        return n * unit;
    }

    private void LoadAll()
    {
        OZT_Log.ResetWarned();

        OZT_Settings s = new OZT_Settings();
        OZ_ConfigLoader<OZT_Settings>.Load(OZT_Const.SETTINGS, "trader settings", s);
        m_Settings = s;

        OZT_GoodsFile g = new OZT_GoodsFile();
        OZ_ConfigLoader<OZT_GoodsFile>.Load(OZT_Const.GOODS, "trader goods", g);
        m_Goods = g;

        OZT_TradersFile t = new OZT_TradersFile();
        OZ_ConfigLoader<OZT_TradersFile>.Load(OZT_Const.TRADERS, "trader points", t);
        m_Traders = t;

        CheckNotes();
        CheckGoods();
        CheckTraders();
        CheckDiscounts();
        CheckLoops();
        m_Problems = OZT_Log.Warned();
        m_Revision++;
    }

    private void Problem(string msg)
    {
        OZT_Log.Warn(msg);
    }

    // Notes: a real quantity item, worth something, listed once; ascending by
    // value; every value a multiple of the smallest, so that change made from
    // the largest note downward always comes out exact.
    private void CheckNotes()
    {
        array<ref OZT_Note> kept = new array<ref OZT_Note>();
        for (int i = 0; i < m_Settings.Notes.Count(); i++)
        {
            OZT_Note n = m_Settings.Notes[i];
            string why = "";
            if (n.Value <= 0)
                why = "its Value is not above zero";
            else if (!ClassKnown(n.ClassName))
                why = "no such class";
            else if (GetGame().ConfigIsExisting("CfgMagazines " + n.ClassName))
                why = "ammunition counts rounds, not quantity, and cannot be a note";
            else if (NoteIn(kept, n.ClassName))
                why = "it is listed twice";
            if (why != "")
            {
                Problem("Settings: the note '" + n.ClassName + "' is dropped: " + why);
                continue;
            }
            int at = 0;
            while (at < kept.Count() && kept[at].Value <= n.Value)
                at++;
            kept.InsertAt(n, at);
        }

        for (int k = kept.Count() - 1; k >= 1; k--)
        {
            if (kept[k].Value % kept[0].Value == 0)
                continue;
            string odd = "Settings: the note '" + kept[k].ClassName + "' is dropped: its value " + kept[k].Value.ToString();
            odd = odd + " is not a multiple of the smallest note's " + kept[0].Value.ToString();
            Problem(odd);
            kept.RemoveOrdered(k);
        }
        m_Settings.Notes = kept;

        if (m_Settings.Provider == OZT_Const.PROVIDER_ITEMS && kept.Count() == 0)
            OZT_Log.Info("Settings: item money with no notes configured: nothing can be paid or paid out until Notes names some");
    }

    private bool NoteIn(array<ref OZT_Note> notes, string type)
    {
        for (int i = 0; i < notes.Count(); i++)
        {
            if (notes[i].ClassName == type)
                return true;
        }
        return false;
    }

    // Goods: the class exists, is not one of the server's banknotes, stands in
    // the book once, and both prices are whole multiples of the smallest note.
    // What comes with goods is goods of the book too, with a price to buy,
    // and fits them.
    private void CheckGoods()
    {
        int unit = Unit();
        array<string> seen = new array<string>();
        array<OZT_Goods> lines = new array<OZT_Goods>();
        m_GoodsCount = 0;
        for (int c = 0; c < m_Goods.Categories.Count(); c++)
        {
            OZT_Category cat = m_Goods.Categories[c];
            array<ref OZT_Goods> kept = new array<ref OZT_Goods>();
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_Goods e = cat.Goods[g];
                if (!ClassKnown(e.ClassName))
                {
                    Problem("Goods: '" + e.ClassName + "' in category '" + cat.Id + "' is skipped: no such class");
                    continue;
                }
                if (NoteIn(m_Settings.Notes, e.ClassName))
                {
                    // Money sold for money: bought below its value or sold
                    // above it, a note would be a money press.
                    Problem("Goods: '" + e.ClassName + "' in category '" + cat.Id + "' is skipped: it is one of this server's banknotes, and money is not goods");
                    continue;
                }
                if (seen.Find(e.ClassName) > -1)
                {
                    Problem("Goods: '" + e.ClassName + "' in category '" + cat.Id + "' is skipped: the class is in the price book already");
                    continue;
                }
                seen.Insert(e.ClassName);
                lines.Insert(e);
                e.Buy = Fitted(e.ClassName, "Buy", e.Buy, unit);
                e.Sell = Fitted(e.ClassName, "Sell", e.Sell, unit);
                kept.Insert(e);
            }
            cat.Goods = kept;
            m_GoodsCount = m_GoodsCount + kept.Count();
        }

        for (int w = 0; w < lines.Count(); w++)
            CheckWith(lines[w], seen, lines);
    }

    // What comes attached to goods: each a class of the book with a price to
    // buy (it is charged at that price: nothing comes free), in a slot of its
    // own. What fails is left out, here in memory: the file is the admin's.
    private void CheckWith(OZT_Goods host, array<string> seen, array<OZT_Goods> lines)
    {
        if (!host.With || host.With.Count() == 0)
            return;
        array<string> comes = new array<string>();
        array<string> taken = new array<string>();
        for (int p = 0; p < host.With.Count(); p++)
        {
            string part = host.With[p];
            string why = "";
            int at = seen.Find(part);
            if (!ClassKnown(part))
                why = "no such class";
            else if (at < 0 || lines[at].Buy <= 0)
                why = "it has no price to buy in the price book";
            else
            {
                string slot = OZT_Fit.SlotFor(host.ClassName, part, taken);
                if (slot == "")
                    why = "it does not fit, or its slot is taken by a part named before it";
                else
                    taken.Insert(slot);
            }
            if (why != "")
            {
                Problem("Goods: '" + host.ClassName + "' comes with '" + part + "', which is left out: " + why);
                continue;
            }
            comes.Insert(part);
        }
        host.With = comes;
    }

    private int Fitted(string type, string side, int price, int unit)
    {
        if (price <= 0 || unit <= 1)
            return price;
        if (price % unit == 0)
            return price;
        int rounded = RoundDownTo(price, unit);
        if (side == "Buy")
            rounded = RoundUpTo(price, unit);
        string msg = "Goods: " + side + " of '" + type + "' is " + price.ToString();
        msg = msg + ", not a multiple of the smallest note (" + unit.ToString() + "); using " + rounded.ToString();
        Problem(msg);
        return rounded;
    }

    // Points: this phase stands objects only; the class exists; the categories
    // a point names are in the book.
    private void CheckTraders()
    {
        array<ref OZT_Point> kept = new array<ref OZT_Point>();
        for (int i = 0; i < m_Traders.Traders.Count(); i++)
        {
            OZT_Point p = m_Traders.Traders[i];
            if (!ClassKnown(p.ClassName))
            {
                Problem("Traders: '" + p.Id + "' is skipped: no such class '" + p.ClassName + "'");
                continue;
            }
            // A trader's character is an ordinary survivor (design §9.2).
            if (p.Kind == OZT_Const.KIND_NPC && !GetGame().IsKindOf(p.ClassName, "SurvivorBase"))
            {
                Problem("Traders: '" + p.Id + "' is skipped: an npc's class must be a survivor, '" + p.ClassName + "' is not");
                continue;
            }
            if (p.Kind == OZT_Const.KIND_NPC)
                CheckOutfit(p.Id, p.Outfit.Items);
            array<string> cats = new array<string>();
            for (int c = 0; c < p.Categories.Count(); c++)
            {
                string id = p.Categories[c];
                if (FindCategory(id))
                    cats.Insert(id);
                else
                    Problem("Traders: '" + p.Id + "' names the category '" + id + "', which the price book does not have");
            }
            p.Categories = cats;
            kept.Insert(p);
        }
        m_Traders.Traders = kept;
    }

    // What a trader's character wears: a class that does not exist is left
    // out and said, the rest is worn (the core's own rule for presets).
    //
    // A list of lists, not recursion: a recursive call that passed an item's
    // own list (`it.Inside`) from inside a loop over the method's array
    // parameter came back with that parameter pointing at the item's list
    // (measured 2026-11-12, static and not: the rest of the outfit read as
    // missing; dayz-modding, enforce-script.md).
    private void CheckOutfit(string id, array<ref OZT_OutfitItem> top)
    {
        array<ref array<ref OZT_OutfitItem>> lists = new array<ref array<ref OZT_OutfitItem>>();
        if (top)
            lists.Insert(top);
        int at = 0;
        while (at < lists.Count())
        {
            array<ref OZT_OutfitItem> items = lists[at];
            at++;
            for (int i = items.Count() - 1; i >= 0; i--)
            {
                OZT_OutfitItem it = items[i];
                if (!it || !ClassKnown(it.ClassName))
                {
                    string name = "";
                    if (it)
                        name = it.ClassName;
                    Problem("Traders: '" + id + "' wears '" + name + "', which is no class; it is left out");
                    items.RemoveOrdered(i);
                    continue;
                }
                if (it.Inside)
                    lists.Insert(it.Inside);
            }
        }
    }

    // A thing that sells for more than it costs is a money press: bought at one
    // trader and sold at another, or at the same one, it makes money out of
    // nothing (the owner, 2026-11-11: no money dupes). The prices compared are
    // the ones a deal would use: the least any trader asks for one piece, rounded
    // up, against the most any trader pays for it in the best condition, rounded
    // down. Under the unit rule a sale is rounded once for the whole stack, so
    // there the pay for one unit is compared before rounding. A finding changes
    // nothing, the prices are the admin's: it is said and counted.
    private void CheckLoops()
    {
        int unit = Unit();
        float best = BestCondition();
        for (int c = 0; c < m_Goods.Categories.Count(); c++)
        {
            OZT_Category cat = m_Goods.Categories[c];
            string cheapAt = "";
            string dearAt = "";
            float cheapest = 0;
            float dearest = 0;
            for (int t = 0; t < m_Traders.Traders.Count(); t++)
            {
                OZT_Point p = m_Traders.Traders[t];
                if (p.Categories.Find(cat.Id) < 0)
                    continue;
                if (cheapAt == "" || p.BuyFactor < cheapest)
                {
                    cheapest = p.BuyFactor;
                    cheapAt = p.Id;
                }
                if (dearAt == "" || p.SellFactor > dearest)
                {
                    dearest = p.SellFactor;
                    dearAt = p.Id;
                }
            }
            if (cheapAt == "")
                continue;

            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_Goods e = cat.Goods[g];
                if (e.Buy <= 0 || e.Sell <= 0)
                    continue;
                // The largest discount the goods can get anywhere: a
                // discount that holds at the cheapest trader makes the cheapest
                // price (design §8.2, the money loop with discounts).
                float most = MostDiscount(cat, e);
                int costs = RoundUpTo(e.Buy * cheapest * (100 - most) / 100, unit);
                float pays = e.Sell * dearest * best;
                if (e.Quantity != OZT_Const.RULE_UNIT)
                    pays = RoundDownTo(pays, unit);
                float over = pays - costs;
                if (over <= 0.001 + pays * 0.000001)
                    continue;
                string msg = "Goods: '" + e.ClassName + "' sells for " + pays.ToString() + " at '" + dearAt + "'";
                msg = msg + " and costs " + costs.ToString() + " at '" + cheapAt + "': a money loop";
                msg = msg + " (Sell " + e.Sell.ToString() + ", Buy " + e.Buy.ToString() + " in the file; the factors are counted";
                msg = msg + ", and a discount of " + most.ToString() + " %)";
                Problem(msg);
            }
        }
    }

    // A discount gives 0 to 100 percent; one the provider computes needs a
    // cap. What is out of range is brought in and said; a computed discount
    // without a cap gives nothing (a missing Max reads zero) and is said too.
    private void CheckDiscounts()
    {
        for (int c = 0; c < m_Goods.Categories.Count(); c++)
        {
            OZT_Category cat = m_Goods.Categories[c];
            CheckDiscountList("category '" + cat.Id + "'", cat.Discounts);
            for (int g = 0; g < cat.Goods.Count(); g++)
                CheckDiscountList("'" + cat.Goods[g].ClassName + "'", cat.Goods[g].Discounts);
        }
    }

    private void CheckDiscountList(string where, array<ref OZT_Discount> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            OZT_Discount d = list[i];
            if (d.By)
            {
                if (d.Max <= 0)
                    Problem("Goods: a discount of " + where + " by '" + d.By.Kind + "' has no Max above zero and gives nothing");
                if (d.Max > 100)
                {
                    Problem("Goods: a discount of " + where + " has a Max above 100; 100 is used");
                    d.Max = 100;
                }
                continue;
            }
            if (d.Percent < 0 || d.Percent > 100)
            {
                Problem("Goods: a discount of " + where + " is " + d.Percent.ToString() + " %; it is brought into 0 to 100");
                d.Percent = Math.Clamp(d.Percent, 0, 100);
            }
        }
    }

    // The most a discount of the goods' category or line can give.
    private float MostDiscount(OZT_Category cat, OZT_Goods e)
    {
        float most = 0;
        array<ref OZT_Discount> all = new array<ref OZT_Discount>();
        OZT_Discount.CopyAll(cat.Discounts, all);
        OZT_Discount.CopyAll(e.Discounts, all);
        for (int i = 0; i < all.Count(); i++)
        {
            float d = all[i].Most();
            if (d > most)
                most = d;
        }
        if (most > 100)
            most = 100;
        return most;
    }

    // The largest of the condition factors: what the best-kept thing sells for.
    private float BestCondition()
    {
        float best = 0;
        for (int level = 0; level < OZT_Const.LEVEL_RUINED; level++)
        {
            float f = m_Settings.Condition.Of(level);
            if (f > best)
                best = f;
        }
        return best;
    }
}
