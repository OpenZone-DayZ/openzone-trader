// Trader > Stock (design §5.6 and §11.1; mockup TraderStock): the stock of each
// point by condition (`stock_list`: per point and category, the stocked lines it
// deals by, with the count in each condition and the Max) and the count of one
// line set by hand (`stock_set`, the bridge verb `stock` as an admin op). Points
// that name one stock share one shelf: setting a line at one sets it at all.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The section's last stock list; the point picked; the line picked (its
    // category and its index there, by row of the lines list); the condition the
    // form edits.
    protected ref OZT_AdminStockList m_TrdSk;
    protected int m_TrdSkPoint = 0;
    protected ref array<int> m_TrdSkRowCat;
    protected ref array<int> m_TrdSkRowLine;
    protected int m_TrdSkCat = -1;
    protected int m_TrdSkLine = -1;
    protected int m_TrdSkLevel = 0;

    // What was picked, by name, so a fresh list keeps it.
    protected string m_TrdSkKeepPoint = "";
    protected string m_TrdSkKeepCat = "";
    protected string m_TrdSkKeepClass = "";

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key == "stock")
            Ask(OZT_Const.SECTION_ADMIN, "stock_list", "{}");
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdSk", w);
        float listH = h - (96 + 24) * s;
        if (listH < 60 * s)
            listH = 60 * s;
        TrdSize("TrdSkPoints", 260 * s, listH);
        float midW = w - (300 + 16 + 300 + 24) * s;
        if (midW < 220 * s)
            midW = 220 * s;
        TrdSize("TrdSkLines", midW, listH);
        TrdPlace("TrdSkRight", (300 + 16) * s + midW, 76 * s, 300 * s, h - 100 * s);
    }

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        super.TrdOnAnswer(op, ok, json, error);
        if (!ok || (op != "stock_list" && op != "stock_set"))
            return;
        OZT_AdminStockList parsed = new OZT_AdminStockList();
        string err;
        if (!JsonFileLoader<OZT_AdminStockList>.LoadData(json, parsed, err))
            return;
        m_TrdSk = parsed.Copy();
        TrdSkFind();
        TrdSkPaint();
    }

    // The pick of before, found again in a fresh list by its names.
    protected void TrdSkFind()
    {
        m_TrdSkPoint = 0;
        m_TrdSkCat = -1;
        m_TrdSkLine = -1;
        for (int i = 0; i < m_TrdSk.Points.Count(); i++)
        {
            if (m_TrdSk.Points[i].Id == m_TrdSkKeepPoint)
                m_TrdSkPoint = i;
        }
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (!p)
            return;
        for (int c = 0; c < p.Cats.Count(); c++)
        {
            if (p.Cats[c].Id != m_TrdSkKeepCat)
                continue;
            for (int l = 0; l < p.Cats[c].Lines.Count(); l++)
            {
                if (p.Cats[c].Lines[l].ClassName == m_TrdSkKeepClass)
                {
                    m_TrdSkCat = c;
                    m_TrdSkLine = l;
                }
            }
        }
    }

    protected OZT_AdminStockPoint TrdSkPointAt()
    {
        if (!m_TrdSk || m_TrdSkPoint < 0 || m_TrdSkPoint >= m_TrdSk.Points.Count())
            return null;
        return m_TrdSk.Points[m_TrdSkPoint];
    }

    protected OZT_AdminStockLine TrdSkLineAt()
    {
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (!p || m_TrdSkCat < 0 || m_TrdSkCat >= p.Cats.Count())
            return null;
        OZT_AdminStockCat cat = p.Cats[m_TrdSkCat];
        if (m_TrdSkLine < 0 || m_TrdSkLine >= cat.Lines.Count())
            return null;
        return cat.Lines[m_TrdSkLine];
    }

    // A condition in words: 0 pristine .. 3 badly damaged.
    protected string TrdLevelWord(int level)
    {
        if (level == 1)
            return TrdTr("#STR_OZT_VPP_LV_1");
        if (level == 2)
            return TrdTr("#STR_OZT_VPP_LV_2");
        if (level == 3)
            return TrdTr("#STR_OZT_VPP_LV_3");
        return TrdTr("#STR_OZT_VPP_LV_0");
    }

    protected int TrdSkCount(OZT_AdminStockLine line, int level)
    {
        if (!line || level < 0 || level >= line.Counts.Count())
            return 0;
        return line.Counts[level];
    }

    // ------------------------------------------------------------------ painting

    protected void TrdSkPaint()
    {
        if (!m_TrdSk)
            return;
        string stockWord = TrdTr("#STR_OZT_VPP_SK_STOCK");
        string linesWord = TrdTr("#STR_OZT_VPP_SK_LINES_N");
        array<string> rows = new array<string>();
        for (int i = 0; i < m_TrdSk.Points.Count(); i++)
        {
            OZT_AdminStockPoint p = m_TrdSk.Points[i];
            int n = 0;
            for (int c = 0; c < p.Cats.Count(); c++)
                n = n + p.Cats[c].Lines.Count();
            string row = p.Name + "  (" + p.Id + ")  -  " + stockWord + " " + p.Stock;
            row = row + ",  " + linesWord + " " + n.ToString();
            rows.Insert(row);
        }
        TrdFill("TrdSkPoints", rows, null, m_TrdSkPoint);
        TrdSkPaintLines();
        TrdSkPaintForm();
        TrdSkPaintStart();
    }

    protected void TrdSkPaintLines()
    {
        if (!m_TrdSkRowCat)
        {
            m_TrdSkRowCat = new array<int>();
            m_TrdSkRowLine = new array<int>();
        }
        m_TrdSkRowCat.Clear();
        m_TrdSkRowLine.Clear();
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        int picked = -1;
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (p)
        {
            string w0 = TrdLevelWord(0);
            string w1 = TrdLevelWord(1);
            string w2 = TrdLevelWord(2);
            string w3 = TrdLevelWord(3);
            for (int c = 0; c < p.Cats.Count(); c++)
            {
                OZT_AdminStockCat cat = p.Cats[c];
                rows.Insert(cat.Name + "  (" + cat.Id + ")");
                inks.Insert(OZT_Metal.ACCENT);
                m_TrdSkRowCat.Insert(c);
                m_TrdSkRowLine.Insert(-1);
                for (int l = 0; l < cat.Lines.Count(); l++)
                {
                    OZT_AdminStockLine line = cat.Lines[l];
                    string row = "    " + line.ClassName + "   " + w0 + " " + TrdSkCount(line, 0).ToString();
                    row = row + "  " + w1 + " " + TrdSkCount(line, 1).ToString();
                    row = row + "  " + w2 + " " + TrdSkCount(line, 2).ToString();
                    row = row + "  " + w3 + " " + TrdSkCount(line, 3).ToString();
                    row = row + "   / " + line.Max.ToString();
                    if (c == m_TrdSkCat && l == m_TrdSkLine)
                        picked = rows.Count();
                    rows.Insert(row);
                    inks.Insert(0);
                    m_TrdSkRowCat.Insert(c);
                    m_TrdSkRowLine.Insert(l);
                }
            }
        }
        if (rows.Count() == 0)
        {
            rows.Insert(TrdTr("#STR_OZT_VPP_SK_NONE"));
            inks.Insert(OZT_Metal.MUTED);
            m_TrdSkRowCat.Insert(-1);
            m_TrdSkRowLine.Insert(-1);
        }
        TrdFill("TrdSkLines", rows, inks, picked);
    }

    protected void TrdSkPaintForm()
    {
        SayTrd("BtnTrdSkLevelText", TrdLevelWord(m_TrdSkLevel));
        OZT_AdminStockLine line = TrdSkLineAt();
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (!line || !p)
        {
            SayTrd("TrdSkHead", TrdTr("#STR_OZT_VPP_SK_PICK"));
            SayTrd("TrdSkLnSub", "");
            SayTrd("TrdSkMaxInfo", "");
            TrdSet("TrdSkCount", "");
            return;
        }
        OZT_AdminStockCat cat = p.Cats[m_TrdSkCat];
        SayTrd("TrdSkHead", line.ClassName);
        string inWord = TrdTr("#STR_OZT_VPP_IN");
        string stockWord = TrdTr("#STR_OZT_VPP_SK_STOCK");
        SayTrd("TrdSkLnSub", inWord + " " + cat.Name + ",  " + stockWord + " " + p.Stock);
        TrdSet("TrdSkCount", TrdSkCount(line, m_TrdSkLevel).ToString());
        string maxText;
        if (line.Max > 0)
        {
            string maxWord = TrdTr("#STR_OZT_VPP_SK_MAX");
            maxText = maxWord + " " + line.Max.ToString();
        }
        else
            maxText = TrdTr("#STR_OZT_VPP_SK_ONLY_SOLD");
        SayTrd("TrdSkMaxInfo", maxText);
    }

    // What a server start does to the stock, from the settings.
    protected void TrdSkPaintStart()
    {
        if (!m_TrdSk)
            return;
        string head = TrdTr("#STR_OZT_VPP_SK_START");
        string refill = TrdTr("#STR_OZT_VPP_MN_START_0");
        if (m_TrdSk.StartStock == 1)
            refill = TrdTr("#STR_OZT_VPP_MN_START_1");
        else if (m_TrdSk.StartStock == 2)
            refill = TrdTr("#STR_OZT_VPP_MN_START_2");
        string destock = TrdTr("#STR_OZT_VPP_MN_DESTOCK_NO");
        if (m_TrdSk.DestockAtStart == 1)
            destock = TrdTr("#STR_OZT_VPP_MN_DESTOCK_YES");
        string pristine = TrdTr("#STR_OZT_VPP_MN_PRISTINE_NO");
        if (m_TrdSk.OnlyPristine == 1)
            pristine = TrdTr("#STR_OZT_VPP_MN_PRISTINE_YES");
        string floorWord = TrdTr("#STR_OZT_VPP_MN_FLOOR");
        string whereText = TrdTr("#STR_OZT_VPP_SK_START_H");
        string text = head + "\n" + refill + "\n" + destock;
        text = text + "\n" + pristine + "\n" + floorWord + " " + TrdNum(m_TrdSk.Floor);
        text = text + "\n\n" + whereText;
        if (m_TrdSk.Cut == 1)
        {
            string cut = TrdTr("#STR_OZT_VPP_SK_CUT");
            text = text + "\n" + cut;
        }
        SayTrd("TrdSkStart", text);
    }

    // ------------------------------------------------------------------ the op

    protected void TrdSkSet()
    {
        OZT_AdminStockLine line = TrdSkLineAt();
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (!line || !p)
        {
            Hint(TrdTr("#STR_OZT_VPP_SK_PICK"));
            return;
        }
        int count = GetEdit("TrdSkCount").Trim().ToInt();
        if (count < 0)
            count = 0;
        OZT_AdminStockSet req = new OZT_AdminStockSet();
        req.Point = p.Id;
        req.Category = p.Cats[m_TrdSkCat].Id;
        req.ClassName = line.ClassName;
        req.Level = m_TrdSkLevel;
        req.Count = count;
        string body;
        string err;
        JsonFileLoader<OZT_AdminStockSet>.MakeData(req, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "stock_set", body);
    }

    protected void TrdSkStep(int delta)
    {
        int count = GetEdit("TrdSkCount").Trim().ToInt() + delta;
        if (count < 0)
            count = 0;
        TrdSet("TrdSkCount", count.ToString());
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && !m_Repaint && m_TrdSk)
        {
            string nm = w.GetName();
            if (nm == "TrdSkPoints")
            {
                if (row >= 0 && row < m_TrdSk.Points.Count())
                {
                    m_TrdSkPoint = row;
                    m_TrdSkCat = -1;
                    m_TrdSkLine = -1;
                    m_TrdSkKeepPoint = m_TrdSk.Points[row].Id;
                    TrdSkPaintLines();
                    TrdSkPaintForm();
                }
                return true;
            }
            if (nm == "TrdSkLines")
            {
                if (m_TrdSkRowLine && row >= 0 && row < m_TrdSkRowLine.Count() && m_TrdSkRowLine[row] >= 0)
                {
                    m_TrdSkCat = m_TrdSkRowCat[row];
                    m_TrdSkLine = m_TrdSkRowLine[row];
                    OZT_AdminStockPoint p = TrdSkPointAt();
                    OZT_AdminStockLine line = TrdSkLineAt();
                    if (p && line)
                    {
                        m_TrdSkKeepPoint = p.Id;
                        m_TrdSkKeepCat = p.Cats[m_TrdSkCat].Id;
                        m_TrdSkKeepClass = line.ClassName;
                    }
                    TrdSkPaintForm();
                }
                return true;
            }
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();
            if (nm == "BtnTrdSkRefresh")
            {
                Ask(OZT_Const.SECTION_ADMIN, "stock_list", "{}");
                return true;
            }
            if (nm == "BtnTrdSkLevel")
            {
                m_TrdSkLevel++;
                if (m_TrdSkLevel > 3)
                    m_TrdSkLevel = 0;
                TrdSkPaintForm();
                return true;
            }
            if (nm == "BtnTrdSkMinus")
            {
                TrdSkStep(-1);
                return true;
            }
            if (nm == "BtnTrdSkPlus")
            {
                TrdSkStep(1);
                return true;
            }
            if (nm == "BtnTrdSkSet")
            {
                TrdSkSet();
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
