// OZ_Trader_Goods.json: the price book. Categories, each with goods.
//
// `Buy` is what the player pays for one, `Sell` what the trader pays for a
// pristine one. Zero or less turns that side off, and so does leaving the key
// out. `Quantity` is the quantity rule: "" one price for the piece, "unit" a
// price per unit of a stack (rounds, nails), "fraction" a price for a full one,
// scaled by what is left when it is sold (a canteen, a bandage).
//
// What is checked HERE is what the file alone can answer, and those fixes are
// written back by the core's loader. Whether a class exists in the game, and
// whether a price fits the banknotes, is checked by OZT_Config in memory only:
// a mod that was not loaded today must not delete a line from the admin's file.

class OZT_Goods
{
    string ClassName = "";
    int Buy = 0;
    int Sell = 0;
    string Quantity = "";

    OZT_Goods Copy()
    {
        OZT_Goods c = new OZT_Goods();
        c.ClassName = ClassName;
        c.Buy = Buy;
        c.Sell = Sell;
        c.Quantity = Quantity;
        return c;
    }
}

class OZT_Category
{
    string Id = "";
    string Name = "";
    ref array<ref OZT_Goods> Goods;

    OZT_Category Copy()
    {
        OZT_Category c = new OZT_Category();
        c.Id = Id;
        c.Name = Name;
        c.Goods = new array<ref OZT_Goods>();
        if (Goods)
        {
            for (int i = 0; i < Goods.Count(); i++)
            {
                OZT_Goods g = Goods[i];
                if (g)
                    c.Goods.Insert(g.Copy());
            }
        }
        return c;
    }
}

class OZT_GoodsFile : OZ_ConfigBase
{
    ref array<ref OZT_Category> Categories;

    override int LatestVersion()
    {
        return 1;
    }

    override void LoadDefaults()
    {
        super.LoadDefaults();
        Categories = new array<ref OZT_Category>();
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        array<ref OZT_Category> kept = new array<ref OZT_Category>();
        array<string> ids = new array<string>();
        if (Categories)
        {
            for (int i = 0; i < Categories.Count(); i++)
            {
                OZT_Category raw = Categories[i];
                if (!raw)
                    continue;
                OZT_Category c = raw.Copy();
                if (!OZT_Ids.IsSafe(c.Id))
                {
                    OZT_Log.Warn("Goods: a category with the id '" + c.Id + "' is dropped: letters, digits, _ and - only, at most 64");
                    warnings++;
                    continue;
                }
                if (ids.Find(c.Id) > -1)
                {
                    OZT_Log.Warn("Goods: the category '" + c.Id + "' is listed twice, the second is dropped");
                    warnings++;
                    continue;
                }
                ids.Insert(c.Id);
                warnings = warnings + CheckLines(c);
                kept.Insert(c);
            }
        }
        Categories = kept;
    }

    // Returns how many lines it had to fix.
    private int CheckLines(OZT_Category c)
    {
        int fixes = 0;
        array<ref OZT_Goods> lines = new array<ref OZT_Goods>();
        for (int g = 0; g < c.Goods.Count(); g++)
        {
            OZT_Goods e = c.Goods[g];
            if (e.ClassName == "")
            {
                OZT_Log.Warn("Goods: a line with no ClassName in category '" + c.Id + "' is dropped");
                fixes++;
                continue;
            }
            if (e.Buy < 0)
                e.Buy = 0;
            if (e.Sell < 0)
                e.Sell = 0;
            bool known = e.Quantity == OZT_Const.RULE_EACH || e.Quantity == OZT_Const.RULE_UNIT;
            if (e.Quantity == OZT_Const.RULE_FRACTION)
                known = true;
            if (!known)
            {
                OZT_Log.Warn("Goods: '" + e.ClassName + "' has the quantity rule '" + e.Quantity + "'; it is \"\", \"unit\" or \"fraction\". Using \"\"");
                e.Quantity = OZT_Const.RULE_EACH;
                fixes++;
            }
            lines.Insert(e);
        }
        c.Goods = lines;
        return fixes;
    }
}
