// Research > Trade (design §5.5; mockup ResearchTrade): what the trader's files hold
// on the nodes of the research tree -- the goods a node opens (a lock of a category
// or a line), the discounts it gives, the points whose access needs it -- who has
// each node (the factions that completed it), where the research family holds, and
// the conditions that name nodes the tree no longer has.
//
// A page that knows two mods lives in their glue (the platform rule): this page
// knows the trader alone. The tree's nodes and who completed them come from the
// research gate of OpenZone_Trader_Research, through the trader's own ops -- `gates`
// (the gate's choices: every node of the tree) and `gate_holders` (the factions
// that completed one). The gate is the server's: it may load as a server mod, so a
// client has no define of it, and the page appears when the server's `gates` name
// the family "research" (asked once as the window opens).
//
// It reads what is in force on the server: the files as the server last gave them,
// not the draft's edits. It sits under the module "research" of the rail, after the
// research glue's own pages: it registers when the answer to `gates` arrives,
// after every glue's layer has registered its own.
//
// A layer of OZ_VppAdminMenu after OZT_VppPaneGates.c (it takes TrdOnHolders from it).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_RS_ID = "rtrade";

    // The nodes the trader's files name, and per node how many locks, discounts and
    // points' access name it; which node each row of the list shows; the node
    // picked; the list's filter (0 all, 1 with access, 2 not in the tree).
    protected ref array<string> m_TrdRsIds;
    protected ref array<int> m_TrdRsLocks;
    protected ref array<int> m_TrdRsCuts;
    protected ref array<int> m_TrdRsAccess;
    protected ref array<int> m_TrdRsRows;
    protected string m_TrdRsPicked = "";
    protected int m_TrdRsFilter = 0;

    // Who has the picked node, as gate_holders said it.
    protected ref array<string> m_TrdRsWho;
    protected string m_TrdRsWhoFor = "";
    protected int m_TrdRsWhoState = 0;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TrdRsProbe, 0, false);
    }

    void ~OZ_VppAdminMenu()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.TrdRsProbe);
    }

    // Does the server run the research gate? `gates` says (a read, not journaled).
    void TrdRsProbe()
    {
        if (!M_SUB_WIDGET)
            return;
        if (m_TrdGates)
            TrdGatesKnown();
        else
            TrdNeedGates();
    }

    override void TrdGatesKnown()
    {
        super.TrdGatesKnown();
        if (!m_TrdGates)
            return;
        for (int f = 0; f < m_TrdGates.Families.Count(); f++)
        {
            if (m_TrdGates.Families[f].Family == "research")
            {
                TrdRsRegister();
                return;
            }
        }
    }

    // The page on the rail under "research": after the research glue's eight pages,
    // which registered in their OnCreate. The label of the module is the research
    // glue's when it registered first; ours stands when it is not loaded.
    void TrdRsRegister()
    {
        if (!M_SUB_WIDGET || !m_TrdPanes || m_TrdPanes.Contains(TRD_RS_ID))
            return;
        Widget pane = TrdAddPage("research", "#STR_OZT_VPP_MOD_RESEARCH", TRD_RS_ID, TRD_RS_ID, "#STR_OZT_VPP_PAGE_RTRADE");
        if (pane && m_TrdW > 0)
            TrdLayout(m_TrdW, m_TrdH);
    }

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != TRD_RS_ID)
            return;
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdNeedGates();
        TrdRsPaint();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == TRD_RS_ID)
            TrdRsPaint();
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdRs", w);
        float colH = h - (76 + 24) * s;
        TrdPlace("TrdRsLeft", 24 * s, 76 * s, 400 * s, colH);
        float listH = colH - (104 + 120) * s;
        if (listH < 60 * s)
            listH = 60 * s;
        TrdSize("TrdRsNodes", 400 * s, listH);
        float rightW = w - (24 + 400 + 16 + 24) * s;
        if (rightW < 300 * s)
            rightW = 300 * s;
        TrdPlace("TrdRsRight", 440 * s, 76 * s, rightW, colH);
        TrdSize("TrdRsDetail", rightW, colH - 54 * s);
    }

    // ------------------------------------------------------------------ reading

    // Every node a list of conditions names under the kind "research": each
    // alternative of each condition counts once for `what` (0 lock, 1 discount,
    // 2 a point's access).
    protected void TrdRsCount(array<ref OZT_Condition> conds, int what)
    {
        if (!conds)
            return;
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c || c.Kind != "research")
                continue;
            array<string> alts = new array<string>();
            c.Arg.Split("|", alts);
            for (int a = 0; a < alts.Count(); a++)
            {
                string id = alts[a].Trim();
                if (id != "")
                    TrdRsAdd(id, what);
            }
        }
    }

    protected void TrdRsCountDiscounts(array<ref OZT_Discount> discounts)
    {
        if (!discounts)
            return;
        for (int i = 0; i < discounts.Count(); i++)
        {
            OZT_Discount d = discounts[i];
            if (!d)
                continue;
            TrdRsCount(d.When, 1);
            if (d.By)
            {
                array<ref OZT_Condition> by = new array<ref OZT_Condition>();
                by.Insert(d.By);
                TrdRsCount(by, 1);
            }
        }
    }

    protected void TrdRsAdd(string id, int what)
    {
        int at = m_TrdRsIds.Find(id);
        if (at < 0)
        {
            at = m_TrdRsIds.Insert(id);
            m_TrdRsLocks.Insert(0);
            m_TrdRsCuts.Insert(0);
            m_TrdRsAccess.Insert(0);
        }
        if (what == 0)
            m_TrdRsLocks.Set(at, m_TrdRsLocks[at] + 1);
        else if (what == 1)
            m_TrdRsCuts.Set(at, m_TrdRsCuts[at] + 1);
        else
            m_TrdRsAccess.Set(at, m_TrdRsAccess[at] + 1);
    }

    // The nodes the files in force name, counted.
    protected void TrdRsCollect()
    {
        if (!m_TrdRsIds)
        {
            m_TrdRsIds = new array<string>();
            m_TrdRsLocks = new array<int>();
            m_TrdRsCuts = new array<int>();
            m_TrdRsAccess = new array<int>();
            m_TrdRsRows = new array<int>();
        }
        m_TrdRsIds.Clear();
        m_TrdRsLocks.Clear();
        m_TrdRsCuts.Clear();
        m_TrdRsAccess.Clear();
        OZT_GoodsFile goods = m_TrdDraft.GoodsAsRead();
        OZT_TradersFile traders = m_TrdDraft.TradersAsRead();
        if (goods)
        {
            for (int c = 0; c < goods.Categories.Count(); c++)
            {
                OZT_Category cat = goods.Categories[c];
                TrdRsCount(cat.Requires, 0);
                TrdRsCountDiscounts(cat.Discounts);
                if (!cat.Goods)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    TrdRsCount(cat.Goods[g].Requires, 0);
                    TrdRsCountDiscounts(cat.Goods[g].Discounts);
                }
            }
        }
        if (traders)
        {
            for (int p = 0; p < traders.Traders.Count(); p++)
                TrdRsCount(traders.Traders[p].Access, 2);
        }
    }

    // Does the tree lack the node? Only when the research gate named its nodes in
    // full (`gates`): with no gate, or a cut list, the page cannot tell.
    protected bool TrdRsMissing(string id)
    {
        OZT_AdminGateKind gk = TrdGateKind("research");
        if (!gk || gk.Cut == 1)
            return false;
        return gk.Values.Find(id) == -1;
    }

    // Does a list of conditions name the node (alone or among alternatives)? `turned`
    // says whether the condition that names it is turned round (Not).
    protected bool TrdRsNames(array<ref OZT_Condition> conds, string id, out bool turned)
    {
        turned = false;
        if (!conds)
            return false;
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c || c.Kind != "research")
                continue;
            array<string> alts = new array<string>();
            c.Arg.Split("|", alts);
            for (int a = 0; a < alts.Count(); a++)
            {
                if (alts[a].Trim() != id)
                    continue;
                if (c.Not == 1)
                    turned = true;
                return true;
            }
        }
        return false;
    }

    protected bool TrdRsDiscountNames(OZT_Discount d, string id)
    {
        bool turned;
        if (TrdRsNames(d.When, id, turned))
            return true;
        if (!d.By)
            return false;
        array<ref OZT_Condition> by = new array<ref OZT_Condition>();
        by.Insert(d.By);
        return TrdRsNames(by, id, turned);
    }

    // ------------------------------------------------------------------ painting

    protected void TrdRsPaint()
    {
        if (!m_TrdDraft || !m_TrdDraft.Loaded(OZT_Const.CFG_GOODS) || !m_TrdDraft.Loaded(OZT_Const.CFG_TRADERS))
        {
            SayTrd("TrdRsSub", TrdTr("#STR_OZT_VPP_READING"));
            return;
        }
        SayTrd("TrdRsSub", TrdTr("#STR_OZT_VPP_RS_SUB"));
        TrdRsCollect();
        TrdRsPaintNodes();
        TrdRsPaintMissing();
        TrdRsPaintDetail();
        string note = "";
        if (m_TrdDraft.Changed() > 0)
            note = TrdTr("#STR_OZT_VPP_RS_DRAFT");
        SayTrd("TrdRsDraftNote", note);
    }

    protected void TrdRsPaintNodes()
    {
        m_TrdRsRows.Clear();
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        int picked = -1;
        string find = TrdLow(GetEdit("TrdRsFind").Trim());
        string locksWord = TrdTr("#STR_OZT_VPP_RS_LOCKS");
        string cutsWord = TrdTr("#STR_OZT_VPP_RS_CUTS");
        string accessWord = TrdTr("#STR_OZT_VPP_RS_ACCESS");
        int withAccess = 0;
        int missing = 0;
        for (int i = 0; i < m_TrdRsIds.Count(); i++)
        {
            string id = m_TrdRsIds[i];
            bool gone = TrdRsMissing(id);
            if (m_TrdRsAccess[i] > 0)
                withAccess++;
            if (gone)
                missing++;
            if (m_TrdRsFilter == 1 && m_TrdRsAccess[i] == 0)
                continue;
            if (m_TrdRsFilter == 2 && !gone)
                continue;
            string label = TrdChoiceLabel("research", id);
            if (label == "")
                label = id;
            string row = label + "  (" + id + ")   " + locksWord + " " + m_TrdRsLocks[i].ToString();
            row = row + "   " + cutsWord + " " + m_TrdRsCuts[i].ToString();
            row = row + "   " + accessWord + " " + m_TrdRsAccess[i].ToString();
            if (find != "" && TrdLow(row).IndexOf(find) < 0)
                continue;
            if (id == m_TrdRsPicked)
                picked = rows.Count();
            rows.Insert(row);
            int ink = 0;
            if (gone)
                ink = OZT_Metal.ALERT;
            inks.Insert(ink);
            m_TrdRsRows.Insert(i);
        }
        TrdFill("TrdRsNodes", rows, inks, picked);

        string shown = TrdTr("#STR_OZT_VPP_RS_SHOW_ALL");
        int n = m_TrdRsIds.Count();
        if (m_TrdRsFilter == 1)
        {
            shown = TrdTr("#STR_OZT_VPP_RS_SHOW_ACCESS");
            n = withAccess;
        }
        else if (m_TrdRsFilter == 2)
        {
            shown = TrdTr("#STR_OZT_VPP_RS_SHOW_MISSING");
            n = missing;
        }
        SayTrd("BtnTrdRsFilterText", shown + " (" + n.ToString() + ")");
    }

    // The conditions that name nodes the tree no longer has.
    protected void TrdRsPaintMissing()
    {
        string ids = "";
        int n = 0;
        for (int i = 0; i < m_TrdRsIds.Count(); i++)
        {
            if (!TrdRsMissing(m_TrdRsIds[i]))
                continue;
            if (ids != "")
                ids = ids + ", ";
            ids = ids + m_TrdRsIds[i];
            n++;
        }
        if (n == 0)
        {
            SayTrd("TrdRsMissing", "");
            return;
        }
        string head = TrdTr("#STR_OZT_VPP_RS_MISSING");
        string tail = TrdTr("#STR_OZT_VPP_RS_MISSING_H");
        SayTrd("TrdRsMissing", head + " " + n.ToString() + ": " + ids + ".\n" + tail);
    }

    // The picked node: what it opens, its discounts, the points whose access needs
    // it, who has it, and where the research family holds.
    protected void TrdRsPaintDetail()
    {
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        string id = m_TrdRsPicked;
        if (id == "" || m_TrdRsIds.Find(id) < 0)
        {
            SayTrd("TrdRsHead", TrdTr("#STR_OZT_VPP_RS_PICK"));
            SayTrd("TrdRsNodeSub", "");
            TrdFill("TrdRsDetail", rows, inks, -1);
            return;
        }
        string label = TrdChoiceLabel("research", id);
        if (label == "")
            label = id;
        SayTrd("TrdRsHead", label);
        string sub = id;
        if (TrdRsMissing(id))
        {
            string gone = TrdTr("#STR_OZT_VPP_RS_GONE");
            sub = id + "  -  " + gone;
        }
        SayTrd("TrdRsNodeSub", sub);

        OZT_GoodsFile goods = m_TrdDraft.GoodsAsRead();
        OZT_TradersFile traders = m_TrdDraft.TradersAsRead();
        TrdRsDetailLocks(goods, id, rows, inks);
        TrdRsDetailCuts(goods, id, rows, inks);
        TrdRsDetailAccess(traders, id, rows, inks);
        TrdRsDetailWho(id, rows, inks);
        TrdRsDetailFamily(traders, rows, inks);
        TrdFill("TrdRsDetail", rows, inks, -1);
    }

    protected void TrdRsHeadRow(array<string> rows, array<int> inks, string key)
    {
        rows.Insert(TrdTr(key));
        inks.Insert(OZT_Metal.ACCENT);
    }

    protected void TrdRsNone(array<string> rows, array<int> inks)
    {
        rows.Insert("    -");
        inks.Insert(OZT_Metal.MUTED);
    }

    protected void TrdRsDetailLocks(OZT_GoodsFile goods, string id, array<string> rows, array<int> inks)
    {
        TrdRsHeadRow(rows, inks, "#STR_OZT_VPP_RS_H_LOCKS");
        int before = rows.Count();
        if (goods)
        {
            string catWord = TrdTr("#STR_OZT_VPP_USE_CATEGORY");
            string lineWord = TrdTr("#STR_OZT_VPP_USE_LINE");
            string inWord = TrdTr("#STR_OZT_VPP_IN");
            string notWord = TrdTr("#STR_OZT_VPP_RS_TURNED");
            bool turned;
            for (int c = 0; c < goods.Categories.Count(); c++)
            {
                OZT_Category cat = goods.Categories[c];
                if (TrdRsNames(cat.Requires, id, turned))
                {
                    string catRow = "    " + catWord + " " + cat.Name + " (" + cat.Id + ")";
                    if (turned)
                        catRow = catRow + "  " + notWord;
                    rows.Insert(catRow);
                    inks.Insert(0);
                }
                if (!cat.Goods)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_Goods e = cat.Goods[g];
                    if (!TrdRsNames(e.Requires, id, turned))
                        continue;
                    string lineRow = "    " + lineWord + " " + e.ClassName + " " + inWord + " " + cat.Name;
                    if (turned)
                        lineRow = lineRow + "  " + notWord;
                    rows.Insert(lineRow);
                    inks.Insert(0);
                }
            }
        }
        if (rows.Count() == before)
            TrdRsNone(rows, inks);
    }

    protected void TrdRsDetailCuts(OZT_GoodsFile goods, string id, array<string> rows, array<int> inks)
    {
        TrdRsHeadRow(rows, inks, "#STR_OZT_VPP_RS_H_CUTS");
        int before = rows.Count();
        if (goods)
        {
            string catWord = TrdTr("#STR_OZT_VPP_USE_CATEGORY");
            string lineWord = TrdTr("#STR_OZT_VPP_USE_LINE");
            string inWord = TrdTr("#STR_OZT_VPP_IN");
            string upTo = TrdTr("#STR_OZT_VPP_GT_UP_TO");
            for (int c = 0; c < goods.Categories.Count(); c++)
            {
                OZT_Category cat = goods.Categories[c];
                string place = catWord + " " + cat.Name + " (" + cat.Id + ")";
                TrdRsCutRows(cat.Discounts, id, place, upTo, rows, inks);
                if (!cat.Goods)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_Goods e = cat.Goods[g];
                    string lineWhere = lineWord + " " + e.ClassName + " " + inWord + " " + cat.Name;
                    TrdRsCutRows(e.Discounts, id, lineWhere, upTo, rows, inks);
                }
            }
        }
        if (rows.Count() == before)
            TrdRsNone(rows, inks);
    }

    protected void TrdRsCutRows(array<ref OZT_Discount> discounts, string id, string place, string upTo, array<string> rows, array<int> inks)
    {
        if (!discounts)
            return;
        for (int i = 0; i < discounts.Count(); i++)
        {
            OZT_Discount d = discounts[i];
            if (!d || !TrdRsDiscountNames(d, id))
                continue;
            string size = "-" + TrdNum(d.Percent) + "%";
            if (d.By)
                size = upTo + " " + TrdNum(d.Max) + "%";
            rows.Insert("    " + size + "   " + place);
            inks.Insert(OZT_Metal.OK);
        }
    }

    protected void TrdRsDetailAccess(OZT_TradersFile traders, string id, array<string> rows, array<int> inks)
    {
        TrdRsHeadRow(rows, inks, "#STR_OZT_VPP_RS_H_ACCESS");
        int before = rows.Count();
        if (traders)
        {
            string pointWord = TrdTr("#STR_OZT_VPP_USE_POINT");
            string notWord = TrdTr("#STR_OZT_VPP_RS_TURNED");
            bool turned;
            for (int p = 0; p < traders.Traders.Count(); p++)
            {
                OZT_Point pt = traders.Traders[p];
                if (!TrdRsNames(pt.Access, id, turned))
                    continue;
                string row = "    " + pointWord + " " + pt.Name + " (" + pt.Id + ")";
                if (turned)
                    row = row + "  " + notWord;
                rows.Insert(row);
                inks.Insert(0);
            }
        }
        if (rows.Count() == before)
            TrdRsNone(rows, inks);
    }

    protected void TrdRsDetailWho(string id, array<string> rows, array<int> inks)
    {
        TrdRsHeadRow(rows, inks, "#STR_OZT_VPP_RS_H_WHO");
        if (m_TrdRsWhoFor != id || m_TrdRsWhoState == 0)
        {
            rows.Insert("    " + TrdTr("#STR_OZT_VPP_GT_ASKING"));
            inks.Insert(OZT_Metal.MUTED);
            return;
        }
        if (m_TrdRsWhoState == 2)
        {
            rows.Insert("    " + TrdTr("#STR_OZT_VPP_GT_NOBODY"));
            inks.Insert(OZT_Metal.ALERT);
            return;
        }
        if (!m_TrdRsWho || m_TrdRsWho.Count() == 0)
        {
            rows.Insert("    " + TrdTr("#STR_OZT_VPP_GT_HOLD_NONE"));
            inks.Insert(OZT_Metal.MUTED);
            return;
        }
        for (int i = 0; i < m_TrdRsWho.Count(); i++)
        {
            rows.Insert("    " + m_TrdRsWho[i]);
            inks.Insert(OZT_Metal.OK);
        }
    }

    // Where the research family holds: switched off for the server (GatesOff of the
    // settings), or on or off at a point (its GatesOn, GatesOff).
    protected void TrdRsDetailFamily(OZT_TradersFile traders, array<string> rows, array<int> inks)
    {
        TrdRsHeadRow(rows, inks, "#STR_OZT_VPP_RS_H_FAMILY");
        string family = TrdFamilyOf("research");
        if (family == "")
            family = "research";
        bool serverOff = false;
        if (m_TrdDraft.Settings && m_TrdDraft.Settings.GatesOff && m_TrdDraft.Settings.GatesOff.Find(family) != -1)
            serverOff = true;
        string onWord = TrdTr("#STR_OZT_VPP_GT_FAM_ON");
        string offWord = TrdTr("#STR_OZT_VPP_GT_FAM_OFF");
        string serverWord = TrdTr("#STR_OZT_VPP_GT_SERVER");
        string hereWord = TrdTr("#STR_OZT_VPP_GT_FAM_HERE");
        string serverState = onWord;
        int serverInk = OZT_Metal.OK;
        if (serverOff)
        {
            serverState = offWord;
            serverInk = OZT_Metal.ALERT;
        }
        rows.Insert("    " + serverWord + ": " + serverState);
        inks.Insert(serverInk);
        if (!traders)
            return;
        for (int p = 0; p < traders.Traders.Count(); p++)
        {
            OZT_Point pt = traders.Traders[p];
            string state = serverState;
            int ink = serverInk;
            if (pt.GatesOn && pt.GatesOn.Find(family) != -1)
            {
                state = hereWord + " " + onWord;
                ink = OZT_Metal.OK;
            }
            else if (pt.GatesOff && pt.GatesOff.Find(family) != -1)
            {
                state = hereWord + " " + offWord;
                ink = OZT_Metal.ALERT;
            }
            rows.Insert("    " + pt.Name + " (" + pt.Id + "): " + state);
            inks.Insert(ink);
        }
    }

    // ------------------------------------------------------------------ who has it

    protected void TrdRsAskWho(string id)
    {
        m_TrdRsWhoFor = id;
        m_TrdRsWhoState = 0;
        OZT_AdminGateAsk ask = new OZT_AdminGateAsk();
        ask.Kind = "research";
        ask.Arg = id;
        string body;
        string err;
        JsonFileLoader<OZT_AdminGateAsk>.MakeData(ask, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "gate_holders", body);
    }

    // The holders of the picked node: 1 known, 2 nobody answers the kind.
    override void TrdOnHolders(OZT_AdminGateHolders h)
    {
        super.TrdOnHolders(h);
        if (h.Kind != "research" || h.Arg != m_TrdRsWhoFor)
            return;
        if (!m_TrdRsWho)
            m_TrdRsWho = new array<string>();
        m_TrdRsWho.Clear();
        for (int i = 0; i < h.Who.Count(); i++)
            m_TrdRsWho.Insert(h.Who[i]);
        m_TrdRsWhoState = 1;
        if (h.Answers != 1)
            m_TrdRsWhoState = 2;
        if (TrdShownKey() == TRD_RS_ID)
            TrdRsPaintDetail();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && !m_Repaint && w.GetName() == "TrdRsNodes")
        {
            if (m_TrdRsRows && row >= 0 && row < m_TrdRsRows.Count())
            {
                m_TrdRsPicked = m_TrdRsIds[m_TrdRsRows[row]];
                TrdRsAskWho(m_TrdRsPicked);
                TrdRsPaintDetail();
            }
            return true;
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill && w.GetName() == "TrdRsFind")
        {
            if (m_TrdRsIds)
                TrdRsPaintNodes();
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();
            if (nm == "BtnTrdRsFilter")
            {
                m_TrdRsFilter++;
                if (m_TrdRsFilter > 2)
                    m_TrdRsFilter = 0;
                if (m_TrdRsIds)
                    TrdRsPaintNodes();
                return true;
            }
            if (nm == "BtnTrdRsRefresh")
            {
                TrdAskGates();
                TrdReread(OZT_Const.CFG_SETTINGS);
                TrdReread(OZT_Const.CFG_GOODS);
                TrdReread(OZT_Const.CFG_TRADERS);
                if (m_TrdRsPicked != "")
                    TrdRsAskWho(m_TrdRsPicked);
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
