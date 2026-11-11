// The deal the player is putting together: what is bought and what is sold.
//
// The cart holds no price of its own. A line shows what the server's quote
// said for it; until the quote answers, a bought line shows the catalogue's
// price for one times the count (the server's own rule for a buy), and a sold
// line shows what `open` said for the whole thing.

class OZT_CartLine
{
    bool Selling;
    string Type;

    // A sold thing: the entity and its network id.
    EntityAI Item;
    int Low;
    int High;

    // Bought: pieces, or units under the unit rule. Sold: units of a stack
    // (0: the whole thing).
    int Count;

    // Sold: the units of the stack and the price of the whole thing, as `open`
    // said. Bought: the price of one.
    int Units;
    int Whole;
    int Each;

    // What the last quote said for the line; below zero until it answers.
    int Price;

    // What the line shows before a quote answers.
    int Shown()
    {
        if (Price >= 0)
            return Price;
        if (!Selling)
            return Each * Count;
        if (Count == 0 || Count >= Units)
            return Whole;
        return -1;
    }
}

class OZT_Cart
{
    ref array<ref OZT_CartLine> Buys;
    ref array<ref OZT_CartLine> Sells;

    // Grows with every change: an answer to a quote is for the cart as it was
    // when the quote was asked.
    int Revision;

    void OZT_Cart()
    {
        Buys = new array<ref OZT_CartLine>();
        Sells = new array<ref OZT_CartLine>();
    }

    bool IsEmpty()
    {
        return Buys.Count() == 0 && Sells.Count() == 0;
    }

    void Clear()
    {
        Buys.Clear();
        Sells.Clear();
        Revision++;
    }

    protected void MarkChanged()
    {
        Revision++;
        for (int b = 0; b < Buys.Count(); b++)
            Buys[b].Price = -1;
        for (int s = 0; s < Sells.Count(); s++)
            Sells[s].Price = -1;
    }

    // More of a class into the BUY cart; one line per class.
    void Buy(string type, int count, int each, int most)
    {
        OZT_CartLine line;
        for (int i = 0; i < Buys.Count(); i++)
        {
            if (Buys[i].Type == type)
                line = Buys[i];
        }
        if (!line)
        {
            line = new OZT_CartLine();
            line.Type = type;
            line.Each = each;
            Buys.Insert(line);
        }
        line.Count = line.Count + count;
        if (line.Count > most)
            line.Count = most;
        MarkChanged();
    }

    void BuyCount(int index, int count)
    {
        if (index < 0 || index >= Buys.Count())
            return;
        if (count <= 0)
            Buys.RemoveOrdered(index);
        else
            Buys[index].Count = count;
        MarkChanged();
    }

    OZT_CartLine SellOf(EntityAI item)
    {
        for (int i = 0; i < Sells.Count(); i++)
        {
            if (Sells[i].Item == item)
                return Sells[i];
        }
        return null;
    }

    // A thing of the player's into the SELL cart: `count` units of a stack,
    // or 0 for the whole of it.
    void Sell(EntityAI item, int low, int high, int count, int units, int whole)
    {
        OZT_CartLine line = SellOf(item);
        if (!line)
        {
            line = new OZT_CartLine();
            line.Selling = true;
            line.Item = item;
            line.Type = item.GetType();
            line.Low = low;
            line.High = high;
            Sells.Insert(line);
        }
        line.Units = units;
        line.Whole = whole;
        line.Count = count;
        if (units > 0 && count >= units)
            line.Count = 0;
        MarkChanged();
    }

    void Unsell(int index)
    {
        if (index < 0 || index >= Sells.Count())
            return;
        Sells.RemoveOrdered(index);
        MarkChanged();
    }

    // How many units of a stack the cart sells: all of them when the whole
    // stack is sold.
    int SoldUnits(EntityAI item, int units)
    {
        OZT_CartLine line = SellOf(item);
        if (!line)
            return 0;
        if (line.Count == 0)
            return units;
        return line.Count;
    }

    // Drops the sold lines whose thing is gone or is no longer taken: after a
    // deal, or when something changed under an open screen. Returns how many.
    int Prune(OZT_OpenRes data)
    {
        int dropped = 0;
        for (int i = Sells.Count() - 1; i >= 0; i--)
        {
            OZT_CartLine line = Sells[i];
            bool keep = false;
            if (line.Item)
            {
                for (int y = 0; y < data.Yours.Count(); y++)
                {
                    OZT_WireOwn own = data.Yours[y];
                    if (own.Low == line.Low && own.High == line.High && own.Why == 0)
                    {
                        keep = true;
                        line.Units = own.Units;
                        line.Whole = own.Price;
                        if (line.Count > 0 && line.Count >= own.Units)
                            line.Count = 0;
                    }
                }
            }
            if (!keep)
            {
                Sells.RemoveOrdered(i);
                dropped++;
            }
        }
        for (int b = Buys.Count() - 1; b >= 0; b--)
        {
            bool sold = false;
            for (int g = 0; g < data.Goods.Count(); g++)
            {
                if (data.Goods[g].ClassName == Buys[b].Type && data.Goods[g].Buy > 0)
                {
                    sold = true;
                    Buys[b].Each = data.Goods[g].Buy;
                }
            }
            if (!sold)
            {
                Buys.RemoveOrdered(b);
                dropped++;
            }
        }
        if (dropped > 0)
            MarkChanged();
        return dropped;
    }

    // The request of a quote or of a deal, as JSON.
    string Request(string trader, int net)
    {
        OZT_DealReq req = new OZT_DealReq();
        req.Trader = trader;
        req.Net = net;
        for (int s = 0; s < Sells.Count(); s++)
        {
            OZT_WireSell ws = new OZT_WireSell();
            ws.Low = Sells[s].Low;
            ws.High = Sells[s].High;
            ws.Count = Sells[s].Count;
            req.Sells.Insert(ws);
        }
        for (int b = 0; b < Buys.Count(); b++)
        {
            OZT_WireBuy wb = new OZT_WireBuy();
            wb.ClassName = Buys[b].Type;
            wb.Count = Buys[b].Count;
            req.Buys.Insert(wb);
        }
        string json;
        string err;
        if (!JsonFileLoader<OZT_DealReq>.MakeData(req, json, err, false))
        {
            OZT_Log.Error("the deal's request could not be written: " + err);
            return "";
        }
        return json;
    }

    // What a quote said, line by line. Lines the answer does not reach (a
    // refusal stops reading) stay unanswered.
    void Take(OZT_DealRes res)
    {
        for (int s = 0; s < Sells.Count(); s++)
        {
            if (s < res.SellPrices.Count())
                Sells[s].Price = res.SellPrices[s];
        }
        for (int b = 0; b < Buys.Count(); b++)
        {
            if (b < res.BuyPrices.Count())
                Buys[b].Price = res.BuyPrices[b];
        }
    }
}
