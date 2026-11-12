// The records of the service `trader`, as JSON bodies.
//
// Arrays of small records, no maps, every string far under a kilobyte: the core
// cuts a body into 900-byte parts and glues it back, but one string VALUE above
// a kilobyte arrives broken (the series' measured limit).
//
// A record that ARRIVES was allocated by the JSON loader and ran no field
// initialiser. The server reads such a record once, at once, into objects of its
// own (Copy), and never keeps the arrived one.

class OZT_OpenReq
{
    string Trader = "";
}

class OZT_WireCategory
{
    string Id = "";
    string Name = "";
}

class OZT_WireGoods
{
    string ClassName = "";
    string Category = "";
    // What the player pays this trader for one (one unit under the unit rule).
    // 0: not sold here.
    int Buy = 0;
    // What this trader pays for a pristine one. 0: not taken here.
    int Sell = 0;
    string Rule = "";
    // How many this trader holds of the line (design §11.1); -1: unlimited.
    int Stock = -1;
    // The condition the line's things are in: 0 pristine .. 3 badly damaged.
    // A stocked class is sent once per condition it holds.
    int Level = 0;
    // A licence (design §11.2): what the player reads, and the class the
    // shelf draws for it. Empty for goods.
    string Name = "";
    string Look = "";
    // What comes attached to one at this trader, each charged at its own
    // price: Buy is the sum.
    ref array<string> With;

    void OZT_WireGoods()
    {
        With = new array<string>();
    }
}

// One thing the player carries that this trader pays for: the entity by the
// two halves of its network id, and what it brings as it is now. What is not in
// the list is not taken here.
class OZT_WireOwn
{
    int Low = 0;
    int High = 0;
    // For the thing itself, or the stack as it stands: its own line of the
    // book, nothing that hangs on it or is loaded in it.
    int Price = 0;
    // 0: sells. 2: something lies inside it, and it goes only together with
    // all of that.
    int Why = 0;
    // Above zero: a stack of this many units, of which a part may be sold.
    int Units = 0;
    // What it brings sold as it is, with everything on it and in its magazines
    // that the trader takes and nothing kept. A stack: the same as Price.
    int Tree = 0;
    // The rounds loaded in the thing itself, and what they bring.
    int Loaded = 0;
    int LoadedPrice = 0;
}

class OZT_OpenRes
{
    int Revision = 0;
    string Trader = "";
    string Name = "";
    float Radius = 0;
    string Provider = "";
    int Physical = 0;
    int Unit = 1;
    string Label = "";
    int Balance = 0;
    // Cash first, then the bank: the bank's share of Balance; -1 when no bank
    // stands behind the notes.
    int Bank = -1;
    int WearBought = 1;
    ref OZT_ConditionFactors Condition;
    ref array<ref OZT_WireCategory> Categories;
    ref array<ref OZT_WireGoods> Goods;
    ref array<ref OZT_WireOwn> Yours;
    // The classes that are this trader's money when money is items: the screen
    // counts them as money and does not show them among the player's things.
    ref array<string> Notes;
    // The licences the player holds (design §11.2).
    ref array<string> Licences;

    void OZT_OpenRes()
    {
        Condition = new OZT_ConditionFactors();
        Categories = new array<ref OZT_WireCategory>();
        Goods = new array<ref OZT_WireGoods>();
        Yours = new array<ref OZT_WireOwn>();
        Notes = new array<string>();
        Licences = new array<string>();
    }
}

// What the player keeps of a thing they sell: a part of it (by the two halves of
// its network id), which then comes off and stays with everything on it; or,
// with Rounds 1, the rounds loaded in the thing named, which may be the sold
// thing itself or a part of it.
class OZT_WireKeep
{
    int Low = 0;
    int High = 0;
    int Rounds = 0;

    OZT_WireKeep Copy()
    {
        OZT_WireKeep c = new OZT_WireKeep();
        c.Low = Low;
        c.High = High;
        c.Rounds = Rounds;
        return c;
    }
}

// One thing the player sells: the entity by the two halves of its network id,
// and how many units of it when it is a stack (0: all of it). A thing that is
// not a stack is sold with what hangs on it and what is loaded in it, less what
// Keep names.
class OZT_WireSell
{
    int Low = 0;
    int High = 0;
    int Count = 0;
    ref array<ref OZT_WireKeep> Keep;

    void OZT_WireSell()
    {
        Keep = new array<ref OZT_WireKeep>();
    }

    // An arrived line ran no constructor: a line without Keep has none.
    OZT_WireSell Copy()
    {
        OZT_WireSell c = new OZT_WireSell();
        c.Low = Low;
        c.High = High;
        c.Count = Count;
        if (Keep)
        {
            for (int i = 0; i < Keep.Count(); i++)
            {
                OZT_WireKeep k = Keep[i];
                if (k)
                    c.Keep.Insert(k.Copy());
            }
        }
        return c;
    }
}

// A part fitted to a thing the same line buys: goods of this trader, bought at
// its own price, that arrive attached. `SlotName` names the slot, "" leaves it to
// the server. `On` is what it hangs on: -1 the bought thing, otherwise an
// earlier entry of the same list.
class OZT_WireFit
{
    string ClassName = "";
    string SlotName = "";
    int On = -1;

    OZT_WireFit Copy()
    {
        OZT_WireFit c = new OZT_WireFit();
        c.ClassName = ClassName;
        c.SlotName = SlotName;
        c.On = On;
        return c;
    }
}

// Goods the player buys, by class: pieces, or units under the unit rule.
//
// With Fit the line buys one piece and the parts named, attached to it. With
// OnLow and OnHigh the class is a part and goes onto that thing of the
// player's, in the slot SlotName names ("" leaves it to the server).
class OZT_WireBuy
{
    string ClassName = "";
    int Count = 0;
    ref array<ref OZT_WireFit> Fit;
    int OnLow = 0;
    int OnHigh = 0;
    string SlotName = "";
    // The condition of the stock line bought from (design §11.1); 0 for
    // everything that is not stocked.
    int Level = 0;

    void OZT_WireBuy()
    {
        Fit = new array<ref OZT_WireFit>();
    }

    // An arrived line ran no constructor: a line without Fit has none.
    OZT_WireBuy Copy()
    {
        OZT_WireBuy c = new OZT_WireBuy();
        c.ClassName = ClassName;
        c.Count = Count;
        c.OnLow = OnLow;
        c.OnHigh = OnHigh;
        c.SlotName = SlotName;
        c.Level = Level;
        if (Fit)
        {
            for (int i = 0; i < Fit.Count(); i++)
            {
                OZT_WireFit f = Fit[i];
                if (f)
                    c.Fit.Insert(f.Copy());
            }
        }
        return c;
    }
}

class OZT_DealReq
{
    string Trader = "";
    ref array<ref OZT_WireSell> Sells;
    ref array<ref OZT_WireBuy> Buys;

    // The net the client showed: what the player pays (above zero) or is paid
    // (below). The server computes its own and refuses a deal whose net differs.
    int Net = 0;

    void OZT_DealReq()
    {
        Sells = new array<ref OZT_WireSell>();
        Buys = new array<ref OZT_WireBuy>();
    }
}

class OZT_DealRes
{
    // The server's own net, on a refusal too: after `changed` the client shows it.
    int Net = 0;
    int Paid = 0;
    int Received = 0;
    int Balance = 0;
    // Cash first, then the bank: what the bank pays (above zero) or takes
    // (below zero) in this deal.
    int Bank = 0;

    // The class of the one thing that went to the hands; "" when none did.
    string InHands = "";

    // The number a refusal carries: things that do not fit (no space), things
    // in the way (not empty).
    int Left = 0;

    // What stays with the player of the things they sell: the parts that come
    // off (kept by choice, or not taken by the trader) and the loaded rounds
    // that come back as piles.
    int Kept = 0;
    int KeptRounds = 0;

    // What the lines come to, on a refusal too as far as they were read: what
    // the trader pays for everything sold, what everything bought costs, and
    // each line's own sum in the order of the request. The screen shows these
    // and computes none of its own.
    int Sold = 0;
    int Bought = 0;
    ref array<int> SellPrices;
    ref array<int> BuyPrices;

    void OZT_DealRes()
    {
        SellPrices = new array<int>();
        BuyPrices = new array<int>();
    }
}
