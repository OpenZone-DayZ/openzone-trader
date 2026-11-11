// The admin's side of the traders (design §9.5 and §9.6): the section
// `traders` of the core's admin registry, which the pane TRADERS of the glue
// pbo OpenZone_Trader_VPP talks to, and the three files in the core's file
// editor (cfg_get / cfg_set).
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

        PlayerBase admin = null;
        if (sender)
            admin = PlayerBase.Cast(sender.GetPlayer());

        OZT_TradersFile file = new OZT_TradersFile();
        if (!OZ_ConfigLoader<OZT_TradersFile>.Load(OZT_Const.TRADERS, TAG, file, true, false))
        {
            error = "STR_OZT_ADMIN_NOT_WRITABLE";
            return "";
        }

        string why = "";
        if (verb == "place")
            why = Place(file, json, admin);
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

    protected string Move(OZT_TradersFile file, string id, PlayerBase admin)
    {
        if (!admin)
            return "STR_OZT_ADMIN_NO_PLAYER";
        int i = IndexOf(file, id);
        if (i < 0)
            return "STR_OZT_ADMIN_NO_POINT";
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
        string why = ClassFor(req.Kind, req.ClassName);
        if (why != "")
            return why;

        OZT_Point p = file.Traders[i];
        p.Name = req.Name;
        p.Kind = req.Kind;
        p.ClassName = req.ClassName;
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
