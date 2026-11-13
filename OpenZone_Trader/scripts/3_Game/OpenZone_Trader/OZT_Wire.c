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
    // 1: a part of the open (the load test of 2026-11-13: a big book's whole
    // open is half a megabyte): the goods of the classes in Only, and the player's
    // side whole -- the money, the things carried, the licences.
    int Part = 0;
    ref array<string> Only;
    // The screen's number of this open: the head and the pages carry it back,
    // and the screen drops what answers an open it no longer waits for.
    int Ask = 0;

    void OZT_OpenReq()
    {
        Only = new array<string>();
    }
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
    // A part of the open (OZT_OpenReq.Part): the goods of these classes only,
    // to be put in the place of what the screen holds of them; no categories.
    int Part = 0;
    ref array<string> Only;
    // The goods as short rows (OZT_WireRows), each naming its category by its
    // place in RowCats; Goods is empty on the wire then and filled on arrival.
    ref array<string> Rows;
    ref array<string> RowCats;
    // The head of an open sent by pages (OZT_OpenPage): the request's number,
    // and the lines of the book the pages will go through, for the screen's
    // progress. No goods come with the head; RowCats names the pages' categories.
    int Ask = 0;
    int Lines = 0;

    void OZT_OpenRes()
    {
        Condition = new OZT_ConditionFactors();
        Categories = new array<ref OZT_WireCategory>();
        Goods = new array<ref OZT_WireGoods>();
        Yours = new array<ref OZT_WireOwn>();
        Notes = new array<string>();
        Licences = new array<string>();
        Only = new array<string>();
        Rows = new array<string>();
        RowCats = new array<string>();
    }
}

// A page of an open's goods, after its head (OZT_OpenRes with Ask): rows in the
// book's order, the lines of the book they have gone through so far, and More 0
// on the last page.
class OZT_OpenPage
{
    string Trader = "";
    int Ask = 0;
    int Seq = 0;
    int Done = 0;
    int More = 0;
    ref array<string> Rows;

    void OZT_OpenPage()
    {
        Rows = new array<string>();
    }
}

// The goods of an open as short rows. A record of OZT_WireGoods costs some 150
// bytes of keys and empty fields as JSON; a row is its values alone, about 25:
//   ClassName|cat|Buy|Sell|Stock|Level|Rule|With|Name|Look
// `cat` is the category's place in RowCats, With is comma-separated, an empty
// field is "-", and the empty fields at the end are left off. A "|" in a name is
// written as "/". Every field has something in it, so splitting never meets two
// separators side by side.
class OZT_WireRows
{
    static void Pack(OZT_OpenRes res)
    {
        map<string, int> at = new map<string, int>();
        res.RowCats.Clear();
        res.Rows.Clear();
        for (int i = 0; i < res.Goods.Count(); i++)
        {
            OZT_WireGoods g = res.Goods[i];
            int c;
            if (!at.Find(g.Category, c))
            {
                c = res.RowCats.Count();
                at.Set(g.Category, c);
                res.RowCats.Insert(g.Category);
            }
            res.Rows.Insert(Row(g, c));
        }
        res.Goods.Clear();
    }

    static string Row(OZT_WireGoods g, int cat)
    {
        array<string> f = new array<string>();
        f.Insert(g.ClassName);
        f.Insert(cat.ToString());
        f.Insert(g.Buy.ToString());
        f.Insert(g.Sell.ToString());
        f.Insert(g.Stock.ToString());
        f.Insert(g.Level.ToString());
        f.Insert(Field(g.Rule));
        string with = "";
        if (g.With)
        {
            for (int w = 0; w < g.With.Count(); w++)
            {
                if (w > 0)
                    with += ",";
                with += g.With[w];
            }
        }
        f.Insert(Field(with));
        f.Insert(Field(g.Name));
        f.Insert(Field(g.Look));
        int n = f.Count();
        while (n > 6 && f[n - 1] == "-")
            n--;
        string row = f[0];
        for (int k = 1; k < n; k++)
            row += "|" + f[k];
        return row;
    }

    protected static string Field(string s)
    {
        if (s == "")
            return "-";
        string t = s;
        t.Replace("|", "/");
        return t;
    }

    protected static string Value(string s)
    {
        if (s == "-")
            return "";
        return s;
    }

    // The rows back into records, after the goods that came as records (an
    // answer has one form or the other). The arrived lists may be absent.
    static void Unpack(OZT_OpenRes res)
    {
        if (!res.Goods)
            res.Goods = new array<ref OZT_WireGoods>();
        if (!res.Rows || res.Rows.Count() == 0)
            return;
        UnpackRows(res.Rows, res.RowCats, res.Goods);
        res.Rows.Clear();
    }

    // Rows into records at the end of `into`, the categories named by `cats`
    // (a page's rows by its head's RowCats).
    static void UnpackRows(array<string> rows, array<string> cats, array<ref OZT_WireGoods> into)
    {
        if (!rows || !into)
            return;
        array<string> f = new array<string>();
        for (int i = 0; i < rows.Count(); i++)
        {
            f.Clear();
            string row = rows[i];
            row.Split("|", f);
            if (f.Count() < 6)
                continue;
            OZT_WireGoods g = new OZT_WireGoods();
            g.ClassName = f[0];
            int c = f[1].ToInt();
            if (cats && c >= 0 && c < cats.Count())
                g.Category = cats[c];
            g.Buy = f[2].ToInt();
            g.Sell = f[3].ToInt();
            g.Stock = f[4].ToInt();
            g.Level = f[5].ToInt();
            if (f.Count() > 6)
                g.Rule = Value(f[6]);
            if (f.Count() > 7)
            {
                string with = Value(f[7]);
                if (with != "")
                    with.Split(",", g.With);
            }
            if (f.Count() > 8)
                g.Name = Value(f[8]);
            if (f.Count() > 9)
                g.Look = Value(f[9]);
            into.Insert(g);
        }
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

    // A committed deal: the classes whose stock it moved, for the screen's part
    // of the open that follows (OZT_OpenReq.Part).
    ref array<string> Moved;

    void OZT_DealRes()
    {
        SellPrices = new array<int>();
        BuyPrices = new array<int>();
        Moved = new array<string>();
    }
}
