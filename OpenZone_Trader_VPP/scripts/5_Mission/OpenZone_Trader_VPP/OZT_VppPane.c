// The pane TRADERS of the OpenZone admin window (design §9.5): the trader
// points of the server, placed and dressed where the admin stands.
//
// It hangs on the core's window through a modded class, as the other glue
// panes do (factions, radio), and talks to the section `traders` of the
// trader mod (OZT_Admin). The admin's rights are the core dispatcher's to
// check; the pane only asks.
//
// Guards: NO_GUI -- the server compiles Mission without the UI; AVPPAdminTools
// and OpenZone_VPP -- names of CfgMods classes, defined by the engine.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_PANE   = "traders";
    static const string TRD_LAYOUT = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_pane.layout";

    // What the section last said, and the point picked on the list.
    protected ref OZT_AdminList m_TrdList;
    protected int m_TrdPicked = -1;

    // The form's two switches, and the second press of REMOVE.
    protected bool m_TrdNpc = false;
    protected bool m_TrdNoChange = false;
    protected bool m_TrdDelArmed = false;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;
        Widget pane = GetGame().GetWorkspace().CreateWidgets(TRD_LAYOUT, M_SUB_WIDGET);
        if (!pane)
        {
            OZT_Log.Error("traders vpp pane: the layout did not load");
            return;
        }
        RegisterPane(TRD_PANE, "TRADERS", pane, "TrdHint");
        OZ_ClientState.AdminWatch().Insert(this.OnTraderAnswer);
        PaintTrdSwitches();
    }

    void ~OZ_VppAdminMenu()
    {
        OZ_ClientState.AdminWatch().Remove(this.OnTraderAnswer);
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);
        if (id == TRD_PANE)
            Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
    }

    // ---- answers --------------------------------------------------------------

    void OnTraderAnswer(string section, string op, bool ok, string json, string error)
    {
        if (section != OZT_Const.SECTION_ADMIN)
            return;
        if (!ok)
        {
            Hint("traders: " + op + ": " + Words(error));
            return;
        }
        OZT_AdminList list = new OZT_AdminList();
        string err;
        if (!JsonFileLoader<OZT_AdminList>.LoadData(json, list, err))
        {
            Hint("traders: the answer could not be read");
            return;
        }
        m_TrdList = list;
        if (m_TrdPicked >= m_TrdList.Points.Count())
            m_TrdPicked = -1;
        FillTrdList();
        PaintTrdCategories();
        if (op != "list")
            Hint("traders: " + op + " done");
    }

    // ---- the list and the form -------------------------------------------------

    protected void FillTrdList()
    {
        TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TrdList"));
        if (!lb || !m_TrdList)
            return;
        m_Repaint = true;
        lb.ClearItems();
        for (int i = 0; i < m_TrdList.Points.Count(); i++)
        {
            OZT_AdminPoint p = m_TrdList.Points[i];
            string line = p.Id + "  " + p.Name + "  (" + p.Kind + ", " + p.ClassName + ")";
            if (p.Standing != 1)
                line = line + "  NOT STANDING";
            lb.AddItem(line, NULL, 0);
            if (i == m_TrdPicked)
                lb.SelectRow(i);
        }
        m_Repaint = false;
        if (m_TrdPicked >= 0)
            FillTrdForm(m_TrdPicked);
    }

    protected void FillTrdForm(int index)
    {
        if (!m_TrdList || index < 0 || index >= m_TrdList.Points.Count())
            return;
        OZT_AdminPoint p = m_TrdList.Points[index];
        SetEdit("TrdId", p.Id);
        SetEdit("TrdName", p.Name);
        SetEdit("TrdClass", p.ClassName);
        SetEdit("TrdRadius", p.Radius.ToString());
        SetEdit("TrdBuy", p.BuyFactor.ToString());
        SetEdit("TrdSell", p.SellFactor.ToString());
        SetEdit("TrdProvider", p.Provider);
        string cats = "";
        for (int c = 0; c < p.Categories.Count(); c++)
        {
            if (c > 0)
                cats = cats + ",";
            cats = cats + p.Categories[c];
        }
        SetEdit("TrdCats", cats);
        // Into locals first: a boolean expression assigned straight into a
        // field is the series' known heap trap (dayz-modding).
        bool npc = p.Kind == OZT_Const.KIND_NPC;
        bool noChange = p.NoExchange == 1;
        m_TrdNpc = npc;
        m_TrdNoChange = noChange;
        PaintTrdSwitches();

        string info = "at " + p.X.ToString() + " " + p.Y.ToString() + " " + p.Z.ToString();
        info = info + ", facing " + p.Yaw.ToString() + ", wears " + p.Worn.ToString();
        SayTrd("TrdInfo", info);
        m_TrdDelArmed = false;
    }

    protected void PaintTrdSwitches()
    {
        string kind = "KIND: object";
        if (m_TrdNpc)
            kind = "KIND: npc (a survivor class)";
        SayTrd("BtnTrdKindText", kind);
        string change = "CHANGES NOTES: yes";
        if (m_TrdNoChange)
            change = "CHANGES NOTES: no";
        SayTrd("BtnTrdChangeText", change);
    }

    protected void PaintTrdCategories()
    {
        if (!m_TrdList)
            return;
        string known = "categories of the price book:";
        for (int i = 0; i < m_TrdList.Categories.Count(); i++)
            known = known + " " + m_TrdList.Categories[i].Id;
        SayTrd("TrdCatsKnown", known);
    }

    protected void SayTrd(string name, string text)
    {
        TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget(name));
        if (t)
            t.SetText(text);
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

    // ---- gestures --------------------------------------------------------------

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (!M_SUB_WIDGET || m_Repaint || !w || w.GetName() != "TrdList")
            return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
        if (m_TrdList && row >= 0 && row < m_TrdList.Points.Count())
        {
            m_TrdPicked = row;
            FillTrdForm(row);
            Hint("traders: " + m_TrdList.Points[row].Id + " - edit and SAVE POINT, or stand somewhere and MOVE HERE");
        }
        return true;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w || !M_SUB_WIDGET)
            return super.OnClick(w, x, y, button);
        string nm = w.GetName();
        if (nm.IndexOf("BtnTrd") != 0)
            return super.OnClick(w, x, y, button);

        if (nm == "BtnTrdKind")
        {
            if (m_TrdNpc)
                m_TrdNpc = false;
            else
                m_TrdNpc = true;
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
        if (nm == "BtnTrdRespawn")
        {
            Ask(OZT_Const.SECTION_ADMIN, "respawn", "{}");
            return true;
        }
        if (nm == "BtnTrdPlace")
        {
            PlaceTrd();
            return true;
        }

        string id = PickedTrd();
        if (id == "")
        {
            Hint("traders: pick a point on the list first");
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
        else if (nm == "BtnTrdRemove")
            RemoveTrd(id);
        return true;
    }

    protected void PlaceTrd()
    {
        OZT_AdminPlace req = new OZT_AdminPlace();
        req.Id = GetEdit("TrdId");
        req.Name = GetEdit("TrdName");
        req.Kind = TrdKind();
        req.ClassName = GetEdit("TrdClass");
        string body;
        string err;
        JsonFileLoader<OZT_AdminPlace>.MakeData(req, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "place", body);
        m_TrdPicked = -1;
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
        req.Provider = GetEdit("TrdProvider");
        req.NoExchange = 0;
        if (m_TrdNoChange)
            req.NoExchange = 1;
        array<string> cats = new array<string>();
        GetEdit("TrdCats").Split(",", cats);
        for (int i = 0; i < cats.Count(); i++)
        {
            string c = cats[i].Trim();
            if (c != "")
                req.Categories.Insert(c);
        }
        string body;
        string err;
        JsonFileLoader<OZT_AdminSet>.MakeData(req, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "set:" + id, body);
    }

    // Two presses: modal windows in VPP are a trap, as the other panes say.
    protected void RemoveTrd(string id)
    {
        if (!m_TrdDelArmed)
        {
            m_TrdDelArmed = true;
            Hint("traders: press REMOVE again to remove " + id);
            return;
        }
        m_TrdDelArmed = false;
        Ask(OZT_Const.SECTION_ADMIN, "remove:" + id, "{}");
        m_TrdPicked = -1;
    }
}

#endif
#endif
#endif
