// The TraderPlus import (design §11.3, the owner 2026-11-12): TraderPlus's own
// files, read where TraderPlus keeps them, become our three files, our stock and
// our licences. Read with record classes of our own: TraderPlus's code is not
// needed and need not be loaded. Each rule below is TraderPlus's as its own code
// reads its files.
//
// A dry run writes the three files into OpenZone\TraderPlusImport with the
// report; an apply writes them in place (the core keeps a copy of the old ones
// in its backup), puts the stock onto our shelves and the licences on the
// characters, and loads everything again.
//
// Everything nested in these records is allocated by the serializer with no
// field initialiser (OZ_ConfigBase.c): read once, right after the load, and
// every array checked for null.

// ---- TraderPlus's records, as its files hold them ----

class OZT_TPStates
{
    bool AcceptWorn;
    bool AcceptDamaged;
    bool AcceptBadlyDamaged;
    float CoefficientWorn;
    float CoefficientDamaged;
    float CoefficientBadlyDamaged;
}

class OZT_TPCurrency
{
    string ClassName;
    int Value;
}

class OZT_TPTrader
{
    int Id;
    string Name;
    string GivenName;
    string Role;
    ref array<float> Position;
    ref array<float> Orientation;
    ref array<string> Clothes;
}

class OZT_TPObject
{
    string ObjectName;
}

class OZT_TPGeneral
{
    ref array<string> Licences;
    string LicenceKeyWord;
    ref OZT_TPStates AcceptedStates;
    bool StoreOnlyToPristineState;
    ref array<ref OZT_TPCurrency> Currencies;
    ref array<ref OZT_TPTrader> Traders;
    ref array<ref OZT_TPObject> TraderObjects;
}

class OZT_TPCategory
{
    string CategoryName;
    ref array<string> Products;
}

class OZT_TPPrices
{
    bool EnableAutoDestockAtRestart;
    int EnableDefaultTraderStock;
    ref array<ref OZT_TPCategory> TraderCategories;
}

class OZT_TPId
{
    int Id;
    ref array<string> Categories;
    ref array<string> LicencesRequired;
    ref array<string> CurrenciesAccepted;
}

class OZT_TPIds
{
    ref array<ref OZT_TPId> IDs;
}

class OZT_TPAccount
{
    string SteamID64;
    ref array<string> Licences;
}

// ---- the import ----

class OZT_TPImport
{
    static const string DEFAULT_ROOT = "$profile:TraderPlus";
    static const string DRY_DIR = "$profile:OpenZone\\TraderPlusImport";

    protected string m_Root;
    protected string m_Report;
    protected ref OZT_TPGeneral m_General;
    protected ref OZT_TPPrices m_Prices;
    protected ref OZT_TPIds m_Ids;

    protected ref OZT_Settings m_Settings;
    protected ref OZT_GoodsFile m_Goods;
    protected ref OZT_TradersFile m_Traders;

    // TraderPlus's names to our ids, and the ids made so far.
    protected ref map<string, string> m_CategoryIds;
    protected ref map<string, string> m_LicenceIds;
    protected ref array<string> m_TakenCategories;
    protected ref array<string> m_TakenLicences;

    // The lines with a stock of ours, by TraderPlus's category and the
    // lower-case class (Key): TraderPlus compares its stock lines without case.
    protected ref map<string, string> m_Stocked;
    // How many units one trade of TraderPlus's moves of a pile we price by
    // the unit, by Key: its stock counts trades, ours units.
    protected ref map<string, int> m_UnitsPer;

    // The stock to put on our shelves: stock, class, level, count.
    protected ref array<string> m_StockIds;
    protected ref array<string> m_StockClasses;
    protected ref array<int> m_StockLevels;
    protected ref array<int> m_StockCounts;

    // For the report.
    protected int m_Twice;
    protected string m_TwiceNames;
    protected int m_Frozen;
    protected string m_FrozenNames;
    protected int m_Rounded;
    protected string m_RoundedNames;
    protected int m_Fitted;
    protected string m_FittedNames;
    protected int m_SellLost;

    // The report for the admin window (OZT_AdminImportItem): a thing done a row, the
    // files looked for, the stocks an apply rewrites and the licences it puts on.
    protected ref array<ref OZT_AdminImportItem> m_Items;
    protected ref array<ref OZT_AdminImportFile> m_Files;
    protected string m_Shelves;
    protected int m_Give;

    static const int NAMES_MAX = 8;
    static const string GENERAL = "TraderPlusGeneralConfig.json";
    static const string PRICES = "TraderPlusPriceConfig.json";
    static const string IDS = "TraderPlusIDsConfig.json";
    static const string DATABASE = "TraderPlusDatabase";
    static const string BANK = "TraderPlusBankDatabase";

    void OZT_TPImport()
    {
        m_Report = "";
        m_Items = new array<ref OZT_AdminImportItem>();
        m_Files = new array<ref OZT_AdminImportFile>();
        m_CategoryIds = new map<string, string>();
        m_LicenceIds = new map<string, string>();
        m_TakenCategories = new array<string>();
        m_TakenLicences = new array<string>();
        m_Stocked = new map<string, string>();
        m_UnitsPer = new map<string, int>();
        m_StockIds = new array<string>();
        m_StockClasses = new array<string>();
        m_StockLevels = new array<int>();
        m_StockCounts = new array<int>();
    }

    // `root`: the folder TraderPlus keeps its files in ("" for its own).
    // Returns the report; `ok` false when TraderPlus's files could not be read.
    // `rep`, when given, takes the report for the admin window too.
    static string Run(string root, bool apply, out bool ok, OZT_AdminImportReport rep = null)
    {
        OZT_TPImport imp = new OZT_TPImport();
        ok = imp.Go(root, apply);
        if (rep)
        {
            int i;
            for (i = 0; i < imp.m_Items.Count(); i++)
                rep.Items.Insert(imp.m_Items[i]);
            for (i = 0; i < imp.m_Files.Count(); i++)
                rep.Files.Insert(imp.m_Files[i]);
            rep.Shelves = imp.m_Shelves;
            rep.Give = imp.m_Give;
        }
        return imp.m_Report;
    }

    protected void Say(string line)
    {
        m_Report = m_Report + line + "\n";
    }

    // One row of the window's report; the caller fills in the rest.
    protected OZT_AdminImportItem Item(string kind, string key, int n)
    {
        OZT_AdminImportItem it = new OZT_AdminImportItem();
        it.Kind = kind;
        it.Key = key;
        it.N = n;
        m_Items.Insert(it);
        return it;
    }

    protected void Seen(string file, bool found, int n)
    {
        OZT_AdminImportFile f = new OZT_AdminImportFile();
        f.File = file;
        if (found)
            f.Found = 1;
        f.N = n;
        m_Files.Insert(f);
    }

    // `names` and one more, ", " between them: the first NAMES_MAX of them, `count`
    // the name's number among them; a name already there is not repeated (a class in
    // two categories is two lines).
    protected static string Plus(string names, string name, int count)
    {
        if (count > NAMES_MAX)
            return names;
        if (names == "")
            return name;
        string all = ", " + names + ", ";
        if (all.Contains(", " + name + ", "))
            return names;
        return names + ", " + name;
    }

    protected void Unread(string file, string err)
    {
        Say("TraderPlus's " + file + " could not be read: " + err);
        OZT_AdminImportItem it = Item("bad", "unread", 0);
        it.Names = file;
        it.More = OZT_AdminText.Cut(err);
        Seen(file, false, 0);
    }

    protected bool Go(string root, bool apply)
    {
        m_Root = root;
        if (m_Root == "")
            m_Root = DEFAULT_ROOT;
        if (!Read())
        {
            WriteReport();
            return false;
        }

        // The server's settings stay, but for what TraderPlus has a word on;
        // the goods and the traders are TraderPlus's alone.
        m_Settings = new OZT_Settings();
        OZ_ConfigLoader<OZT_Settings>.Load(OZT_Const.SETTINGS, "trader settings", m_Settings, true, false);
        m_Goods = new OZT_GoodsFile();
        m_Goods.LoadDefaults();
        m_Traders = new OZT_TradersFile();
        m_Traders.LoadDefaults();

        TakeLicences();
        TakeSettings();
        TakeGoods();
        TakeTraders();
        TakeStock();

        if (!apply)
        {
            if (!FileExist(DRY_DIR))
                MakeDirectory(DRY_DIR);
            OZ_ConfigLoader<OZT_Settings>.Save(DRY_DIR + "\\OZ_Trader_Settings.json", "trader import", m_Settings, false);
            OZ_ConfigLoader<OZT_GoodsFile>.Save(DRY_DIR + "\\OZ_Trader_Goods.json", "trader import", m_Goods, false);
            OZ_ConfigLoader<OZT_TradersFile>.Save(DRY_DIR + "\\OZ_Trader_Traders.json", "trader import", m_Traders, false);
            m_Give = GiveLicences(false);
            Say("dry run: the three files are in OpenZone\\TraderPlusImport; nothing is applied");
            Say("an apply would put licences on characters: " + m_Give.ToString());
            Item("ok", "dry", 0);
            WriteReport();
            return true;
        }

        OZ_ConfigLoader<OZT_Settings>.Save(OZT_Const.SETTINGS, "trader settings", m_Settings, true);
        OZ_ConfigLoader<OZT_GoodsFile>.Save(OZT_Const.GOODS, "trader goods", m_Goods, true);
        OZ_ConfigLoader<OZT_TradersFile>.Save(OZT_Const.TRADERS, "trader points", m_Traders, true);
        OZT_Config.ReloadLive();
        PutStock();
        m_Give = GiveLicences(true);
        string counters = OZT_Config.Get().Counters();
        Say("applied: our three files are written, the old ones are in the core's backup");
        Say("licences put on characters: " + m_Give.ToString());
        Say("loaded again: " + counters);
        Say("TraderPlus still makes its own traders: leave it the vehicles only");
        Item("ok", "applied", 0);
        Item("ok", "given", m_Give);
        OZT_AdminImportItem loaded = Item("ok", "loaded", 0);
        loaded.More = OZT_AdminText.Cut(counters);
        Item("skip", "tp_left", 0);
        WriteReport();
        return true;
    }

    protected void WriteReport()
    {
        if (!FileExist(DRY_DIR))
            MakeDirectory(DRY_DIR);
        FileHandle fh = OpenFile(DRY_DIR + "\\report.txt", FileMode.WRITE);
        if (fh == 0)
            return;
        FPrint(fh, m_Report);
        CloseFile(fh);
    }

    // ---- reading ----

    protected bool Read()
    {
        string cfg = m_Root + "\\TraderPlusConfig\\";
        string err;
        m_General = new OZT_TPGeneral();
        if (!JsonFileLoader<OZT_TPGeneral>.LoadFile(cfg + GENERAL, m_General, err))
        {
            Unread(GENERAL, err);
            return false;
        }
        if (!m_General.Licences)
            m_General.Licences = new array<string>();
        if (!m_General.Currencies)
            m_General.Currencies = new array<ref OZT_TPCurrency>();
        if (!m_General.Traders)
            m_General.Traders = new array<ref OZT_TPTrader>();
        Seen(GENERAL, true, m_General.Traders.Count());

        m_Prices = new OZT_TPPrices();
        if (!JsonFileLoader<OZT_TPPrices>.LoadFile(cfg + PRICES, m_Prices, err))
        {
            Unread(PRICES, err);
            return false;
        }
        if (!m_Prices.TraderCategories)
            m_Prices.TraderCategories = new array<ref OZT_TPCategory>();
        Seen(PRICES, true, m_Prices.TraderCategories.Count());

        m_Ids = new OZT_TPIds();
        if (!JsonFileLoader<OZT_TPIds>.LoadFile(cfg + IDS, m_Ids, err))
        {
            Unread(IDS, err);
            return false;
        }
        if (!m_Ids.IDs)
            m_Ids.IDs = new array<ref OZT_TPId>();
        Seen(IDS, true, m_Ids.IDs.Count());

        // The stock and the accounts are TraderPlus's to have or not: none is no error.
        int stocked = 0;
        for (int i = 0; i < m_Ids.IDs.Count(); i++)
        {
            if (m_Ids.IDs[i] && FileExist(m_Root + "\\" + DATABASE + "\\ID_" + m_Ids.IDs[i].Id.ToString()))
                stocked++;
        }
        Seen(DATABASE, stocked > 0, stocked);
        array<string> accounts = new array<string>();
        Accounts(accounts);
        Seen(BANK, accounts.Count() > 0, accounts.Count());
        return true;
    }

    // ---- names ----

    // An id of ours for a name of TraderPlus's: letters and digits kept,
    // lower case, the rest one underscore; unique in `taken`.
    protected string Slug(string name, string fallback, array<string> taken)
    {
        string low = name;
        low.ToLower();
        string id = "";
        bool gap = false;
        for (int i = 0; i < low.Length(); i++)
        {
            string ch = low.Get(i);
            int code = ch.ToAscii();
            bool letter = code >= 97 && code <= 122;
            bool digit = code >= 48 && code <= 57;
            if (letter || digit)
            {
                if (gap && id != "")
                    id = id + "_";
                id = id + ch;
                gap = false;
            }
            else
            {
                gap = true;
            }
        }
        if (id.Length() > 56)
            id = id.Substring(0, 56);
        if (id == "")
            id = fallback;
        string unique = id;
        int n = 2;
        while (taken.Find(unique) > -1)
        {
            unique = id + "_" + n.ToString();
            n++;
        }
        taken.Insert(unique);
        return unique;
    }

    // Our id of a TraderPlus licence; the licence is made when the settings
    // have none of that name yet.
    protected string LicenceFor(string name)
    {
        string lid;
        if (m_LicenceIds.Find(name, lid))
            return lid;
        OZT_LicenceDef d = new OZT_LicenceDef();
        d.Id = Slug(name, "licence_" + m_Settings.Licences.Count().ToString(), m_TakenLicences);
        d.Name = name;
        d.Look = "Paper";
        m_LicenceIds.Set(name, d.Id);
        m_Settings.Licences.Insert(d);
        return d.Id;
    }

    protected OZT_Condition NeedLicence(string name)
    {
        OZT_Condition need = new OZT_Condition();
        need.Kind = "licence";
        need.Arg = LicenceFor(name);
        return need;
    }

    protected bool IsVehicle(string type)
    {
        if (GetGame().IsKindOf(type, "Transport"))
            return true;
        return GetGame().IsKindOf(type, "CarScript");
    }

    // ---- what maps ----

    protected void TakeLicences()
    {
        m_Settings.Licences = new array<ref OZT_LicenceDef>();
        for (int i = 0; i < m_General.Licences.Count(); i++)
        {
            if (m_General.Licences[i] != "")
                LicenceFor(m_General.Licences[i]);
        }
        int n = m_Settings.Licences.Count();
        Say("licences: " + n.ToString());
        OZT_AdminImportItem it = Item("ok", "licences", n);
        for (int l = 0; l < n; l++)
            it.Names = Plus(it.Names, m_Settings.Licences[l].Name, l + 1);
    }

    protected void TakeSettings()
    {
        // The notes: every class of every value, ascending by value.
        array<ref OZT_Note> notes = new array<ref OZT_Note>();
        int unknown = 0;
        string unknownNames = "";
        for (int c = 0; c < m_General.Currencies.Count(); c++)
        {
            OZT_TPCurrency cur = m_General.Currencies[c];
            if (!cur || cur.Value <= 0)
                continue;
            array<string> names = new array<string>();
            cur.ClassName.Split(",", names);
            for (int n = 0; n < names.Count(); n++)
            {
                string cls = names[n].Trim();
                if (cls == "")
                    continue;
                if (!OZT_Config.ClassKnown(cls))
                {
                    unknown++;
                    unknownNames = Plus(unknownNames, cls, unknown);
                    continue;
                }
                OZT_Note note = new OZT_Note();
                note.ClassName = cls;
                note.Value = cur.Value;
                InsertNote(notes, note);
            }
        }
        if (notes.Count() > 0)
            m_Settings.Notes = notes;
        Say("notes: " + notes.Count().ToString() + " class(es); unknown to the game, skipped: " + unknown.ToString());
        OZT_AdminImportItem it = Item("ok", "notes", notes.Count());
        it.M = unknown;
        it.Names = unknownNames;
        if (OZT_Config.ClassKnown("TraderPlus_Wallet") && m_Settings.Holders.Find("TraderPlus_Wallet") < 0)
            m_Settings.Holders.Insert("TraderPlus_Wallet");

        // What a thing that is not pristine brings: TraderPlus's share, or
        // nothing when it does not take that condition.
        OZT_TPStates st = m_General.AcceptedStates;
        if (st)
        {
            OZT_ConditionFactors f = new OZT_ConditionFactors();
            f.Pristine = 1;
            f.Worn = 0;
            f.Damaged = 0;
            f.BadlyDamaged = 0;
            if (st.AcceptWorn)
                f.Worn = st.CoefficientWorn;
            if (st.AcceptDamaged)
                f.Damaged = st.CoefficientDamaged;
            if (st.AcceptBadlyDamaged)
                f.BadlyDamaged = st.CoefficientBadlyDamaged;
            m_Settings.Condition = f;
        }

        m_Settings.StockOnlyPristine = 0;
        if (m_General.StoreOnlyToPristineState)
            m_Settings.StockOnlyPristine = 1;
        int start = m_Prices.EnableDefaultTraderStock;
        if (start < 0 || start > 2)
            start = 0;
        m_Settings.StartStock = start;
        m_Settings.DestockAtStart = 0;
        if (m_Prices.EnableAutoDestockAtRestart)
            m_Settings.DestockAtStart = 1;
    }

    protected void InsertNote(array<ref OZT_Note> notes, OZT_Note note)
    {
        for (int i = 0; i < notes.Count(); i++)
        {
            if (notes[i].ClassName == note.ClassName)
                return;
            if (notes[i].Value > note.Value)
            {
                notes.InsertAt(note, i);
                return;
            }
        }
        notes.Insert(note);
    }

    protected void TakeGoods()
    {
        int lines = 0;
        int stocked = 0;
        int vehicles = 0;
        int unknown = 0;
        int gated = 0;
        string vehicleNames = "";
        string unknownNames = "";
        string gatedNames = "";
        string keyword = m_General.LicenceKeyWord;
        for (int c = 0; c < m_Prices.TraderCategories.Count(); c++)
        {
            OZT_TPCategory tc = m_Prices.TraderCategories[c];
            if (!tc || tc.CategoryName == "")
                continue;
            OZT_Category cat = new OZT_Category();
            cat.Id = Slug(tc.CategoryName, "category_" + c.ToString(), m_TakenCategories);
            cat.Name = tc.CategoryName;
            cat.Goods = new array<ref OZT_Goods>();
            cat.Requires = new array<ref OZT_Condition>();
            cat.Discounts = new array<ref OZT_Discount>();
            m_CategoryIds.Set(tc.CategoryName, cat.Id);

            // TraderPlus shows a category whose name holds the licence key
            // word only to the holders of the licence of that very name.
            if (keyword != "" && tc.CategoryName.IndexOf(keyword) > -1)
            {
                cat.Requires.Insert(NeedLicence(tc.CategoryName));
                gated++;
                gatedNames = Plus(gatedNames, tc.CategoryName, gated);
            }

            if (!tc.Products)
                tc.Products = new array<string>();
            // A class in another category keeps its line there too (as in
            // TraderPlus, a line belongs to its category); listed twice in
            // one category, the first line is the one TraderPlus finds.
            array<string> seen = new array<string>();
            for (int p = 0; p < tc.Products.Count(); p++)
            {
                array<string> f = new array<string>();
                tc.Products[p].Split(",", f);
                if (f.Count() < 6)
                    continue;
                string cls = f[0].Trim();
                if (cls == "")
                    continue;
                string low = cls;
                low.ToLower();
                if (seen.Find(low) > -1)
                {
                    m_Twice++;
                    m_TwiceNames = Plus(m_TwiceNames, cls, m_Twice);
                    continue;
                }
                seen.Insert(low);
                OZT_Goods g = Line(cls, f, keyword, tc.CategoryName);
                if (!g)
                {
                    if (IsVehicle(cls))
                    {
                        vehicles++;
                        vehicleNames = Plus(vehicleNames, cls, vehicles);
                    }
                    else
                    {
                        unknown++;
                        unknownNames = Plus(unknownNames, cls, unknown);
                    }
                    continue;
                }
                cat.Goods.Insert(g);
                lines++;
                if (g.Stock)
                    stocked++;
            }
            m_Goods.Categories.Insert(cat);
        }
        int cats = m_Goods.Categories.Count();
        Say("categories: " + cats.ToString() + ", behind a licence by their name: " + gated.ToString());
        Say("goods lines: " + lines.ToString());
        OZT_AdminImportItem it = Item("ok", "categories", cats);
        it.M = gated;
        it.Names = gatedNames;
        it = Item("ok", "goods", lines);
        it.M = stocked;
        it.K = lines - stocked;
        if (m_Rounded > 0)
        {
            Say("piles priced by the unit, its price rounded in the trader's favour: " + m_Rounded.ToString() + " (" + m_RoundedNames + ")");
            it = Item("warn", "rounded", m_Rounded);
            it.Names = m_RoundedNames;
        }
        if (m_Fitted > 0)
        {
            int unit = NoteUnit();
            string fit = "prices made multiples of the smallest note (" + unit.ToString() + "), buy up and sell down: ";
            fit = fit + m_Fitted.ToString() + " (" + m_FittedNames + ")";
            Say(fit);
            if (m_SellLost > 0)
                Say("of them the trader now pays nothing for: " + m_SellLost.ToString());
            it = Item("warn", "fitted", m_Fitted);
            it.M = unit;
            it.K = m_SellLost;
            it.Names = m_FittedNames;
        }
        if (m_Frozen > 0)
        {
            Say("lines with MaxStock 0 (TraderPlus takes none of them from players), imported with no sell price: " + m_Frozen.ToString() + " (" + m_FrozenNames + ")");
            it = Item("warn", "frozen", m_Frozen);
            it.Names = m_FrozenNames;
        }
        if (m_Twice > 0)
        {
            Say("a class listed twice in one category, the first line kept: " + m_Twice.ToString() + " (" + m_TwiceNames + ")");
            it = Item("warn", "twice", m_Twice);
            it.Names = m_TwiceNames;
        }
        if (vehicles > 0)
        {
            Say("vehicles, left to TraderPlus: " + vehicles.ToString() + " (" + vehicleNames + ")");
            it = Item("skip", "vehicles", vehicles);
            it.Names = vehicleNames;
        }
        if (unknown > 0)
        {
            Say("classes the game does not know, skipped: " + unknown.ToString() + " (" + unknownNames + ")");
            it = Item("skip", "unknown", unknown);
            it.Names = unknownNames;
        }
    }

    // One product line of TraderPlus's to ours; null: a vehicle or a class
    // the game does not know. f: class, coefficient, max stock, trade
    // quantity, buy, sell[, destock].
    protected OZT_Goods Line(string cls, array<string> f, string keyword, string category)
    {
        float coef = f[1].ToFloat();
        int most = f[2].ToInt();
        float qty = f[3].ToFloat();
        int buy = f[4].ToInt();
        float sellRaw = f[5].ToFloat();
        float destock = 0;
        if (f.Count() > 6)
            destock = f[6].ToFloat();

        OZT_Goods g = new OZT_Goods();
        g.With = new array<string>();
        g.Requires = new array<ref OZT_Condition>();
        g.Discounts = new array<ref OZT_Discount>();
        g.Quantity = OZT_Const.RULE_EACH;
        g.Buy = buy;
        if (g.Buy < 0)
            g.Buy = 0;

        // A licence: TraderPlus sells one as a product of the licence's own
        // name, which holds the key word.
        if (keyword != "" && cls.IndexOf(keyword) > -1 && m_General.Licences.Find(cls) > -1)
        {
            g.ClassName = OZT_Const.LICENCE_PREFIX + LicenceFor(cls);
            g.Sell = 0;
            Fit(g);
            return g;
        }
        if (IsVehicle(cls) || !OZT_Config.ClassKnown(cls))
            return null;
        g.ClassName = cls;

        // Sell: -1 off, below 1 a share of the buy price, else the price.
        float sell = sellRaw;
        if (sell > 0 && sell < 1)
            sell = buy * sellRaw;
        if (sell < 0)
            sell = 0;
        g.Sell = sell;
        // TraderPlus takes nothing past a line's MaxStock: of a line of 0
        // it takes none at all.
        if (most == 0 && g.Sell > 0)
        {
            g.Sell = 0;
            m_Frozen++;
            m_FrozenNames = Plus(m_FrozenNames, cls, m_Frozen);
        }

        // One trade of a pile moves so many units at once; ours prices one.
        // A pile is priced by the unit even at one unit a trade: its line
        // there is the price of one round, not of a pile.
        int per = Units(cls, qty);
        if (per >= 1)
        {
            int unitBuy = Math.Ceil(1.0 * g.Buy / per);
            int unitSell = Math.Floor(1.0 * g.Sell / per);
            if (unitBuy * per != g.Buy || unitSell * per != g.Sell)
            {
                m_Rounded++;
                m_RoundedNames = Plus(m_RoundedNames, cls, m_Rounded);
            }
            g.Quantity = OZT_Const.RULE_UNIT;
            g.Buy = unitBuy;
            g.Sell = unitSell;
            m_UnitsPer.Set(Key(category, cls), per);
        }

        // MaxStock -1 is unlimited: no stock block, as before.
        if (most >= 0)
        {
            g.Stock = new OZT_GoodsStock();
            int each = per;
            if (each < 1)
                each = 1;
            g.Stock.Max = most * each;
            g.Stock.Coefficient = coef;
            if (g.Stock.Coefficient <= 0)
                g.Stock.Coefficient = 1;
            // Its curve steps once a trade, ours once a unit: the same curve.
            if (per > 1)
                g.Stock.Coefficient = Math.Pow(g.Stock.Coefficient, 1.0 / per);
            g.Stock.Destock = destock;
            m_Stocked.Set(Key(category, cls), cls);
        }
        Fit(g);
        return g;
    }

    // The smallest amount that changes hands under the new settings, as
    // OZT_Config.Unit() reads it from the loaded ones.
    protected int NoteUnit()
    {
        if (m_Settings.Provider != OZT_Const.PROVIDER_ITEMS)
            return 1;
        int least = 0;
        for (int i = 0; i < m_Settings.Notes.Count(); i++)
        {
            int v = m_Settings.Notes[i].Value;
            if (v > 0 && (least == 0 || v < least))
                least = v;
        }
        if (least < 1)
            return 1;
        return least;
    }

    // Every price a multiple of the smallest note, buy up and sell down, as
    // the load would make it (OZT_Config.Fitted): done here, so the file says
    // what the server will charge and the report names what changed.
    protected void Fit(OZT_Goods g)
    {
        int unit = NoteUnit();
        if (unit <= 1)
            return;
        int buy = g.Buy;
        int sell = g.Sell;
        if (buy > 0)
            g.Buy = OZT_Config.RoundUpTo(buy, unit);
        if (sell > 0)
            g.Sell = OZT_Config.RoundDownTo(sell, unit);
        if (g.Buy == buy && g.Sell == sell)
            return;
        m_Fitted++;
        m_FittedNames = Plus(m_FittedNames, g.ClassName, m_Fitted);
        if (sell > 0 && g.Sell == 0)
            m_SellLost++;
    }

    // How many units one trade of TraderPlus's moves of a pile or of a stack
    // that splits, as its TradeQuantity reads (TraderPlus's CalculateTradeQty):
    // -1 and 0 a full stack, a share below 1 that share of one, more than a
    // stack a stack, else that many units. 0: no pile, a thing sold whole.
    protected int Units(string cls, float qty)
    {
        int most = 0;
        if (GetGame().IsKindOf(cls, "Ammunition_Base"))
        {
            most = GetGame().ConfigGetInt("CfgMagazines " + cls + " count");
        }
        else if (GetGame().ConfigGetInt("CfgVehicles " + cls + " canBeSplit") == 1)
        {
            float top = GetGame().ConfigGetFloat("CfgVehicles " + cls + " varQuantityMax");
            most = top;
        }
        if (most <= 1)
            return 0;
        if (qty <= 0 || qty >= most)
            return most;
        if (qty < 1)
        {
            int part = Math.Round(qty * most);
            if (part < 1)
                part = 1;
            return part;
        }
        int n = qty;
        return n;
    }

    protected void TakeTraders()
    {
        int npcs = 0;
        int objects = 0;
        int ownMoney = 0;
        string npcNames = "";
        string ownNames = "";
        for (int t = 0; t < m_General.Traders.Count(); t++)
        {
            OZT_TPTrader tp = m_General.Traders[t];
            if (!tp || !tp.Position || tp.Position.Count() < 3)
                continue;
            OZT_Point p = new OZT_Point();
            p.Id = "tp-" + t.ToString();
            p.Name = tp.GivenName;
            if (p.Name == "")
                p.Name = tp.Role;
            if (p.Name == "")
                p.Name = p.Id;
            p.ClassName = tp.Name;
            p.Kind = OZT_Const.KIND_OBJECT;
            if (GetGame().IsKindOf(tp.Name, "SurvivorBase"))
                p.Kind = OZT_Const.KIND_NPC;
            p.Pos = new array<float>();
            p.Pos.Insert(tp.Position[0]);
            p.Pos.Insert(tp.Position[1]);
            p.Pos.Insert(tp.Position[2]);
            if (tp.Orientation && tp.Orientation.Count() > 0)
                p.Yaw = tp.Orientation[0];
            p.Radius = 3;
            p.BuyFactor = 1;
            p.SellFactor = 1;
            // Points of one TraderPlus ID sell from one shelf, as there.
            p.Stock = "tp" + tp.Id.ToString();
            p.Categories = new array<string>();
            p.Access = new array<ref OZT_Condition>();
            p.GatesOn = new array<string>();
            p.GatesOff = new array<string>();
            p.Outfit = new OZT_Outfit();
            p.Outfit.Items = new array<ref OZT_OutfitItem>();
            OZT_TPId tid = IdOf(tp.Id);
            if (tid && Fill(p, tid))
            {
                ownMoney++;
                ownNames = Plus(ownNames, p.Name, ownMoney);
            }
            if (tp.Clothes && p.Kind == OZT_Const.KIND_NPC)
            {
                for (int k = 0; k < tp.Clothes.Count(); k++)
                {
                    if (tp.Clothes[k] == "")
                        continue;
                    OZT_OutfitItem item = new OZT_OutfitItem();
                    item.ClassName = tp.Clothes[k];
                    item.Inside = new array<ref OZT_OutfitItem>();
                    p.Outfit.Items.Insert(item);
                }
            }
            if (p.Kind == OZT_Const.KIND_NPC)
            {
                npcs++;
                npcNames = Plus(npcNames, p.Name, npcs);
            }
            else
            {
                objects++;
            }
            m_Traders.Traders.Insert(p);
        }
        Say("traders: " + npcs.ToString() + " character(s), " + objects.ToString() + " object(s)");
        OZT_AdminImportItem it = Item("ok", "traders", npcs + objects);
        it.M = npcs;
        it.K = objects;
        it.Names = npcNames;
        if (ownMoney > 0)
        {
            Say("traders with currencies of their own, now on the server's notes: " + ownMoney.ToString());
            it = Item("skip", "own_money", ownMoney);
            it.Names = ownNames;
        }
        if (m_General.TraderObjects && m_General.TraderObjects.Count() > 0)
        {
            int decor = m_General.TraderObjects.Count();
            Say("decoration objects, not taken: " + decor.ToString());
            it = Item("skip", "decor", decor);
            for (int o = 0; o < decor; o++)
            {
                if (m_General.TraderObjects[o])
                    it.Names = Plus(it.Names, m_General.TraderObjects[o].ObjectName, o + 1);
            }
        }
    }

    protected OZT_TPId IdOf(int id)
    {
        for (int i = 0; i < m_Ids.IDs.Count(); i++)
        {
            if (m_Ids.IDs[i] && m_Ids.IDs[i].Id == id)
                return m_Ids.IDs[i];
        }
        return null;
    }

    // A point's categories and licences from its ID. True: the ID names
    // currencies of its own, which are not taken.
    protected bool Fill(OZT_Point p, OZT_TPId tid)
    {
        if (tid.Categories)
        {
            for (int c = 0; c < tid.Categories.Count(); c++)
            {
                string cid;
                if (m_CategoryIds.Find(tid.Categories[c], cid) && p.Categories.Find(cid) < 0)
                    p.Categories.Insert(cid);
            }
        }
        // TraderPlus opens an ID only to the holders of every licence it
        // names; so does an Access of ours.
        if (tid.LicencesRequired)
        {
            for (int l = 0; l < tid.LicencesRequired.Count(); l++)
            {
                if (tid.LicencesRequired[l] != "")
                    p.Access.Insert(NeedLicence(tid.LicencesRequired[l]));
            }
        }
        bool own = tid.CurrenciesAccepted && tid.CurrenciesAccepted.Count() > 0;
        return own;
    }

    // ---- the stock and the licences ----

    // A line of TraderPlus's: its category and the class, without case.
    protected string Key(string category, string cls)
    {
        string low = cls;
        low.ToLower();
        return category + "|" + low;
    }

    // TraderPlus's stock of each ID, in its own files: one per category, a
    // line "class count level", the count in trades. Unlimited lines it does
    // not keep, and neither do we. Ours is one shelf for the trader: a class
    // two of its categories held lands there as the sum.
    protected void TakeStock()
    {
        int lines = 0;
        for (int i = 0; i < m_Ids.IDs.Count(); i++)
        {
            OZT_TPId tid = m_Ids.IDs[i];
            if (!tid || !tid.Categories)
                continue;
            string stock = "tp" + tid.Id.ToString();
            for (int c = 0; c < tid.Categories.Count(); c++)
            {
                string path = m_Root + "\\TraderPlusDatabase\\ID_" + tid.Id.ToString() + "\\" + tid.Categories[c] + ".json";
                if (!FileExist(path))
                    continue;
                OZT_TPCategory held = new OZT_TPCategory();
                string err;
                if (!JsonFileLoader<OZT_TPCategory>.LoadFile(path, held, err) || !held.Products)
                    continue;
                for (int p = 0; p < held.Products.Count(); p++)
                {
                    array<string> f = new array<string>();
                    held.Products[p].Split(" ", f);
                    if (f.Count() < 3)
                        continue;
                    string key = Key(tid.Categories[c], f[0]);
                    string cls;
                    if (!m_Stocked.Find(key, cls))
                        continue;
                    int count = f[1].ToInt();
                    int level = f[2].ToInt();
                    if (count <= 0 || level < 0 || level >= OZT_Const.LEVEL_RUINED)
                        continue;
                    int per;
                    if (m_UnitsPer.Find(key, per))
                    {
                        count = count * per;
                        level = 0;
                    }
                    m_StockIds.Insert(stock);
                    m_StockClasses.Insert(cls);
                    m_StockLevels.Insert(level);
                    m_StockCounts.Insert(count);
                    lines++;
                }
            }
        }
        Say("stock lines: " + lines.ToString());

        // Every shelf the apply rewrites, and its lines: "tp1: 4 · tp2: 3".
        array<string> shelves = new array<string>();
        array<int> onShelf = new array<int>();
        for (int s = 0; s < m_StockIds.Count(); s++)
        {
            int at = shelves.Find(m_StockIds[s]);
            if (at < 0)
            {
                shelves.Insert(m_StockIds[s]);
                onShelf.Insert(1);
            }
            else
            {
                onShelf[at] = onShelf[at] + 1;
            }
        }
        string summary = "";
        m_Shelves = "";
        for (int k = 0; k < shelves.Count(); k++)
        {
            m_Shelves = Plus(m_Shelves, shelves[k], k + 1);
            if (k < NAMES_MAX)
            {
                if (summary != "")
                    summary = summary + " · ";
                summary = summary + shelves[k] + ": " + onShelf[k].ToString();
            }
        }
        OZT_AdminImportItem it = Item("ok", "stock", lines);
        it.Names = summary;
    }

    protected void PutStock()
    {
        array<string> cleared = new array<string>();
        for (int i = 0; i < m_StockIds.Count(); i++)
        {
            string stock = m_StockIds[i];
            if (cleared.Find(stock) < 0)
            {
                OZT_Stock.Shelf(stock).Clear();
                cleared.Insert(stock);
            }
            OZT_Stock.Add(stock, m_StockClasses[i], m_StockLevels[i], m_StockCounts[i]);
        }
        for (int s = 0; s < cleared.Count(); s++)
            OZT_Stock.Save(cleared[s]);
    }

    // TraderPlus's bank accounts, a file each.
    protected void Accounts(array<string> paths)
    {
        string dir = m_Root + "\\" + BANK + "\\";
        string name;
        FileAttr attr;
        // Every name, sorted out here: the series lists folders this way
        // (OZL_State.Scan).
        FindFileHandle fh = FindFile(dir + "*", name, attr, FindFileFlags.ALL);
        if (fh == 0)
            return;
        bool more = true;
        while (more)
        {
            int len = name.Length();
            if (len > 13 && name.IndexOf("Account_") == 0)
            {
                if (name.Substring(len - 5, 5) == ".json")
                    paths.Insert(dir + name);
            }
            more = FindNextFile(fh, name, attr);
        }
        CloseFindFile(fh);
    }

    // The licences held on TraderPlus's bank accounts, onto our characters; with
    // `grant` false only counted, for the dry run.
    protected int GiveLicences(bool grant)
    {
        int given = 0;
        array<string> paths = new array<string>();
        Accounts(paths);
        for (int i = 0; i < paths.Count(); i++)
            given = given + GiveOf(paths[i], grant);
        return given;
    }

    protected int GiveOf(string path, bool grant)
    {
        int given = 0;
        OZT_TPAccount acc = new OZT_TPAccount();
        string err;
        if (!JsonFileLoader<OZT_TPAccount>.LoadFile(path, acc, err) || !acc.Licences || acc.SteamID64 == "")
            return 0;
        for (int i = 0; i < acc.Licences.Count(); i++)
        {
            string lid;
            if (!m_LicenceIds.Find(acc.Licences[i], lid))
                continue;
            if (!grant)
                given++;
            else if (OZT_Licences.Grant(acc.SteamID64, lid))
                given++;
        }
        return given;
    }
}
