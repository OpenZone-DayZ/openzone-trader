// The module TRADER of the OpenZone admin window (VPP rework 2026-11-12, design
// §5.6): its pages on the window's rail, in the window's metal. This file is the
// first of the trader's layers of OZ_VppAdminMenu: it registers the pages, routes
// their answers and their events, and holds the page Points (design §9.5): the
// trader points of the server, placed and dressed where the admin stands.
//
// Every page has a layout of its own (ui/OpenZone_Trader_VPP/ozt_vpp_<key>.json,
// built by layout_build) and a layer of its own in the file OZT_VppPane<Page>.c,
// whose name sorts after this one: the files of a module folder compile in
// alphabetical order, and a layer that overrides a hook must come after the layer
// that declares it. A page's layer overrides the hooks below (TrdShown, TrdLayout,
// TrdOnAnswer, TrdOnConfig) for its own key and calls super for the rest.
//
// Every name this module adds to the window's class starts with Trd / m_Trd: every
// glue mods the same OZ_VppAdminMenu, and a second declaration of one name in
// another glue is a compile error of the whole module. Confirm-strip actions it
// owns start with "trd:".
//
// The pane talks to the section `traders` of the trader mod (OZT_Admin). The
// admin's rights are the core dispatcher's to check; the pages only ask.
//
// Guards: NO_GUI -- the server compiles Mission without the UI; AVPPAdminTools
// and OpenZone_VPP -- names of CfgMods classes, defined by the engine.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_MOD     = "trader";
    static const string TRD_LAYOUTS = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_";

    // The pages' roots by key ("points", ...) and the key of each rail id; the
    // content area the shell gave the panes, in pixels (OnWindowResized).
    protected ref map<string, Widget> m_TrdPanes;
    protected ref map<string, string> m_TrdKeys;
    protected float m_TrdW = 0;
    protected float m_TrdH = 0;

    // Points: what the section last said, and the point picked on the list (by
    // id: a refreshed list keeps the pick when the point is still there).
    protected ref OZT_AdminList m_TrdList;
    protected int    m_TrdPicked = -1;
    protected string m_TrdPickedId = "";

    // The card's switches: the picked point's kind, its change tab, whether it is
    // a thing of the map; and the kind of a new point.
    protected bool m_TrdNpc = false;
    protected bool m_TrdNoChange = false;
    protected bool m_TrdMap = false;
    protected bool m_TrdNewNpc = false;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;

        m_TrdPanes = new map<string, Widget>();
        m_TrdKeys = new map<string, string>();
        array<string> keys = new array<string>();
        array<string> labels = new array<string>();
        TrdPageList(keys, labels);
        for (int i = 0; i < keys.Count(); i++)
            TrdAddPage(TRD_MOD, "#STR_OZT_VPP_MOD", TRD_MOD + "." + keys[i], keys[i], labels[i]);

        OZ_ClientState.AdminWatch().Insert(this.OnTraderAnswer);
        PaintTrdSwitches();
        TrdPaintCard();
    }

    void ~OZ_VppAdminMenu()
    {
        OZ_ClientState.AdminWatch().Remove(this.OnTraderAnswer);
    }

    // The module's pages in the rail's order, with their captions.
    protected void TrdPageList(array<string> keys, array<string> labels)
    {
        keys.Insert("points");
        labels.Insert("#STR_OZT_VPP_PAGE_POINTS");
        keys.Insert("licences");
        labels.Insert("#STR_OZT_VPP_PAGE_LICENCES");
        keys.Insert("import");
        labels.Insert("#STR_OZT_VPP_PAGE_IMPORT");
    }

    // One page: its own layout under the window, registered on the rail.
    protected Widget TrdAddPage(string module, string moduleLabel, string id, string key, string label)
    {
        string path = TRD_LAYOUTS + key + ".layout";
        Widget pane = GetGame().GetWorkspace().CreateWidgets(path, M_SUB_WIDGET);
        if (!pane)
        {
            OZT_Log.Error("trader vpp: a page's layout did not load: " + path);
            return null;
        }
        m_TrdPanes.Set(key, pane);
        m_TrdKeys.Set(id, key);
        RegisterPage(module, moduleLabel, id, label, pane, "");
        return pane;
    }

    // Is a page of ours on screen, and which: "" when none.
    protected string TrdShownKey()
    {
        string key = "";
        if (m_TrdKeys)
            m_TrdKeys.Find(CurrentPane(), key);
        return key;
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);
        string key;
        if (m_TrdKeys && m_TrdKeys.Find(id, key))
            TrdShown(key);
    }

    // A page of ours came on screen. Each page's layer takes its own key.
    protected void TrdShown(string key)
    {
        if (key == "points")
            Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
    }

    // The window was resized; every page lays itself out from w x h (pixels) and
    // reads no size back (a widget measured before the engine's first layout pass
    // reads 0).
    override void OnWindowResized(float w, float h)
    {
        super.OnWindowResized(w, h);
        m_TrdW = w;
        m_TrdH = h;
        if (M_SUB_WIDGET && m_TrdPanes)
            TrdLayout(w, h);
    }

    protected void TrdLayout(float w, float h)
    {
        float s = UnitScale();
        TrdLayHead("TrdPt", w);

        // The list takes what the card leaves; the card keeps its 520 units.
        float listW = w - (24 + 16 + 520 + 24) * s;
        if (listW < 200 * s)
            listW = 200 * s;
        float listH = h - (166 + 24) * s;
        if (listH < 60 * s)
            listH = 60 * s;
        TrdSize("TrdList", listW, listH);
        float cardH = h - (76 + 24) * s;
        if (cardH < 200 * s)
            cardH = 200 * s;
        TrdPlace("TrdPtCard", listW + 40 * s, 76 * s, 520 * s, cardH);
    }

    // ------------------------------------------------------------------ helpers

    protected Widget TrdW(string name)
    {
        if (!M_SUB_WIDGET)
            return null;
        return M_SUB_WIDGET.FindAnyWidget(name);
    }

    // Pixels, as exact-size widgets take them.
    protected void TrdSize(string name, float w, float h)
    {
        Widget x = TrdW(name);
        if (x)
            x.SetSize(w, h);
    }

    protected void TrdPlace(string name, float x, float y, float w, float h)
    {
        Widget it = TrdW(name);
        if (!it)
            return;
        it.SetPos(x, y);
        it.SetSize(w, h);
    }

    protected void TrdMove(string name, float x, float y)
    {
        Widget it = TrdW(name);
        if (it)
            it.SetPos(x, y);
    }

    // A page head's rule follows the window's width.
    protected void TrdLayHead(string prefix, float w)
    {
        TrdSize(prefix + "HeadRule", w, UnitScale());
    }

    // A string of the trader's stringtable in the client's language.
    protected string TrdTr(string key)
    {
        return Widget.TranslateString(key);
    }

    protected void SayTrd(string name, string text)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (t)
            t.SetText(text);
    }

    protected void TrdInk(string name, string text, int color)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(color);
    }

    // A listbox from rows: `inks` colours rows (0: the layout's own), `picked` is
    // selected and kept in view. The core's m_Repaint swallows the selection
    // events SelectRow may raise.
    protected void TrdFill(string name, array<string> rows, array<int> inks, int picked)
    {
        TextListboxWidget lb = TextListboxWidget.Cast(TrdW(name));
        if (!lb)
            return;
        m_Repaint = true;
        lb.ClearItems();
        for (int i = 0; i < rows.Count(); i++)
        {
            lb.AddItem(rows[i], null, 0);
            if (inks && i < inks.Count() && inks[i] != 0)
                lb.SetItemColor(i, 0, inks[i]);
        }
        if (picked >= 0 && picked < rows.Count())
        {
            lb.SelectRow(picked);
            lb.EnsureVisible(picked);
        }
        m_Repaint = false;
    }

    // The verb of an op: "move:npc" -> "move".
    protected string TrdVerb(string op)
    {
        int colon = op.IndexOf(":");
        if (colon < 0)
            return op;
        return op.Substring(0, colon);
    }

    // A comma list typed by the admin, trimmed, the empty ones left out.
    protected void TrdSplit(string text, array<string> into)
    {
        into.Clear();
        array<string> parts = new array<string>();
        text.Split(",", parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            string one = parts[i].Trim();
            if (one != "")
                into.Insert(one);
        }
    }

    protected string TrdJoin(array<string> items, string sep)
    {
        string all = "";
        if (!items)
            return all;
        for (int i = 0; i < items.Count(); i++)
        {
            if (i > 0)
                all = all + sep;
            all = all + items[i];
        }
        return all;
    }

    // ------------------------------------------------------------------ answers

    void OnTraderAnswer(string section, string op, bool ok, string json, string error)
    {
        if (!M_SUB_WIDGET)
            return;
        if (section == OZ_AdminSect.CONFIG)
        {
            TrdOnConfig(op, ok, json, error);
            return;
        }
        if (section != OZT_Const.SECTION_ADMIN)
            return;
        TrdOnAnswer(op, ok, json, error);
    }

    // An answer of the core's config section (the trader's files, later layers).
    protected void TrdOnConfig(string op, bool ok, string json, string error)
    {
    }

    // An answer of the section `traders`. A failure is in the window's journal
    // already, with the server's words; the pages only follow what changed.
    protected void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        if (!ok || !TrdListOp(TrdVerb(op)))
            return;
        OZT_AdminList list = new OZT_AdminList();
        string err;
        if (!JsonFileLoader<OZT_AdminList>.LoadData(json, list, err))
        {
            string unreadable = TrdTr("#STR_OZT_VPP_UNREADABLE");
            Hint(unreadable + " " + op);
            return;
        }
        TrdTakeList(list);
    }

    // The ops the section answers with the points list.
    protected bool TrdListOp(string verb)
    {
        array<string> verbs = {"list", "respawn", "place", "adopt", "move", "turn", "remove", "set", "outfit", "licence", "import_traderplus"};
        return verbs.Find(verb) != -1;
    }

    // The list as the script's own copy: what the serializer made lives only as
    // long as its parse (the core's OZ_ConfigBase header).
    protected void TrdTakeList(OZT_AdminList parsed)
    {
        OZT_AdminList list = new OZT_AdminList();
        for (int i = 0; i < parsed.Points.Count(); i++)
        {
            OZT_AdminPoint src = parsed.Points[i];
            if (!src)
                continue;
            OZT_AdminPoint p = new OZT_AdminPoint();
            p.Id = src.Id;
            p.Name = src.Name;
            p.Kind = src.Kind;
            p.ClassName = src.ClassName;
            p.Model = src.Model;
            p.X = src.X;
            p.Y = src.Y;
            p.Z = src.Z;
            p.Yaw = src.Yaw;
            p.Radius = src.Radius;
            if (src.Categories)
            {
                for (int c = 0; c < src.Categories.Count(); c++)
                    p.Categories.Insert(src.Categories[c]);
            }
            p.BuyFactor = src.BuyFactor;
            p.SellFactor = src.SellFactor;
            p.Provider = src.Provider;
            p.NoExchange = src.NoExchange;
            p.Worn = src.Worn;
            p.Standing = src.Standing;
            list.Points.Insert(p);
        }
        for (int k = 0; k < parsed.Categories.Count(); k++)
        {
            OZT_WireCategory wsrc = parsed.Categories[k];
            if (!wsrc)
                continue;
            OZT_WireCategory wc = new OZT_WireCategory();
            wc.Id = wsrc.Id;
            wc.Name = wsrc.Name;
            list.Categories.Insert(wc);
        }
        m_TrdList = list;

        m_TrdPicked = -1;
        for (int n = 0; n < m_TrdList.Points.Count(); n++)
        {
            if (m_TrdList.Points[n].Id == m_TrdPickedId)
                m_TrdPicked = n;
        }
        if (m_TrdPicked < 0)
            m_TrdPickedId = "";
        FillTrdList();
        PaintTrdCategories();
        TrdPaintCard();
    }

    // ------------------------------------------------------------------ points

    protected void FillTrdList()
    {
        if (!m_TrdList)
            return;
        string notStanding = TrdTr("#STR_OZT_VPP_PT_NOT_STANDING");
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        for (int i = 0; i < m_TrdList.Points.Count(); i++)
        {
            OZT_AdminPoint p = m_TrdList.Points[i];
            string kind = TrdKindWord(p.Kind);
            string line = p.Name + "  (" + p.Id + ")  " + kind;
            line = line + "  " + TrdThing(p);
            int ink = 0;
            if (p.Standing != 1)
            {
                line = line + "  -  " + notStanding;
                ink = OZT_Metal.ALERT;
            }
            rows.Insert(line);
            inks.Insert(ink);
        }
        TrdFill("TrdList", rows, inks, m_TrdPicked);
    }

    // The kind as the admin reads it.
    protected string TrdKindWord(string kind)
    {
        if (kind == OZT_Const.KIND_NPC)
            return TrdTr("#STR_OZT_VPP_KIND_NPC");
        if (kind == OZT_Const.KIND_MAP)
            return TrdTr("#STR_OZT_VPP_KIND_MAP");
        return TrdTr("#STR_OZT_VPP_KIND_OBJECT");
    }

    // The card of the picked point, or its empty state.
    protected void TrdPaintCard()
    {
        if (!m_TrdList || m_TrdPicked < 0 || m_TrdPicked >= m_TrdList.Points.Count())
        {
            SayTrd("TrdPtHead", TrdTr("#STR_OZT_VPP_PT_PICK"));
            SayTrd("TrdPtHeadSub", "");
            SayTrd("TrdInfo", "");
            return;
        }
        FillTrdForm(m_TrdPicked);
    }

    protected void FillTrdForm(int index)
    {
        if (!m_TrdList || index < 0 || index >= m_TrdList.Points.Count())
            return;
        OZT_AdminPoint p = m_TrdList.Points[index];
        SetEdit("TrdName", p.Name);
        SetEdit("TrdClass", TrdThing(p));
        SetEdit("TrdRadius", p.Radius.ToString());
        SetEdit("TrdBuy", p.BuyFactor.ToString());
        SetEdit("TrdSell", p.SellFactor.ToString());
        SetEdit("TrdProvider", p.Provider);
        SetEdit("TrdCats", TrdJoin(p.Categories, ","));
        // Into locals first: a boolean expression assigned straight into a
        // field is the series' known heap trap (dayz-modding).
        bool npc = p.Kind == OZT_Const.KIND_NPC;
        bool noChange = p.NoExchange == 1;
        bool isMap = p.Kind == OZT_Const.KIND_MAP;
        m_TrdNpc = npc;
        m_TrdNoChange = noChange;
        m_TrdMap = isMap;
        PaintTrdSwitches();

        string kind = TrdKindWord(p.Kind);
        SayTrd("TrdPtHead", p.Name + "  (" + p.Id + ")");
        string sub = kind + "  " + TrdThing(p) + "  -  ";
        if (p.Standing == 1)
            sub = sub + TrdTr("#STR_OZT_VPP_PT_STANDING");
        else
            sub = sub + TrdTr("#STR_OZT_VPP_PT_NOT_STANDING");
        SayTrd("TrdPtHeadSub", sub);

        string facing = TrdTr("#STR_OZT_VPP_PT_FACING");
        string wears = TrdTr("#STR_OZT_VPP_PT_WEARS");
        string info = p.X.ToString() + "  " + p.Y.ToString() + "  " + p.Z.ToString();
        info = info + ",  " + facing + " " + p.Yaw.ToString();
        info = info + ",  " + wears + " " + p.Worn.ToString();
        SayTrd("TrdInfo", info);
    }

    protected void PaintTrdSwitches()
    {
        string kind = TrdKindWord(OZT_Const.KIND_OBJECT);
        if (m_TrdNpc)
            kind = TrdKindWord(OZT_Const.KIND_NPC);
        if (m_TrdMap)
            kind = TrdKindWord(OZT_Const.KIND_MAP);
        SayTrd("BtnTrdKindText", kind);

        string newKind = TrdKindWord(OZT_Const.KIND_OBJECT);
        if (m_TrdNewNpc)
            newKind = TrdKindWord(OZT_Const.KIND_NPC);
        SayTrd("BtnTrdNewKindText", newKind);

        string change = TrdTr("#STR_OZT_VPP_CHANGE_YES");
        if (m_TrdNoChange)
            change = TrdTr("#STR_OZT_VPP_CHANGE_NO");
        SayTrd("BtnTrdChangeText", change);
    }

    protected void PaintTrdCategories()
    {
        if (!m_TrdList)
            return;
        string known = TrdTr("#STR_OZT_VPP_PT_CATS_KNOWN");
        for (int i = 0; i < m_TrdList.Categories.Count(); i++)
        {
            if (i > 0)
                known = known + ",";
            known = known + " " + m_TrdList.Categories[i].Id;
        }
        SayTrd("TrdCatsKnown", known);
    }

    // What stands at a point: its class, or a classless prop's model.
    protected string TrdThing(OZT_AdminPoint p)
    {
        if (p.ClassName != "")
            return p.ClassName;
        return p.Model;
    }

    protected string PickedTrd()
    {
        if (!m_TrdList || m_TrdPicked < 0 || m_TrdPicked >= m_TrdList.Points.Count())
            return "";
        return m_TrdList.Points[m_TrdPicked].Id;
    }

    protected string TrdKind()
    {
        if (m_TrdNpc)
            return OZT_Const.KIND_NPC;
        return OZT_Const.KIND_OBJECT;
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (!M_SUB_WIDGET || !w)
            return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
        if (m_Repaint)
            return true;
        if (w.GetName() != "TrdList")
            return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
        if (m_TrdList && row >= 0 && row < m_TrdList.Points.Count())
        {
            m_TrdPicked = row;
            m_TrdPickedId = m_TrdList.Points[row].Id;
            TrdPaintCard();
        }
        return true;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w || !M_SUB_WIDGET)
            return super.OnClick(w, x, y, button);
        string nm = w.GetName();

        if (nm == "BtnTrdKind")
        {
            if (m_TrdMap)
            {
                Hint(TrdTr("#STR_OZT_VPP_PT_MAP_FIXED"));
                return true;
            }
            if (m_TrdNpc)
                m_TrdNpc = false;
            else
                m_TrdNpc = true;
            PaintTrdSwitches();
            return true;
        }
        if (nm == "BtnTrdNewKind")
        {
            if (m_TrdNewNpc)
                m_TrdNewNpc = false;
            else
                m_TrdNewNpc = true;
            PaintTrdSwitches();
            return true;
        }
        if (nm == "BtnTrdChange")
        {
            if (m_TrdNoChange)
                m_TrdNoChange = false;
            else
                m_TrdNoChange = true;
            PaintTrdSwitches();
            return true;
        }
        if (nm == "BtnTrdRefresh")
        {
            Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
            return true;
        }
        // Respawning every point goes through the confirm strip (design §3.4).
        if (nm == "BtnTrdRespawn")
        {
            Confirm(TrdTr("#STR_OZT_VPP_PT_RESPAWN_ASK"), "#STR_OZT_VPP_YES_RESPAWN", "trd:respawn");
            return true;
        }
        if (nm == "BtnTrdPlace")
        {
            PlaceTrd();
            return true;
        }
        if (nm == "BtnTrdAdopt")
        {
            AdoptTrd();
            return true;
        }

        bool onPoint = nm == "BtnTrdMove" || nm == "BtnTrdTurn" || nm == "BtnTrdOutfit";
        if (nm == "BtnTrdSave" || nm == "BtnTrdRemove")
            onPoint = true;
        if (!onPoint)
            return super.OnClick(w, x, y, button);

        string id = PickedTrd();
        if (id == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_PICK_FIRST"));
            return true;
        }
        if (nm == "BtnTrdMove")
            Ask(OZT_Const.SECTION_ADMIN, "move:" + id, "{}");
        else if (nm == "BtnTrdTurn")
            Ask(OZT_Const.SECTION_ADMIN, "turn:" + id, "{}");
        else if (nm == "BtnTrdOutfit")
            Ask(OZT_Const.SECTION_ADMIN, "outfit:" + id, "{}");
        else if (nm == "BtnTrdSave")
            SaveTrd(id);
        else
            RemoveTrd(id);
        return true;
    }

    // A new point where the admin stands: the id, class and kind of the new
    // point's fields; its name is its id until the card says otherwise.
    protected void PlaceTrd()
    {
        OZT_AdminPlace req = new OZT_AdminPlace();
        req.Id = GetEdit("TrdId").Trim();
        req.Name = req.Id;
        req.Kind = OZT_Const.KIND_OBJECT;
        if (m_TrdNewNpc)
            req.Kind = OZT_Const.KIND_NPC;
        req.ClassName = GetEdit("TrdNewClass").Trim();
        string body;
        string err;
        JsonFileLoader<OZT_AdminPlace>.MakeData(req, body, err, false);
        m_TrdPickedId = req.Id;
        Ask(OZT_Const.SECTION_ADMIN, "place", body);
    }

    // The thing where the admin looked before opening the window becomes a
    // point of kind map: the camera has not moved since, so its ray still
    // points at it. The server finds the thing again and makes nothing.
    protected void AdoptTrd()
    {
        Object thing = AimedTrd();
        if (!thing)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_NOTHING_AIMED"));
            return;
        }
        OZT_AdminAdopt req = new OZT_AdminAdopt();
        req.Id = GetEdit("TrdId").Trim();
        req.Name = req.Id;
        req.ClassName = thing.GetType();
        if (req.ClassName == "")
            req.Model = thing.GetShapeName();
        vector at = thing.GetPosition();
        req.X = at[0];
        req.Y = at[1];
        req.Z = at[2];
        string body;
        string err;
        JsonFileLoader<OZT_AdminAdopt>.MakeData(req, body, err, false);
        m_TrdPickedId = req.Id;
        Ask(OZT_Const.SECTION_ADMIN, "adopt", body);
        string adopting = TrdTr("#STR_OZT_VPP_PT_ADOPTING");
        Hint(adopting + " " + OZT_ClientPoints.KeyOf(thing));
    }

    // The first thing on the camera's ray, the player left out.
    protected Object AimedTrd()
    {
        Object me = GetGame().GetPlayer();
        vector from = GetGame().GetCurrentCameraPosition();
        vector to = from + GetGame().GetCurrentCameraDirection() * 50;
        vector contact;
        vector normal;
        int comp;
        set<Object> hits = new set<Object>();
        DayZPhysics.RaycastRV(from, to, contact, normal, comp, hits, null, me, true, false, ObjIntersectView, 0.0);
        for (int i = 0; i < hits.Count(); i++)
        {
            Object o = hits.Get(i);
            if (o && o != me)
                return o;
        }
        return null;
    }

    protected void SaveTrd(string id)
    {
        OZT_AdminSet req = new OZT_AdminSet();
        req.Name = GetEdit("TrdName");
        req.Kind = TrdKind();
        req.ClassName = GetEdit("TrdClass");
        req.Radius = GetEdit("TrdRadius").ToFloat();
        req.BuyFactor = GetEdit("TrdBuy").ToFloat();
        req.SellFactor = GetEdit("TrdSell").ToFloat();
        req.Provider = GetEdit("TrdProvider").Trim();
        req.NoExchange = 0;
        if (m_TrdNoChange)
            req.NoExchange = 1;
        TrdSplit(GetEdit("TrdCats"), req.Categories);
        string body;
        string err;
        JsonFileLoader<OZT_AdminSet>.MakeData(req, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "set:" + id, body);
    }

    // Removing a point asks on the window's confirm strip (design §3.4).
    protected void RemoveTrd(string id)
    {
        string ask = TrdTr("#STR_OZT_VPP_PT_REMOVE_ASK");
        string tail = TrdTr("#STR_OZT_VPP_PT_REMOVE_TAIL");
        Confirm(ask + " " + id + tail, "#STR_OZT_VPP_YES_REMOVE", "trd:remove:" + id);
    }

    // The confirm strip's "yes" for the trader's actions; the rest go on.
    override void OnConfirmed(string action)
    {
        if (action.IndexOf("trd:remove:") == 0)
        {
            string id = action.Substring(11, action.Length() - 11);
            Ask(OZT_Const.SECTION_ADMIN, "remove:" + id, "{}");
            m_TrdPicked = -1;
            m_TrdPickedId = "";
            return;
        }
        if (action == "trd:respawn")
        {
            Ask(OZT_Const.SECTION_ADMIN, "respawn", "{}");
            return;
        }
        super.OnConfirmed(action);
    }
}

#endif
#endif
#endif
