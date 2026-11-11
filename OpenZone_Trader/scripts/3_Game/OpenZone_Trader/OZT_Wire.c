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
