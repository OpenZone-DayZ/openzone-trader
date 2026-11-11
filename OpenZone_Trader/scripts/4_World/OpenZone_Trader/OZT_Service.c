// The service `trader` in the core's registry: what a client asks.
//
// The core's dispatcher checks only what every service shares (the sender's
// identity, the size of the body). Who may trade, where and what is checked
// here. A refusal is a string-table key, never a sentence.

class OZT_Open
{
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
        res.WearBought = OZT_Config.Get().Settings().WearBought;
        if (money.Physical())
        {
            array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
            for (int n = 0; n < notes.Count(); n++)
                res.Notes.Insert(notes[n].ClassName);
        }
        res.Condition = OZT_Config.Get().Settings().Condition.Copy();

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
                OZT_BookEntry e = OZT_Book.EntryOf(cat.Goods[g].ClassName);
                if (!e)
                    continue;
                OZT_WireGoods wg = new OZT_WireGoods();
                wg.ClassName = e.ClassName;
                wg.Category = e.Category;
                wg.Buy = OZT_Book.BuyPrice(trader, e, 1);
                if (e.Sell > 0)
                    wg.Sell = OZT_Config.RoundDownTo(e.Sell * trader.SellFactor, unit);
                wg.Rule = e.Rule;
                if (wg.Buy <= 0 && wg.Sell <= 0)
                    continue;
                res.Goods.Insert(wg);
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
            OZT_BookEntry entry = OZT_Book.EntryOf(own.GetType());
            int units = OZT_Stack.Units(own);
            int price = OZT_Book.SellPrice(trader, entry, own, units);
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
