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

    // Is the stack in the condition of new goods? What a deal brings is new,
    // and it joins such a stack only (the owner, 2026-11-11): what differs in
    // condition is neither stacked nor averaged.
    static bool LikeNew(EntityAI e)
    {
        return e.GetHealthLevel() == GameConstants.STATE_PRISTINE;
    }
}

class OZT_BookEntry
{
    string ClassName;
    string Category;
    int Buy;
    int Sell;
    string Rule;

    // What comes attached to it, as the checked file names it.
    ref array<string> With;

    // Its category's gates and its own, together (design §8.2).
    ref array<ref OZT_Condition> Requires;
    ref array<ref OZT_Discount> Discounts;
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
                e.With = new array<string>();
                if (line.With)
                {
                    for (int w = 0; w < line.With.Count(); w++)
                        e.With.Insert(line.With[w]);
                }
                e.Requires = new array<ref OZT_Condition>();
                OZT_Condition.CopyAll(cat.Requires, e.Requires);
                OZT_Condition.CopyAll(line.Requires, e.Requires);
                e.Discounts = new array<ref OZT_Discount>();
                OZT_Discount.CopyAll(cat.Discounts, e.Discounts);
                OZT_Discount.CopyAll(line.Discounts, e.Discounts);
                s_ByClass.Set(e.ClassName, e);
            }
        }

        // The server's banknotes, for the change tab: given and taken at face
        // value. The goods check keeps them out of the goods file, so no class
        // stands here twice.
        array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
        for (int n = 0; n < notes.Count(); n++)
        {
            OZT_BookEntry money = new OZT_BookEntry();
            money.ClassName = notes[n].ClassName;
            money.Category = OZT_Const.CATEGORY_CHANGE;
            money.Buy = notes[n].Value;
            money.Sell = notes[n].Value;
            money.Rule = OZT_Const.RULE_UNIT;
            money.With = new array<string>();
            // Money has no gates: a note is worth its face value to everyone.
            money.Requires = new array<ref OZT_Condition>();
            money.Discounts = new array<ref OZT_Discount>();
            s_ByClass.Set(money.ClassName, money);
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
        if (e.Category == OZT_Const.CATEGORY_CHANGE)
            return Changes(trader);
        return trader.Categories.Find(e.Category) > -1;
    }

    // Does this trader change banknotes? Only item money has notes, and it
    // takes two kinds of them to change one into the other.
    static bool Changes(OZT_Point trader)
    {
        if (!trader || trader.NoExchange == 1)
            return false;
        OZT_MoneyProvider money = OZT_Money.Of(trader);
        if (!money || !money.Physical())
            return false;
        return OZT_Config.Get().Settings().Notes.Count() > 1;
    }

    // What the player pays this trader for `count` of it: units of a stack
    // under the unit rule, pieces otherwise. 0: not sold here. One piece's price
    // goes UP to a multiple of the smallest note and is then multiplied: the
    // price a catalogue shows for one, times the count.
    static int BuyPrice(OZT_Point trader, OZT_BookEntry e, int count)
    {
        if (count <= 0 || !Deals(trader, e))
            return 0;
        if (e.Buy <= 0)
            return 0;
        // A note at face value: no factor of the trader's, nothing to round.
        if (e.Category == OZT_Const.CATEGORY_CHANGE)
            return e.Buy * count;
        int unit = OZT_Config.Get().Unit();
        int piece = OZT_Config.RoundUpTo(e.Buy * trader.BuyFactor, unit);
        return piece * count;
    }

    // Are the goods locked for this player here (their Requires)? A lock nobody
    // answers is off. Without a player there is no lock.
    static bool Locked(PlayerBase player, OZT_Point trader, OZT_BookEntry e)
    {
        if (!player || !e || !e.Requires)
            return false;
        return !OZT_Gates.All(player, trader, e.Requires, true);
    }

    // What this player pays for `count` of it here: 0 when it is not sold to
    // them (not dealt in, no price, locked); otherwise one piece's price, less
    // the player's best discount and rounded UP to a note, times the count.
    // Without a player: the plain price.
    static int BuyFor(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int count)
    {
        int plain = BuyPrice(trader, e, 1);
        if (plain <= 0 || count <= 0)
            return 0;
        if (!player || e.Category == OZT_Const.CATEGORY_CHANGE)
            return plain * count;
        if (Locked(player, trader, e))
            return 0;
        float pct = OZT_Gates.Best(player, trader, e.Discounts);
        if (pct <= 0)
            return plain * count;
        int unit = OZT_Config.Get().Unit();
        int piece = OZT_Config.RoundUpTo(plain * (100 - pct) / 100, unit);
        return piece * count;
    }

    // What comes attached to the goods at this trader: the classes of its
    // `With` that this trader sells this player. At a trader who does not sell
    // a part the goods are bare: all he hands out stays inside what he deals
    // in, which is what the loop check counts.
    static void Comes(PlayerBase player, OZT_Point trader, OZT_BookEntry e, out array<string> parts)
    {
        parts = new array<string>();
        if (!e || !e.With)
            return;
        for (int i = 0; i < e.With.Count(); i++)
        {
            string part = e.With[i];
            if (BuyFor(player, trader, EntryOf(part), 1) > 0)
                parts.Insert(part);
        }
    }

    // What the player pays for `count` of the goods as this trader sells
    // them: each with what comes with it, every part at its own price. Nothing
    // comes free, so a sum of classes makes no loop while no class alone does.
    static int BuyWith(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int count)
    {
        int own = BuyFor(player, trader, e, count);
        if (own <= 0)
            return 0;
        array<string> parts;
        Comes(player, trader, e, parts);
        int sum = own;
        for (int i = 0; i < parts.Count(); i++)
            sum = sum + BuyFor(player, trader, EntryOf(parts[i]), count);
        return sum;
    }

    // What this trader pays for `count` units of `item` (the count matters
    // for a stack that can be split). 0: not taken. The line's total goes DOWN
    // to a multiple of the smallest note.
    static int SellPrice(OZT_Point trader, OZT_BookEntry e, EntityAI item, int count)
    {
        if (!item || !Deals(trader, e))
            return 0;
        if (e.Sell <= 0)
            return 0;
        int level = item.GetHealthLevel();
        if (level >= OZT_Const.LEVEL_RUINED)
            return 0;
        // A note at face value, worn or not: what money counts it as.
        if (e.Category == OZT_Const.CATEGORY_CHANGE)
            return e.Sell * count;
        float price = e.Sell * trader.SellFactor * OZT_Config.Get().Settings().Condition.Of(level);
        if (e.Rule == OZT_Const.RULE_UNIT)
        {
            price = price * count;
        }
        else if (OZT_Stack.Countable(item))
        {
            // A stack that can be split is paid by what is in it, under any
            // rule: the price is for a full stack, and what is sold, the whole
            // of it or a part, brings its share. One unit never brings the
            // price of a stack (the owner, 2026-11-11: no money dupes).
            float most = OZT_Stack.Max(item);
            if (most < count)
                most = count;
            if (most < 1)
                most = 1;
            price = price * count / most;
        }
        else if (e.Rule == OZT_Const.RULE_FRACTION)
        {
            price = price * OZT_Stack.Fraction(item);
        }
        int unit = OZT_Config.Get().Unit();
        return OZT_Config.RoundDownTo(price, unit);
    }
}
