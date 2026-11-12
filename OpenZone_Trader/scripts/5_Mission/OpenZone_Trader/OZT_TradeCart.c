// The deal the player is putting together: what is bought and what is sold.
//
// The cart holds no price of its own. A line shows what the server's quote
// said for it; until the quote answers, a bought line shows the catalogue's
// price for one times the count (the server's own rule for a buy), and a sold
// line shows what `open` said the thing brings as it is, with everything on it.

// A part fitted to the thing a BUY line buys: goods of the trader at their
// own price, hanging on the thing (On below zero) or on an earlier part.
class OZT_CartFit
{
    string Type;
    string SlotName;
    int On;
    int Each;
}

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

    // Bought: the condition of the stock line it comes from (design §11.1),
    // 0 for everything that is not stocked.
    int Level;

    // Bought: the parts fitted to the thing, and, for a part bought onto a
    // thing the player carries, that thing and the slot.
    ref array<ref OZT_CartFit> Fits;
    int OnLow;
    int OnHigh;
    string OnSlot;

    // Sold: the parts that come off and stay, and the things whose loaded
    // rounds stay (Rounds 1).
    ref array<ref OZT_WireKeep> Keep;

    void OZT_CartLine()
    {
        Fits = new array<ref OZT_CartFit>();
        Keep = new array<ref OZT_WireKeep>();
    }

    bool IsOnto()
    {
        return OnLow != 0 || OnHigh != 0;
    }

    // What the line shows before a quote answers.
    int Shown()
    {
        if (Price >= 0)
            return Price;
        if (!Selling)
        {
            int sum = Each * Count;
            for (int i = 0; i < Fits.Count(); i++)
                sum = sum + Fits[i].Each;
            return sum;
        }
        if (Count == 0 || Count >= Units)
            return Whole;
        return -1;
    }

    // Does the line keep this part, or the rounds loaded in this thing?
    bool Keeps(int low, int high, int rounds)
    {
        for (int i = 0; i < Keep.Count(); i++)
        {
            OZT_WireKeep k = Keep[i];
            if (k.Low == low && k.High == high && k.Rounds == rounds)
                return true;
        }
        return false;
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

    // More of a class into the BUY cart; one line per class, besides the
    // lines that carry parts or go onto a thing (those buy one each).
    void Buy(string type, int count, int each, int most, int level = 0)
    {
        OZT_CartLine line;
        for (int i = 0; i < Buys.Count(); i++)
        {
            OZT_CartLine have = Buys[i];
            if (have.Type == type && have.Level == level && have.Fits.Count() == 0 && !have.IsOnto())
                line = have;
        }
        if (!line)
        {
            line = new OZT_CartLine();
            line.Type = type;
            line.Each = each;
            line.Level = level;
            Buys.Insert(line);
        }
        line.Count = line.Count + count;
        if (line.Count > most)
            line.Count = most;
        MarkChanged();
    }

    // One piece of goods on a line of its own, to fit parts to. Returns the
    // line's index.
    int BuyOne(string type, int each)
    {
        OZT_CartLine line = new OZT_CartLine();
        line.Type = type;
        line.Each = each;
        line.Count = 1;
        Buys.Insert(line);
        MarkChanged();
        return Buys.Count() - 1;
    }

    // A line that buys more than one piece gives one piece a line of its own,
    // to fit parts to. Returns the index of the line of one.
    int SplitOne(int index)
    {
        if (index < 0 || index >= Buys.Count())
            return -1;
        OZT_CartLine line = Buys[index];
        if (line.Count <= 1)
            return index;
        line.Count = line.Count - 1;
        int one = BuyOne(line.Type, line.Each);
        Buys[one].Level = line.Level;
        return one;
    }

    // How many of a class in a condition the cart buys, on every line.
    int Buying(string type, int level)
    {
        int n = 0;
        for (int i = 0; i < Buys.Count(); i++)
        {
            OZT_CartLine line = Buys[i];
            if (line.Type == type && line.Level == level)
                n = n + line.Count;
        }
        return n;
    }

    void AddFit(int index, string type, string slot, int on, int each)
    {
        if (index < 0 || index >= Buys.Count())
            return;
        OZT_CartFit fit = new OZT_CartFit();
        fit.Type = type;
        fit.SlotName = slot;
        fit.On = on;
        fit.Each = each;
        Buys[index].Fits.Insert(fit);
        MarkChanged();
    }

    // A fitted part off its line, and every part that hangs on it.
    void DropFit(int index, int fit)
    {
        if (index < 0 || index >= Buys.Count())
            return;
        array<ref OZT_CartFit> fits = Buys[index].Fits;
        if (fit < 0 || fit >= fits.Count())
            return;
        array<int> gone = new array<int>();
        gone.Insert(fit);
        bool grew = true;
        while (grew)
        {
            grew = false;
            for (int i = 0; i < fits.Count(); i++)
            {
                if (gone.Find(i) < 0 && gone.Find(fits[i].On) > -1)
                {
                    gone.Insert(i);
                    grew = true;
                }
            }
        }
        array<ref OZT_CartFit> kept = new array<ref OZT_CartFit>();
        array<int> moved = new array<int>();
        for (int k = 0; k < fits.Count(); k++)
        {
            if (gone.Find(k) > -1)
            {
                moved.Insert(-1);
                continue;
            }
            moved.Insert(kept.Count());
            kept.Insert(fits[k]);
        }
        for (int m = 0; m < kept.Count(); m++)
        {
            if (kept[m].On >= 0)
                kept[m].On = moved[kept[m].On];
        }
        Buys[index].Fits = kept;
        MarkChanged();
    }

    // A part bought onto a thing the player carries. Returns the line.
    int BuyOnto(string type, int each, int low, int high, string slot)
    {
        OZT_CartLine line = new OZT_CartLine();
        line.Type = type;
        line.Each = each;
        line.Count = 1;
        line.OnLow = low;
        line.OnHigh = high;
        line.OnSlot = slot;
        Buys.Insert(line);
        MarkChanged();
        return Buys.Count() - 1;
    }

    // Keep a part of a sold thing, or its loaded rounds, or sell it again.
    void ToggleKeep(int index, int low, int high, int rounds)
    {
        if (index < 0 || index >= Sells.Count())
            return;
        OZT_CartLine line = Sells[index];
        for (int i = 0; i < line.Keep.Count(); i++)
        {
            OZT_WireKeep k = line.Keep[i];
            if (k.Low == low && k.High == high && k.Rounds == rounds)
            {
                line.Keep.RemoveOrdered(i);
                MarkChanged();
                return;
            }
        }
        OZT_WireKeep add = new OZT_WireKeep();
        add.Low = low;
        add.High = high;
        add.Rounds = rounds;
        line.Keep.Insert(add);
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
                    if (own.Low == line.Low && own.High == line.High)
                    {
                        keep = true;
                        line.Units = own.Units;
                        line.Whole = own.Price;
                        if (own.Tree > 0)
                            line.Whole = own.Tree;
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
            OZT_CartLine onto = Buys[b];
            if (onto.IsOnto() && !GetGame().GetObjectByNetworkId(onto.OnLow, onto.OnHigh))
            {
                Buys.RemoveOrdered(b);
                dropped++;
                continue;
            }
            // The line's own record: its class in its condition, still for
            // sale and not sold out (design §11.1).
            bool sold = false;
            for (int g = 0; g < data.Goods.Count(); g++)
            {
                OZT_WireGoods rec = data.Goods[g];
                if (rec.ClassName == onto.Type && rec.Level == onto.Level && rec.Buy > 0 && rec.Stock != 0)
                {
                    sold = true;
                    onto.Each = rec.Buy;
                }
            }
            if (!sold)
            {
                Buys.RemoveOrdered(b);
                dropped++;
            }
        }
        dropped = dropped + CutToStock(data);
        if (dropped > 0)
            MarkChanged();
        return dropped;
    }

    // Lines that buy more of a stock line than the shelf now holds give the
    // rest up, the last line first: somebody else bought it meanwhile.
    // Returns how many lines changed.
    protected int CutToStock(OZT_OpenRes data)
    {
        int cut = 0;
        for (int g = 0; g < data.Goods.Count(); g++)
        {
            OZT_WireGoods rec = data.Goods[g];
            if (rec.Stock < 0)
                continue;
            int over = Buying(rec.ClassName, rec.Level) - rec.Stock;
            for (int b = Buys.Count() - 1; b >= 0; b--)
            {
                if (over <= 0)
                    break;
                OZT_CartLine line = Buys[b];
                if (line.Type != rec.ClassName || line.Level != rec.Level)
                    continue;
                int off = over;
                if (off > line.Count)
                    off = line.Count;
                line.Count = line.Count - off;
                over = over - off;
                if (line.Count <= 0)
                    Buys.RemoveOrdered(b);
                cut++;
            }
        }
        return cut;
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
            for (int k = 0; k < Sells[s].Keep.Count(); k++)
                ws.Keep.Insert(Sells[s].Keep[k].Copy());
            req.Sells.Insert(ws);
        }
        for (int b = 0; b < Buys.Count(); b++)
        {
            OZT_CartLine line = Buys[b];
            OZT_WireBuy wb = new OZT_WireBuy();
            wb.ClassName = line.Type;
            wb.Count = line.Count;
            wb.OnLow = line.OnLow;
            wb.OnHigh = line.OnHigh;
            wb.SlotName = line.OnSlot;
            wb.Level = line.Level;
            for (int f = 0; f < line.Fits.Count(); f++)
            {
                OZT_WireFit wf = new OZT_WireFit();
                wf.ClassName = line.Fits[f].Type;
                wf.SlotName = line.Fits[f].SlotName;
                wf.On = line.Fits[f].On;
                wb.Fit.Insert(wf);
            }
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
