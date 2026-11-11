// The `oz_trader` verb, by the bridge's contract for project verbs: the name is
// added to KnownVerbs and IsKnownVerb and routed in Dispatch. Every op answers
// within its tick.
//
//   world_exec verb=oz_trader args={"op":"status"}
//
// Every value of args is a string. The answer is a summary (the bridge cuts it
// at 400 characters); a full answer of the service goes to OZT_Const.LAST_ANSWER.
class OZT_VerbOps
{
    static bool Run(map<string, string> args, out string detail)
    {
        string op = Arg(args, "op", "status");

        if (op == "status")
        {
            detail = OZT_Module.ReadyLine() + " revision=" + OZT_Config.Get().Revision().ToString();
            return true;
        }
        if (op == "reload")
            return OpReload(detail);

        PlayerBase player = Player();
        if (!player)
        {
            detail = "nobody is connected: start a client first";
            return false;
        }

        if (op == "balance")
            return OpBalance(player, args, detail);
        if (op == "price")
            return OpPrice(player, args, detail);
        if (op == "pay")
            return OpPay(player, args, detail);
        if (op == "damage")
            return OpDamage(player, args, detail);
        if (op == "open")
            return OpOpen(player, args, detail);
        // -- ops of the later tasks go above this line --
        detail = "unknown op '" + op + "'; known: " + KnownOps();
        return false;
    }

    static string KnownOps()
    {
        return "status, reload, balance, price, pay, damage, open";
    }

    // ---- what the ops share ---------------------------------------------------

    static string Arg(map<string, string> args, string key, string fallback)
    {
        if (!args)
            return fallback;
        string v;
        if (args.Find(key, v))
            return v;
        return fallback;
    }

    static PlayerBase Player()
    {
        array<Man> men = new array<Man>();
        GetGame().GetPlayers(men);
        if (men.Count() == 0)
            return null;
        return PlayerBase.Cast(men.Get(0));
    }

    // The trader an op means: trader=<id>, the stand's by default.
    static OZT_Point TraderOf(map<string, string> args)
    {
        return OZT_Config.Get().FindTrader(Arg(args, "trader", "stand"));
    }

    // The nth item of a class the player carries, in the engine's own order.
    static EntityAI Carried(PlayerBase player, string type, int nth)
    {
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        int seen = 0;
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI e = all[i];
            if (!e || e == player || e.GetType() != type)
                continue;
            if (seen == nth)
                return e;
            seen++;
        }
        return null;
    }

    // One request through the handler the core's dispatcher calls for a client,
    // as this player. The full answer goes to OZT_Const.LAST_ANSWER.
    static string Ask(PlayerBase player, string op, string json, out bool ok, out string why)
    {
        ok = false;
        why = "the service 'trader' is not registered";
        OZ_ServiceHandler svc = OZ_ServiceRegistry.Get(OZT_Const.SERVICE);
        if (!svc)
            return "";
        string body = svc.Handle(op, json, player.GetIdentity(), ok, why);
        SaveAnswer(body);
        return body;
    }

    static void SaveAnswer(string body)
    {
        FileHandle fh = OpenFile(OZT_Const.LAST_ANSWER, FileMode.WRITE);
        if (!fh)
            return;
        FPrint(fh, body);
        CloseFile(fh);
    }

    // open [trader=<id>]: what a client's `open` gets.
    static bool OpOpen(PlayerBase player, map<string, string> args, out string detail)
    {
        OZT_OpenReq req = new OZT_OpenReq();
        req.Trader = Arg(args, "trader", "stand");
        string json;
        string err;
        JsonFileLoader<OZT_OpenReq>.MakeData(req, json, err, false);

        bool ok;
        string why;
        string body = Ask(player, OZT_Const.OP_OPEN, json, ok, why);
        if (!ok)
        {
            detail = "open ok=no why=" + why;
            return true;
        }

        // Read it back the way a client will: a body that does not parse here
        // would not parse there.
        OZT_OpenRes res = new OZT_OpenRes();
        if (!JsonFileLoader<OZT_OpenRes>.LoadData(body, res, err))
        {
            detail = "open ok=yes, but the body does not parse back: " + err;
            return false;
        }
        detail = "open ok=yes trader=" + res.Trader + " goods=" + res.Goods.Count().ToString();
        detail = detail + " categories=" + res.Categories.Count().ToString() + " provider=" + res.Provider;
        detail = detail + " unit=" + res.Unit.ToString() + " balance=" + res.Balance.ToString();
        detail = detail + " bytes=" + body.Length().ToString() + " revision=" + res.Revision.ToString();
        return true;
    }

    // ---- the ops of this task -------------------------------------------------

    // The files again, as an admin's edit would be picked up.
    static bool OpReload(out string detail)
    {
        OZT_Config.ServerLoad();
        OZT_Book.Rebuild();
        OZT_Points.Respawn();
        // -- later tasks refresh their parts above this line --
        detail = "reloaded: " + OZT_Config.Get().Counters() + " revision=" + OZT_Config.Get().Revision().ToString();
        return true;
    }

    static bool OpBalance(PlayerBase player, map<string, string> args, out string detail)
    {
        OZT_MoneyProvider p = OZT_Money.Of(TraderOf(args));
        if (!p)
        {
            detail = "no money provider is registered";
            return false;
        }
        detail = "provider=" + p.Id() + " unit=" + p.Unit().ToString() + " balance=" + p.Balance(player).ToString();
        if (!p.Physical())
            return true;
        array<ref OZT_NoteStack> stacks;
        OZT_ItemMoney.Stacks(player, null, stacks);
        detail = detail + " stacks:";
        for (int i = 0; i < stacks.Count(); i++)
            detail = detail + " " + stacks[i].Value.ToString() + "x" + stacks[i].Units.ToString();
        return true;
    }

    // price class=<Class> [count=<n>] [nth=<k>]: what the trader asks for
    // `count` of the class, and what it pays for the nth such item the player
    // carries (for `count` units of it under the unit rule, all of it by default).
    static bool OpPrice(PlayerBase player, map<string, string> args, out string detail)
    {
        OZT_Point trader = TraderOf(args);
        string type = Arg(args, "class", "");
        OZT_BookEntry e = OZT_Book.EntryOf(type);
        if (!trader || !e)
        {
            detail = "price: trader or class not in the files (trader='" + Arg(args, "trader", "stand") + "' class='" + type + "')";
            return true;
        }
        int count = Arg(args, "count", "1").ToInt();
        detail = type + " rule='" + e.Rule + "' buy(" + count.ToString() + ")=" + OZT_Book.BuyPrice(trader, e, count).ToString();
        EntityAI item = Carried(player, type, Arg(args, "nth", "0").ToInt());
        if (!item)
        {
            detail = detail + " sell=(the player carries none)";
            return true;
        }
        int units = OZT_Stack.Units(item);
        if (args.Contains("count") && count < units)
            units = count;
        detail = detail + " sell(" + units.ToString() + " of " + OZT_Stack.Units(item).ToString() + ", level " + item.GetHealthLevel().ToString() + ")=";
        detail = detail + OZT_Book.SellPrice(trader, e, item, units).ToString();
        return true;
    }

    // pay amount=<n>: the owner's paying rule over the notes the player really
    // carries, with nothing moved: what would be spent and what comes back.
    static bool OpPay(PlayerBase player, map<string, string> args, out string detail)
    {
        int amount = Arg(args, "amount", "0").ToInt();
        array<ref OZT_NoteStack> stacks;
        OZT_ItemMoney.Stacks(player, null, stacks);
        int change = OZT_ItemMoney.Pay(stacks, amount);
        if (change < 0)
        {
            detail = "pay " + amount.ToString() + ": the notes do not cover it";
            return true;
        }
        detail = "pay " + amount.ToString() + ": spend";
        for (int i = 0; i < stacks.Count(); i++)
        {
            if (stacks[i].Spend > 0)
                detail = detail + " " + stacks[i].Value.ToString() + "x" + stacks[i].Spend.ToString() + "/" + stacks[i].Units.ToString();
        }
        detail = detail + "; change " + change.ToString() + " =";
        array<int> counts;
        if (!OZT_ItemMoney.Change(change, counts))
        {
            detail = detail + " CANNOT BE MADE";
            return true;
        }
        array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
        for (int k = notes.Count() - 1; k >= 0; k--)
        {
            if (counts[k] > 0)
                detail = detail + " " + notes[k].Value.ToString() + "x" + counts[k].ToString();
        }
        return true;
    }

    // damage class=<Class> [nth=<k>] health=<0..1>: a stand helper, so that a
    // sold item can be worn or damaged.
    static bool OpDamage(PlayerBase player, map<string, string> args, out string detail)
    {
        string type = Arg(args, "class", "");
        EntityAI item = Carried(player, type, Arg(args, "nth", "0").ToInt());
        if (!item)
        {
            detail = "damage: the player carries no " + type;
            return false;
        }
        item.SetHealth01("", "", Arg(args, "health", "1").ToFloat());
        detail = type + " health level " + item.GetHealthLevel().ToString();
        return true;
    }
}

modded class DZMCP_BridgeCore
{
    override protected string KnownVerbs()
    {
        return super.KnownVerbs() + ", oz_trader";
    }

    override protected bool IsKnownVerb(string verb)
    {
        if (verb == "oz_trader")
            return true;
        return super.IsKnownVerb(verb);
    }

    override protected void Dispatch(string verb, string raw)
    {
        if (verb != "oz_trader")
        {
            super.Dispatch(verb, raw);
            return;
        }

        DZMCP_CommandFull full = new DZMCP_CommandFull();
        string parseError;
        if (!m_Json.ReadFromString(full, raw, parseError))
        {
            FinishCommand(DZMCP_STATUS_FAILED, "oz_trader: the args block could not be parsed -- every value must be a string: " + Excerpt(parseError));
            return;
        }

        string detail;
        if (OZT_VerbOps.Run(full.args, detail))
            FinishCommand(DZMCP_STATUS_DONE, detail);
        else
            FinishCommand(DZMCP_STATUS_FAILED, detail);
    }
}
