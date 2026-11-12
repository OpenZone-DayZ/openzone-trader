// The service `trader` in the core's registry: what a client asks.
//
// The core's dispatcher checks only what every service shares (the sender's
// identity, the size of the body). Who may trade, where and what is checked
// here. A refusal is a string-table key, never a sentence.

class OZT_Open
{
    // One goods record of the answer: what one costs here with what comes with
    // it, and what that is; for a stocked class, of one condition and with the
    // count the stock holds. Locked goods are not shown to this player (design
    // §8.2): nothing to buy, and the shelf shows nothing it cannot sell. Null:
    // nothing to show.
    protected static OZT_WireGoods Record(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int level, int unit)
    {
        OZT_WireGoods wg = new OZT_WireGoods();
        wg.ClassName = e.ClassName;
        wg.Category = e.Category;
        wg.Level = level;
        OZT_LicenceDef licence = OZT_Licences.Def(OZT_Licences.IdOf(e.ClassName));
        if (licence)
        {
            wg.Name = licence.Name;
            wg.Look = licence.Look;
        }
        if (e.Stocked)
            wg.Stock = OZT_Book.StockOf(trader, e, level, null);
        wg.Buy = OZT_Book.ShelfWith(player, trader, e, level);
        if (level == 0)
        {
            array<string> comes;
            OZT_Book.ComesAt(player, trader, e, 1, null, comes);
            for (int w = 0; w < comes.Count(); w++)
                wg.With.Insert(comes[w]);
        }
        if (e.Sell > 0)
            wg.Sell = OZT_Config.RoundDownTo(e.Sell * trader.SellFactor, unit);
        wg.Rule = e.Rule;
        if (wg.Buy <= 0 && wg.Sell <= 0)
            return null;
        return wg;
    }

    // The answer of `open`: the trader, what this player may buy and sell here
    // and for how much, and the player's money. Prices carry the trader's own
    // factors already.
    static string Answer(PlayerBase player, string json, out bool ok, out string error)
    {
        ok = false;
        error = OZT_Const.NO_INTERNAL;

        OZT_OpenReq req = new OZT_OpenReq();
        string err;
        if (!JsonFileLoader<OZT_OpenReq>.LoadData(json, req, err))
        {
            OZT_Log.Dbg("open refused: unreadable body (" + err + ")");
            return "";
        }

        OZT_Point trader = OZT_Config.Get().FindTrader(req.Trader);
        float away = -1;
        if (trader)
            away = OZT_Points.Away(trader.Id, player);
        if (away < 0)
        {
            error = OZT_Const.NO_CLOSED;
            return "";
        }
        if (away > trader.Radius)
        {
            error = OZT_Const.NO_TOO_FAR;
            return "";
        }
        if (!OZT_Gates.Admits(player, trader))
        {
            error = OZT_Const.NO_ACCESS;
            return "";
        }

        OZT_MoneyProvider money = OZT_Money.Of(trader);
        if (!money)
            return "";

        OZT_OpenRes res = new OZT_OpenRes();
        res.Revision = OZT_Config.Get().Revision();
        res.Trader = trader.Id;
        res.Name = trader.Name;
        res.Radius = trader.Radius;
        res.Provider = money.Id();
        if (money.Physical())
            res.Physical = 1;
        res.Unit = money.Unit();
        res.Label = money.Label();
        res.Balance = money.Balance(player);
        if (money.Then())
            res.Bank = money.Then().Balance(player);
        res.WearBought = OZT_Config.Get().Settings().WearBought;
        if (money.Physical())
        {
            array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
            for (int n = 0; n < notes.Count(); n++)
                res.Notes.Insert(notes[n].ClassName);
        }
        res.Condition = OZT_Config.Get().Settings().Condition.Copy();
        array<string> granted = OZT_Licences.Of(OZT_Licences.UidOf(player));
        for (int h = 0; h < granted.Count(); h++)
            res.Licences.Insert(granted[h]);

        int unit = OZT_Config.Get().Unit();
        for (int c = 0; c < trader.Categories.Count(); c++)
        {
            OZT_Category cat = OZT_Config.Get().FindCategory(trader.Categories[c]);
            if (!cat)
                continue;
            OZT_WireCategory wc = new OZT_WireCategory();
            wc.Id = cat.Id;
            wc.Name = cat.Name;
            res.Categories.Insert(wc);

            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                // A class in two of his categories stands in the first only:
                // its line there is the one his deals use.
                OZT_BookEntry e = OZT_Book.EntryFor(trader, cat.Goods[g].ClassName);
                if (!e || e.Category != cat.Id)
                    continue;
                if (!e.Stocked)
                {
                    OZT_WireGoods wg = Record(player, trader, e, 0, unit);
                    if (wg)
                        res.Goods.Insert(wg);
                    continue;
                }
                // A stocked class (design §11.1): a record per condition the
                // stock holds; nothing held, one record that says so.
                bool any = false;
                for (int lv = 0; lv < OZT_Const.LEVEL_RUINED; lv++)
                {
                    if (OZT_Book.StockOf(trader, e, lv, null) <= 0)
                        continue;
                    OZT_WireGoods held = Record(player, trader, e, lv, unit);
                    if (!held)
                        continue;
                    res.Goods.Insert(held);
                    any = true;
                }
                if (any)
                    continue;
                OZT_WireGoods none = Record(player, trader, e, 0, unit);
                if (none)
                    res.Goods.Insert(none);
            }
        }

        // The change tab: the server's notes at face value, when this trader
        // changes them. The screen names the tab itself.
        if (OZT_Book.Changes(trader))
        {
            OZT_WireCategory change = new OZT_WireCategory();
            change.Id = OZT_Const.CATEGORY_CHANGE;
            change.Name = "";
            res.Categories.Insert(change);
            array<ref OZT_Note> changed = OZT_Config.Get().Settings().Notes;
            for (int m = 0; m < changed.Count(); m++)
            {
                OZT_WireGoods note = new OZT_WireGoods();
                note.ClassName = changed[m].ClassName;
                note.Category = OZT_Const.CATEGORY_CHANGE;
                note.Buy = changed[m].Value;
                note.Sell = changed[m].Value;
                note.Rule = OZT_Const.RULE_UNIT;
                res.Goods.Insert(note);
            }
        }

        // What the player carries that this trader pays for, each with what it
        // brings as it is now. The screen marks everything else as not taken.
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI own = all[i];
            if (!own || own == player)
                continue;
            OZT_BookEntry entry = OZT_Book.EntryFor(trader, own.GetType());
            int units = OZT_Stack.Units(own);
            // A stocked line pays at the stock it holds now, as the deal will
            // (design §11.1): no ceiling, so where the curve brings less than
            // the smallest note the trader takes nothing and the screen says so.
            int price = OZT_Book.SellPriceAt(trader, entry, own, units, null);
            if (price <= 0)
                continue;
            OZT_WireOwn wo = new OZT_WireOwn();
            int low;
            int high;
            own.GetNetworkID(low, high);
            wo.Low = low;
            wo.High = high;
            wo.Price = price;
            if (OZT_Stack.Countable(own))
            {
                wo.Units = units;
                wo.Tree = price;
            }
            else
            {
                // Sold whole, with nothing else in the deal: what hangs on it
                // and is loaded in it is paid by its own lines, and what lies
                // inside it stands in the way.
                OZT_Tree tree = OZT_Tree.Read(trader, own, null, null);
                wo.Tree = tree.Total;
                int loaded;
                int loadedPrice;
                tree.LoadedIn(own, loaded, loadedPrice);
                wo.Loaded = loaded;
                wo.LoadedPrice = loadedPrice;
                if (tree.Inside > 0)
                    wo.Why = 2;
            }
            res.Yours.Insert(wo);
        }

        string body;
        if (!JsonFileLoader<OZT_OpenRes>.MakeData(res, body, err, false))
        {
            OZT_Log.Error("open: the answer could not be written: " + err);
            return "";
        }
        ok = true;
        error = "";
        return body;
    }
}

class OZT_Service : OZ_ServiceHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";
        if (!sender)
            return "";

        // The player is the SENDER's, never something the body names.
        PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
        if (!player)
        {
            error = OZT_Const.NO_INTERNAL;
            return "";
        }

        if (op == OZT_Const.OP_OPEN)
            return OZT_Open.Answer(player, json, ok, error);
        if (op == OZT_Const.OP_DEAL)
            return OZT_Deal.Run(player, json, true, ok, error);
        // The same request, with nothing changed and the net not compared:
        // what the screen shows before Confirm is the server's own answer.
        if (op == OZT_Const.OP_QUOTE)
            return OZT_Deal.Run(player, json, false, ok, error);
        // -- operations of the later tasks go above this line --
        return "";
    }
}
