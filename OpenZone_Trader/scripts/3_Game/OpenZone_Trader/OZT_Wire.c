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
    // -1 while stock is unlimited: the headroom for finite stock (design §11).
    int Stock = -1;
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
    int WearBought = 1;
    ref OZT_ConditionFactors Condition;
    ref array<ref OZT_WireCategory> Categories;
    ref array<ref OZT_WireGoods> Goods;

    void OZT_OpenRes()
    {
        Condition = new OZT_ConditionFactors();
        Categories = new array<ref OZT_WireCategory>();
        Goods = new array<ref OZT_WireGoods>();
    }
}

// One thing the player sells: the entity by the two halves of its network id,
// and how many units of it when it is a stack (0: all of it).
class OZT_WireSell
{
    int Low = 0;
    int High = 0;
    int Count = 0;

    OZT_WireSell Copy()
    {
        OZT_WireSell c = new OZT_WireSell();
        c.Low = Low;
        c.High = High;
        c.Count = Count;
        return c;
    }
}

// Goods the player buys, by class: pieces, or units under the unit rule.
class OZT_WireBuy
{
    string ClassName = "";
    int Count = 0;

    OZT_WireBuy Copy()
    {
        OZT_WireBuy c = new OZT_WireBuy();
        c.ClassName = ClassName;
        c.Count = Count;
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

    // The class of the one thing that went to the hands; "" when none did.
    string InHands = "";

    // The number a refusal carries: things that do not fit (no space), things
    // in the way (not empty).
    int Left = 0;
}
