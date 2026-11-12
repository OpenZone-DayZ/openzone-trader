// The admin's side of the traders (design §9.5 and §9.6): the section
// `traders` of the core's admin registry, which the pages of the module TRADER
// of the admin window (glue pbo OpenZone_Trader_VPP) talk to, and the three
// files in the core's file editor (cfg_get / cfg_set), which the structured
// pages edit too (VPP rework 2026-11-12, design §4.3 and §5.6).
//
// The core's dispatcher has checked the admin's rights before any of this
// runs (OZ_Perm.IsAdmin), and nothing here checks them again: one gate.
//
// An edit is made to the FILE, read again for it: the live configuration has
// been through the cross-checks, which leave out what this server cannot use
// today (a category the price book lacks, a class of a mod not loaded), and
// writing it back would delete the admin's own lines. Then everything is
// loaded again and the points are made again, as the stand's reload does.

class OZT_Admin : OZ_AdminSection
{
    static const string TAG = "trader points";

    // Reads change nothing: other admins are not told about them (the core's
    // OZ_AdminTell, VPP rework 2026-11-12). The window's own list is the same.
    override bool IsRead(string op)
    {
        array<string> reads = {"list", "stock_list", "licence_list", "problems", "gates", "gate_holders"};
        return reads.Find(op) != -1;
    }

    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        string verb = op;
        string arg = "";
        int colon = op.IndexOf(":");
        if (colon > 0)
        {
            verb = op.Substring(0, colon);
            arg = op.Substring(colon + 1, op.Length() - colon - 1);
        }

        if (verb == "list")
        {
            ok = true;
            error = "";
            return ListJson();
        }
        if (verb == "respawn")
        {
            OZT_Config.ReloadLive();
            OZT_Log.Info("admin: the points made again");
            ok = true;
            error = "";
            return ListJson();
        }

        // The admin window's pages (VPP rework 2026-11-12, design §4.3).
        if (verb == "problems")
        {
            ok = true;
            error = "";
            return ProblemsJson();
        }
        if (verb == "gates")
        {
            ok = true;
            error = "";
            return GatesJson();
        }
        if (verb == "gate_holders")
        {
            string holders = HoldersJson(json);
            if (holders == "")
            {
                error = "STR_OZT_ADMIN_BAD_REQUEST";
                return "";
            }
            ok = true;
            error = "";
            return holders;
        }
        if (verb == "stock_list")
        {
            ok = true;
            error = "";
            return StockListJson();
        }
        if (verb == "stock_set")
        {
            string stockWhy = StockSet(json);
            if (stockWhy != "")
            {
                error = stockWhy;
                return "";
            }
            ok = true;
            error = "";
            return StockListJson();
        }
        if (verb == "licence_list")
        {
            ok = true;
            error = "";
            return LicenceListJson();
        }

        PlayerBase admin = null;
        if (sender)
            admin = PlayerBase.Cast(sender.GetPlayer());

        if (verb == "licence")
        {
            string said = GiveLicence(json);
            if (said != "")
            {
                error = said;
                return "";
            }
            ok = true;
            error = "";
            return ListJson();
        }
        // The report goes back whether the import ran or not: what it could not
        // read is in it.
        if (verb == "import_traderplus")
        {
            OZT_AdminImportReport rep = new OZT_AdminImportReport();
            string told = ImportTraderPlus(json, rep);
            string repJson;
            string repErr;
            JsonFileLoader<OZT_AdminImportReport>.MakeData(rep, repJson, repErr, false);
            if (told != "")
            {
                error = told;
                return repJson;
            }
            ok = true;
            error = "";
            return repJson;
        }

        OZT_TradersFile file = new OZT_TradersFile();
        if (!OZ_ConfigLoader<OZT_TradersFile>.Load(OZT_Const.TRADERS, TAG, file, true, false))
        {
            error = "STR_OZT_ADMIN_NOT_WRITABLE";
            return "";
        }

        string why = "";
        if (verb == "place")
            why = Place(file, json, admin);
        else if (verb == "adopt")
            why = Adopt(file, json, admin);
        else if (verb == "move")
            why = Move(file, arg, admin);
        else if (verb == "turn")
            why = Turn(file, arg, admin);
        else if (verb == "remove")
            why = RemovePoint(file, arg);
        else if (verb == "set")
            why = SetPoint(file, arg, json);
        else if (verb == "outfit")
            why = Dress(file, arg, admin);
        else
            return "";
        if (why != "")
        {
            error = why;
            return "";
        }

        OZ_ConfigLoader<OZT_TradersFile>.Save(OZT_Const.TRADERS, TAG, file, true);
        OZT_Config.ReloadLive();
        OZT_Log.Info("admin: " + verb + " '" + arg + "' done, " + OZT_Config.Get().Counters());
        ok = true;
        error = "";
        return ListJson();
    }

    // ---- the ops ------------------------------------------------------------

    // A licence granted or taken back (design §11.2): by Steam id, or by the
    // name of a player online.
    protected string GiveLicence(string json)
    {
        OZT_AdminLicence req = new OZT_AdminLicence();
        string err;
        if (!JsonFileLoader<OZT_AdminLicence>.LoadData(json, req, err))
            return "STR_OZT_ADMIN_BAD_REQUEST";
        if (!OZT_Licences.Def(req.Id))
            return "STR_OZT_ADMIN_NO_LICENCE";
        string uid = UidByName(req.Player);
        if (uid == "")
            return "STR_OZT_ADMIN_NO_SUCH_PLAYER";
        if (req.Grant == 1)
            OZT_Licences.Grant(uid, req.Id);
        else
            OZT_Licences.Revoke(uid, req.Id);
        OZT_Log.Info("admin: licence '" + req.Id + "' grant=" + req.Grant.ToString() + " for a player");
        return "";
    }

    // TraderPlus's files to ours (design §11.3): a dry run unless Apply is 1.
    // The report goes to OpenZone\TraderPlusImport\report.txt, the log and `rep`,
    // a line a row (a string travels whole only below 1023 bytes).
    protected string ImportTraderPlus(string json, OZT_AdminImportReport rep)
    {
        OZT_AdminImport req = new OZT_AdminImport();
        string err;
        if (json != "" && !JsonFileLoader<OZT_AdminImport>.LoadData(json, req, err))
            return "STR_OZT_ADMIN_BAD_REQUEST";
        // A folder under the server's profile, and nowhere else.
        if (req.Root != "")
        {
            if (req.Root.IndexOf("$profile:") != 0 || req.Root.Contains(".."))
                return "STR_OZT_ADMIN_BAD_REQUEST";
        }
        bool apply = req.Apply == 1;
        bool done;
        string report = OZT_TPImport.Run(req.Root, apply, done);
        OZT_Log.Info("admin: TraderPlus import, apply=" + req.Apply.ToString() + "\n" + report);
        rep.Apply = req.Apply;
        if (done)
            rep.Done = 1;
        array<string> lines = new array<string>();
        report.Split("\n", lines);
        for (int i = 0; i < lines.Count(); i++)
        {
            string line = lines[i].Trim();
            if (line != "")
                rep.Lines.Insert(OZT_AdminText.Cut(line));
        }
        if (!done)
            return "STR_OZT_ADMIN_NO_TRADERPLUS";
        return "";
    }

    // ---- the admin window's reads --------------------------------------------

    static const int STOCK_LINES_MAX = 2000;
    static const int HOLDERS_MAX = 500;
    static const int OPENS_MAX = 30;
    static const int CHOICES_MAX = 800;
    static const int WHO_MAX = 200;

    // The warnings of the files' last load, since it started.
    protected string ProblemsJson()
    {
        OZT_AdminProblems p = new OZT_AdminProblems();
        array<string> lines = new array<string>();
        OZT_Log.Problems(lines);
        for (int i = 0; i < lines.Count(); i++)
            p.Lines.Insert(OZT_AdminText.Cut(lines[i]));
        p.Count = OZT_Log.Warned();
        if (p.Count > p.Lines.Count())
            p.Cut = 1;
        p.Counters = OZT_Config.Get().Counters();
        string body;
        string err;
        JsonFileLoader<OZT_AdminProblems>.MakeData(p, body, err, false);
        return body;
    }

    // Every gate provider: its family, whether its side is there, its kinds and
    // for each the values an admin may pick (bounded; Cut says so).
    protected string GatesJson()
    {
        OZT_AdminGates gates = new OZT_AdminGates();
        for (int i = 0; i < OZT_Gates.Count(); i++)
        {
            OZT_GateProvider prov = OZT_Gates.At(i);
            if (!prov)
                continue;
            OZT_AdminGateFamily fam = new OZT_AdminGateFamily();
            fam.Family = prov.Family();
            if (prov.Answers())
                fam.Answers = 1;
            array<string> kinds = new array<string>();
            prov.Kinds(kinds);
            for (int k = 0; k < kinds.Count(); k++)
            {
                OZT_AdminGateKind gk = new OZT_AdminGateKind();
                gk.Kind = kinds[k];
                array<string> values = new array<string>();
                array<string> labels = new array<string>();
                prov.Choices(kinds[k], values, labels);
                for (int v = 0; v < values.Count(); v++)
                {
                    if (v >= CHOICES_MAX)
                    {
                        gk.Cut = 1;
                        break;
                    }
                    gk.Values.Insert(OZT_AdminText.Cut(values[v]));
                    string label = "";
                    if (v < labels.Count())
                        label = labels[v];
                    gk.Labels.Insert(OZT_AdminText.Cut(label));
                }
                fam.Kinds.Insert(gk);
            }
            gates.Families.Insert(fam);
        }
        string body;
        string err;
        JsonFileLoader<OZT_AdminGates>.MakeData(gates, body, err, false);
        return body;
    }

    // Who holds `Kind Arg` now; "" when the request is unreadable.
    protected string HoldersJson(string json)
    {
        OZT_AdminGateAsk ask = new OZT_AdminGateAsk();
        string err;
        if (!JsonFileLoader<OZT_AdminGateAsk>.LoadData(json, ask, err))
            return "";
        OZT_AdminGateHolders h = new OZT_AdminGateHolders();
        h.Kind = ask.Kind;
        h.Arg = ask.Arg;
        OZT_GateProvider prov = OZT_Gates.ByKind(ask.Kind);
        if (prov && prov.Answers())
        {
            h.Answers = 1;
            array<string> who = new array<string>();
            prov.Holders(ask.Kind, ask.Arg, who);
            for (int i = 0; i < who.Count(); i++)
            {
                if (i >= WHO_MAX)
                {
                    h.Cut = 1;
                    break;
                }
                h.Who.Insert(OZT_AdminText.Cut(who[i]));
            }
        }
        string body;
        JsonFileLoader<OZT_AdminGateHolders>.MakeData(h, body, err, false);
        return body;
    }

    // Per point and category, the stocked lines it deals by (OZT_Book.EntryFor:
    // a class two of its categories list is the first one's) and their counts.
    protected string StockListJson()
    {
        OZT_AdminStockList list = new OZT_AdminStockList();
        OZT_Settings cfg = OZT_Config.Get().Settings();
        list.StartStock = cfg.StartStock;
        list.DestockAtStart = cfg.DestockAtStart;
        list.OnlyPristine = cfg.StockOnlyPristine;
        list.Floor = cfg.StockFloor;
        int lines = 0;
        array<ref OZT_Point> points = OZT_Config.Get().Traders().Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            OZT_Point p = points[i];
            OZT_AdminStockPoint sp = new OZT_AdminStockPoint();
            sp.Id = p.Id;
            sp.Name = p.Name;
            sp.Stock = p.StockId();
            for (int c = 0; c < p.Categories.Count(); c++)
            {
                OZT_Category cat = OZT_Config.Get().FindCategory(p.Categories[c]);
                if (!cat)
                    continue;
                OZT_AdminStockCat sc = new OZT_AdminStockCat();
                sc.Id = cat.Id;
                sc.Name = cat.Name;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_BookEntry e = OZT_Book.EntryFor(p, cat.Goods[g].ClassName);
                    if (!e || e.Category != cat.Id || !e.Stocked)
                        continue;
                    if (lines >= STOCK_LINES_MAX)
                    {
                        list.Cut = 1;
                        break;
                    }
                    OZT_AdminStockLine sl = new OZT_AdminStockLine();
                    sl.ClassName = e.ClassName;
                    sl.Max = e.MaxStock;
                    for (int lv = 0; lv < OZT_Const.LEVEL_RUINED; lv++)
                        sl.Counts.Insert(OZT_Stock.Count(sp.Stock, e.ClassName, lv));
                    sc.Lines.Insert(sl);
                    lines++;
                }
                if (sc.Lines.Count() > 0)
                    sp.Cats.Insert(sc);
            }
            list.Points.Insert(sp);
        }
        string body;
        string err;
        JsonFileLoader<OZT_AdminStockList>.MakeData(list, body, err, false);
        return body;
    }

    // The count of one stocked line of a point's category, in one condition: the
    // stand verb `stock`, as an admin op. The players at the points that sell from
    // the stock are told, as after a deal.
    protected string StockSet(string json)
    {
        OZT_AdminStockSet req = new OZT_AdminStockSet();
        string err;
        if (!JsonFileLoader<OZT_AdminStockSet>.LoadData(json, req, err))
            return "STR_OZT_ADMIN_BAD_REQUEST";
        OZT_Point p = OZT_Config.Get().FindTrader(req.Point);
        if (!p)
            return "STR_OZT_ADMIN_NO_POINT";
        if (req.Level < 0 || req.Level >= OZT_Const.LEVEL_RUINED || req.Count < 0)
            return "STR_OZT_ADMIN_BAD_REQUEST";
        OZT_BookEntry e = OZT_Book.EntryIn(req.Category, req.ClassName);
        if (!e || !e.Stocked || p.Categories.Find(req.Category) < 0)
            return "STR_OZT_ADMIN_NOT_STOCKED";
        string stock = p.StockId();
        int was = OZT_Stock.Count(stock, req.ClassName, req.Level);
        OZT_Stock.Add(stock, req.ClassName, req.Level, req.Count - was);
        OZT_Stock.Save(stock);
        OZT_StockNews.Tell(stock, null);
        string said = "admin: stock '" + stock + "' " + req.ClassName + " level " + req.Level.ToString();
        said = said + ": " + was.ToString() + " -> " + req.Count.ToString();
        OZT_Log.Info(said);
        return "";
    }

    // The licences of the settings, who holds them (one file per player in the
    // licence folder), and who is online. A holder online goes by name; anyone
    // else by the Steam ID, which this answer carries for the admin's page alone
    // and no log line ever writes.
    protected string LicenceListJson()
    {
        OZT_AdminLicenceList list = new OZT_AdminLicenceList();
        map<string, string> online = new map<string, string>();
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pb = PlayerBase.Cast(players[i]);
            if (!pb || !pb.GetIdentity())
                continue;
            string name = pb.GetIdentity().GetName();
            online.Set(pb.GetIdentity().GetPlainId(), name);
            list.Online.Insert(name);
        }

        array<string> uids = new array<string>();
        LicenceFiles(uids);
        map<string, int> held = new map<string, int>();
        for (int u = 0; u < uids.Count(); u++)
        {
            array<string> ids = OZT_Licences.Of(uids[u]);
            if (ids.Count() == 0)
                continue;
            if (list.Holders.Count() >= HOLDERS_MAX)
            {
                list.Cut = 1;
                break;
            }
            OZT_AdminLicenceHolder h = new OZT_AdminLicenceHolder();
            string shown;
            if (online.Find(uids[u], shown))
            {
                h.Name = shown;
                h.Online = 1;
            }
            else
            {
                h.Uid = uids[u];
            }
            for (int k = 0; k < ids.Count(); k++)
            {
                h.Ids.Insert(ids[k]);
                held.Set(ids[k], held.Get(ids[k]) + 1);
            }
            list.Holders.Insert(h);
        }

        array<ref OZT_LicenceDef> defs = OZT_Config.Get().Settings().Licences;
        for (int d = 0; d < defs.Count(); d++)
        {
            OZT_AdminLicenceDef def = new OZT_AdminLicenceDef();
            def.Id = defs[d].Id;
            def.Name = defs[d].Name;
            def.Look = defs[d].Look;
            def.Holders = held.Get(def.Id);
            LicenceUses(def);
            list.Defs.Insert(def);
        }
        string body;
        string err;
        JsonFileLoader<OZT_AdminLicenceList>.MakeData(list, body, err, false);
        return body;
    }

    // The Steam IDs that have a licence file: the file names of the folder.
    protected void LicenceFiles(array<string> uids)
    {
        string name;
        FileAttr attr;
        FindFileHandle h = FindFile(OZT_Const.LICENCE_DIR + "\\*.json", name, attr, FindFileFlags.ALL);
        if (h == 0)
            return;
        bool more = true;
        while (more)
        {
            int len = name.Length();
            if (len > 5 && name.Substring(len - 5, 5) == ".json")
            {
                string uid = name.Substring(0, len - 5);
                if (OZT_Ids.IsSafe(uid) && uids.Find(uid) < 0)
                    uids.Insert(uid);
            }
            more = FindNextFile(h, name, attr);
        }
        CloseFindFile(h);
    }

    // Where a licence is sold and what it opens, in the live files.
    protected void LicenceUses(OZT_AdminLicenceDef def)
    {
        string goods = OZT_Const.LICENCE_PREFIX + def.Id;
        array<ref OZT_Category> cats = OZT_Config.Get().Goods().Categories;
        for (int c = 0; c < cats.Count(); c++)
        {
            OZT_Category cat = cats[c];
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                if (cat.Goods[g].ClassName == goods)
                    def.Sold.Insert(cat.Id + "|" + cat.Goods[g].Buy.ToString());
            }
            if (NamesLicence(cat.Requires, def.Id))
                OpensAdd(def, "category|" + cat.Id);
            if (DiscountNames(cat.Discounts, def.Id))
                OpensAdd(def, "discount|" + cat.Id);
            for (int l = 0; l < cat.Goods.Count(); l++)
            {
                OZT_Goods line = cat.Goods[l];
                if (NamesLicence(line.Requires, def.Id))
                    OpensAdd(def, "line|" + line.ClassName + "|" + cat.Id);
                if (DiscountNames(line.Discounts, def.Id))
                    OpensAdd(def, "discount|" + line.ClassName + "|" + cat.Id);
            }
        }
        array<ref OZT_Point> points = OZT_Config.Get().Traders().Traders;
        for (int p = 0; p < points.Count(); p++)
        {
            if (NamesLicence(points[p].Access, def.Id))
                OpensAdd(def, "point|" + points[p].Id);
        }
    }

    protected void OpensAdd(OZT_AdminLicenceDef def, string what)
    {
        if (def.Opens.Count() < OPENS_MAX)
            def.Opens.Insert(what);
    }

    // Does a condition of the kind `licence` name the licence (alone or among
    // alternatives)?
    protected bool NamesLicence(array<ref OZT_Condition> conds, string id)
    {
        if (!conds)
            return false;
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c || c.Kind != "licence")
                continue;
            array<string> alts = new array<string>();
            c.Arg.Split("|", alts);
            for (int a = 0; a < alts.Count(); a++)
            {
                if (alts[a].Trim() == id)
                    return true;
            }
        }
        return false;
    }

    protected bool DiscountNames(array<ref OZT_Discount> discounts, string id)
    {
        if (!discounts)
            return false;
        for (int i = 0; i < discounts.Count(); i++)
        {
            OZT_Discount d = discounts[i];
            if (!d)
                continue;
            if (NamesLicence(d.When, id))
                return true;
            if (d.By)
            {
                array<ref OZT_Condition> by = new array<ref OZT_Condition>();
                by.Insert(d.By);
                if (NamesLicence(by, id))
                    return true;
            }
        }
        return false;
    }

    // A Steam id as it is; a name of a player online to that player's id.
    static string UidByName(string who)
    {
        if (who == "")
            return "";
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players[i]);
            if (!p || !p.GetIdentity())
                continue;
            if (p.GetIdentity().GetName() == who || p.GetIdentity().GetPlainId() == who)
                return p.GetIdentity().GetPlainId();
        }
        // An id of a player who is not online: digits only, 17 of them.
        if (who.Length() != 17)
            return "";
        for (int c = 0; c < who.Length(); c++)
        {
            int code = who.Get(c).ToAscii();
            if (code < 48 || code > 57)
                return "";
        }
        return who;
    }

    protected string Place(OZT_TradersFile file, string json, PlayerBase admin)
    {
        if (!admin)
            return "STR_OZT_ADMIN_NO_PLAYER";
        OZT_AdminPlace req = new OZT_AdminPlace();
        string err;
        if (!JsonFileLoader<OZT_AdminPlace>.LoadData(json, req, err))
            return "STR_OZT_ADMIN_BAD_REQUEST";
        if (!OZT_Ids.IsSafe(req.Id) || IndexOf(file, req.Id) > -1)
            return "STR_OZT_ADMIN_BAD_ID";
        string why = ClassFor(req.Kind, req.ClassName);
        if (why != "")
            return why;

        OZT_Point p = new OZT_Point();
        p.Id = req.Id;
        p.Name = req.Name;
        if (p.Name == "")
            p.Name = req.Id;
        p.Kind = req.Kind;
        p.ClassName = req.ClassName;
        p.Pos = new array<float>();
        vector at = admin.GetPosition();
        p.Pos.Insert(at[0]);
        p.Pos.Insert(at[1]);
        p.Pos.Insert(at[2]);
        p.Yaw = Heading(admin);
        p.Radius = 3;
        // Every category of the price book to begin with: a new trader that
        // deals in nothing would look broken.
        p.Categories = new array<string>();
        array<ref OZT_Category> cats = OZT_Config.Get().Goods().Categories;
        for (int c = 0; c < cats.Count(); c++)
            p.Categories.Insert(cats[c].Id);
        p.Outfit = new OZT_Outfit();
        p.Outfit.Items = new array<ref OZT_OutfitItem>();
        file.Traders.Insert(p);
        return "";
    }

    // A thing already in the world becomes a trader (kind map): what the
    // admin's client saw, found again here near the place it named. Nothing is
    // made; the thing keeps its place and its facing.
    protected string Adopt(OZT_TradersFile file, string json, PlayerBase admin)
    {
        if (!admin)
            return "STR_OZT_ADMIN_NO_PLAYER";
        OZT_AdminAdopt req = new OZT_AdminAdopt();
        string err;
        if (!JsonFileLoader<OZT_AdminAdopt>.LoadData(json, req, err))
            return "STR_OZT_ADMIN_BAD_REQUEST";
        if (!OZT_Ids.IsSafe(req.Id) || IndexOf(file, req.Id) > -1)
            return "STR_OZT_ADMIN_BAD_ID";
        vector seen = Vector(req.X, req.Y, req.Z);
        Object thing = OZT_Points.FindMapThing(req.ClassName, req.Model, seen);
        if (!thing)
            return "STR_OZT_ADMIN_NO_THING";
        // A trader's own body is a point already; a character or an item never
        // stays put (FindMapThing leaves those out).
        if (OZT_Points.IdOf(thing) != "")
            return "STR_OZT_ADMIN_TAKEN";

        OZT_Point p = new OZT_Point();
        p.Id = req.Id;
        p.Name = req.Name;
        if (p.Name == "")
            p.Name = req.Id;
        p.Kind = OZT_Const.KIND_MAP;
        p.ClassName = thing.GetType();
        p.Model = "";
        if (p.ClassName == "")
            p.Model = thing.GetShapeName();
        vector at = thing.GetPosition();
        p.Pos = new array<float>();
        p.Pos.Insert(at[0]);
        p.Pos.Insert(at[1]);
        p.Pos.Insert(at[2]);
        vector ori = thing.GetOrientation();
        p.Yaw = ori[0];
        p.Radius = 3;
        p.Categories = new array<string>();
        array<ref OZT_Category> cats = OZT_Config.Get().Goods().Categories;
        for (int c = 0; c < cats.Count(); c++)
            p.Categories.Insert(cats[c].Id);
        p.Outfit = new OZT_Outfit();
        p.Outfit.Items = new array<ref OZT_OutfitItem>();
        file.Traders.Insert(p);
        return "";
    }

    protected string Move(OZT_TradersFile file, string id, PlayerBase admin)
    {
        if (!admin)
            return "STR_OZT_ADMIN_NO_PLAYER";
        int i = IndexOf(file, id);
        if (i < 0)
            return "STR_OZT_ADMIN_NO_POINT";
        if (file.Traders[i].Kind == OZT_Const.KIND_MAP)
            return "STR_OZT_ADMIN_MAP_FIXED";
        vector at = admin.GetPosition();
        OZT_Point p = file.Traders[i];
        p.Pos = new array<float>();
        p.Pos.Insert(at[0]);
        p.Pos.Insert(at[1]);
        p.Pos.Insert(at[2]);
        return "";
    }

    protected string Turn(OZT_TradersFile file, string id, PlayerBase admin)
    {
        if (!admin)
            return "STR_OZT_ADMIN_NO_PLAYER";
        int i = IndexOf(file, id);
        if (i < 0)
            return "STR_OZT_ADMIN_NO_POINT";
        if (file.Traders[i].Kind == OZT_Const.KIND_MAP)
            return "STR_OZT_ADMIN_MAP_FIXED";
        file.Traders[i].Yaw = Heading(admin);
        return "";
    }

    protected string RemovePoint(OZT_TradersFile file, string id)
    {
        int i = IndexOf(file, id);
        if (i < 0)
            return "STR_OZT_ADMIN_NO_POINT";
        file.Traders.RemoveOrdered(i);
        return "";
    }

    protected string SetPoint(OZT_TradersFile file, string id, string json)
    {
        int i = IndexOf(file, id);
        if (i < 0)
            return "STR_OZT_ADMIN_NO_POINT";
        OZT_AdminSet req = new OZT_AdminSet();
        string err;
        if (!JsonFileLoader<OZT_AdminSet>.LoadData(json, req, err))
            return "STR_OZT_ADMIN_BAD_REQUEST";
        OZT_Point p = file.Traders[i];
        // A map point keeps its kind and its thing: the pane's kind and class
        // name what the server makes, and a map point's thing is the map's.
        if (p.Kind != OZT_Const.KIND_MAP)
        {
            string why = ClassFor(req.Kind, req.ClassName);
            if (why != "")
                return why;
            p.Kind = req.Kind;
            p.ClassName = req.ClassName;
        }
        p.Name = req.Name;
        if (req.Radius > 0)
            p.Radius = req.Radius;
        p.Categories = new array<string>();
        if (req.Categories)
        {
            for (int c = 0; c < req.Categories.Count(); c++)
                p.Categories.Insert(req.Categories[c]);
        }
        if (req.BuyFactor > 0)
            p.BuyFactor = req.BuyFactor;
        if (req.SellFactor > 0)
            p.SellFactor = req.SellFactor;
        p.Provider = req.Provider;
        p.NoExchange = 0;
        if (req.NoExchange == 1)
            p.NoExchange = 1;
        return "";
    }

    // What the admin wears, and what hangs on it, becomes the point's outfit:
    // the cargo is left out (design §9.3). What the hands hold goes to the
    // hands.
    protected string Dress(OZT_TradersFile file, string id, PlayerBase admin)
    {
        if (!admin)
            return "STR_OZT_ADMIN_NO_PLAYER";
        int i = IndexOf(file, id);
        if (i < 0)
            return "STR_OZT_ADMIN_NO_POINT";
        if (file.Traders[i].Kind == OZT_Const.KIND_MAP)
            return "STR_OZT_ADMIN_MAP_FIXED";
        OZT_Outfit outfit = new OZT_Outfit();
        outfit.Items = new array<ref OZT_OutfitItem>();
        GameInventory inv = admin.GetInventory();
        for (int s = 0; s < inv.GetAttachmentSlotsCount(); s++)
        {
            int slot = inv.GetAttachmentSlotId(s);
            EntityAI worn = inv.FindAttachment(slot);
            if (!worn)
                continue;
            outfit.Items.Insert(WornOf(worn, InventorySlots.GetSlotName(slot)));
        }
        EntityAI held = admin.GetHumanInventory().GetEntityInHands();
        if (held)
            outfit.Items.Insert(WornOf(held, "hands"));
        file.Traders[i].Outfit = outfit;
        return "";
    }

    // One thing and its parts, by slot. Recursion one element at a time with
    // an object parameter: the safe shape (dayz-modding, enforce-script.md).
    protected OZT_OutfitItem WornOf(EntityAI e, string slotName)
    {
        OZT_OutfitItem it = new OZT_OutfitItem();
        it.ClassName = e.GetType();
        it.SlotName = slotName;
        it.Inside = new array<ref OZT_OutfitItem>();
        GameInventory inv = e.GetInventory();
        for (int s = 0; s < inv.GetAttachmentSlotsCount(); s++)
        {
            int slot = inv.GetAttachmentSlotId(s);
            EntityAI part = inv.FindAttachment(slot);
            if (part)
                it.Inside.Insert(WornOf(part, InventorySlots.GetSlotName(slot)));
        }
        return it;
    }

    // ---- what the ops share ----------------------------------------------------

    protected int IndexOf(OZT_TradersFile file, string id)
    {
        for (int i = 0; i < file.Traders.Count(); i++)
        {
            if (file.Traders[i].Id == id)
                return i;
        }
        return -1;
    }

    // A kind and a class the server can make: any class for an object, a
    // survivor for a character.
    protected string ClassFor(string kind, string type)
    {
        if (kind != OZT_Const.KIND_OBJECT && kind != OZT_Const.KIND_NPC)
            return "STR_OZT_ADMIN_BAD_KIND";
        if (type == "" || !GetGame().ConfigIsExisting("CfgVehicles " + type))
            return "STR_OZT_ADMIN_BAD_CLASS";
        if (kind == OZT_Const.KIND_NPC && !GetGame().IsKindOf(type, "SurvivorBase"))
            return "STR_OZT_ADMIN_BAD_CLASS";
        return "";
    }

    // Where the admin faces, in the degrees a point's Yaw takes.
    protected float Heading(PlayerBase admin)
    {
        vector ori = admin.GetOrientation();
        return ori[0];
    }

    // The points as the file has them, whether each stands, and the
    // categories of the price book.
    protected string ListJson()
    {
        OZT_AdminList list = new OZT_AdminList();
        OZT_TradersFile file = new OZT_TradersFile();
        OZ_ConfigLoader<OZT_TradersFile>.Load(OZT_Const.TRADERS, TAG, file, true, false);
        for (int i = 0; i < file.Traders.Count(); i++)
        {
            OZT_Point p = file.Traders[i];
            OZT_AdminPoint a = new OZT_AdminPoint();
            a.Id = p.Id;
            a.Name = p.Name;
            a.Kind = p.Kind;
            a.ClassName = p.ClassName;
            a.Model = p.Model;
            a.X = p.Pos[0];
            a.Y = p.Pos[1];
            a.Z = p.Pos[2];
            a.Yaw = p.Yaw;
            a.Radius = p.Radius;
            for (int c = 0; c < p.Categories.Count(); c++)
                a.Categories.Insert(p.Categories[c]);
            a.BuyFactor = p.BuyFactor;
            a.SellFactor = p.SellFactor;
            a.Provider = p.Provider;
            a.NoExchange = p.NoExchange;
            if (p.Outfit && p.Outfit.Items)
                a.Worn = p.Outfit.Items.Count();
            if (OZT_Points.BodyOf(p.Id))
                a.Standing = 1;
            list.Points.Insert(a);
        }
        array<ref OZT_Category> cats = OZT_Config.Get().Goods().Categories;
        for (int k = 0; k < cats.Count(); k++)
        {
            OZT_WireCategory wc = new OZT_WireCategory();
            wc.Id = cats[k].Id;
            wc.Name = cats[k].Name;
            list.Categories.Insert(wc);
        }
        string body;
        string err;
        JsonFileLoader<OZT_AdminList>.MakeData(list, body, err, false);
        return body;
    }
}

// The three files in the core's file editor: a text that does not parse
// leaves the file as it is; one that does is checked, written and loaded
// again with everything that stands on it. Three classes, not one generic:
// `new T()` in an Enforce template does not compile (the research mod's
// measurement).
class OZT_SettingsApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZT_Settings c = new OZT_Settings();
        string err;
        if (!JsonFileLoader<OZT_Settings>.LoadData(json, c, err))
        {
            OZT_Log.Warn("admin: the settings were not taken: " + err);
            return false;
        }
        int warnings;
        c.Validate(warnings);
        OZ_ConfigLoader<OZT_Settings>.Save(OZT_Const.SETTINGS, "trader settings", c, true);
        OZT_Config.ReloadLive();
        return true;
    }
}

class OZT_GoodsApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZT_GoodsFile c = new OZT_GoodsFile();
        string err;
        if (!JsonFileLoader<OZT_GoodsFile>.LoadData(json, c, err))
        {
            OZT_Log.Warn("admin: the goods were not taken: " + err);
            return false;
        }
        int warnings;
        c.Validate(warnings);
        OZ_ConfigLoader<OZT_GoodsFile>.Save(OZT_Const.GOODS, "trader goods", c, true);
        OZT_Config.ReloadLive();
        return true;
    }
}

class OZT_TradersApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZT_TradersFile c = new OZT_TradersFile();
        string err;
        if (!JsonFileLoader<OZT_TradersFile>.LoadData(json, c, err))
        {
            OZT_Log.Warn("admin: the traders were not taken: " + err);
            return false;
        }
        int warnings;
        c.Validate(warnings);
        OZ_ConfigLoader<OZT_TradersFile>.Save(OZT_Const.TRADERS, OZT_Admin.TAG, c, true);
        OZT_Config.ReloadLive();
        return true;
    }
}
