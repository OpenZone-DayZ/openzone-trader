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
        if (op == "fit")
            return OpFit(player, args, detail);
        if (op == "quote")
            return OpDeal(player, args, false, detail);
        if (op == "deal")
            return OpDeal(player, args, true, detail);
        if (op == "show")
            return OpShow(player, args, detail);
        if (op == "tree")
            return OpTree(player, args, detail);
        // -- ops of the later tasks go above this line --
        detail = "unknown op '" + op + "'; known: " + KnownOps();
        return false;
    }

    static string KnownOps()
    {
        return "status, reload, balance, price, pay, damage, open, fit, quote, deal, show, tree";
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
        int sells = 0;
        for (int y = 0; y < res.Yours.Count(); y++)
        {
            if (res.Yours[y].Why == 0)
                sells++;
        }
        int full = res.Yours.Count() - sells;
        detail = detail + " yours=" + res.Yours.Count().ToString() + " (sells " + sells.ToString() + ", not empty " + full.ToString() + ")";
        return true;
    }

    // A list of things in an argument: Class[:count][#nth], separated by commas.
    // count 0 means "not given"; nth picks among several the player carries.
    static void Items(string list, out array<string> names, out array<int> counts, out array<int> nths)
    {
        names = new array<string>();
        counts = new array<int>();
        nths = new array<int>();
        if (list == "")
            return;
        array<string> tokens = new array<string>();
        list.Split(",", tokens);
        for (int i = 0; i < tokens.Count(); i++)
        {
            string token = tokens[i].Trim();
            if (token == "")
                continue;
            int nth = 0;
            int hash = token.IndexOf("#");
            if (hash > -1)
            {
                int afterHash = token.Length() - hash - 1;
                nth = token.Substring(hash + 1, afterHash).ToInt();
                token = token.Substring(0, hash);
            }
            int count = 0;
            int colon = token.IndexOf(":");
            if (colon > -1)
            {
                int afterColon = token.Length() - colon - 1;
                count = token.Substring(colon + 1, afterColon).ToInt();
                token = token.Substring(0, colon);
            }
            names.Insert(token);
            counts.Insert(count);
            nths.Insert(nth);
        }
    }

    // fit [buy=Class:n,...] [notes=Class:n,...] [gone=Class#nth,...] [make=1]:
    // where a plan puts new things, with the `gone` things taken out of the
    // picture. buy: n pieces, a free slot first when the settings wear bought
    // gear, then cargo. notes: n new stacks, note holders first. One thing with
    // no place goes to free hands. With make=1 the gone things are deleted and
    // every planned thing is created at its place in the same call, as a commit
    // does, and what the engine did is compared with the plan.
    static bool OpFit(PlayerBase player, map<string, string> args, out string detail)
    {
        array<string> names;
        array<int> counts;
        array<int> nths;

        array<EntityAI> gone = new array<EntityAI>();
        Items(Arg(args, "gone", ""), names, counts, nths);
        for (int g = 0; g < names.Count(); g++)
        {
            EntityAI leaving = Carried(player, names[g], nths[g]);
            if (!leaving)
            {
                detail = "fit: the player carries no " + names[g] + " number " + nths[g].ToString();
                return false;
            }
            gone.Insert(leaving);
        }

        OZT_InvModel model = new OZT_InvModel(player, gone);
        bool wear = OZT_Config.Get().Settings().WearBought == 1;
        array<string> classes = new array<string>();
        array<ref OZT_Place> places = new array<ref OZT_Place>();
        array<string> homeless = new array<string>();
        string report = "";

        Items(Arg(args, "buy", ""), names, counts, nths);
        for (int b = 0; b < names.Count(); b++)
        {
            int pieces = counts[b];
            if (pieces < 1)
                pieces = 1;
            for (int n = 0; n < pieces; n++)
            {
                OZT_Place place;
                bool placed = false;
                if (wear)
                    placed = model.PlaceOnSlot(names[b], place);
                if (!placed)
                    placed = model.PlaceInCargo(names[b], false, place);
                if (!placed)
                {
                    homeless.Insert(names[b]);
                    continue;
                }
                classes.Insert(names[b]);
                places.Insert(place);
                report = report + " | " + names[b] + ": " + place.Text();
            }
        }

        Items(Arg(args, "notes", ""), names, counts, nths);
        for (int s = 0; s < names.Count(); s++)
        {
            int piles = counts[s];
            if (piles < 1)
                piles = 1;
            for (int k = 0; k < piles; k++)
            {
                OZT_Place spot;
                if (!model.PlaceInCargo(names[s], true, spot))
                {
                    homeless.Insert(names[s]);
                    continue;
                }
                classes.Insert(names[s]);
                places.Insert(spot);
                report = report + " | " + names[s] + ": " + spot.Text();
            }
        }

        int unplaced = homeless.Count();
        if (unplaced == 1)
        {
            OZT_Place hands;
            if (model.PlaceInHands(homeless[0], hands))
            {
                classes.Insert(homeless[0]);
                places.Insert(hands);
                report = report + " | " + homeless[0] + ": HANDS";
                unplaced = 0;
            }
        }
        for (int u = 0; u < homeless.Count() && unplaced > 0; u++)
            report = report + " | " + homeless[u] + ": NO PLACE";

        string head = "fit: placed=" + classes.Count().ToString() + " unplaced=" + unplaced.ToString();
        if (Arg(args, "make", "0") == "1")
        {
            for (int d = 0; d < gone.Count(); d++)
                GetGame().ObjectDelete(gone[d]);
            int made = 0;
            int asPlanned = 0;
            for (int m = 0; m < classes.Count(); m++)
            {
                EntityAI fresh = OZT_InvModel.Make(player, places[m], classes[m]);
                if (!fresh)
                    continue;
                made++;
                if (OZT_InvModel.Stands(fresh, places[m]))
                    asPlanned++;
            }
            head = head + " made=" + made.ToString() + " where planned=" + asPlanned.ToString();
        }
        model.Dispose();

        SaveAnswer(head + report);
        detail = head + report;
        return true;
    }

    // quote / deal: [trader=<id>] [sell=Class[:units][#nth],...] [sellnear=Class]
    // [buy=Class:n,...] [net=<n>]
    //
    // `sell` names things the player carries; `sellnear` a thing lying within
    // five metres (to ask for something that is not the player's). A quote runs
    // every check and the whole plan and changes nothing. A deal is sent through
    // the service handler, as a client's is; without net= it carries the net the
    // server itself quotes, as an honest client would.
    static bool OpDeal(PlayerBase player, map<string, string> args, bool commit, out string detail)
    {
        OZT_DealReq req = new OZT_DealReq();
        req.Trader = Arg(args, "trader", "stand");

        array<string> names;
        array<int> counts;
        array<int> nths;
        Items(Arg(args, "sell", ""), names, counts, nths);
        for (int s = 0; s < names.Count(); s++)
        {
            EntityAI sold = Carried(player, names[s], nths[s]);
            if (!sold)
            {
                detail = "the player carries no " + names[s] + " number " + nths[s].ToString();
                return false;
            }
            req.Sells.Insert(SellOf(sold, counts[s]));
        }

        string near = Arg(args, "sellnear", "");
        if (near != "")
        {
            EntityAI lying = Nearby(player, near);
            if (!lying)
            {
                detail = "no " + near + " lies within five metres";
                return false;
            }
            req.Sells.Insert(SellOf(lying, 0));
        }

        Items(Arg(args, "buy", ""), names, counts, nths);
        for (int b = 0; b < names.Count(); b++)
        {
            OZT_WireBuy buy = new OZT_WireBuy();
            buy.ClassName = names[b];
            buy.Count = counts[b];
            if (buy.Count < 1)
                buy.Count = 1;
            req.Buys.Insert(buy);
        }

        bool ok;
        string why;
        string err;
        string json;
        JsonFileLoader<OZT_DealReq>.MakeData(req, json, err, false);
        // Through the service handler, as a client's is.
        string quoted = Ask(player, OZT_Const.OP_QUOTE, json, ok, why);
        if (!commit)
        {
            detail = "quote " + Told(quoted, ok, why);
            if (Arg(args, "twice", "0") == "1")
            {
                bool ok2;
                string why2;
                string again = Ask(player, OZT_Const.OP_QUOTE, json, ok2, why2);
                detail = detail + " | asked again at once: " + Told(again, ok2, why2);
            }
            return true;
        }

        if (args.Contains("net"))
            req.Net = Arg(args, "net", "0").ToInt();
        else
        {
            OZT_DealRes q = new OZT_DealRes();
            JsonFileLoader<OZT_DealRes>.LoadData(quoted, q, err);
            req.Net = q.Net;
        }
        JsonFileLoader<OZT_DealReq>.MakeData(req, json, err, false);
        string body = Ask(player, OZT_Const.OP_DEAL, json, ok, why);
        detail = "deal " + Told(body, ok, why);
        return true;
    }

    static OZT_WireSell SellOf(EntityAI e, int count)
    {
        OZT_WireSell line = new OZT_WireSell();
        int low;
        int high;
        e.GetNetworkID(low, high);
        line.Low = low;
        line.High = high;
        line.Count = count;
        return line;
    }

    // The nearest thing of a class that lies loose within five metres.
    static EntityAI Nearby(PlayerBase player, string type)
    {
        array<Object> found = new array<Object>();
        vector here = player.GetPosition();
        GetGame().GetObjectsAtPosition(here, 5, found, null);
        for (int i = 0; i < found.Count(); i++)
        {
            EntityAI e = EntityAI.Cast(found[i]);
            if (e && e.GetType() == type && !e.GetHierarchyParent())
                return e;
        }
        return null;
    }

    // An answer of the deal in one line.
    static string Told(string body, bool ok, string why)
    {
        OZT_DealRes res = new OZT_DealRes();
        string err;
        if (body != "")
            JsonFileLoader<OZT_DealRes>.LoadData(body, res, err);
        string s;
        if (ok)
            s = "ok=yes";
        else
            s = "ok=no why=" + why;
        s = s + " net=" + res.Net.ToString() + " paid=" + res.Paid.ToString() + " received=" + res.Received.ToString();
        s = s + " balance=" + res.Balance.ToString() + " hands=" + res.InHands + " left=" + res.Left.ToString();
        return s;
    }

    // What a request keeps, from two arguments: keep=Class[#nth],... names
    // parts that stay with the player, keeprounds=Class[#nth],... things whose
    // loaded rounds stay.
    static bool Keeps(PlayerBase player, map<string, string> args, out array<ref OZT_WireKeep> keep, out string detail)
    {
        keep = new array<ref OZT_WireKeep>();
        array<string> names;
        array<int> counts;
        array<int> nths;
        for (int which = 0; which < 2; which++)
        {
            string list = Arg(args, "keep", "");
            if (which == 1)
                list = Arg(args, "keeprounds", "");
            Items(list, names, counts, nths);
            for (int i = 0; i < names.Count(); i++)
            {
                EntityAI part = Carried(player, names[i], nths[i]);
                if (!part)
                {
                    detail = "the player carries no " + names[i] + " number " + nths[i].ToString();
                    return false;
                }
                OZT_WireKeep line = new OZT_WireKeep();
                int low;
                int high;
                part.GetNetworkID(low, high);
                line.Low = low;
                line.High = high;
                line.Rounds = which;
                keep.Insert(line);
            }
        }
        return true;
    }

    // tree item=Class[#nth] [keep=Class[#nth],...] [keeprounds=Class[#nth],...]:
    // the thing as a deal reads it when it is sold alone: every node with its
    // state, every lot of rounds, the sums. Nothing is changed.
    static bool OpTree(PlayerBase player, map<string, string> args, out string detail)
    {
        OZT_Point trader = TraderOf(args);
        if (!trader)
        {
            detail = "tree: no trader '" + Arg(args, "trader", "stand") + "'";
            return false;
        }
        array<string> names;
        array<int> counts;
        array<int> nths;
        Items(Arg(args, "item", ""), names, counts, nths);
        if (names.Count() != 1)
        {
            detail = "tree needs item=Class[#nth]";
            return false;
        }
        EntityAI root = Carried(player, names[0], nths[0]);
        if (!root)
        {
            detail = "the player carries no " + names[0] + " number " + nths[0].ToString();
            return false;
        }
        array<ref OZT_WireKeep> keep;
        if (!Keeps(player, args, keep, detail))
            return false;

        OZT_Tree tree = OZT_Tree.Read(trader, root, keep, null);
        if (tree.Taken())
            detail = "tree: " + tree.Text();
        else
            detail = "tree: the trader does not take it: " + tree.Text();
        SaveAnswer(detail);
        return true;
    }

    // show [trader=<id>]: tell the client to put the trade screen up for a
    // trader, the way the trade action will.
    static bool OpShow(PlayerBase player, map<string, string> args, out string detail)
    {
        string id = Arg(args, "trader", "stand");
        PlayerIdentity to = player.GetIdentity();
        if (!to)
        {
            detail = "show: the player has no identity";
            return false;
        }
        OZ_Rpc.Show(to, OZT_Const.SHOW_PREFIX + id);
        detail = "show: sent '" + OZT_Const.SHOW_PREFIX + id + "'";
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

    // damage class=<Class> [nth=<k>] health=<0..1> | rounds=<0..1>: a stand
    // helper, so that a sold item can be worn or damaged. With rounds= the
    // thing is left as it is and every round loaded in the magazine gets that
    // damage (0: new).
    static bool OpDamage(PlayerBase player, map<string, string> args, out string detail)
    {
        string type = Arg(args, "class", "");
        EntityAI item = Carried(player, type, Arg(args, "nth", "0").ToInt());
        if (!item)
        {
            detail = "damage: the player carries no " + type;
            return false;
        }
        if (args.Contains("rounds"))
        {
            Magazine mag = Magazine.Cast(item);
            if (!mag)
            {
                detail = "damage: rounds= needs a magazine or a pile, " + type + " is neither";
                return false;
            }
            float dmg = Arg(args, "rounds", "0").ToFloat();
            int held = mag.GetAmmoCount();
            int done = 0;
            for (int i = 0; i < held; i++)
            {
                if (mag.SetCartridgeDamageAtIndex(i, dmg))
                    done++;
            }
            detail = type + ": " + done.ToString() + " of " + held.ToString() + " rounds set to damage " + dmg.ToString();
            return true;
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
