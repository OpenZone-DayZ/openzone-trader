// Research > Trade (design §5.5; mockup ResearchTrade): what the trader's files hold
// on the nodes of the research tree -- the goods a node opens (a lock of a category
// or a line), the discounts it gives, the points whose access needs it -- who has
// each node (the factions that completed it), where the research family holds, and
// the conditions that name nodes the tree no longer has. A view: the edits are on the
// trader's pages, and two buttons lead there.
//
// A page that knows two mods lives in their glue (the platform rule): this page
// knows the trader alone. The tree's nodes and who completed them come from the
// research gate of OpenZone_Trader_Research, through the trader's own ops -- `gates`
// (the gate's choices: every node of the tree, each labelled "name - branch (owners) -
// T<tier>", which OZT_VppKit.LabelParts takes apart) and `gate_holders` (the factions
// that completed one; asked and kept by the Conditions page's layer, TrdGtHoldWant).
// The gate is the server's: it may load as a server mod, so a client has no define of
// it, and the page appears when the server's `gates` name the family "research"
// (asked once as the window opens).
//
// It reads what is in force on the server: the files as the server last gave them,
// not the draft's edits. It sits under the module "research" of the rail, after the
// research glue's own pages: it registers when the answer to `gates` arrives,
// after every glue's layer has registered its own.
//
// The page is converted from the mockup (ui/css2layout.json, page `rtrade`). The
// node's cards are slices of row templates stacked in two columns (a card's head, its
// items, its foot); the family's box stands under the taller column, placed by script.
//
// A layer of OZ_VppAdminMenu after OZT_VppPaneGates.c (it takes TrdOnHolders and the
// kit from it).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_RS_ID = "rtrade";
    static const string TRD_RS_L  = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_rtrade_";

    // The slices of a card, in the order of a column's lists (m_TrdRsA, m_TrdRsB), and
    // their heights in layout units.
    static const int TRD_RS_HEAD = 0;
    static const int TRD_RS_ITEM = 1;
    static const int TRD_RS_FOOT = 2;
    static const int TRD_RS_GAP  = 3;
    static const int TRD_RS_WHO  = 4;
    static const float TRD_RS_HEAD_H = 38;
    static const float TRD_RS_ITEM_H = 83;
    static const float TRD_RS_FOOT_H = 61;
    static const float TRD_RS_GAP_H  = 14;
    static const float TRD_RS_WHO_H  = 42;

    // The most rows the list makes, and the most items a card shows.
    static const int TRD_RS_LIST_MAX = 300;
    static const int TRD_RS_CARD_MAX = 30;

    // The nodes the trader's files name, and per node how many locks, discounts and
    // points' access name it; the node picked; the list's filter (0 all, 1 with access,
    // 2 not in the tree) and the owner it is narrowed to ("": all).
    protected ref array<string> m_TrdRsIds;
    protected ref array<int> m_TrdRsLocks;
    protected ref array<int> m_TrdRsCuts;
    protected ref array<int> m_TrdRsAccess;
    protected string m_TrdRsPicked;
    protected int m_TrdRsFilter;
    protected string m_TrdRsOwner;

    // The list's rows; the two columns of the node's cards, a list per slice; the
    // points' chips of the family box; the columns' heights in layout units.
    protected ref OZ_VppFlexRows m_TrdRsRows;
    protected ref array<ref OZ_VppFlexRows> m_TrdRsA;
    protected ref array<ref OZ_VppFlexRows> m_TrdRsB;
    protected ref OZT_VppChips m_TrdRsChips;
    protected float m_TrdRsColA;
    protected float m_TrdRsColB;

    // Where the Conditions page opens: at the first use of the node the missing box
    // shows (and which node that is), at the first use of the picked node. The node a
    // similar one was last looked for, and what was found.
    protected string m_TrdRsGoneId;
    protected string m_TrdRsGoneKey;
    protected string m_TrdRsUseKey;
    protected string m_TrdRsLikeOf;
    protected string m_TrdRsLike;

    // The frames left on which what stands by its words is placed again.
    protected int m_TrdRsFlowDue;

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
        TrdAddPage("research", "#STR_OZT_VPP_MOD_RESEARCH", TRD_RS_ID, TRD_RS_ID, "#STR_OZT_VPP_PAGE_RTRADE");
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

    // The window changed its size: the rows follow their lists, and a resize puts the
    // stretched parts back where the layout has them, so what the script places is
    // placed again.
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (!M_SUB_WIDGET || TrdShownKey() != TRD_RS_ID || !TrdRsLoaded())
            return;
        TrdRsFit();
        TrdRsReflow();
        m_TrdRsFlowDue = 3;
    }

    // Every frame while the window is open: the placing is done again on the frames
    // after a paint (a row made on this frame may not measure its texts yet).
    override void Tick()
    {
        super.Tick();
        if (m_TrdRsFlowDue <= 0)
            return;
        m_TrdRsFlowDue--;
        if (M_SUB_WIDGET && TrdShownKey() == TRD_RS_ID && TrdRsLoaded())
            TrdRsReflow();
    }

    protected void TrdRsFit()
    {
        if (m_TrdRsRows)
            m_TrdRsRows.Fit();
        if (!m_TrdRsA)
            return;
        for (int i = 0; i < m_TrdRsA.Count(); i++)
        {
            m_TrdRsA[i].Fit();
            m_TrdRsB[i].Fit();
        }
    }

    // Everything of the page that stands by the length of its words, placed: nothing is
    // made or read here, only measured and moved.
    protected void TrdRsReflow()
    {
        TrdRsPlaceLeft();
        TrdRsPlaceItems();
        TrdRsPlaceFam();
    }

    // ------------------------------------------------------------------ reading

    protected bool TrdRsLoaded()
    {
        if (!m_TrdDraft)
            return false;
        bool goods = m_TrdDraft.Loaded(OZT_Const.CFG_GOODS);
        bool points = m_TrdDraft.Loaded(OZT_Const.CFG_TRADERS);
        bool settings = m_TrdDraft.Loaded(OZT_Const.CFG_SETTINGS);
        return goods && points && settings;
    }

    // Every node a list of conditions names under the kind "research": each
    // alternative of each condition counts once for `what` (0 lock, 1 discount,
    // 2 a point's access).
    protected void TrdRsCount(array<ref OZT_Condition> conds, int what)
    {
        if (!conds)
            return;
        array<string> alts = new array<string>();
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c || c.Kind != "research")
                continue;
            OZT_VppKit.Alts(c.Arg, alts);
            for (int a = 0; a < alts.Count(); a++)
                TrdRsAdd(alts[a], what);
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

    // A node as the tree names it: its name, branch, owners and tier out of the gate's
    // label. False, and the label or the id as the name, when the label carries none.
    protected bool TrdRsNodeParts(string id, out string name, out string branch, out string owners, out string tier)
    {
        name = id;
        branch = "";
        owners = "";
        tier = "";
        string label = TrdChoiceLabel("research", id);
        if (label == "")
            return false;
        return OZT_VppKit.LabelParts(label, name, branch, owners, tier);
    }

    protected string TrdRsNodeName(string id)
    {
        string name;
        string branch;
        string owners;
        string tier;
        TrdRsNodeParts(id, name, branch, owners, tier);
        return name;
    }

    // Is `owner` one of a node's owners ("a, b")?
    protected bool TrdRsOwnedBy(string owners, string owner)
    {
        array<string> own = new array<string>();
        owners.Split(",", own);
        for (int i = 0; i < own.Count(); i++)
        {
            if (own[i].Trim() == owner)
                return true;
        }
        return false;
    }

    // The condition of a list that names the node under the kind "research" (alone or
    // among alternatives); null when none does.
    protected OZT_Condition TrdRsCondOf(array<ref OZT_Condition> conds, string id)
    {
        if (!conds)
            return null;
        array<string> alts = new array<string>();
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c || c.Kind != "research")
                continue;
            OZT_VppKit.Alts(c.Arg, alts);
            if (alts.Find(id) != -1)
                return c;
        }
        return null;
    }

    // The condition of a discount that names the node: one it waits for, or the one
    // its provider computes by.
    protected OZT_Condition TrdRsDiscCond(OZT_Discount d, string id)
    {
        if (!d)
            return null;
        OZT_Condition c = TrdRsCondOf(d.When, id);
        if (c)
            return c;
        if (!d.By)
            return null;
        array<ref OZT_Condition> by = new array<ref OZT_Condition>();
        by.Insert(d.By);
        return TrdRsCondOf(by, id);
    }

    protected int TrdRsCountOf(array<ref OZT_Condition> conds)
    {
        if (!conds)
            return 0;
        return conds.Count();
    }

    // Where the research family stands at a point: 0 holds as the server's, 1 switched
    // on at the point, 2 switched off at the point, 3 off on the server.
    protected int TrdRsFamAt(OZT_Point pt, OZT_Settings settings, string family)
    {
        if (pt.GatesOn && pt.GatesOn.Find(family) != -1)
            return 1;
        if (pt.GatesOff && pt.GatesOff.Find(family) != -1)
            return 2;
        if (settings && settings.GatesOff && settings.GatesOff.Find(family) != -1)
            return 3;
        return 0;
    }

    protected string TrdRsFamily()
    {
        string family = TrdFamilyOf("research");
        if (family == "")
            family = "research";
        return family;
    }

    // The points that deal in a category, in words: where the research family holds
    // and where it is switched off (there a lock passes and a discount is not given).
    protected string TrdRsPointsLine(OZT_TradersFile traders, OZT_Settings settings, string catId, bool cut)
    {
        string family = TrdRsFamily();
        string on = "";
        string off = "";
        int more = 0;
        int n = 0;
        if (traders)
        {
            for (int p = 0; p < traders.Traders.Count(); p++)
            {
                OZT_Point pt = traders.Traders[p];
                if (!pt.Categories || pt.Categories.Find(catId) == -1)
                    continue;
                n++;
                if (n > 4)
                {
                    more++;
                    continue;
                }
                int state = TrdRsFamAt(pt, settings, family);
                if (state >= 2)
                {
                    if (off != "")
                        off = off + ", ";
                    off = off + pt.Name;
                }
                else
                {
                    if (on != "")
                        on = on + ", ";
                    on = on + pt.Name;
                }
            }
        }
        if (n == 0)
            return TrdTr("#STR_OZT_VPP_RS_L_NO_POINTS");
        string tail = "";
        if (more > 0)
            tail = " +" + more.ToString();
        if (off == "")
        {
            string pointsWord = TrdTr("#STR_OZT_VPP_RS_L_POINTS");
            if (cut)
                pointsWord = TrdTr("#STR_OZT_VPP_RS_L_GIVEN");
            return pointsWord + " " + on + tail;
        }
        string onWord = TrdTr("#STR_OZT_VPP_RS_L_OPEN");
        string offWord = TrdTr("#STR_OZT_VPP_RS_L_PASSES");
        if (cut)
        {
            onWord = TrdTr("#STR_OZT_VPP_RS_L_GIVEN");
            offWord = TrdTr("#STR_OZT_VPP_RS_L_NOT_GIVEN");
        }
        if (on == "")
            on = "—";
        string line = onWord + " " + on + " · " + offWord + " " + off;
        return line + tail;
    }

    // A research condition in words: its nodes' names, "or" between alternatives,
    // "without" before a condition turned round.
    protected string TrdRsCondWords(OZT_Condition c)
    {
        array<string> alts = new array<string>();
        OZT_VppKit.Alts(c.Arg, alts);
        string orWord = TrdTr("#STR_OZT_VPP_RS_OR");
        string words = "";
        for (int i = 0; i < alts.Count(); i++)
        {
            if (words != "")
                words = words + " " + orWord + " ";
            words = words + TrdRsNodeName(alts[i]);
        }
        if (c.Not == 1)
        {
            string notWord = TrdTr("#STR_OZT_VPP_RS_NOT");
            words = notWord + " " + words;
        }
        return words;
    }

    // Who holds a condition now, in words; asked when not known yet (the answer paints
    // the node again).
    protected string TrdRsHolders(OZT_Condition c)
    {
        string said;
        int state = TrdGtHoldOf(c.Kind, c.Arg, said);
        if (state == 0)
        {
            TrdGtHoldWant(c.Kind, c.Arg);
            return "...";
        }
        if (state == 2)
            return TrdTr("#STR_OZT_VPP_RS_NO_ANSWER");
        if (said == "")
            return TrdTr("#STR_OZT_VPP_RS_NOBODY");
        return said;
    }

    // "Condition: <nodes> (+1) · held now by: <who>": the condition of a lock that
    // names the node; `total` conditions stand in the lock together.
    protected string TrdRsCondLine(OZT_Condition c, int total, string headKey)
    {
        string head = TrdTr(headKey);
        string have = TrdTr("#STR_OZT_VPP_RS_L_HAVE");
        string line = head + " " + TrdRsCondWords(c);
        if (total > 1)
        {
            int rest = total - 1;
            line = line + " (+" + rest.ToString() + ")";
        }
        line = line + " · " + have + " " + TrdRsHolders(c);
        return line;
    }

    // A point's kind as the admin reads it.
    protected string TrdRsKindWord(string kind)
    {
        if (kind == OZT_Const.KIND_NPC)
            return TrdTr("#STR_OZT_VPP_KIND_NPC");
        if (kind == OZT_Const.KIND_MAP)
            return TrdTr("#STR_OZT_VPP_KIND_MAP");
        return TrdTr("#STR_OZT_VPP_KIND_OBJECT");
    }

    // The first place of the files in force that names a node: its target on the
    // Conditions page ("c|<category>", "l|<category>|<class>", "p|<point>") and what
    // it is in words. False when nothing names it.
    protected bool TrdRsFirstUse(string id, out string key, out string words)
    {
        key = "";
        words = "";
        OZT_GoodsFile goods = m_TrdDraft.GoodsAsRead();
        OZT_TradersFile traders = m_TrdDraft.TradersAsRead();
        string inWord = TrdTr("#STR_OZT_VPP_IN");
        string what;
        int i;
        if (goods)
        {
            for (int c = 0; c < goods.Categories.Count(); c++)
            {
                OZT_Category cat = goods.Categories[c];
                if (TrdRsCondOf(cat.Requires, id))
                {
                    what = TrdTr("#STR_OZT_VPP_RS_W_LOCK_CAT");
                    key = "c|" + cat.Id;
                    words = what + " " + cat.Name;
                    return true;
                }
                if (cat.Discounts)
                {
                    for (i = 0; i < cat.Discounts.Count(); i++)
                    {
                        if (!TrdRsDiscCond(cat.Discounts[i], id))
                            continue;
                        what = TrdTr("#STR_OZT_VPP_RS_W_CUT_CAT");
                        key = "c|" + cat.Id;
                        words = what + " " + cat.Name;
                        return true;
                    }
                }
                if (!cat.Goods)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_Goods e = cat.Goods[g];
                    bool named = false;
                    if (TrdRsCondOf(e.Requires, id))
                    {
                        what = TrdTr("#STR_OZT_VPP_RS_W_LOCK_LINE");
                        named = true;
                    }
                    else if (e.Discounts)
                    {
                        for (i = 0; i < e.Discounts.Count(); i++)
                        {
                            if (TrdRsDiscCond(e.Discounts[i], id))
                                named = true;
                        }
                        if (named)
                            what = TrdTr("#STR_OZT_VPP_RS_W_CUT_LINE");
                    }
                    if (!named)
                        continue;
                    key = "l|" + cat.Id + "|" + e.ClassName;
                    words = what + " " + e.ClassName + " " + inWord + " " + cat.Name;
                    return true;
                }
            }
        }
        if (traders)
        {
            for (int p = 0; p < traders.Traders.Count(); p++)
            {
                OZT_Point pt = traders.Traders[p];
                if (!TrdRsCondOf(pt.Access, id))
                    continue;
                what = TrdTr("#STR_OZT_VPP_RS_W_ACCESS");
                key = "p|" + pt.Id;
                words = what + " " + pt.Name;
                return true;
            }
        }
        return false;
    }

    // How many edits turn one id into another, counted up to `most` (more: most + 1).
    protected int TrdRsDistance(string a, string b, int most)
    {
        int la = a.Length();
        int lb = b.Length();
        int diff = la - lb;
        if (diff < 0)
            diff = -diff;
        if (diff > most)
            return most + 1;
        array<int> prev = new array<int>();
        array<int> cur = new array<int>();
        int i;
        int j;
        for (j = 0; j <= lb; j++)
        {
            prev.Insert(j);
            cur.Insert(0);
        }
        for (i = 1; i <= la; i++)
        {
            cur.Set(0, i);
            string ca = a.Get(i - 1);
            for (j = 1; j <= lb; j++)
            {
                int cost = 1;
                if (ca == b.Get(j - 1))
                    cost = 0;
                int best = prev[j] + 1;
                if (cur[j - 1] + 1 < best)
                    best = cur[j - 1] + 1;
                if (prev[j - 1] + cost < best)
                    best = prev[j - 1] + cost;
                cur.Set(j, best);
            }
            for (j = 0; j <= lb; j++)
                prev.Set(j, cur[j]);
        }
        return prev[lb];
    }

    // A node of the tree whose id is a slip of the pen away from one the tree lacks
    // (the same first letters, three edits at most): "id (Name)", or "". Looked for
    // once per node.
    protected string TrdRsSimilar(string id)
    {
        if (id == m_TrdRsLikeOf)
            return m_TrdRsLike;
        m_TrdRsLikeOf = id;
        m_TrdRsLike = "";
        OZT_AdminGateKind gk = TrdGateKind("research");
        if (!gk || id.Length() < 5)
            return "";
        string start = id.Substring(0, 3);
        int bestAt = -1;
        int bestD = 4;
        for (int i = 0; i < gk.Values.Count(); i++)
        {
            string other = gk.Values[i];
            if (other.Length() < 5 || other.Substring(0, 3) != start)
                continue;
            int d = TrdRsDistance(id, other, 3);
            if (d < bestD)
            {
                bestD = d;
                bestAt = i;
            }
        }
        if (bestAt == -1)
            return "";
        m_TrdRsLike = gk.Values[bestAt] + " (" + TrdRsNodeName(gk.Values[bestAt]) + ")";
        return m_TrdRsLike;
    }

    // ------------------------------------------------------------------ painting

    protected void TrdRsPaint()
    {
        if (!M_SUB_WIDGET)
            return;
        if (!TrdRsLoaded())
        {
            SayTrd("TrdRsSub", "#STR_OZT_VPP_READING");
            return;
        }
        SayTrd("TrdRsSub", "#STR_OZT_VPP_RS_SUB_S");
        TrdRsCollect();
        TrdRsPaintOwner();
        TrdRsPaintNodes();
        TrdRsPaintGone();
        TrdRsPaintDetail();
        string note = "";
        if (m_TrdDraft.Changed() > 0)
            note = "#STR_OZT_VPP_RS_DRAFT";
        SayTrd("TrdRsDraftNote", note);
        m_TrdRsFlowDue = 3;
    }

    // The owners of the nodes the files name, each once.
    protected void TrdRsOwners(array<string> into)
    {
        into.Clear();
        string name;
        string branch;
        string owners;
        string tier;
        array<string> own = new array<string>();
        for (int i = 0; i < m_TrdRsIds.Count(); i++)
        {
            if (!TrdRsNodeParts(m_TrdRsIds[i], name, branch, owners, tier))
                continue;
            own.Clear();
            owners.Split(",", own);
            for (int k = 0; k < own.Count(); k++)
            {
                string one = own[k].Trim();
                if (one != "" && into.Find(one) == -1)
                    into.Insert(one);
            }
        }
    }

    // The owner the list is narrowed to; an owner no node has any more is "all".
    protected void TrdRsPaintOwner()
    {
        array<string> owners = new array<string>();
        TrdRsOwners(owners);
        if (m_TrdRsOwner != "" && owners.Find(m_TrdRsOwner) == -1)
            m_TrdRsOwner = "";
        if (m_TrdRsOwner == "")
            SayTrd("TrdRsOwnerText", "#STR_OZT_VPP_RS_OWNER_ALL");
        else
            SayTrd("TrdRsOwnerText", m_TrdRsOwner);
    }

    protected void TrdRsOwnerNext()
    {
        array<string> owners = new array<string>();
        TrdRsOwners(owners);
        int at = owners.Find(m_TrdRsOwner) + 1;
        if (at >= owners.Count())
            m_TrdRsOwner = "";
        else
            m_TrdRsOwner = owners[at];
        TrdRsPaintOwner();
        TrdRsPaintNodes();
    }

    // Does a search name the node: its name, id or owners, or a class one of whose
    // conditions names it?
    protected bool TrdRsFound(OZT_GoodsFile goods, string id, string name, string owners, string find)
    {
        if (OZT_VppKit.Holds(name, find) || OZT_VppKit.Holds(id, find) || OZT_VppKit.Holds(owners, find))
            return true;
        if (!goods)
            return false;
        for (int c = 0; c < goods.Categories.Count(); c++)
        {
            OZT_Category cat = goods.Categories[c];
            if (!cat.Goods)
                continue;
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_Goods e = cat.Goods[g];
                if (!OZT_VppKit.Holds(e.ClassName, find))
                    continue;
                if (TrdRsCondOf(e.Requires, id))
                    return true;
                if (!e.Discounts)
                    continue;
                for (int i = 0; i < e.Discounts.Count(); i++)
                {
                    if (TrdRsDiscCond(e.Discounts[i], id))
                        return true;
                }
            }
        }
        return false;
    }

    // The list: a node a row -- its name, its owners and tier, how many locks,
    // discounts and points' access name it. The owner and the search narrow it and the
    // counts on the filters; a filter picks among what is left.
    protected void TrdRsPaintNodes()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdRsNodesRows");
        if (!host)
            return;
        if (!m_TrdRsRows)
            m_TrdRsRows = new OZ_VppFlexRows(m_Flex, host, TRD_RS_L + "node.layout", "OZT_RsNode", 367, 48);
        float scrolled = OZT_VppKit.ScrollAt(M_SUB_WIDGET, "TrdRsNodesScroll");
        m_TrdRsRows.Clear();

        OZT_GoodsFile goods = m_TrdDraft.GoodsAsRead();
        string find = OZT_VppKit.Low(GetEdit("TrdRsFind").Trim());
        string tierWord = TrdTr("#STR_OZT_VPP_RS_TIER");
        string goneWord = TrdTr("#STR_OZT_VPP_RS_GONE");
        int total = 0;
        int withAccess = 0;
        int missing = 0;
        int hidden = 0;
        string name;
        string branch;
        string owners;
        string tier;
        Widget row;
        for (int i = 0; i < m_TrdRsIds.Count(); i++)
        {
            string id = m_TrdRsIds[i];
            bool shaped = TrdRsNodeParts(id, name, branch, owners, tier);
            if (m_TrdRsOwner != "" && !TrdRsOwnedBy(owners, m_TrdRsOwner))
                continue;
            if (find != "" && !TrdRsFound(goods, id, name, owners, find))
                continue;
            bool gone = TrdRsMissing(id);
            total++;
            if (m_TrdRsAccess[i] > 0)
                withAccess++;
            if (gone)
                missing++;
            if (m_TrdRsFilter == 1 && m_TrdRsAccess[i] == 0)
                continue;
            if (m_TrdRsFilter == 2 && !gone)
                continue;
            if (m_TrdRsRows.Count() >= TRD_RS_LIST_MAX)
            {
                hidden++;
                continue;
            }
            row = m_TrdRsRows.Add(id);
            if (!row)
                continue;
            string meta = id;
            int metaInk = OZT_Metal.MUTED;
            if (gone)
            {
                meta = goneWord;
                metaInk = OZT_Metal.ALERT;
            }
            else if (shaped)
                meta = owners + " · " + tierWord + " " + tier;
            RowText(row, "NdName", name, OZT_Metal.TEXT);
            RowText(row, "NdMeta", meta, metaInk);
            TrdRsCountCell(row, "NdLocks", m_TrdRsLocks[i]);
            TrdRsCountCell(row, "NdCuts", m_TrdRsCuts[i]);
            TrdRsCountCell(row, "NdAccess", m_TrdRsAccess[i]);
            TrdRsMarkNode(row, id == m_TrdRsPicked);
        }
        m_TrdRsRows.Fit();
        OZT_VppKit.ScrollTo(M_SUB_WIDGET, "TrdRsNodesScroll", scrolled);

        string countWord = TrdTr("#STR_OZT_VPP_RS_COUNT");
        string count = countWord + " " + total.ToString();
        if (hidden > 0)
        {
            string moreWord = TrdTr("#STR_OZT_VPP_RS_MORE");
            count = count + " · " + moreWord + " " + hidden.ToString();
        }
        SayTrd("TrdRsCount", count);

        string allWord = TrdTr("#STR_OZT_VPP_RS_F_ALL");
        string accessWord = TrdTr("#STR_OZT_VPP_RS_F_ACCESS");
        string missingWord = TrdTr("#STR_OZT_VPP_RS_F_GONE");
        SayTrd("BtnTrdRsAllText", allWord + " · " + total.ToString());
        SayTrd("BtnTrdRsAccessText", accessWord + " · " + withAccess.ToString());
        SayTrd("BtnTrdRsGoneText", missingWord + " · " + missing.ToString());
        int goneInk = OZT_Metal.MUTED;
        if (missing > 0)
            goneInk = OZT_Metal.ALERT;
        OZT_VppKit.PillPaint(M_SUB_WIDGET, "BtnTrdRsAll", m_TrdRsFilter == 0, OZT_Metal.MUTED);
        OZT_VppKit.PillPaint(M_SUB_WIDGET, "BtnTrdRsAccess", m_TrdRsFilter == 1, OZT_Metal.MUTED);
        OZT_VppKit.PillPaint(M_SUB_WIDGET, "BtnTrdRsGone", m_TrdRsFilter == 2, goneInk);
        TrdRsPlaceLeft();
        m_TrdRsFlowDue = 3;
    }

    // A count of a row: the number, or a dash for none.
    protected void TrdRsCountCell(Widget row, string name, int n)
    {
        if (n > 0)
            RowText(row, name, n.ToString(), OZT_Metal.TEXT);
        else
            RowText(row, name, "—", OZT_Metal.MUTED);
    }

    protected void TrdRsMarkNode(Widget row, bool picked)
    {
        if (picked)
            OZT_VppKit.PartBox(row, "OZT_RsNode", OZT_Metal.ACCENT, OZT_Metal.PICK);
        else
            OZT_VppKit.PartBox(row, "OZT_RsNode", OZT_Metal.HEAD, OZT_Metal.HEAD);
    }

    // The nodes the tree no longer has: how many, and one of them -- the picked one
    // when it is one of them, else the first -- with where the files name it, what
    // follows from it, and a node of the tree it may have been meant to be.
    protected void TrdRsPaintGone()
    {
        int n = 0;
        string shown = "";
        for (int i = 0; i < m_TrdRsIds.Count(); i++)
        {
            if (!TrdRsMissing(m_TrdRsIds[i]))
                continue;
            n++;
            if (shown == "" || m_TrdRsIds[i] == m_TrdRsPicked)
                shown = m_TrdRsIds[i];
        }
        TrdShow("TrdRsGoneBox", n > 0);
        m_TrdRsGoneId = shown;
        m_TrdRsGoneKey = "";
        if (n == 0)
            return;
        string head = TrdTr("#STR_OZT_VPP_RS_MISSING");
        SayTrd("TrdRsGoneHead", head + " · " + n.ToString());
        string where;
        string goneKey;
        TrdRsFirstUse(shown, goneKey, where);
        m_TrdRsGoneKey = goneKey;
        SayTrd("TrdRsGoneId", shown);
        SayTrd("TrdRsGoneWhere", where);
        string hint = TrdTr("#STR_OZT_VPP_RS_MISSING_H");
        string like = TrdRsSimilar(shown);
        if (like != "")
        {
            string similar = TrdTr("#STR_OZT_VPP_RS_SIMILAR");
            hint = hint + " " + similar + " " + like + ".";
        }
        SayTrd("TrdRsGoneHint", hint);
    }

    // The left column by its words: the three filters one after another, the missing
    // node's id and where it is named, and the list as tall as what the box under it
    // leaves (the whole column when the box is hidden).
    protected void TrdRsPlaceLeft()
    {
        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        float bx;
        float by;
        Widget first = M_SUB_WIDGET.FindAnyWidget("BtnTrdRsAll");
        if (first)
        {
            first.GetPos(x, y);
            w = OZT_VppKit.HugBox(M_SUB_WIDGET, "BtnTrdRsAll", "BtnTrdRsAllText", 11 * s);
            x = x + w + 6 * s;
            OZT_VppKit.MoveX(M_SUB_WIDGET.FindAnyWidget("BtnTrdRsAccess"), x);
            w = OZT_VppKit.HugBox(M_SUB_WIDGET, "BtnTrdRsAccess", "BtnTrdRsAccessText", 11 * s);
            x = x + w + 6 * s;
            OZT_VppKit.MoveX(M_SUB_WIDGET.FindAnyWidget("BtnTrdRsGone"), x);
            OZT_VppKit.HugBox(M_SUB_WIDGET, "BtnTrdRsGone", "BtnTrdRsGoneText", 11 * s);
        }

        Widget box = M_SUB_WIDGET.FindAnyWidget("TrdRsGoneBox");
        Widget goneId = M_SUB_WIDGET.FindAnyWidget("TrdRsGoneId");
        Widget where = M_SUB_WIDGET.FindAnyWidget("TrdRsGoneWhere");
        if (box && goneId && where && box.IsVisible())
        {
            goneId.GetPos(x, y);
            box.GetSize(bx, by);
            w = OZT_VppKit.FitWord(box, "TrdRsGoneId");
            if (w > bx * 0.6)
            {
                w = bx * 0.6;
                OZT_VppKit.SetWidth(goneId, w);
            }
            x = x + w + 8 * s;
            OZT_VppKit.MoveX(where, x);
            OZT_VppKit.SetWidth(where, bx - 13 * s - x);
        }

        Widget list = M_SUB_WIDGET.FindAnyWidget("TrdRsNodes");
        Widget scroll = M_SUB_WIDGET.FindAnyWidget("TrdRsNodesScroll");
        Widget note = M_SUB_WIDGET.FindAnyWidget("TrdRsDraftNote");
        if (!list || !scroll || !box || !note)
            return;
        list.GetPos(x, y);
        list.GetSize(w, h);
        box.GetPos(bx, by);
        float bottom = by - 8 * s;
        if (!box.IsVisible())
        {
            note.GetPos(bx, by);
            bottom = by - 9 * s;
        }
        h = bottom - y;
        if (h < 48 * s)
            h = 48 * s;
        list.SetSize(w, h);
        scroll.SetSize(w, h);
        scroll.Update();
    }

    // ------------------------------------------------------------------ the node

    protected void TrdRsMakeCols()
    {
        Widget a = M_SUB_WIDGET.FindAnyWidget("TrdRsColA");
        Widget b = M_SUB_WIDGET.FindAnyWidget("TrdRsColB");
        if (!a || !b || m_TrdRsA)
            return;
        m_TrdRsA = new array<ref OZ_VppFlexRows>();
        m_TrdRsB = new array<ref OZ_VppFlexRows>();
        TrdRsMakeCol(m_TrdRsA, a);
        TrdRsMakeCol(m_TrdRsB, b);
    }

    protected void TrdRsMakeCol(array<ref OZ_VppFlexRows> col, Widget host)
    {
        col.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_RS_L + "cardhead.layout", "OZT_RsCardHead", 449, TRD_RS_HEAD_H));
        col.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_RS_L + "item.layout", "OZT_RsItem", 449, TRD_RS_ITEM_H));
        col.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_RS_L + "cardfoot.layout", "OZT_RsCardFoot", 449, TRD_RS_FOOT_H));
        col.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_RS_L + "gap.layout", "OZT_RsGap", 449, TRD_RS_GAP_H));
        col.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_RS_L + "who.layout", "OZT_RsWho", 449, TRD_RS_WHO_H));
    }

    // The picked node: its name and what the tree says of it, then its cards -- what it
    // opens and the points whose access needs it on the left, its discounts and who has
    // it on the right -- and under them where the research family holds.
    protected void TrdRsPaintDetail()
    {
        TrdRsMakeCols();
        if (!m_TrdRsA)
            return;
        int i;
        for (i = 0; i < m_TrdRsA.Count(); i++)
        {
            m_TrdRsA[i].Clear();
            m_TrdRsB[i].Clear();
        }
        m_TrdRsColA = 0;
        m_TrdRsColB = 0;
        m_TrdRsUseKey = "";

        OZT_GoodsFile goods = m_TrdDraft.GoodsAsRead();
        OZT_TradersFile traders = m_TrdDraft.TradersAsRead();
        OZT_Settings settings = m_TrdDraft.SettingsAsRead();
        string id = m_TrdRsPicked;
        bool known = id != "" && m_TrdRsIds.Find(id) != -1;
        TrdShow("BtnTrdRsOpenGates", known);
        bool tree = known && m_Panes.Contains("research.tree") && !TrdRsMissing(id);
        TrdShow("BtnTrdRsOpenTree", tree);
        if (!known)
        {
            SayTrd("TrdRsHead", "#STR_OZT_VPP_RS_PICK");
            SayTrd("TrdRsNodeSub", "");
            TrdRsPaintFam(traders, settings);
            m_TrdRsFlowDue = 3;
            return;
        }

        string name;
        string branch;
        string owners;
        string tier;
        bool shaped = TrdRsNodeParts(id, name, branch, owners, tier);
        string sub = id;
        int subInk = OZT_Metal.MUTED;
        if (TrdRsMissing(id))
        {
            string goneWord = TrdTr("#STR_OZT_VPP_RS_GONE");
            sub = id + " · " + goneWord;
            subInk = OZT_Metal.ALERT;
        }
        else if (shaped)
        {
            string tierWord = TrdTr("#STR_OZT_VPP_RS_TIER");
            string branchWord = TrdTr("#STR_OZT_VPP_RS_BRANCH");
            string ownerWord = TrdTr("#STR_OZT_VPP_RS_OWNER_W");
            sub = id + " · " + tierWord + " " + tier;
            sub = sub + " · " + branchWord + " «" + branch + "»";
            if (owners != "")
                sub = sub + " · " + ownerWord + " " + owners;
        }
        SayTrd("TrdRsHead", name);
        TrdInk("TrdRsNodeSub", sub, subInk);

        m_TrdRsColA = TrdRsCardLocks(goods, traders, settings, id);
        m_TrdRsColA = m_TrdRsColA + TrdRsGapRow(m_TrdRsA);
        m_TrdRsColA = m_TrdRsColA + TrdRsCardAccess(traders, id);
        m_TrdRsColB = TrdRsCardCuts(goods, traders, settings, id);
        m_TrdRsColB = m_TrdRsColB + TrdRsGapRow(m_TrdRsB);
        m_TrdRsColB = m_TrdRsColB + TrdRsCardWho(id);
        for (i = 0; i < m_TrdRsA.Count(); i++)
        {
            m_TrdRsA[i].Fit();
            m_TrdRsB[i].Fit();
        }
        TrdRsPlaceItems();
        TrdRsPaintFam(traders, settings);
        m_TrdRsFlowDue = 3;
    }

    // The slices of a card, each into its column; each returns its height in units.
    protected Widget TrdRsHeadRow(array<ref OZ_VppFlexRows> col)
    {
        return col[TRD_RS_HEAD].Add("");
    }

    protected void TrdRsHeadSay(Widget head, string title, string count)
    {
        if (!head)
            return;
        RowText(head, "ChTitle", title, OZT_Metal.MUTED);
        RowText(head, "ChCount", count, OZT_Metal.MUTED);
    }

    protected float TrdRsFootRow(array<ref OZ_VppFlexRows> col, string note)
    {
        Widget row = col[TRD_RS_FOOT].Add("");
        if (row)
            RowText(row, "CfNote", note, OZT_Metal.MUTED);
        return TRD_RS_FOOT_H;
    }

    protected float TrdRsGapRow(array<ref OZ_VppFlexRows> col)
    {
        col[TRD_RS_GAP].Add("");
        return TRD_RS_GAP_H;
    }

    // A one-line slice: a sign ("dot" or "ring") in a colour, a name, and a state
    // written from the right.
    protected float TrdRsLineRow(array<ref OZ_VppFlexRows> col, string image, int signInk, string name, int nameInk, string state, int stateInk)
    {
        Widget row = col[TRD_RS_WHO].Add("");
        if (!row)
            return TRD_RS_WHO_H;
        OZT_VppKit.SignSet(row, "WhoDot", image, signInk);
        RowText(row, "WhoName", name, nameInk);
        RowText(row, "WhoState", state, stateInk);
        return TRD_RS_WHO_H;
    }

    // An item of a card: a tag, a name with a few words beside it, a value at the right
    // end ("" for none), and two lines under them.
    protected float TrdRsItemRow(array<ref OZ_VppFlexRows> col, string tag, string name, string sub, string val, string l1, string l2)
    {
        Widget row = col[TRD_RS_ITEM].Add("");
        if (!row)
            return TRD_RS_ITEM_H;
        OZT_VppKit.PartSay(row, "ItTagText", tag);
        OZT_VppKit.PartSay(row, "ItName", name);
        OZT_VppKit.PartSay(row, "ItSub", sub);
        OZT_VppKit.PartSay(row, "ItVal", val);
        OZT_VppKit.PartSay(row, "ItL1", l1);
        OZT_VppKit.PartSay(row, "ItL2", l2);
        return TRD_RS_ITEM_H;
    }

    // A card with nothing in it says so; a card cut at its most says how many more.
    protected float TrdRsTailRow(array<ref OZ_VppFlexRows> col, int all, int shown)
    {
        if (all == 0)
            return TrdRsLineRow(col, "ring", OZT_Metal.FAINT, "#STR_OZT_VPP_RS_NONE", OZT_Metal.MUTED, "", OZT_Metal.MUTED);
        if (all <= shown)
            return 0;
        int rest = all - shown;
        string moreWord = TrdTr("#STR_OZT_VPP_RS_MORE");
        return TrdRsLineRow(col, "ring", OZT_Metal.FAINT, moreWord + " " + rest.ToString(), OZT_Metal.MUTED, "", OZT_Metal.MUTED);
    }

    // The first use of the picked node is where the Conditions page opens.
    protected void TrdRsUse(string key)
    {
        if (m_TrdRsUseKey == "")
            m_TrdRsUseKey = key;
    }

    // What the node opens: the categories and the lines whose lock names it.
    protected float TrdRsCardLocks(OZT_GoodsFile goods, OZT_TradersFile traders, OZT_Settings settings, string id)
    {
        Widget head = TrdRsHeadRow(m_TrdRsA);
        float h = TRD_RS_HEAD_H;
        int lines = 0;
        int cats = 0;
        int shown = 0;
        string tagCat = TrdTr("#STR_OZT_VPP_RS_TAG_CAT");
        string tagLine = TrdTr("#STR_OZT_VPP_RS_TAG_LINE");
        string inWord = TrdTr("#STR_OZT_VPP_IN");
        string condKey = "#STR_OZT_VPP_RS_L_COND";
        string points;
        string words;
        OZT_Condition c;
        if (goods)
        {
            for (int ci = 0; ci < goods.Categories.Count(); ci++)
            {
                OZT_Category cat = goods.Categories[ci];
                c = TrdRsCondOf(cat.Requires, id);
                if (c)
                {
                    cats++;
                    TrdRsUse("c|" + cat.Id);
                    if (shown < TRD_RS_CARD_MAX)
                    {
                        points = TrdRsPointsLine(traders, settings, cat.Id, false);
                        words = TrdRsCondLine(c, TrdRsCountOf(cat.Requires), condKey);
                        h = h + TrdRsItemRow(m_TrdRsA, tagCat, cat.Name, cat.Id, "", points, words);
                        shown++;
                    }
                }
                if (!cat.Goods)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_Goods e = cat.Goods[g];
                    c = TrdRsCondOf(e.Requires, id);
                    if (!c)
                        continue;
                    lines++;
                    TrdRsUse("l|" + cat.Id + "|" + e.ClassName);
                    if (shown >= TRD_RS_CARD_MAX)
                        continue;
                    points = TrdRsPointsLine(traders, settings, cat.Id, false);
                    words = TrdRsCondLine(c, TrdRsCountOf(e.Requires), condKey);
                    h = h + TrdRsItemRow(m_TrdRsA, tagLine, e.ClassName, inWord + " " + cat.Name, "", points, words);
                    shown++;
                }
            }
        }
        h = h + TrdRsTailRow(m_TrdRsA, lines + cats, shown);
        string linesWord = TrdTr("#STR_OZT_VPP_RS_N_LINES");
        string catsWord = TrdTr("#STR_OZT_VPP_RS_N_CATS");
        string count = linesWord + " " + lines.ToString() + " · " + catsWord + " " + cats.ToString();
        TrdRsHeadSay(head, "#STR_OZT_VPP_RS_H_LOCKS", count);
        h = h + TrdRsFootRow(m_TrdRsA, "#STR_OZT_VPP_RS_NOTE_LOCKS");
        return h;
    }

    // The points whose access names the node.
    protected float TrdRsCardAccess(OZT_TradersFile traders, string id)
    {
        Widget head = TrdRsHeadRow(m_TrdRsA);
        float h = TRD_RS_HEAD_H;
        int n = 0;
        int shown = 0;
        string tag = TrdTr("#STR_OZT_VPP_RS_TAG_POINT");
        string accessWord = TrdTr("#STR_OZT_VPP_RS_L_ACCESS");
        string haveWord = TrdTr("#STR_OZT_VPP_RS_L_HAVE_C");
        OZT_Condition c;
        if (traders)
        {
            for (int p = 0; p < traders.Traders.Count(); p++)
            {
                OZT_Point pt = traders.Traders[p];
                c = TrdRsCondOf(pt.Access, id);
                if (!c)
                    continue;
                n++;
                TrdRsUse("p|" + pt.Id);
                if (shown >= TRD_RS_CARD_MAX)
                    continue;
                string l1 = accessWord + " " + TrdRsCondWords(c);
                int rest = TrdRsCountOf(pt.Access) - 1;
                if (rest > 0)
                    l1 = l1 + " (+" + rest.ToString() + ")";
                string l2 = haveWord + " " + TrdRsHolders(c);
                string sub = TrdRsKindWord(pt.Kind) + " · " + pt.Id;
                h = h + TrdRsItemRow(m_TrdRsA, tag, pt.Name, sub, "", l1, l2);
                shown++;
            }
        }
        h = h + TrdRsTailRow(m_TrdRsA, n, shown);
        string pointsWord = TrdTr("#STR_OZT_VPP_RS_N_POINTS");
        TrdRsHeadSay(head, "#STR_OZT_VPP_RS_H_ACCESS", pointsWord + " " + n.ToString());
        h = h + TrdRsFootRow(m_TrdRsA, "#STR_OZT_VPP_RS_NOTE_ACCESS");
        return h;
    }

    // The discounts that name the node: a category's and a line's.
    protected float TrdRsCardCuts(OZT_GoodsFile goods, OZT_TradersFile traders, OZT_Settings settings, string id)
    {
        Widget head = TrdRsHeadRow(m_TrdRsB);
        float h = TRD_RS_HEAD_H;
        int n = 0;
        int shown = 0;
        string tagCat = TrdTr("#STR_OZT_VPP_RS_TAG_CAT");
        string tagLine = TrdTr("#STR_OZT_VPP_RS_TAG_LINE");
        string inWord = TrdTr("#STR_OZT_VPP_IN");
        int i;
        if (goods)
        {
            for (int ci = 0; ci < goods.Categories.Count(); ci++)
            {
                OZT_Category cat = goods.Categories[ci];
                if (cat.Discounts)
                {
                    for (i = 0; i < cat.Discounts.Count(); i++)
                    {
                        if (!TrdRsDiscCond(cat.Discounts[i], id))
                            continue;
                        n++;
                        TrdRsUse("c|" + cat.Id);
                        if (shown >= TRD_RS_CARD_MAX)
                            continue;
                        h = h + TrdRsCutRow(cat.Discounts[i], id, tagCat, cat.Name, cat.Id, TrdRsPointsLine(traders, settings, cat.Id, true));
                        shown++;
                    }
                }
                if (!cat.Goods)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_Goods e = cat.Goods[g];
                    if (!e.Discounts)
                        continue;
                    for (i = 0; i < e.Discounts.Count(); i++)
                    {
                        if (!TrdRsDiscCond(e.Discounts[i], id))
                            continue;
                        n++;
                        TrdRsUse("l|" + cat.Id + "|" + e.ClassName);
                        if (shown >= TRD_RS_CARD_MAX)
                            continue;
                        h = h + TrdRsCutRow(e.Discounts[i], id, tagLine, e.ClassName, inWord + " " + cat.Name, TrdRsPointsLine(traders, settings, cat.Id, true));
                        shown++;
                    }
                }
            }
        }
        h = h + TrdRsTailRow(m_TrdRsB, n, shown);
        string cutsWord = TrdTr("#STR_OZT_VPP_RS_N_CUTS");
        TrdRsHeadSay(head, "#STR_OZT_VPP_RS_H_CUTS", cutsWord + " " + n.ToString());
        h = h + TrdRsFootRow(m_TrdRsB, "#STR_OZT_VPP_RS_NOTE_CUTS");
        return h;
    }

    // A discount's item: its size, what it waits for (every condition of it, the
    // node's own with who holds it now) or who computes it, and where it is given.
    protected float TrdRsCutRow(OZT_Discount d, string id, string tag, string name, string sub, string points)
    {
        string val = "−" + TrdNum(d.Percent) + " %";
        string l1;
        OZT_Condition own = TrdRsDiscCond(d, id);
        if (d.By)
        {
            string byWord = TrdTr("#STR_OZT_VPP_RS_L_BY");
            val = "≤ " + TrdNum(d.Max) + " %";
            l1 = byWord + " " + TrdRsCondWords(d.By);
        }
        else
        {
            string whenWord = TrdTr("#STR_OZT_VPP_RS_L_WHEN");
            string andWord = TrdTr("#STR_OZT_VPP_GT_AND");
            string all = "";
            int whens = TrdRsCountOf(d.When);
            for (int i = 0; i < whens; i++)
            {
                OZT_Condition c = d.When[i];
                if (!c)
                    continue;
                if (all != "")
                    all = all + " " + andWord + " ";
                if (c.Kind == "research")
                    all = all + TrdRsCondWords(c);
                else
                    all = all + TrdCondWords(c);
            }
            l1 = whenWord + " " + all;
        }
        if (own)
        {
            string have = TrdTr("#STR_OZT_VPP_RS_L_HAVE");
            l1 = l1 + " · " + have + " " + TrdRsHolders(own);
        }
        return TrdRsItemRow(m_TrdRsB, tag, name, sub, val, l1, points);
    }

    // Who has the node: the factions that completed it, a line each.
    protected float TrdRsCardWho(string id)
    {
        Widget head = TrdRsHeadRow(m_TrdRsB);
        TrdRsHeadSay(head, "#STR_OZT_VPP_RS_H_WHO", "");
        float h = TRD_RS_HEAD_H;
        string said;
        int state = TrdGtHoldOf("research", id, said);
        string doneHead = TrdTr("#STR_OZT_VPP_RS_DONE_HEAD");
        if (state == 0)
        {
            TrdGtHoldWant("research", id);
            h = h + TrdRsLineRow(m_TrdRsB, "ring", OZT_Metal.FAINT, doneHead, OZT_Metal.TEXT, "...", OZT_Metal.MUTED);
        }
        else if (state == 2)
            h = h + TrdRsLineRow(m_TrdRsB, "ring", OZT_Metal.ALERT, doneHead, OZT_Metal.TEXT, "#STR_OZT_VPP_RS_NO_ANSWER", OZT_Metal.ALERT);
        else if (said == "")
            h = h + TrdRsLineRow(m_TrdRsB, "ring", OZT_Metal.FAINT, doneHead, OZT_Metal.TEXT, "#STR_OZT_VPP_RS_NOBODY", OZT_Metal.MUTED);
        else
        {
            array<string> who = new array<string>();
            said.Split(",", who);
            for (int i = 0; i < who.Count(); i++)
            {
                string one = who[i].Trim();
                if (one == "")
                    continue;
                h = h + TrdRsLineRow(m_TrdRsB, "dot", OZT_Metal.OK, one, OZT_Metal.TEXT, "#STR_OZT_VPP_RS_DONE", OZT_Metal.MUTED);
            }
        }
        h = h + TrdRsFootRow(m_TrdRsB, "#STR_OZT_VPP_RS_NOTE_WHO");
        return h;
    }

    // Every item of the two columns placed by its words: the tag as wide as its word,
    // the name after it, the words beside the name in what the value leaves.
    protected void TrdRsPlaceItems()
    {
        if (!m_TrdRsA)
            return;
        int i;
        OZ_VppFlexRows items = m_TrdRsA[TRD_RS_ITEM];
        for (i = 0; i < items.Count(); i++)
            TrdRsPlaceItem(items.Row(i));
        items = m_TrdRsB[TRD_RS_ITEM];
        for (i = 0; i < items.Count(); i++)
            TrdRsPlaceItem(items.Row(i));
    }

    protected void TrdRsPlaceItem(Widget row)
    {
        if (!row)
            return;
        float s = UnitScale();
        Widget tag = row.FindAnyWidget("ItTag");
        Widget name = row.FindAnyWidget("ItName");
        Widget sub = row.FindAnyWidget("ItSub");
        Widget val = row.FindAnyWidget("ItVal");
        if (!tag || !name || !sub || !val)
            return;
        float x;
        float y;
        float vx;
        float vy;
        float vw;
        float vh;
        tag.GetPos(x, y);
        float tw = OZT_VppKit.HugBox(row, "ItTag", "ItTagText", 6 * s);
        x = x + tw + 8 * s;
        // The value is written from the right: the words end before its first letter.
        val.GetPos(vx, vy);
        val.GetSize(vw, vh);
        float right = vx + vw;
        float valW = OZT_VppKit.TextW(row, "ItVal");
        if (valW > 0)
            right = right - valW - 8 * s;
        float nameW = OZT_VppKit.FitWord(row, "ItName");
        if (x + nameW > right)
        {
            nameW = right - x;
            OZT_VppKit.SetWidth(name, nameW);
        }
        OZT_VppKit.MoveX(name, x);
        x = x + nameW + 8 * s;
        OZT_VppKit.MoveX(sub, x);
        OZT_VppKit.SetWidth(sub, right - x);
    }

    // ------------------------------------------------------------------ the family

    // Where the research family holds: on the server (the settings' GatesOff), and at
    // every point (its GatesOn, GatesOff), a chip a point.
    protected void TrdRsPaintFam(OZT_TradersFile traders, OZT_Settings settings)
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdRsFamChips");
        if (!host)
            return;
        if (!m_TrdRsChips)
            m_TrdRsChips = new OZT_VppChips(m_Flex, host, TRD_RS_L + "chip.layout", "OZT_RsChip");
        m_TrdRsChips.Clear();

        string family = TrdRsFamily();
        bool serverOff = false;
        if (settings && settings.GatesOff && settings.GatesOff.Find(family) != -1)
            serverOff = true;
        string serverWord = TrdTr("#STR_OZT_VPP_RS_SERVER");
        string onWord = TrdTr("#STR_OZT_VPP_GT_FAM_ON");
        string offWord = TrdTr("#STR_OZT_VPP_GT_FAM_OFF");
        if (serverOff)
            TrdInk("TrdRsFamSrv", family + " · " + serverWord + " " + offWord, OZT_Metal.ALERT);
        else
            TrdInk("TrdRsFamSrv", family + " · " + serverWord + " " + onWord, OZT_Metal.OK);

        string asWord = TrdTr("#STR_OZT_VPP_RS_FAM_AS");
        string onHere = TrdTr("#STR_OZT_VPP_RS_FAM_ON_HERE");
        string offHere = TrdTr("#STR_OZT_VPP_RS_FAM_OFF_HERE");
        string offSrv = TrdTr("#STR_OZT_VPP_RS_FAM_OFF_SRV");
        Widget chip;
        if (traders)
        {
            for (int p = 0; p < traders.Traders.Count(); p++)
            {
                OZT_Point pt = traders.Traders[p];
                chip = m_TrdRsChips.Add(pt.Id);
                if (!chip)
                    continue;
                int state = TrdRsFamAt(pt, settings, family);
                string words = asWord;
                if (state == 1)
                    words = onHere;
                else if (state == 2)
                    words = offHere;
                else if (state == 3)
                    words = offSrv;
                RowText(chip, "ChipName", pt.Name, OZT_Metal.TEXT);
                if (state >= 2)
                {
                    OZT_VppKit.SignSet(chip, "ChipDot", "ring", OZT_Metal.MUTED);
                    RowText(chip, "ChipState", words, OZT_Metal.ALERT);
                }
                else
                {
                    OZT_VppKit.SignSet(chip, "ChipDot", "dot", OZT_Metal.OK);
                    RowText(chip, "ChipState", words, OZT_Metal.MUTED);
                }
            }
        }
        TrdRsPlaceFam();
    }

    // The family's box under the taller column: the server's words after the caption,
    // the chips as wide as their words and in lines, the note under them, the box as
    // tall as what it holds -- and the page's scrolled part as tall as all of it.
    protected void TrdRsPlaceFam()
    {
        Widget fam = M_SUB_WIDGET.FindAnyWidget("TrdRsFam");
        Widget body = M_SUB_WIDGET.FindAnyWidget("TrdRsRightBody");
        Widget scroll = M_SUB_WIDGET.FindAnyWidget("TrdRsRightScroll");
        Widget chipsBox = M_SUB_WIDGET.FindAnyWidget("TrdRsFamChips");
        Widget note = M_SUB_WIDGET.FindAnyWidget("TrdRsFamNote");
        Widget srv = M_SUB_WIDGET.FindAnyWidget("TrdRsFamSrv");
        Widget cap = M_SUB_WIDGET.FindAnyWidget("TrdRsFamCap");
        if (!fam || !body || !scroll || !chipsBox || !note || !m_TrdRsChips)
            return;
        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        float fw;
        float fh;
        fam.GetSize(fw, fh);

        if (srv && cap)
        {
            cap.GetPos(x, y);
            x = x + OZT_VppKit.FitWord(fam, "TrdRsFamCap") + 10 * s;
            OZT_VppKit.MoveX(srv, x);
            OZT_VppKit.SetWidth(srv, fw - 15 * s - x);
        }

        Widget chip;
        float nameW;
        float stateW;
        for (int i = 0; i < m_TrdRsChips.Count(); i++)
        {
            chip = m_TrdRsChips.Chip(i);
            if (!chip)
                continue;
            nameW = OZT_VppKit.FitWord(chip, "ChipName");
            x = 28 * s + nameW + 8 * s;
            OZT_VppKit.MoveX(chip.FindAnyWidget("ChipState"), x);
            stateW = OZT_VppKit.FitWord(chip, "ChipState");
            w = x + stateW + 12 * s;
            chip.GetSize(x, h);
            chip.SetSize(w, h);
            OZT_VppKit.SetWidth(chip.FindAnyWidget("OZT_RsChipBg"), w);
            m_TrdRsChips.SetWidth(i, w);
        }
        chipsBox.GetSize(w, h);
        float chipsH = 0;
        if (m_TrdRsChips.Count() > 0)
            chipsH = m_TrdRsChips.Flow(w, 8 * s, 30 * s, null, 0);
        chipsBox.SetSize(w, chipsH);

        float noteY = 38 * s;
        if (chipsH > 0)
            noteY = noteY + chipsH + 10 * s;
        note.GetPos(x, y);
        note.SetPos(x, noteY);
        fh = noteY + 36 * s + 15 * s;

        float top = m_TrdRsColA;
        if (m_TrdRsColB > top)
            top = m_TrdRsColB;
        float famY = 92 * s;
        if (top > 0)
            famY = famY + top * s + 14 * s;
        fam.GetPos(x, y);
        fam.SetPos(x, famY);
        fam.SetSize(fw, fh);
        Widget edge = M_SUB_WIDGET.FindAnyWidget("TrdRsFamEdge");
        Widget ground = M_SUB_WIDGET.FindAnyWidget("TrdRsFamBg");
        if (edge)
            edge.SetSize(fw, fh);
        if (ground)
            ground.SetSize(fw - 2 * s, fh - 2 * s);

        scroll.GetSize(w, h);
        float need = famY + fh + 16 * s;
        if (need < h)
            need = h;
        if (w > 0)
            body.SetSize(w, need);
        scroll.Update();
    }

    // ------------------------------------------------------------------ who has it

    // Who holds a research condition has come (kept by the Conditions page's layer):
    // the node on screen says it.
    override void TrdOnHolders(OZT_AdminGateHolders h)
    {
        super.TrdOnHolders(h);
        if (h.Kind != "research" || TrdShownKey() != TRD_RS_ID || !TrdRsLoaded())
            return;
        TrdRsPaintDetail();
    }

    // ------------------------------------------------------------------ gestures

    // The Conditions page at a place that names a node, its list narrowed to the node.
    protected void TrdRsOpenGates(string key, string id)
    {
        m_TrdGtPut = true;
        SetEdit("TrdGtFind", id);
        m_TrdGtPut = false;
        TrdGtOpenAt(key);
    }

    protected void TrdRsPick(string id)
    {
        if (id == "" || id == m_TrdRsPicked)
            return;
        m_TrdRsPicked = id;
        if (m_TrdRsRows)
        {
            for (int i = 0; i < m_TrdRsRows.Count(); i++)
                TrdRsMarkNode(m_TrdRsRows.Row(i), m_TrdRsRows.Key(i) == id);
        }
        TrdRsPaintGone();
        TrdRsPaintDetail();
        TrdRsPlaceLeft();
        OZT_VppKit.ScrollTo(M_SUB_WIDGET, "TrdRsRightScroll", 0);
    }

    // Is a widget one of this page's own?
    protected bool TrdRsOwns(Widget w)
    {
        if (!m_TrdPanes)
            return false;
        Widget pane = m_TrdPanes.Get(TRD_RS_ID);
        if (!pane)
            return false;
        Widget at = w;
        while (at)
        {
            if (at == pane)
                return true;
            at = at.GetParent();
        }
        return false;
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill && w.GetName() == "TrdRsFind" && TrdRsOwns(w))
        {
            if (m_TrdRsIds && TrdRsLoaded())
                TrdRsPaintNodes();
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET && TrdRsClick(w))
            return true;
        return super.OnClick(w, x, y, button);
    }

    protected bool TrdRsClick(Widget w)
    {
        if (!TrdRsOwns(w))
            return false;
        string nm = w.GetName();
        if (nm == "BtnTrdRsRefresh")
        {
            m_TrdRsLikeOf = "";
            TrdGtHoldForget();
            TrdAskGates();
            TrdReread(OZT_Const.CFG_SETTINGS);
            TrdReread(OZT_Const.CFG_GOODS);
            TrdReread(OZT_Const.CFG_TRADERS);
            return true;
        }
        if (EditBoxWidget.Cast(w))
            return false;
        if (!TrdRsLoaded() || !m_TrdRsIds)
        {
            Hint("#STR_OZT_VPP_READING");
            return true;
        }
        if (nm == "BtnTrdRsAll" || nm == "BtnTrdRsAccess" || nm == "BtnTrdRsGone")
        {
            m_TrdRsFilter = 0;
            if (nm == "BtnTrdRsAccess")
                m_TrdRsFilter = 1;
            else if (nm == "BtnTrdRsGone")
                m_TrdRsFilter = 2;
            TrdRsPaintNodes();
            return true;
        }
        if (nm == "TrdRsOwner")
        {
            TrdRsOwnerNext();
            return true;
        }
        if (nm == "BtnTrdRsFix")
        {
            if (m_TrdRsGoneId != "")
                TrdRsOpenGates(m_TrdRsGoneKey, m_TrdRsGoneId);
            return true;
        }
        if (nm == "BtnTrdRsOpenGates")
        {
            if (m_TrdRsPicked != "")
                TrdRsOpenGates(m_TrdRsUseKey, m_TrdRsPicked);
            return true;
        }
        if (nm == "BtnTrdRsOpenTree")
        {
            if (m_Panes.Contains("research.tree"))
                ShowPane("research.tree");
            return true;
        }
        if (m_TrdRsRows)
        {
            int i = m_TrdRsRows.IndexOf(w);
            if (i != -1)
            {
                TrdRsPick(m_TrdRsRows.Key(i));
                return true;
            }
        }
        return false;
    }
}

#endif
#endif
#endif
