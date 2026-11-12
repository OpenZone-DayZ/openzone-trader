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

    // Finite goods (design §11.1): the line's Stock block; unlimited without.
    bool Stocked;
    int MaxStock;
    float Coefficient = 1;
    float Destock;
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
                e.Coefficient = 1;
                if (line.Stock)
                {
                    e.Stocked = true;
                    e.MaxStock = line.Stock.Max;
                    e.Coefficient = line.Stock.Coefficient;
                    e.Destock = line.Stock.Destock;
                }
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
            money.Coefficient = 1;
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

    // ---- stock (design §11.1) ----------------------------------------------------

    // The share of a stocked line's price its condition gives: a pristine line
    // is the goods' own price, a worn one the settings' factor of it.
    static float LevelShare(int level)
    {
        if (level <= 0)
            return 1;
        return OZT_Config.Get().Settings().Condition.Of(level);
    }

    // The health a thing bought from a worn line is made with: the middle of
    // its level's band (worn at 0.7 and below, damaged at 0.5, badly damaged
    // at 0.3). Below zero: new.
    static float LevelHealth(int level)
    {
        if (level == 1)
            return 0.6;
        if (level == 2)
            return 0.4;
        if (level == 3)
            return 0.2;
        return -1;
    }

    // The line a thing sold in this condition goes into.
    static int StockLevel(int level)
    {
        if (OZT_Config.Get().Settings().StockOnlyPristine == 1)
            return 0;
        return level;
    }

    static float Curve(float coef, int n)
    {
        if (coef == 1 || n <= 0)
            return 1;
        return Math.Pow(coef, n);
    }

    // The line as it stands, or as the deal behind `cursor` has left it.
    static int StockOf(OZT_Point trader, OZT_BookEntry e, int level, OZT_StockCursor cursor)
    {
        if (cursor)
            return cursor.Count(e.ClassName, level);
        return OZT_Stock.Count(trader.StockId(), e.ClassName, level);
    }

    // What this player pays here for `count` of the goods from the line of
    // `level`. Unlimited goods: BuyFor, pristine only. Stocked goods: unit by
    // unit at the stock each leaves, Buy x factor x share x Coefficient^(S-1-j),
    // each rounded up to a note, the player's discount after that; 0 when the
    // line holds fewer than `count`. With a cursor the units are taken from it.
    static int BuyForAt(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int level, int count, OZT_StockCursor cursor)
    {
        if (!e || count <= 0)
            return 0;
        if (!e.Stocked)
        {
            if (level != 0)
                return 0;
            return BuyFor(player, trader, e, count);
        }
        if (BuyPrice(trader, e, 1) <= 0)
            return 0;
        if (player && Locked(player, trader, e))
            return 0;
        int stock = StockOf(trader, e, level, cursor);
        if (stock < count)
            return 0;
        float pct = 0;
        if (player)
            pct = OZT_Gates.Best(player, trader, e.Discounts);
        int unit = OZT_Config.Get().Unit();
        float base = e.Buy * trader.BuyFactor * LevelShare(level);
        int sum = 0;
        for (int j = 0; j < count; j++)
        {
            int piece = OZT_Config.RoundUpTo(base * Curve(e.Coefficient, stock - 1 - j), unit);
            if (pct > 0)
                piece = OZT_Config.RoundUpTo(piece * (100 - pct) / 100, unit);
            sum = sum + piece;
        }
        if (cursor)
            cursor.Move(e.ClassName, level, -count);
        return sum;
    }

    // What one of the line costs now, for the shelf: a sold-out line shows
    // the price its first unit would have.
    static int ShelfPrice(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int level)
    {
        if (!e || !e.Stocked)
            return BuyFor(player, trader, e, 1);
        int stock = StockOf(trader, e, level, null);
        if (stock >= 1)
            return BuyForAt(player, trader, e, level, 1, null);
        if (BuyFor(player, trader, e, 1) <= 0)
            return 0;
        float pct = 0;
        if (player)
            pct = OZT_Gates.Best(player, trader, e.Discounts);
        int unit = OZT_Config.Get().Unit();
        int piece = OZT_Config.RoundUpTo(e.Buy * trader.BuyFactor * LevelShare(level), unit);
        if (pct > 0)
            piece = OZT_Config.RoundUpTo(piece * (100 - pct) / 100, unit);
        return piece;
    }

    // What comes attached to `count` of the goods at this trader, stock
    // counted: a stocked part the line cannot give `count` of stays off, as at a
    // trader who does not sell it. Nothing is taken here.
    static void ComesAt(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int count, OZT_StockCursor cursor, out array<string> parts)
    {
        array<string> all;
        Comes(player, trader, e, all);
        parts = new array<string>();
        for (int i = 0; i < all.Count(); i++)
        {
            OZT_BookEntry part = EntryOf(all[i]);
            if (part && part.Stocked && StockOf(trader, part, 0, cursor) < count)
                continue;
            parts.Insert(all[i]);
        }
    }

    // The shelf's price of one of a line with what comes with it (pristine
    // only: a worn line comes bare).
    static int ShelfWith(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int level)
    {
        int own = ShelfPrice(player, trader, e, level);
        if (own <= 0 || level != 0)
            return own;
        array<string> parts;
        ComesAt(player, trader, e, 1, null, parts);
        int sum = own;
        for (int i = 0; i < parts.Count(); i++)
            sum = sum + ShelfPrice(player, trader, EntryOf(parts[i]), 0);
        return sum;
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
        float price = SellPlain(trader, e, item, count);
        int unit = OZT_Config.Get().Unit();
        return OZT_Config.RoundDownTo(price, unit);
    }

    // What the trader pays for `count` units of `item`, stock counted (design
    // §11.1). Unlimited goods: SellPrice. A stocked line pays its units at the
    // stock each grows it to, x Coefficient^(max(S+j,1)-1) (TraderPlus's own
    // formula); a unit is a unit of a stack under the unit rule, the thing
    // otherwise. With a cursor the units are brought into it when the trader
    // takes them.
    static int SellPriceAt(OZT_Point trader, OZT_BookEntry e, EntityAI item, int count, OZT_StockCursor cursor)
    {
        if (!e || !e.Stocked)
            return SellPrice(trader, e, item, count);
        if (!item || !Deals(trader, e) || e.Sell <= 0)
            return 0;
        int level = item.GetHealthLevel();
        if (level >= OZT_Const.LEVEL_RUINED)
            return 0;
        int units = 1;
        if (e.Rule == OZT_Const.RULE_UNIT)
            units = count;
        if (units < 1)
            return 0;
        float each = SellPlain(trader, e, item, count) / units;
        int into = StockLevel(level);
        // A pile's units go into the pristine line: what a deal hands back
        // of a pile is new.
        if (e.Rule == OZT_Const.RULE_UNIT)
            into = 0;
        int paid = SellUnits(trader, e, into, units, each, cursor);
        return paid;
    }

    // `units` units brought into the line of `level` at `each` apiece before
    // the curve; the sum rounded down to a note. Brought into the cursor when
    // anything is paid.
    static int SellUnits(OZT_Point trader, OZT_BookEntry e, int level, int units, float each, OZT_StockCursor cursor)
    {
        int stock = StockOf(trader, e, level, cursor);
        float sum = 0;
        for (int j = 0; j < units; j++)
        {
            int n = stock + j;
            if (n < 1)
                n = 1;
            sum = sum + each * Curve(e.Coefficient, n - 1);
        }
        int paid = OZT_Config.RoundDownTo(sum, OZT_Config.Get().Unit());
        if (paid > 0 && cursor)
            cursor.Move(e.ClassName, level, units);
        return paid;
    }

    // The trader's price for `count` units of `item` before rounding, by the
    // goods' quantity rule and the thing's condition.
    static float SellPlain(OZT_Point trader, OZT_BookEntry e, EntityAI item, int count)
    {
        int level = item.GetHealthLevel();
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
        return price;
    }
}
