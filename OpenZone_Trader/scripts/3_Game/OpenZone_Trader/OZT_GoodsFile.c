// OZ_Trader_Goods.json: the price book. Categories, each with goods.
//
// `Buy` is what the player pays for one, `Sell` what the trader pays for a
// pristine one. Zero or less turns that side off, and so does leaving the key
// out. `Quantity` is the quantity rule: "" one price for the piece, "unit" a
// price per unit of a stack (rounds, nails), "fraction" a price for a full one,
// scaled by what is left when it is sold (a canteen, a bandage). A stack that
// can be split is paid by its share of a full stack under every rule but
// "unit": one unit never brings the price of a stack.
//
// `Stock` makes the goods finite (design §11.1): without it, or with Max -1,
// they are unlimited, as before. `Max` is the start stock and the destock
// threshold (0: the trader holds only what players sell him; write it with its
// Coefficient, a block of zeros reads as no block), `Coefficient` the price's
// change per unit of stock (price x Coefficient^(stock-1)), `Destock` the share
// a full line loses at a server start. TraderPlus's product line maps onto it
// one to one.
//
// `With` names classes that come attached to the goods: a pistol with its
// magazine. Each is goods of the book itself and is charged at its own price:
// the goods cost the sum, nothing comes free.
//
// What is checked HERE is what the file alone can answer, and those fixes are
// written back by the core's loader. Whether a class exists in the game, and
// whether a price fits the banknotes, is checked by OZT_Config in memory only:
// a mod that was not loaded today must not delete a line from the admin's file.

class OZT_GoodsStock
{
    int Max = 0;
    float Coefficient = 1;
    float Destock = 0;

    // Does the block say anything? A block of zeros is one the file did not
    // carry: "only what players sell" is written with its Coefficient.
    bool Said()
    {
        return Max != 0 || Coefficient != 0 || Destock != 0;
    }

    OZT_GoodsStock Copy()
    {
        OZT_GoodsStock c = new OZT_GoodsStock();
        c.Max = Max;
        c.Coefficient = Coefficient;
        c.Destock = Destock;
        return c;
    }
}

class OZT_Goods
{
    string ClassName = "";
    int Buy = 0;
    int Sell = 0;
    string Quantity = "";
    ref array<string> With;

    // Null: unlimited.
    ref OZT_GoodsStock Stock;

    // The gates of this line (design §8.2): who may buy it, and its discounts.
    ref array<ref OZT_Condition> Requires;
    ref array<ref OZT_Discount> Discounts;

    // A line read from a file ran no initialiser: one without With has none.
    OZT_Goods Copy()
    {
        OZT_Goods c = new OZT_Goods();
        c.ClassName = ClassName;
        c.Buy = Buy;
        c.Sell = Sell;
        c.Quantity = Quantity;
        // The loader allocates a nested object the file does not carry, all
        // zeros (measured 2026-11-12: every goods line came out stocked): a
        // block counts only when it says something. Max -1 is TraderPlus's
        // word for unlimited.
        if (Stock && Stock.Max >= 0 && Stock.Said())
            c.Stock = Stock.Copy();
        c.Requires = new array<ref OZT_Condition>();
        OZT_Condition.CopyAll(Requires, c.Requires);
        c.Discounts = new array<ref OZT_Discount>();
        OZT_Discount.CopyAll(Discounts, c.Discounts);
        c.With = new array<string>();
        if (With)
        {
            for (int i = 0; i < With.Count(); i++)
            {
                string part = With[i];
                if (part != "" && part != ClassName && c.With.Find(part) < 0)
                    c.With.Insert(part);
            }
        }
        return c;
    }
}

class OZT_Category
{
    string Id = "";
    string Name = "";
    ref array<ref OZT_Goods> Goods;

    // The gates of every line of the category, besides each line's own.
    ref array<ref OZT_Condition> Requires;
    ref array<ref OZT_Discount> Discounts;

    OZT_Category Copy()
    {
        OZT_Category c = new OZT_Category();
        c.Id = Id;
        c.Name = Name;
        c.Requires = new array<ref OZT_Condition>();
        OZT_Condition.CopyAll(Requires, c.Requires);
        c.Discounts = new array<ref OZT_Discount>();
        OZT_Discount.CopyAll(Discounts, c.Discounts);
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
            if (e.Stock)
                fixes = fixes + CheckStock(e);
            lines.Insert(e);
        }
        c.Goods = lines;
        return fixes;
    }

    // A nested field the file did not carry reads zero: a coefficient of zero
    // cannot be meant.
    private int CheckStock(OZT_Goods e)
    {
        int fixes = 0;
        OZT_GoodsStock st = e.Stock;
        if (st.Max < 0)
        {
            OZT_Log.Warn("Goods: '" + e.ClassName + "' has Stock.Max " + st.Max.ToString() + "; using 0 (leave Stock out for unlimited goods)");
            st.Max = 0;
            fixes++;
        }
        if (st.Coefficient <= 0)
            st.Coefficient = 1;
        if (st.Destock < 0 || st.Destock > 1)
        {
            OZT_Log.Warn("Goods: '" + e.ClassName + "' has Stock.Destock " + st.Destock.ToString() + "; it is a share, 0..1. Using 0");
            st.Destock = 0;
            fixes++;
        }
        return fixes;
    }
}
