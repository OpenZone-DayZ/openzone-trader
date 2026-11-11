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
        s = s + " money=" + m_Settings.Provider + " gates=none";
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

    // To a multiple of `unit`, half up.
    static int RoundTo(float amount, int unit)
    {
        if (unit < 1)
            unit = 1;
        float parts = Math.Floor(amount / unit + 0.5);
        int n = parts;
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

    // Goods: the class exists, stands in the book once, and both prices are
    // whole multiples of the smallest note.
    private void CheckGoods()
    {
        int unit = Unit();
        array<string> seen = new array<string>();
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
                if (seen.Find(e.ClassName) > -1)
                {
                    Problem("Goods: '" + e.ClassName + "' in category '" + cat.Id + "' is skipped: the class is in the price book already");
                    continue;
                }
                seen.Insert(e.ClassName);
                e.Buy = Fitted(e.ClassName, "Buy", e.Buy, unit);
                e.Sell = Fitted(e.ClassName, "Sell", e.Sell, unit);
                kept.Insert(e);
            }
            cat.Goods = kept;
            m_GoodsCount = m_GoodsCount + kept.Count();
        }
    }

    private int Fitted(string type, string side, int price, int unit)
    {
        if (price <= 0 || unit <= 1)
            return price;
        if (price % unit == 0)
            return price;
        int rounded = RoundTo(price, unit);
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
            if (p.Kind == OZT_Const.KIND_NPC)
            {
                Problem("Traders: '" + p.Id + "' is an npc; the NPC is not built yet, the point is skipped");
                continue;
            }
            if (!ClassKnown(p.ClassName))
            {
                Problem("Traders: '" + p.Id + "' is skipped: no such class '" + p.ClassName + "'");
                continue;
            }
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
}
