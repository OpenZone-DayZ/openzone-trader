// Trader > Points (design §5.6 and §9.5; mockup TraderPoints): the trader points of
// the server, placed and dressed where the admin stands. The list is the section's
// `list` (what stands where); the card edits the picked point in the draft of the
// traders file -- its name, kind, class, radius, factors, categories, its money,
// its stock and the licences its Access asks for -- and the draft's bar applies it
// (cfg_set). What needs the admin's own place or gear stays an op of the section:
// a new point where the admin stands, the aimed thing made a trader, move, turn,
// outfit; and remove, which asks on the confirm strip first, as respawning every
// point does. Those ops rewrite the file on the server, so they wait while the
// draft holds a change to it.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // What the section last said, and the point picked on the list (by id: a
    // refreshed list keeps the pick while the point is there).
    protected ref OZT_AdminList m_TrdList;
    protected int    m_TrdPicked = -1;
    protected string m_TrdPickedId = "";

    // The picked point's kind, change tab and whether it is a thing of the map;
    // the kind of a new point; the licence the card's picker shows.
    protected bool m_TrdNpc = false;
    protected bool m_TrdNoChange = false;
    protected bool m_TrdMap = false;
    protected bool m_TrdNewNpc = false;
    protected int  m_TrdPtLic = 0;

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "points")
            return;
        Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedGates();
        PaintTrdSwitches();
        TrdPaintCard();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "points")
            TrdPaintCard();
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdPt", w);

        // The list takes what the card leaves; the card keeps its 520 units; the
        // draft's bar takes the bottom 56.
        float listW = w - (24 + 16 + 520 + 24) * s;
        if (listW < 200 * s)
            listW = 200 * s;
        float listH = h - (166 + 16 + 56) * s;
        if (listH < 60 * s)
            listH = 60 * s;
        TrdSize("TrdList", listW, listH);
        float cardH = h - (76 + 16 + 56) * s;
        if (cardH < 200 * s)
            cardH = 200 * s;
        TrdPlace("TrdPtCard", listW + 40 * s, 76 * s, 520 * s, cardH);
    }

    // ------------------------------------------------------------------ answers

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        super.TrdOnAnswer(op, ok, json, error);
        string verb = TrdVerb(op);
        array<string> listOps = {"list", "respawn", "place", "adopt", "move", "turn", "remove", "set", "outfit", "licence"};
        if (!ok || listOps.Find(verb) == -1)
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
        // The ops that wrote the traders file: the draft reads it again.
        array<string> writes = {"place", "adopt", "move", "turn", "remove", "set", "outfit"};
        if (writes.Find(verb) != -1)
            TrdReread(OZT_Const.CFG_TRADERS);
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

    // ------------------------------------------------------------------ the list

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

    protected OZT_AdminPoint TrdListPoint()
    {
        if (!m_TrdList || m_TrdPicked < 0 || m_TrdPicked >= m_TrdList.Points.Count())
            return null;
        return m_TrdList.Points[m_TrdPicked];
    }

    // The picked point as the draft holds it; null before the file has come.
    protected OZT_Point TrdPtDraft()
    {
        if (!m_TrdDraft || !m_TrdDraft.Traders || m_TrdPickedId == "")
            return null;
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            if (points[i].Id == m_TrdPickedId)
                return points[i];
        }
        return null;
    }

    // ------------------------------------------------------------------ the card

    protected void TrdPaintCard()
    {
        OZT_AdminPoint lp = TrdListPoint();
        OZT_Point p = TrdPtDraft();
        if (!lp && !p)
        {
            SayTrd("TrdPtHead", TrdTr("#STR_OZT_VPP_PT_PICK"));
            SayTrd("TrdPtHeadSub", "");
            SayTrd("TrdInfo", "");
            array<string> none = new array<string>();
            TrdFill("TrdPtLic", none, null, -1);
            TrdPtPaintLicPick();
            return;
        }
        if (!p)
        {
            FillTrdForm(m_TrdPicked);
            return;
        }
        TrdFillCard(p, lp);
    }

    // The card from the list, until the traders file has come.
    protected void FillTrdForm(int index)
    {
        if (!m_TrdList || index < 0 || index >= m_TrdList.Points.Count())
            return;
        OZT_AdminPoint p = m_TrdList.Points[index];
        TrdSet("TrdName", p.Name);
        TrdSet("TrdClass", TrdThing(p));
        TrdSet("TrdRadius", TrdNum(p.Radius));
        TrdSet("TrdBuy", TrdNum(p.BuyFactor));
        TrdSet("TrdSell", TrdNum(p.SellFactor));
        TrdSet("TrdProvider", p.Provider);
        TrdSet("TrdCats", TrdJoin(p.Categories, ","));
        TrdSet("TrdPtStock", "");
        array<string> none = new array<string>();
        TrdFill("TrdPtLic", none, null, -1);
        // Into locals first: a boolean expression assigned straight into a
        // field is the series' known heap trap (dayz-modding).
        bool npc = p.Kind == OZT_Const.KIND_NPC;
        bool noChange = p.NoExchange == 1;
        bool isMap = p.Kind == OZT_Const.KIND_MAP;
        m_TrdNpc = npc;
        m_TrdNoChange = noChange;
        m_TrdMap = isMap;
        PaintTrdSwitches();
        TrdPtPaintMoney(p.Provider);
        TrdPaintHead(p.Name, p.Id, p.Kind, TrdThing(p), p.Standing);
        TrdPaintInfo(p.X, p.Y, p.Z, p.Yaw, p.Worn);
    }

    protected void TrdFillCard(OZT_Point p, OZT_AdminPoint lp)
    {
        string thing = p.ClassName;
        if (thing == "")
            thing = p.Model;
        TrdSet("TrdName", p.Name);
        TrdSet("TrdClass", thing);
        TrdSet("TrdRadius", TrdNum(p.Radius));
        TrdSet("TrdBuy", TrdNum(p.BuyFactor));
        TrdSet("TrdSell", TrdNum(p.SellFactor));
        TrdSet("TrdCats", TrdJoin(p.Categories, ","));
        TrdSet("TrdProvider", p.Provider);
        TrdSet("TrdPtStock", p.Stock);
        bool npc = p.Kind == OZT_Const.KIND_NPC;
        bool noChange = p.NoExchange == 1;
        bool isMap = p.Kind == OZT_Const.KIND_MAP;
        m_TrdNpc = npc;
        m_TrdNoChange = noChange;
        m_TrdMap = isMap;
        PaintTrdSwitches();
        TrdPtPaintMoney(p.Provider);
        TrdPtPaintAccess(p);
        TrdPtPaintLicPick();

        int standing = 0;
        if (lp)
            standing = lp.Standing;
        TrdPaintHead(p.Name, p.Id, p.Kind, thing, standing);
        int worn = 0;
        if (p.Outfit && p.Outfit.Items)
            worn = p.Outfit.Items.Count();
        float px = 0;
        float py = 0;
        float pz = 0;
        if (p.Pos && p.Pos.Count() == 3)
        {
            px = p.Pos[0];
            py = p.Pos[1];
            pz = p.Pos[2];
        }
        TrdPaintInfo(px, py, pz, p.Yaw, worn);
    }

    protected void TrdPaintHead(string name, string id, string kind, string thing, int standing)
    {
        SayTrd("TrdPtHead", name + "  (" + id + ")");
        string kindWord = TrdKindWord(kind);
        string state = TrdTr("#STR_OZT_VPP_PT_NOT_STANDING");
        if (standing == 1)
            state = TrdTr("#STR_OZT_VPP_PT_STANDING");
        SayTrd("TrdPtHeadSub", kindWord + "  " + thing + "  -  " + state);
    }

    protected void TrdPaintInfo(float x, float y, float z, float yaw, int worn)
    {
        string facing = TrdTr("#STR_OZT_VPP_PT_FACING");
        string wears = TrdTr("#STR_OZT_VPP_PT_WEARS");
        string info = TrdNum(x) + "  " + TrdNum(y) + "  " + TrdNum(z);
        info = info + ",  " + facing + " " + TrdNum(yaw);
        info = info + ",  " + wears + " " + worn.ToString();
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

    // The point's money in words: the server's, cash, cash then the bank, the bank,
    // or the provider's own id.
    protected void TrdPtPaintMoney(string provider)
    {
        string word = provider;
        if (provider == "")
            word = TrdTr("#STR_OZT_VPP_MONEY_SERVER");
        else if (provider == OZT_Const.PROVIDER_ITEMS)
            word = TrdTr("#STR_OZT_VPP_MONEY_CASH");
        else if (provider == "items+traderplus")
            word = TrdTr("#STR_OZT_VPP_MONEY_BOTH");
        else if (provider == "traderplus")
            word = TrdTr("#STR_OZT_VPP_MONEY_BANK");
        SayTrd("BtnTrdPtMoneyText", word);
    }

    // Who may trade here (the point's Access), a condition a row.
    protected void TrdPtPaintAccess(OZT_Point p)
    {
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        if (p && p.Access)
        {
            for (int i = 0; i < p.Access.Count(); i++)
            {
                rows.Insert(TrdCondWords(p.Access[i]));
                if (p.Access[i].Kind == "licence")
                    inks.Insert(0);
                else
                    inks.Insert(OZT_Metal.MUTED);
            }
        }
        TrdFill("TrdPtLic", rows, inks, -1);
    }

    // The settings' licence the card's picker shows; null when there is none.
    protected OZT_LicenceDef TrdPtLicDef()
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Settings.Licences)
            return null;
        array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
        if (defs.Count() == 0)
            return null;
        if (m_TrdPtLic < 0 || m_TrdPtLic >= defs.Count())
            m_TrdPtLic = 0;
        return defs[m_TrdPtLic];
    }

    protected void TrdPtPaintLicPick()
    {
        OZT_LicenceDef d = TrdPtLicDef();
        if (!d)
        {
            SayTrd("BtnTrdPtLicPickText", TrdTr("#STR_OZT_VPP_PT_NO_LICENCES"));
            return;
        }
        SayTrd("BtnTrdPtLicPickText", d.Name + "  (" + d.Id + ")");
    }

    // ------------------------------------------------------------------ edits

    protected bool TrdPtIsField(string nm)
    {
        array<string> fields = {"TrdName", "TrdClass", "TrdRadius", "TrdBuy", "TrdSell", "TrdCats", "TrdProvider", "TrdPtStock"};
        return fields.Find(nm) != -1;
    }

    // A field of the card typed into: the draft's point follows it. A value equal
    // to the draft's is no edit (an OnChange the script's own fill may raise).
    protected void TrdPtEdit(string nm)
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        string v = GetEdit(nm).Trim();
        float f = v.ToFloat();
        bool changed = false;
        if (nm == "TrdName")
        {
            if (p.Name != v)
            {
                p.Name = v;
                changed = true;
            }
        }
        else if (nm == "TrdClass")
        {
            if (p.Kind != OZT_Const.KIND_MAP && p.ClassName != v)
            {
                p.ClassName = v;
                changed = true;
            }
        }
        else if (nm == "TrdRadius")
        {
            if (f > 0 && f != p.Radius)
            {
                p.Radius = f;
                changed = true;
            }
        }
        else if (nm == "TrdBuy")
        {
            if (f > 0 && f != p.BuyFactor)
            {
                p.BuyFactor = f;
                changed = true;
            }
        }
        else if (nm == "TrdSell")
        {
            if (f > 0 && f != p.SellFactor)
            {
                p.SellFactor = f;
                changed = true;
            }
        }
        else if (nm == "TrdCats")
        {
            array<string> cats = new array<string>();
            TrdSplit(v, cats);
            if (TrdJoin(cats, ",") != TrdJoin(p.Categories, ","))
            {
                TrdSplit(v, p.Categories);
                changed = true;
            }
        }
        else if (nm == "TrdProvider")
        {
            if (p.Provider != v)
            {
                p.Provider = v;
                TrdPtPaintMoney(v);
                changed = true;
            }
        }
        else if (nm == "TrdPtStock")
        {
            if (p.Stock != v)
            {
                p.Stock = v;
                changed = true;
            }
        }
        if (changed)
            TrdTouch(OZT_Const.CFG_TRADERS);
    }

    protected void TrdPtKind()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        if (p.Kind == OZT_Const.KIND_MAP)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_MAP_FIXED"));
            return;
        }
        if (p.Kind == OZT_Const.KIND_NPC)
            p.Kind = OZT_Const.KIND_OBJECT;
        else
            p.Kind = OZT_Const.KIND_NPC;
        bool npc = p.Kind == OZT_Const.KIND_NPC;
        m_TrdNpc = npc;
        PaintTrdSwitches();
        TrdTouch(OZT_Const.CFG_TRADERS);
    }

    protected void TrdPtChange()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        if (p.NoExchange == 1)
            p.NoExchange = 0;
        else
            p.NoExchange = 1;
        bool noChange = p.NoExchange == 1;
        m_TrdNoChange = noChange;
        PaintTrdSwitches();
        TrdTouch(OZT_Const.CFG_TRADERS);
    }

    // The money a point may name, one step on: the server's, cash, cash then the
    // TraderPlus bank, the bank.
    protected void TrdPtMoneyNext()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        array<string> ids = {"", OZT_Const.PROVIDER_ITEMS, "items+traderplus", "traderplus"};
        int at = ids.Find(p.Provider) + 1;
        if (at >= ids.Count())
            at = 0;
        p.Provider = ids[at];
        TrdSet("TrdProvider", p.Provider);
        TrdPtPaintMoney(p.Provider);
        TrdTouch(OZT_Const.CFG_TRADERS);
    }

    protected void TrdPtLicAdd()
    {
        OZT_Point p = TrdPtDraft();
        OZT_LicenceDef d = TrdPtLicDef();
        if (!p || !d)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_NO_LICENCES"));
            return;
        }
        if (!p.Access)
            p.Access = new array<ref OZT_Condition>();
        for (int i = 0; i < p.Access.Count(); i++)
        {
            if (p.Access[i].Kind == "licence" && p.Access[i].Arg == d.Id)
            {
                Hint(TrdTr("#STR_OZT_VPP_PT_LIC_HAS"));
                return;
            }
        }
        OZT_Condition c = new OZT_Condition();
        c.Kind = "licence";
        c.Arg = d.Id;
        c.Not = 0;
        p.Access.Insert(c);
        TrdPtPaintAccess(p);
        TrdTouch(OZT_Const.CFG_TRADERS);
    }

    protected void TrdPtLicDel()
    {
        OZT_Point p = TrdPtDraft();
        int row = TrdPickedRow("TrdPtLic");
        if (!p || !p.Access || row < 0 || row >= p.Access.Count())
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_LIC_PICK"));
            return;
        }
        if (p.Access[row].Kind != "licence")
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_LIC_ELSEWHERE"));
            return;
        }
        p.Access.RemoveOrdered(row);
        TrdPtPaintAccess(p);
        TrdTouch(OZT_Const.CFG_TRADERS);
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (!M_SUB_WIDGET || !w || m_Repaint || w.GetName() != "TrdList")
            return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
        if (m_TrdList && row >= 0 && row < m_TrdList.Points.Count())
        {
            m_TrdPicked = row;
            m_TrdPickedId = m_TrdList.Points[row].Id;
            TrdPaintCard();
        }
        return true;
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill && TrdPtIsField(w.GetName()))
        {
            TrdPtEdit(w.GetName());
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w || !M_SUB_WIDGET)
            return super.OnClick(w, x, y, button);
        string nm = w.GetName();

        if (nm == "BtnTrdKind")
        {
            TrdPtKind();
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
            TrdPtChange();
            return true;
        }
        if (nm == "BtnTrdPtMoney")
        {
            TrdPtMoneyNext();
            return true;
        }
        if (nm == "BtnTrdPtLicPick")
        {
            m_TrdPtLic++;
            TrdPtPaintLicPick();
            return true;
        }
        if (nm == "BtnTrdPtLicAdd")
        {
            TrdPtLicAdd();
            return true;
        }
        if (nm == "BtnTrdPtLicDel")
        {
            TrdPtLicDel();
            return true;
        }
        if (nm == "BtnTrdRefresh")
        {
            Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
            TrdReread(OZT_Const.CFG_TRADERS);
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
            if (!TrdDraftHolds(OZT_Const.CFG_TRADERS))
                PlaceTrd();
            return true;
        }
        if (nm == "BtnTrdAdopt")
        {
            if (!TrdDraftHolds(OZT_Const.CFG_TRADERS))
                AdoptTrd();
            return true;
        }

        bool onPoint = nm == "BtnTrdMove" || nm == "BtnTrdTurn" || nm == "BtnTrdOutfit";
        if (nm == "BtnTrdRemove")
            onPoint = true;
        if (!onPoint)
            return super.OnClick(w, x, y, button);

        string id = PickedTrd();
        if (id == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_PICK_FIRST"));
            return true;
        }
        if (TrdDraftHolds(OZT_Const.CFG_TRADERS))
            return true;
        if (nm == "BtnTrdMove")
            Ask(OZT_Const.SECTION_ADMIN, "move:" + id, "{}");
        else if (nm == "BtnTrdTurn")
            Ask(OZT_Const.SECTION_ADMIN, "turn:" + id, "{}");
        else if (nm == "BtnTrdOutfit")
            Ask(OZT_Const.SECTION_ADMIN, "outfit:" + id, "{}");
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

    // Removing a point asks on the window's confirm strip (design §3.4).
    protected void RemoveTrd(string id)
    {
        string ask = TrdTr("#STR_OZT_VPP_PT_REMOVE_ASK");
        string tail = TrdTr("#STR_OZT_VPP_PT_REMOVE_TAIL");
        Confirm(ask + " " + id + tail, "#STR_OZT_VPP_YES_REMOVE", "trd:remove:" + id);
    }

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
