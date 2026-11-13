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
        // OZT_Book.ShelfWith, with what comes along asked once and only for
        // goods that name something: it was asked twice a record, an array or
        // two made each time (load test, 2026-11-13).
        int own = OZT_Book.ShelfPrice(player, trader, e, level);
        wg.Buy = own;
        if (level == 0 && e.With && e.With.Count() > 0)
        {
            array<string> comes;
            OZT_Book.ComesAt(player, trader, e, 1, null, comes);
            for (int w = 0; w < comes.Count(); w++)
            {
                wg.With.Insert(comes[w]);
                if (own > 0)
                    wg.Buy = wg.Buy + OZT_Book.ShelfPrice(player, trader, OZT_Book.EntryFor(trader, comes[w]), 0);
            }
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
        return AnswerTo(player, req, ok, error);
    }

    // The records of one line of the book at this trader: a stocked class has a
    // record per condition the stock holds, or one that says nothing is held.
    static void AddRecords(PlayerBase player, OZT_Point trader, OZT_BookEntry e, int unit, array<ref OZT_WireGoods> into)
    {
        if (!e.Stocked)
        {
            OZT_WireGoods wg = Record(player, trader, e, 0, unit);
            if (wg)
                into.Insert(wg);
            return;
        }
        bool any = false;
        for (int lv = 0; lv < OZT_Const.LEVEL_RUINED; lv++)
        {
            if (OZT_Book.StockOf(trader, e, lv, null) <= 0)
                continue;
            OZT_WireGoods held = Record(player, trader, e, lv, unit);
            if (!held)
                continue;
            into.Insert(held);
            any = true;
        }
        if (any)
            return;
        OZT_WireGoods none = Record(player, trader, e, 0, unit);
        if (none)
            into.Insert(none);
    }

    // Why this player may not have the open of this trader now, as a key of the
    // string table; "" when they may.
    static string Refusal(PlayerBase player, OZT_Point trader)
    {
        float away = -1;
        if (trader)
            away = OZT_Points.Away(trader.Id, player);
        if (away < 0)
            return OZT_Const.NO_CLOSED;
        if (away > trader.Radius)
            return OZT_Const.NO_TOO_FAR;
        if (!OZT_Gates.Admits(player, trader))
            return OZT_Const.NO_ACCESS;
        return "";
    }

    // The head of an answer: the trader, the money and what the player holds of
    // it, the licences; no goods and nothing the player carries.
    static OZT_OpenRes HeadOf(PlayerBase player, OZT_Point trader, OZT_MoneyProvider money)
    {
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
        return res;
    }

    // The categories of a trader's shelf in its order, the change tab last when
    // the trader changes notes.
    static void CategoriesOf(OZT_Point trader, array<ref OZT_WireCategory> into)
    {
        for (int c = 0; c < trader.Categories.Count(); c++)
        {
            OZT_Category cat = OZT_Config.Get().FindCategory(trader.Categories[c]);
            if (!cat)
                continue;
            OZT_WireCategory wc = new OZT_WireCategory();
            wc.Id = cat.Id;
            wc.Name = cat.Name;
            into.Insert(wc);
        }
        if (OZT_Book.Changes(trader))
        {
            OZT_WireCategory change = new OZT_WireCategory();
            change.Id = OZT_Const.CATEGORY_CHANGE;
            change.Name = "";
            into.Insert(change);
        }
    }

    // The change tab: the server's notes at face value, when this trader
    // changes them. The screen names the tab itself.
    static void NotesOf(OZT_Point trader, array<ref OZT_WireGoods> into)
    {
        if (!OZT_Book.Changes(trader))
            return;
        array<ref OZT_Note> changed = OZT_Config.Get().Settings().Notes;
        for (int m = 0; m < changed.Count(); m++)
        {
            OZT_WireGoods note = new OZT_WireGoods();
            note.ClassName = changed[m].ClassName;
            note.Category = OZT_Const.CATEGORY_CHANGE;
            note.Buy = changed[m].Value;
            note.Sell = changed[m].Value;
            note.Rule = OZT_Const.RULE_UNIT;
            into.Insert(note);
        }
    }

    // What the player carries that this trader pays for, each with what it
    // brings as it is now. The screen marks everything else as not taken.
    static void YoursOf(PlayerBase player, OZT_Point trader, array<ref OZT_WireOwn> into)
    {
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
            into.Insert(wo);
        }
    }

    // The answer to a request already read, whole in one message: a part of an
    // open (the classes in req.Only, and the player's side whole), or a whole
    // open for the stand's verb. A screen's whole open goes by pages
    // (OZT_OpenStreams).
    static string AnswerTo(PlayerBase player, OZT_OpenReq req, out bool ok, out string error)
    {
        ok = false;
        error = OZT_Const.NO_INTERNAL;
        string err;
        bool part = req.Part == 1;

        OZT_Point trader = OZT_Config.Get().FindTrader(req.Trader);
        string refused = Refusal(player, trader);
        if (refused != "")
        {
            error = refused;
            return "";
        }

        OZT_MoneyProvider money = OZT_Money.Of(trader);
        if (!money)
            return "";

        // How long a big book takes (TickCount ticks at 10 MHz): the goods, the
        // player's things, the writing of the answer, each said in the debug log.
        int t0 = TickCount(0);
        OZT_OpenRes res = HeadOf(player, trader, money);

        int unit = OZT_Config.Get().Unit();
        if (part)
        {
            // The classes asked for, each once, at the line this trader deals by;
            // a class it no longer deals in gets no record, and the screen drops
            // what it had of it.
            res.Part = 1;
            map<string, bool> asked = new map<string, bool>();
            if (req.Only)
            {
                for (int q = 0; q < req.Only.Count(); q++)
                {
                    string type = req.Only[q];
                    if (type == "" || asked.Contains(type))
                        continue;
                    asked.Set(type, true);
                    res.Only.Insert(type);
                    OZT_BookEntry pe = OZT_Book.EntryFor(trader, type);
                    if (!pe || trader.Categories.Find(pe.Category) < 0)
                        continue;
                    AddRecords(player, trader, pe, unit, res.Goods);
                }
            }
        }
        else
        {
            CategoriesOf(trader, res.Categories);
            for (int c = 0; c < trader.Categories.Count(); c++)
            {
                OZT_Category cat = OZT_Config.Get().FindCategory(trader.Categories[c]);
                if (!cat)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    // A class in two of the trader's categories stands in the
                    // first only: its line there is the one the deals use.
                    OZT_BookEntry e = OZT_Book.EntryFor(trader, cat.Goods[g].ClassName);
                    if (!e || e.Category != cat.Id)
                        continue;
                    AddRecords(player, trader, e, unit, res.Goods);
                }
            }
        }

        int tGoods = TickCount(t0);

        if (!part)
            NotesOf(trader, res.Goods);
        YoursOf(player, trader, res.Yours);
        int tYours = TickCount(t0);

        int records = res.Goods.Count();
        OZT_WireRows.Pack(res);
        string body;
        if (!JsonFileLoader<OZT_OpenRes>.MakeData(res, body, err, false))
        {
            OZT_Log.Error("open: the answer could not be written: " + err);
            return "";
        }
        if (OZ_Log.IsDebug())
        {
            int tAll = TickCount(t0);
            string kind = "open ";
            if (part)
                kind = "open part ";
            string said = kind + trader.Id + ": " + records.ToString() + " record(s), ";
            said = said + body.Length().ToString() + " B in " + Ms(tAll) + " ms (goods " + Ms(tGoods);
            said = said + ", yours " + Ms(tYours - tGoods);
            said = said + ", json " + Ms(tAll - tYours) + ")";
            OZT_Log.Dbg(said);
        }
        ok = true;
        error = "";
        return body;
    }

    // TickCount ticks as milliseconds with one decimal.
    static string Ms(int ticks)
    {
        int tenths = ticks / 1000;
        int whole = tenths / 10;
        int part = tenths - whole * 10;
        return whole.ToString() + "." + part.ToString();
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
        {
            OZT_OpenReq req = new OZT_OpenReq();
            string err;
            if (!JsonFileLoader<OZT_OpenReq>.LoadData(json, req, err))
            {
                error = OZT_Const.NO_INTERNAL;
                OZT_Log.Dbg("open refused: unreadable body (" + err + ")");
                return "";
            }
            // A part, or an open from a screen that does not number them: the
            // answer whole, now.
            if (req.Part == 1 || req.Ask <= 0)
                return OZT_Open.AnswerTo(player, req, ok, error);
            // A screen's whole open: its head now, the goods by pages after it.
            if (!OZT_OpenStreams.Start(player, req, error))
                return "";
            error = OZ_Const.DEFER;
            return "";
        }
        int t0 = TickCount(0);
        string body = "";
        if (op == OZT_Const.OP_DEAL)
            body = OZT_Deal.Run(player, json, true, ok, error);
        // The same request, with nothing changed and the net not compared:
        // what the screen shows before Confirm is the server's own answer.
        else if (op == OZT_Const.OP_QUOTE)
            body = OZT_Deal.Run(player, json, false, ok, error);
        // -- operations of the later tasks go above this line --
        else
            return "";
        if (OZ_Log.IsDebug())
        {
            string said = op + ": " + OZT_Open.Ms(TickCount(t0)) + " ms, " + body.Length().ToString() + " B";
            OZT_Log.Dbg(said);
        }
        return body;
    }
}
