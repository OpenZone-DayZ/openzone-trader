// The price book at run time, and how a deal counts "how much of it".

// A stack is counted in units when the engine lets it be split: a pile of nails,
// a wad of notes, a pile of rounds. A canteen has a quantity too, but that is
// what is left in it, not a number of things.
class OZT_Stack
{
    static bool Countable(EntityAI e)
    {
        Magazine mag = Magazine.Cast(e);
        if (mag)
            return mag.IsAmmoPile();
        ItemBase ib = ItemBase.Cast(e);
        if (!ib)
            return false;
        return ib.HasQuantity() && ib.IsSplitable();
    }

    static int Units(EntityAI e)
    {
        if (!Countable(e))
            return 1;
        Magazine mag = Magazine.Cast(e);
        if (mag)
            return mag.GetAmmoCount();
        ItemBase ib = ItemBase.Cast(e);
        int n = ib.GetQuantity();
        return n;
    }

    // The most one stack of this thing holds where it lies in a cargo.
    static int Max(EntityAI e)
    {
        if (!Countable(e))
            return 1;
        Magazine mag = Magazine.Cast(e);
        if (mag)
            return mag.GetAmmoMax();
        ItemBase ib = ItemBase.Cast(e);
        return ib.GetTargetQuantityMax(-1);
    }

    // Measured in phase 0: the bool these engine calls return means "the item
    // was destroyed", so it is not read; and a pile's rounds are its amount.
    // AddQuantity with both destroy flags off never deletes the stack: a stack
    // the deal empties is deleted by the deal, with ObjectDelete.
    static void Add(EntityAI e, int units)
    {
        Magazine mag = Magazine.Cast(e);
        if (mag)
        {
            mag.ServerSetAmmoCount(mag.GetAmmoCount() + units);
            return;
        }
        ItemBase ib = ItemBase.Cast(e);
        if (ib)
            ib.AddQuantity(units, false, false);
    }

    static void Set(EntityAI e, int units)
    {
        Magazine mag = Magazine.Cast(e);
        if (mag)
        {
            mag.ServerSetAmmoCount(units);
            return;
        }
        ItemBase ib = ItemBase.Cast(e);
        if (ib)
            ib.SetQuantity(units);
    }

    // What is left of a thing that is used up, 0..1; 1 for anything that has no
    // such notion.
    static float Fraction(EntityAI e)
    {
        ItemBase ib = ItemBase.Cast(e);
        if (!ib || !ib.HasQuantity())
            return 1;
        if (Magazine.Cast(e))
            return 1;
        float most = ib.GetQuantityMax();
        if (most <= 0)
            return 1;
        float part = ib.GetQuantity() / most;
        return Math.Clamp(part, 0, 1);
    }
}

class OZT_BookEntry
{
    string ClassName;
    string Category;
    int Buy;
    int Sell;
    string Rule;
}

class OZT_Book
{
    private static ref map<string, ref OZT_BookEntry> s_ByClass = new map<string, ref OZT_BookEntry>();

    // From the checked files; called after every load.
    static void Rebuild()
    {
        s_ByClass.Clear();
        OZT_GoodsFile file = OZT_Config.Get().Goods();
        for (int c = 0; c < file.Categories.Count(); c++)
        {
            OZT_Category cat = file.Categories[c];
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_Goods line = cat.Goods[g];
                OZT_BookEntry e = new OZT_BookEntry();
                e.ClassName = line.ClassName;
                e.Category = cat.Id;
                e.Buy = line.Buy;
                e.Sell = line.Sell;
                e.Rule = line.Quantity;
                s_ByClass.Set(e.ClassName, e);
            }
        }
    }

    // A 4_World class held by a static: let go at mission end.
    static void Reset()
    {
        s_ByClass.Clear();
    }

    static OZT_BookEntry EntryOf(string type)
    {
        return s_ByClass.Get(type);
    }

    static bool Deals(OZT_Point trader, OZT_BookEntry e)
    {
        if (!trader || !e)
            return false;
        return trader.Categories.Find(e.Category) > -1;
    }

    // What the player pays this trader for `count` of it: units of a stack
    // under the unit rule, pieces otherwise. 0: not sold here.
    static int BuyPrice(OZT_Point trader, OZT_BookEntry e, int count)
    {
        if (count <= 0 || !Deals(trader, e))
            return 0;
        if (e.Buy <= 0)
            return 0;
        int unit = OZT_Config.Get().Unit();
        int piece = OZT_Config.RoundTo(e.Buy * trader.BuyFactor, unit);
        return piece * count;
    }

    // What this trader pays for `count` units of `item` (the count matters
    // under the unit rule only). 0: not taken.
    static int SellPrice(OZT_Point trader, OZT_BookEntry e, EntityAI item, int count)
    {
        if (!item || !Deals(trader, e))
            return 0;
        if (e.Sell <= 0)
            return 0;
        int level = item.GetHealthLevel();
        if (level >= OZT_Const.LEVEL_RUINED)
            return 0;
        float price = e.Sell * trader.SellFactor * OZT_Config.Get().Settings().Condition.Of(level);
        if (e.Rule == OZT_Const.RULE_UNIT)
            price = price * count;
        else if (e.Rule == OZT_Const.RULE_FRACTION)
            price = price * OZT_Stack.Fraction(item);
        int unit = OZT_Config.Get().Unit();
        return OZT_Config.RoundTo(price, unit);
    }
}
