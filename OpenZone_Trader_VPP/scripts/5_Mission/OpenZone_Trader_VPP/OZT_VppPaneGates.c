// Trader > Conditions and discounts (design §5.6 and §8.2; mockup TraderGates): who
// sees goods and at what price. A target on the left -- the server, a category of
// the price book, one of its lines, a point -- and its conditions in the middle: a
// category's or a line's lock (Requires) and discounts, a point's Access, and the
// gate families switched off for the server (settings GatesOff) or switched on and
// off at a point (GatesOn, GatesOff). Everything is edited in the draft of the
// three files and applied by the draft's bar.
//
// The picker on the right offers what the provider of the condition's kind names
// (`gates`): factions, licences, and the research tree's nodes when the research
// glue is loaded -- this page knows the trader alone; the research gate fills its
// own choices. A kind with no choices is a free word. A picked value shows who
// holds it now (`gate_holders`).
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // Kinds of target.
    static const int TRD_GT_SERVER   = 0;
    static const int TRD_GT_CATEGORY = 1;
    static const int TRD_GT_LINE     = 2;
    static const int TRD_GT_POINT    = 3;

    // Every line of the book, or only those with conditions.
    protected bool m_TrdGtAll = false;

    // The target rows: kind, the category's or point's index, the line's index.
    protected ref array<int> m_TrdGtRowsKind;
    protected ref array<int> m_TrdGtRowsA;
    protected ref array<int> m_TrdGtRowsB;

    // The target picked (-1: none).
    protected int m_TrdGtTgt = -1;
    protected int m_TrdGtTgtA = -1;
    protected int m_TrdGtTgtB = -1;

    // The condition rows of the target: the discount (-1: the lock) and the
    // condition in its list (-2: the discount's By; -3: a discount with none).
    protected ref array<int> m_TrdGtCondDisc;
    protected ref array<int> m_TrdGtCondAt;
    protected int m_TrdGtCond = -1;

    // The editor's switches: Not, and whether a new discount is computed by its
    // provider (By) rather than given while conditions hold (When).
    protected bool m_TrdGtNot = false;
    protected bool m_TrdGtBy = false;

    // The picker's rows (values) and the value picked; the family of each row of
    // the families list.
    protected ref array<string> m_TrdGtPickVals;
    protected string m_TrdGtPicked = "";
    protected ref array<string> m_TrdGtFamRows;

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "gates")
            return;
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdNeedGates();
        TrdGtPaint();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "gates")
            TrdGtPaint();
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdGt", w);
        float bottom = h - (16 + 56) * s;
        float targetsH = bottom - 110 * s;
        if (targetsH < 60 * s)
            targetsH = 60 * s;
        TrdSize("TrdGtTargets", 280 * s, targetsH);
        float midW = w - (320 + 16 + 300 + 24) * s;
        if (midW < 400 * s)
            midW = 400 * s;
        TrdPlace("TrdGtMid", 320 * s, 76 * s, midW, bottom - 76 * s);
        TrdSize("TrdGtFams", midW, 64 * s);
        TrdSize("TrdGtRows", midW, 150 * s);
        TrdPlace("TrdGtRight", (320 + 16) * s + midW, 76 * s, 300 * s, bottom - 76 * s);
    }

    // ------------------------------------------------------------------ targets

    protected bool TrdGtLoaded()
    {
        if (!m_TrdDraft)
            return false;
        bool goods = m_TrdDraft.Loaded(OZT_Const.CFG_GOODS);
        bool points = m_TrdDraft.Loaded(OZT_Const.CFG_TRADERS);
        bool settings = m_TrdDraft.Loaded(OZT_Const.CFG_SETTINGS);
        return goods && points && settings;
    }

    protected OZT_Category TrdGtCat(int i)
    {
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        if (i < 0 || i >= cats.Count())
            return null;
        return cats[i];
    }

    protected OZT_Goods TrdGtLine(int c, int l)
    {
        OZT_Category cat = TrdGtCat(c);
        if (!cat || !cat.Goods || l < 0 || l >= cat.Goods.Count())
            return null;
        return cat.Goods[l];
    }

    protected OZT_Point TrdGtPoint(int i)
    {
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        if (i < 0 || i >= points.Count())
            return null;
        return points[i];
    }

    // The file the picked target lives in.
    protected string TrdGtFile()
    {
        if (m_TrdGtTgt == TRD_GT_POINT)
            return OZT_Const.CFG_TRADERS;
        if (m_TrdGtTgt == TRD_GT_SERVER)
            return OZT_Const.CFG_SETTINGS;
        return OZT_Const.CFG_GOODS;
    }

    // The picked target's lock: a category's or a line's Requires, a point's
    // Access; made when the file had none. Null for the server.
    protected array<ref OZT_Condition> TrdGtLock()
    {
        if (m_TrdGtTgt == TRD_GT_CATEGORY)
        {
            OZT_Category cat = TrdGtCat(m_TrdGtTgtA);
            if (!cat)
                return null;
            if (!cat.Requires)
                cat.Requires = new array<ref OZT_Condition>();
            return cat.Requires;
        }
        if (m_TrdGtTgt == TRD_GT_LINE)
        {
            OZT_Goods e = TrdGtLine(m_TrdGtTgtA, m_TrdGtTgtB);
            if (!e)
                return null;
            if (!e.Requires)
                e.Requires = new array<ref OZT_Condition>();
            return e.Requires;
        }
        if (m_TrdGtTgt == TRD_GT_POINT)
        {
            OZT_Point p = TrdGtPoint(m_TrdGtTgtA);
            if (!p)
                return null;
            if (!p.Access)
                p.Access = new array<ref OZT_Condition>();
            return p.Access;
        }
        return null;
    }

    // The picked target's discounts: a category's or a line's. Null otherwise.
    protected array<ref OZT_Discount> TrdGtDiscs()
    {
        if (m_TrdGtTgt == TRD_GT_CATEGORY)
        {
            OZT_Category cat = TrdGtCat(m_TrdGtTgtA);
            if (!cat)
                return null;
            if (!cat.Discounts)
                cat.Discounts = new array<ref OZT_Discount>();
            return cat.Discounts;
        }
        if (m_TrdGtTgt == TRD_GT_LINE)
        {
            OZT_Goods e = TrdGtLine(m_TrdGtTgtA, m_TrdGtTgtB);
            if (!e)
                return null;
            if (!e.Discounts)
                e.Discounts = new array<ref OZT_Discount>();
            return e.Discounts;
        }
        return null;
    }

    protected int TrdGtCount(array<ref OZT_Condition> conds)
    {
        if (!conds)
            return 0;
        return conds.Count();
    }

    protected int TrdGtDiscCount(array<ref OZT_Discount> discs)
    {
        if (!discs)
            return 0;
        return discs.Count();
    }

    // "  lock 2  discounts 1": what a target holds, short.
    protected string TrdGtBadges(int locks, int discs)
    {
        string badges = "";
        if (locks > 0)
        {
            string lockWord = TrdTr("#STR_OZT_VPP_GT_LOCK_N");
            badges = badges + "   " + lockWord + " " + locks.ToString();
        }
        if (discs > 0)
        {
            string discWord = TrdTr("#STR_OZT_VPP_GT_DISC_N");
            badges = badges + "   " + discWord + " " + discs.ToString();
        }
        return badges;
    }

    protected void TrdGtAddRow(array<string> rows, array<int> inks, string text, int ink, int kind, int a, int b)
    {
        rows.Insert(text);
        inks.Insert(ink);
        m_TrdGtRowsKind.Insert(kind);
        m_TrdGtRowsA.Insert(a);
        m_TrdGtRowsB.Insert(b);
    }

    protected void TrdGtPaintTargets()
    {
        if (!m_TrdGtRowsKind)
        {
            m_TrdGtRowsKind = new array<int>();
            m_TrdGtRowsA = new array<int>();
            m_TrdGtRowsB = new array<int>();
        }
        m_TrdGtRowsKind.Clear();
        m_TrdGtRowsA.Clear();
        m_TrdGtRowsB.Clear();
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        int picked = -1;

        string serverWord = TrdTr("#STR_OZT_VPP_GT_SERVER");
        TrdGtAddRow(rows, inks, serverWord, OZT_Metal.ACCENT, TRD_GT_SERVER, -1, -1);
        if (m_TrdGtTgt == TRD_GT_SERVER)
            picked = 0;

        string catWord = TrdTr("#STR_OZT_VPP_GT_CAT");
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        for (int c = 0; c < cats.Count(); c++)
        {
            OZT_Category cat = cats[c];
            string badges = TrdGtBadges(TrdGtCount(cat.Requires), TrdGtDiscCount(cat.Discounts));
            string row = catWord + ": " + cat.Name + " (" + cat.Id + ")" + badges;
            if (m_TrdGtTgt == TRD_GT_CATEGORY && m_TrdGtTgtA == c)
                picked = rows.Count();
            TrdGtAddRow(rows, inks, row, 0, TRD_GT_CATEGORY, c, -1);
            if (!cat.Goods)
                continue;
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_Goods e = cat.Goods[g];
                int locks = TrdGtCount(e.Requires);
                int discs = TrdGtDiscCount(e.Discounts);
                bool keep = m_TrdGtAll || locks > 0 || discs > 0;
                bool isPicked = m_TrdGtTgt == TRD_GT_LINE && m_TrdGtTgtA == c && m_TrdGtTgtB == g;
                if (!keep && !isPicked)
                    continue;
                if (isPicked)
                    picked = rows.Count();
                string lineRow = "      " + e.ClassName + TrdGtBadges(locks, discs);
                TrdGtAddRow(rows, inks, lineRow, OZT_Metal.MUTED, TRD_GT_LINE, c, g);
            }
        }

        string pointWord = TrdTr("#STR_OZT_VPP_GT_POINT");
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int p = 0; p < points.Count(); p++)
        {
            OZT_Point pt = points[p];
            string pBadges = TrdGtBadges(TrdGtCount(pt.Access), 0);
            string pRow = pointWord + ": " + pt.Name + " (" + pt.Id + ")" + pBadges;
            if (m_TrdGtTgt == TRD_GT_POINT && m_TrdGtTgtA == p)
                picked = rows.Count();
            TrdGtAddRow(rows, inks, pRow, OZT_Metal.ACCENT, TRD_GT_POINT, p, -1);
        }
        TrdFill("TrdGtTargets", rows, inks, picked);
        string all = TrdTr("#STR_OZT_VPP_GT_SHOW_WITH");
        if (m_TrdGtAll)
            all = TrdTr("#STR_OZT_VPP_GT_SHOW_ALL");
        SayTrd("BtnTrdGtAllText", all);
    }

    // ------------------------------------------------------------------ painting

    protected void TrdGtPaint()
    {
        if (!TrdGtLoaded())
        {
            SayTrd("TrdGtSub", TrdTr("#STR_OZT_VPP_READING"));
            return;
        }
        SayTrd("TrdGtSub", TrdTr("#STR_OZT_VPP_GT_SUB"));
        TrdGtPaintTargets();
        TrdGtPaintHead();
        TrdGtPaintFams();
        TrdGtPaintConds();
        TrdGtPaintSwitches();
        TrdGtPaintPicker();
    }

    protected void TrdGtPaintHead()
    {
        string head = TrdTr("#STR_OZT_VPP_GT_PICK");
        string sub = "";
        if (m_TrdGtTgt == TRD_GT_SERVER)
        {
            head = TrdTr("#STR_OZT_VPP_GT_SERVER");
            sub = TrdTr("#STR_OZT_VPP_GT_SERVER_H");
        }
        else if (m_TrdGtTgt == TRD_GT_CATEGORY)
        {
            OZT_Category cat = TrdGtCat(m_TrdGtTgtA);
            if (cat)
            {
                head = cat.Name + "  (" + cat.Id + ")";
                sub = TrdTr("#STR_OZT_VPP_GT_CAT_H");
            }
        }
        else if (m_TrdGtTgt == TRD_GT_LINE)
        {
            OZT_Goods e = TrdGtLine(m_TrdGtTgtA, m_TrdGtTgtB);
            OZT_Category lineCat = TrdGtCat(m_TrdGtTgtA);
            if (e && lineCat)
            {
                head = e.ClassName;
                string inWord = TrdTr("#STR_OZT_VPP_IN");
                sub = inWord + " " + lineCat.Name + ",  " + e.Buy.ToString() + " / " + e.Sell.ToString();
            }
        }
        else if (m_TrdGtTgt == TRD_GT_POINT)
        {
            OZT_Point p = TrdGtPoint(m_TrdGtTgtA);
            if (p)
            {
                head = p.Name + "  (" + p.Id + ")";
                sub = TrdTr("#STR_OZT_VPP_GT_POINT_H");
            }
        }
        SayTrd("TrdGtHead", head);
        SayTrd("TrdGtTargetSub", sub);
    }

    // Is a family off for the whole server (the settings' GatesOff)?
    protected bool TrdGtServerOff(string family)
    {
        array<string> off = m_TrdDraft.Settings.GatesOff;
        if (!off)
            return false;
        return off.Find(family) != -1;
    }

    protected void TrdGtPaintFams()
    {
        if (!m_TrdGtFamRows)
            m_TrdGtFamRows = new array<string>();
        m_TrdGtFamRows.Clear();
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        array<string> families = new array<string>();
        TrdFamilies(families);
        string onWord = TrdTr("#STR_OZT_VPP_GT_FAM_ON");
        string offWord = TrdTr("#STR_OZT_VPP_GT_FAM_OFF");
        string asServer = TrdTr("#STR_OZT_VPP_GT_FAM_AS_SERVER");
        string hereWord = TrdTr("#STR_OZT_VPP_GT_FAM_HERE");
        OZT_Point p = null;
        if (m_TrdGtTgt == TRD_GT_POINT)
            p = TrdGtPoint(m_TrdGtTgtA);
        for (int i = 0; i < families.Count(); i++)
        {
            string fam = families[i];
            string server = onWord;
            int ink = OZT_Metal.OK;
            if (TrdGtServerOff(fam))
            {
                server = offWord;
                ink = OZT_Metal.ALERT;
            }
            string row = fam + ":  " + server;
            if (p)
            {
                if (p.GatesOn && p.GatesOn.Find(fam) != -1)
                {
                    row = fam + ":  " + hereWord + " " + onWord;
                    ink = OZT_Metal.OK;
                }
                else if (p.GatesOff && p.GatesOff.Find(fam) != -1)
                {
                    row = fam + ":  " + hereWord + " " + offWord;
                    ink = OZT_Metal.ALERT;
                }
                else
                    row = fam + ":  " + asServer + " (" + server + ")";
            }
            else if (m_TrdGtTgt != TRD_GT_SERVER)
                ink = OZT_Metal.MUTED;
            rows.Insert(row);
            inks.Insert(ink);
            m_TrdGtFamRows.Insert(fam);
        }
        TrdFill("TrdGtFams", rows, inks, -1);
    }

    // A family's row clicked: on the server row it goes off or on for every point;
    // on a point it steps "as the server" -> on here -> off here.
    protected void TrdGtFamClick(int row)
    {
        if (!m_TrdGtFamRows || row < 0 || row >= m_TrdGtFamRows.Count())
            return;
        string fam = m_TrdGtFamRows[row];
        if (m_TrdGtTgt == TRD_GT_SERVER)
        {
            array<string> off = m_TrdDraft.Settings.GatesOff;
            int at = off.Find(fam);
            if (at != -1)
                off.RemoveOrdered(at);
            else
                off.Insert(fam);
            TrdTouch(OZT_Const.CFG_SETTINGS);
            TrdGtPaintFams();
            return;
        }
        if (m_TrdGtTgt != TRD_GT_POINT)
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_FAM_WHERE"));
            return;
        }
        OZT_Point p = TrdGtPoint(m_TrdGtTgtA);
        if (!p)
            return;
        if (!p.GatesOn)
            p.GatesOn = new array<string>();
        if (!p.GatesOff)
            p.GatesOff = new array<string>();
        int on = p.GatesOn.Find(fam);
        int offAt = p.GatesOff.Find(fam);
        if (on != -1)
        {
            p.GatesOn.RemoveOrdered(on);
            p.GatesOff.Insert(fam);
        }
        else if (offAt != -1)
            p.GatesOff.RemoveOrdered(offAt);
        else
            p.GatesOn.Insert(fam);
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdGtPaintFams();
    }

    // The target's lock and discounts, a condition a row.
    protected void TrdGtPaintConds()
    {
        if (!m_TrdGtCondDisc)
        {
            m_TrdGtCondDisc = new array<int>();
            m_TrdGtCondAt = new array<int>();
        }
        m_TrdGtCondDisc.Clear();
        m_TrdGtCondAt.Clear();
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();

        array<ref OZT_Condition> lockList = TrdGtLock();
        string lockWord = TrdTr("#STR_OZT_VPP_GT_LOCK");
        if (lockList)
        {
            for (int i = 0; i < lockList.Count(); i++)
            {
                string words = TrdCondWords(lockList[i]);
                rows.Insert(lockWord + ":  " + words);
                inks.Insert(0);
                m_TrdGtCondDisc.Insert(-1);
                m_TrdGtCondAt.Insert(i);
            }
        }

        array<ref OZT_Discount> discs = TrdGtDiscs();
        if (discs)
        {
            string whenWord = TrdTr("#STR_OZT_VPP_GT_WHEN");
            string andWord = TrdTr("#STR_OZT_VPP_GT_AND");
            string alwaysWord = TrdTr("#STR_OZT_VPP_GT_ALWAYS");
            string byWord = TrdTr("#STR_OZT_VPP_GT_BY");
            string upTo = TrdTr("#STR_OZT_VPP_GT_UP_TO");
            for (int d = 0; d < discs.Count(); d++)
            {
                OZT_Discount disc = discs[d];
                if (disc.By)
                {
                    string byText = TrdCondWords(disc.By);
                    string byRow = upTo + " " + TrdNum(disc.Max) + "%:  " + byWord + " " + byText;
                    rows.Insert(byRow);
                    inks.Insert(OZT_Metal.OK);
                    m_TrdGtCondDisc.Insert(d);
                    m_TrdGtCondAt.Insert(-2);
                    continue;
                }
                string pct = "-" + TrdNum(disc.Percent) + "%";
                if (!disc.When || disc.When.Count() == 0)
                {
                    rows.Insert(pct + ":  " + alwaysWord);
                    inks.Insert(OZT_Metal.OK);
                    m_TrdGtCondDisc.Insert(d);
                    m_TrdGtCondAt.Insert(-3);
                    continue;
                }
                for (int k = 0; k < disc.When.Count(); k++)
                {
                    string condText = TrdCondWords(disc.When[k]);
                    string condRow = "        " + andWord + " " + condText;
                    if (k == 0)
                        condRow = pct + ":  " + whenWord + " " + condText;
                    rows.Insert(condRow);
                    inks.Insert(OZT_Metal.OK);
                    m_TrdGtCondDisc.Insert(d);
                    m_TrdGtCondAt.Insert(k);
                }
            }
        }
        if (m_TrdGtCond >= rows.Count())
            m_TrdGtCond = -1;
        TrdFill("TrdGtRows", rows, inks, m_TrdGtCond);
    }

    // The condition of a row of the list; null for a discount with none.
    protected OZT_Condition TrdGtCondAt(int row)
    {
        if (!m_TrdGtCondDisc || row < 0 || row >= m_TrdGtCondDisc.Count())
            return null;
        int disc = m_TrdGtCondDisc[row];
        int at = m_TrdGtCondAt[row];
        if (disc == -1)
        {
            array<ref OZT_Condition> lockList = TrdGtLock();
            if (!lockList || at < 0 || at >= lockList.Count())
                return null;
            return lockList[at];
        }
        OZT_Discount d = TrdGtDiscAt(row);
        if (!d)
            return null;
        if (at == -2)
            return d.By;
        if (!d.When || at < 0 || at >= d.When.Count())
            return null;
        return d.When[at];
    }

    protected OZT_Discount TrdGtDiscAt(int row)
    {
        if (!m_TrdGtCondDisc || row < 0 || row >= m_TrdGtCondDisc.Count())
            return null;
        int disc = m_TrdGtCondDisc[row];
        array<ref OZT_Discount> discs = TrdGtDiscs();
        if (disc < 0 || !discs || disc >= discs.Count())
            return null;
        return discs[disc];
    }

    protected void TrdGtPaintSwitches()
    {
        string notText = TrdTr("#STR_OZT_VPP_GT_AS_WRITTEN");
        if (m_TrdGtNot)
            notText = TrdTr("#STR_OZT_VPP_NOT");
        SayTrd("BtnTrdGtNotText", notText);
        string mode = TrdTr("#STR_OZT_VPP_GT_MODE_WHEN");
        if (m_TrdGtBy)
            mode = TrdTr("#STR_OZT_VPP_GT_MODE_BY");
        SayTrd("BtnTrdGtModeText", mode);
    }

    // A row picked: the editor takes its condition and its discount.
    protected void TrdGtTakeRow(int row)
    {
        m_TrdGtCond = row;
        OZT_Condition c = TrdGtCondAt(row);
        if (c)
        {
            TrdSet("TrdGtKind", c.Kind);
            TrdSet("TrdGtArg", c.Arg);
            bool turned = c.Not == 1;
            m_TrdGtNot = turned;
        }
        OZT_Discount d = TrdGtDiscAt(row);
        if (d)
        {
            TrdSet("TrdGtPct", TrdNum(d.Percent));
            TrdSet("TrdGtMax", TrdNum(d.Max));
            bool computed = d.By != null;
            m_TrdGtBy = computed;
        }
        TrdGtPaintSwitches();
        TrdGtPaintPicker();
    }

    // ------------------------------------------------------------------ the picker

    protected void TrdGtPaintPicker()
    {
        if (!m_TrdGtPickVals)
            m_TrdGtPickVals = new array<string>();
        m_TrdGtPickVals.Clear();
        string kind = GetEdit("TrdGtKind").Trim();
        string kindWord = TrdCondKindWord(kind);
        array<string> rows = new array<string>();
        OZT_AdminGateKind gk = TrdGateKind(kind);
        string sub;
        if (kind == "")
            sub = TrdTr("#STR_OZT_VPP_GT_NEED_KIND");
        else if (!m_TrdGates)
            sub = TrdTr("#STR_OZT_VPP_GT_ASKING");
        else if (TrdFamilyOf(kind) == "")
            sub = TrdTr("#STR_OZT_VPP_GT_NOBODY");
        else if (!gk || gk.Values.Count() == 0)
            sub = TrdTr("#STR_OZT_VPP_GT_FREE");
        else
        {
            string count = TrdTr("#STR_OZT_VPP_GT_CHOICES");
            sub = kindWord + ":  " + count + " " + gk.Values.Count().ToString();
            if (gk.Cut == 1)
                sub = sub + "+";
        }
        SayTrd("TrdGtPickSub", sub);
        if (gk)
        {
            string find = TrdLow(GetEdit("TrdGtPickFind").Trim());
            for (int i = 0; i < gk.Values.Count(); i++)
            {
                string label = "";
                if (i < gk.Labels.Count())
                    label = gk.Labels[i];
                string row = label + "  (" + gk.Values[i] + ")";
                if (find != "" && TrdLow(row).IndexOf(find) < 0)
                    continue;
                rows.Insert(row);
                m_TrdGtPickVals.Insert(gk.Values[i]);
            }
        }
        TrdFill("TrdGtPick", rows, null, -1);
    }

    protected void TrdGtPickRow(int row)
    {
        if (!m_TrdGtPickVals || row < 0 || row >= m_TrdGtPickVals.Count())
            return;
        m_TrdGtPicked = m_TrdGtPickVals[row];
        OZT_AdminGateAsk ask = new OZT_AdminGateAsk();
        ask.Kind = GetEdit("TrdGtKind").Trim();
        ask.Arg = m_TrdGtPicked;
        string body;
        string err;
        JsonFileLoader<OZT_AdminGateAsk>.MakeData(ask, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "gate_holders", body);
        SayTrd("TrdGtHolders", TrdTr("#STR_OZT_VPP_GT_ASKING"));
    }

    // The value picked into the condition: as its only value, or one more
    // alternative ("a|b": any of them).
    protected void TrdGtPickInto(bool add)
    {
        if (m_TrdGtPicked == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_PICK_VALUE"));
            return;
        }
        string arg = GetEdit("TrdGtArg").Trim();
        if (!add || arg == "")
        {
            TrdSet("TrdGtArg", m_TrdGtPicked);
            return;
        }
        array<string> alts = new array<string>();
        arg.Split("|", alts);
        for (int i = 0; i < alts.Count(); i++)
        {
            if (alts[i].Trim() == m_TrdGtPicked)
                return;
        }
        TrdSet("TrdGtArg", arg + "|" + m_TrdGtPicked);
    }

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        super.TrdOnAnswer(op, ok, json, error);
        if (op != "gate_holders")
            return;
        if (!ok)
        {
            SayTrd("TrdGtHolders", Words(error));
            return;
        }
        OZT_AdminGateHolders parsed = new OZT_AdminGateHolders();
        string err;
        if (!JsonFileLoader<OZT_AdminGateHolders>.LoadData(json, parsed, err))
            return;
        OZT_AdminGateHolders h = parsed.Copy();
        TrdOnHolders(h);
    }

    // Who holds a value: the picker says it for its value; the research trade
    // page takes the answer for its node too (a later layer).
    protected void TrdOnHolders(OZT_AdminGateHolders h)
    {
        if (h.Arg != m_TrdGtPicked)
            return;
        string text;
        if (h.Answers != 1)
            text = TrdTr("#STR_OZT_VPP_GT_NOBODY");
        else if (h.Who.Count() == 0)
            text = TrdTr("#STR_OZT_VPP_GT_HOLD_NONE");
        else
        {
            string holdWord = TrdTr("#STR_OZT_VPP_GT_HOLD");
            text = holdWord + " " + TrdJoin(h.Who, ", ");
            if (h.Cut == 1)
                text = text + " ...";
        }
        SayTrd("TrdGtHolders", text);
    }

    // ------------------------------------------------------------------ edits

    // A condition from the editor; null when it names no kind.
    protected OZT_Condition TrdGtNewCond()
    {
        string kind = GetEdit("TrdGtKind").Trim();
        if (kind == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_NEED_KIND"));
            return null;
        }
        OZT_Condition c = new OZT_Condition();
        c.Kind = kind;
        c.Arg = GetEdit("TrdGtArg").Trim();
        c.Not = 0;
        if (m_TrdGtNot)
            c.Not = 1;
        return c;
    }

    protected bool TrdGtNeedTarget()
    {
        if (m_TrdGtTgt == TRD_GT_CATEGORY || m_TrdGtTgt == TRD_GT_LINE || m_TrdGtTgt == TRD_GT_POINT)
            return true;
        Hint(TrdTr("#STR_OZT_VPP_GT_NEED_TARGET"));
        return false;
    }

    protected void TrdGtAddLock()
    {
        if (!TrdGtNeedTarget())
            return;
        OZT_Condition c = TrdGtNewCond();
        array<ref OZT_Condition> lockList = TrdGtLock();
        if (!c || !lockList)
            return;
        lockList.Insert(c);
        TrdTouch(TrdGtFile());
        TrdGtPaintConds();
        TrdGtPaintTargets();
    }

    protected void TrdGtAddDisc()
    {
        if (!TrdGtNeedTarget())
            return;
        array<ref OZT_Discount> discs = TrdGtDiscs();
        if (!discs)
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_NO_DISC_HERE"));
            return;
        }
        OZT_Discount d = new OZT_Discount();
        d.When = new array<ref OZT_Condition>();
        string kind = GetEdit("TrdGtKind").Trim();
        if (m_TrdGtBy)
        {
            OZT_Condition byCond = TrdGtNewCond();
            if (!byCond)
                return;
            d.By = byCond;
            d.Max = GetEdit("TrdGtMax").Trim().ToFloat();
        }
        else
        {
            d.Percent = GetEdit("TrdGtPct").Trim().ToFloat();
            if (kind != "")
            {
                OZT_Condition whenCond = TrdGtNewCond();
                if (whenCond)
                    d.When.Insert(whenCond);
            }
        }
        discs.Insert(d);
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGtPaintConds();
        TrdGtPaintTargets();
    }

    // One more condition a discount waits for (all of them hold).
    protected void TrdGtAddAnd()
    {
        OZT_Discount d = TrdGtDiscAt(m_TrdGtCond);
        if (!d || d.By)
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_PICK_DISC"));
            return;
        }
        OZT_Condition c = TrdGtNewCond();
        if (!c)
            return;
        if (!d.When)
            d.When = new array<ref OZT_Condition>();
        d.When.Insert(c);
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGtPaintConds();
    }

    // The editor into the picked row: its condition, and its discount's numbers.
    protected void TrdGtSaveRow()
    {
        if (m_TrdGtCond < 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_PICK_ROW"));
            return;
        }
        OZT_Condition c = TrdGtCondAt(m_TrdGtCond);
        if (c)
        {
            string kind = GetEdit("TrdGtKind").Trim();
            if (kind == "")
            {
                Hint(TrdTr("#STR_OZT_VPP_GT_NEED_KIND"));
                return;
            }
            c.Kind = kind;
            c.Arg = GetEdit("TrdGtArg").Trim();
            c.Not = 0;
            if (m_TrdGtNot)
                c.Not = 1;
        }
        OZT_Discount d = TrdGtDiscAt(m_TrdGtCond);
        if (d)
        {
            if (d.By)
                d.Max = GetEdit("TrdGtMax").Trim().ToFloat();
            else
                d.Percent = GetEdit("TrdGtPct").Trim().ToFloat();
        }
        TrdTouch(TrdGtFile());
        TrdGtPaintConds();
        TrdGtPaintTargets();
    }

    // The picked row out: a lock's condition, a discount's condition (the
    // discount goes with its last one), or the discount itself.
    protected void TrdGtDelRow()
    {
        int row = m_TrdGtCond;
        if (!m_TrdGtCondDisc || row < 0 || row >= m_TrdGtCondDisc.Count())
        {
            Hint(TrdTr("#STR_OZT_VPP_GT_PICK_ROW"));
            return;
        }
        int disc = m_TrdGtCondDisc[row];
        int at = m_TrdGtCondAt[row];
        if (disc == -1)
        {
            array<ref OZT_Condition> lockList = TrdGtLock();
            if (lockList && at >= 0 && at < lockList.Count())
                lockList.RemoveOrdered(at);
        }
        else
        {
            array<ref OZT_Discount> discs = TrdGtDiscs();
            OZT_Discount d = TrdGtDiscAt(row);
            if (d && discs)
            {
                bool whole = at < 0 || !d.When || d.When.Count() <= 1;
                if (whole)
                    discs.RemoveOrdered(disc);
                else
                    d.When.RemoveOrdered(at);
            }
        }
        m_TrdGtCond = -1;
        TrdTouch(TrdGtFile());
        TrdGtPaintConds();
        TrdGtPaintTargets();
    }

    // The editor's kind one step on or back through the kinds the providers answer.
    protected void TrdGtKindStep(int step)
    {
        array<string> kinds = new array<string>();
        TrdKinds(kinds);
        if (kinds.Count() == 0)
            return;
        int at = kinds.Find(GetEdit("TrdGtKind").Trim()) + step;
        if (at < 0)
            at = kinds.Count() - 1;
        if (at >= kinds.Count())
            at = 0;
        TrdSet("TrdGtKind", kinds[at]);
        m_TrdGtPicked = "";
        TrdGtPaintPicker();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && !m_Repaint)
        {
            string nm = w.GetName();
            if (nm == "TrdGtTargets")
            {
                if (m_TrdGtRowsKind && row >= 0 && row < m_TrdGtRowsKind.Count())
                {
                    m_TrdGtTgt = m_TrdGtRowsKind[row];
                    m_TrdGtTgtA = m_TrdGtRowsA[row];
                    m_TrdGtTgtB = m_TrdGtRowsB[row];
                    m_TrdGtCond = -1;
                    TrdGtPaintHead();
                    TrdGtPaintFams();
                    TrdGtPaintConds();
                }
                return true;
            }
            if (nm == "TrdGtRows")
            {
                TrdGtTakeRow(row);
                return true;
            }
            if (nm == "TrdGtFams")
            {
                TrdGtFamClick(row);
                return true;
            }
            if (nm == "TrdGtPick")
            {
                TrdGtPickRow(row);
                return true;
            }
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill)
        {
            string nm = w.GetName();
            if (nm == "TrdGtKind" || nm == "TrdGtPickFind")
            {
                TrdGtPaintPicker();
                return true;
            }
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();
            if (nm.IndexOf("BtnTrdGt") == 0 && nm != "BtnTrdGtRefresh" && !TrdGtLoaded())
            {
                Hint(TrdTr("#STR_OZT_VPP_READING"));
                return true;
            }
            if (nm == "BtnTrdGtAll")
            {
                if (m_TrdGtAll)
                    m_TrdGtAll = false;
                else
                    m_TrdGtAll = true;
                TrdGtPaintTargets();
                return true;
            }
            if (nm == "BtnTrdGtRefresh")
            {
                TrdAskGates();
                TrdReread(OZT_Const.CFG_SETTINGS);
                TrdReread(OZT_Const.CFG_GOODS);
                TrdReread(OZT_Const.CFG_TRADERS);
                return true;
            }
            if (nm == "BtnTrdGtKindPrev")
            {
                TrdGtKindStep(-1);
                return true;
            }
            if (nm == "BtnTrdGtKindNext")
            {
                TrdGtKindStep(1);
                return true;
            }
            if (nm == "BtnTrdGtNot")
            {
                if (m_TrdGtNot)
                    m_TrdGtNot = false;
                else
                    m_TrdGtNot = true;
                TrdGtPaintSwitches();
                return true;
            }
            if (nm == "BtnTrdGtMode")
            {
                if (m_TrdGtBy)
                    m_TrdGtBy = false;
                else
                    m_TrdGtBy = true;
                TrdGtPaintSwitches();
                return true;
            }
            if (nm == "BtnTrdGtSave")
            {
                TrdGtSaveRow();
                return true;
            }
            if (nm == "BtnTrdGtDel")
            {
                TrdGtDelRow();
                return true;
            }
            if (nm == "BtnTrdGtAddAnd")
            {
                TrdGtAddAnd();
                return true;
            }
            if (nm == "BtnTrdGtAddLock")
            {
                TrdGtAddLock();
                return true;
            }
            if (nm == "BtnTrdGtAddDisc")
            {
                TrdGtAddDisc();
                return true;
            }
            if (nm == "BtnTrdGtPickSet")
            {
                TrdGtPickInto(false);
                return true;
            }
            if (nm == "BtnTrdGtPickAdd")
            {
                TrdGtPickInto(true);
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
