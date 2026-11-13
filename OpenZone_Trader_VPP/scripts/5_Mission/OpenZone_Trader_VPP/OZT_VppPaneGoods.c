// Trader > Goods (design §5.6 and §8; board TraderGoods, layout ozt_vpp_goods.layout):
// the price book -- its categories and their lines -- edited in the draft of the goods
// file and applied by the module's draft bar or the head's own button (cfg_set
// trader_goods). A line's class, prices, quantity rule, stock block and the parts that
// come with it; a category's id and name. The table names every line as the game does,
// counts the lines the server's load will remark on, and says of the picked line what
// that load will say. The locks and discounts of both are the conditions page's.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The table holds no more rows than this; the search narrows it.
    static const int TRD_GD_ROWS_MAX = 400;

    // The category picked and the line picked in it (-1: none).
    protected int m_TrdGdCat;
    protected int m_TrdGdLine;

    // The heads as last written (their words are placed by their widths).
    protected string m_TrdGdCatTitle;
    protected string m_TrdGdLineTitle;

    // The rows of the categories and of the table (a row's key is its index in the
    // file), and the chips of the picked line's parts.
    protected ref OZ_VppFlexRows m_TrdGdCatRows;
    protected ref OZ_VppFlexRows m_TrdGdRows;
    protected ref OZT_VppChips   m_TrdGdPartChips;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        m_TrdGdCat = 0;
        m_TrdGdLine = -1;
        m_TrdGdCatTitle = "";
        m_TrdGdLineTitle = "";
    }

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "goods")
            return;
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdGdPaint();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "goods")
            TrdGdPaint();
    }

    // The window was resized: the rows follow their lists, and what the script placed
    // by a width (the heads' words, the parts' chips) is placed again.
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (m_TrdGdCatRows)
            m_TrdGdCatRows.Fit();
        if (m_TrdGdRows)
            m_TrdGdRows.Fit();
        if (TrdShownKey() != "goods")
            return;
        TrdGdFlowHeads();
        TrdGdFlowParts();
    }

    // ------------------------------------------------------------------ reading

    protected OZT_Category TrdGdCatAt(int i)
    {
        if (!m_TrdDraft || !m_TrdDraft.Goods || !m_TrdDraft.Goods.Categories)
            return null;
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        if (i < 0 || i >= cats.Count())
            return null;
        return cats[i];
    }

    protected OZT_Goods TrdGdLineAt()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat || !cat.Goods || m_TrdGdLine < 0 || m_TrdGdLine >= cat.Goods.Count())
            return null;
        return cat.Goods[m_TrdGdLine];
    }

    // The quantity rule in words, and in the table's short word.
    protected string TrdGdRuleWord(string rule)
    {
        if (rule == OZT_Const.RULE_UNIT)
            return TrdTr("#STR_OZT_VPP_GD_RULE_UNIT");
        if (rule == OZT_Const.RULE_FRACTION)
            return TrdTr("#STR_OZT_VPP_GD_RULE_FRACTION");
        return TrdTr("#STR_OZT_VPP_GD_RULE_EACH");
    }

    protected string TrdGdRuleShort(string rule)
    {
        if (rule == OZT_Const.RULE_UNIT)
            return TrdTr("#STR_OZT_VPP_GD_RULE_UNIT_S");
        if (rule == OZT_Const.RULE_FRACTION)
            return TrdTr("#STR_OZT_VPP_GD_RULE_FRACTION_S");
        return TrdTr("#STR_OZT_VPP_GD_RULE_EACH_S");
    }

    // The biggest discount a list can give, in percent.
    protected float TrdGdMostCut(array<ref OZT_Discount> discounts)
    {
        float most = 0;
        if (!discounts)
            return 0;
        for (int i = 0; i < discounts.Count(); i++)
        {
            float d = discounts[i].Most();
            if (d > most)
                most = d;
        }
        return most;
    }

    // A class of the game's configs, or a licence of the settings.
    protected bool TrdGdClassKnown(string cls)
    {
        if (cls == "")
            return false;
        string licence = OZT_Licences.IdOf(cls);
        if (licence != "")
        {
            if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Settings.Licences)
                return true;
            for (int i = 0; i < m_TrdDraft.Settings.Licences.Count(); i++)
            {
                if (m_TrdDraft.Settings.Licences[i].Id == licence)
                    return true;
            }
            return false;
        }
        return OZT_Config.ClassKnown(cls);
    }

    // What the players read for a class: a licence's name from the settings, a
    // thing's name as the game's config has it in the client's language; the class
    // itself when neither has one.
    protected string TrdGdNameOf(string cls)
    {
        if (cls == "")
            return "?";
        string licence = OZT_Licences.IdOf(cls);
        if (licence != "")
        {
            if (m_TrdDraft && m_TrdDraft.Settings && m_TrdDraft.Settings.Licences)
            {
                for (int i = 0; i < m_TrdDraft.Settings.Licences.Count(); i++)
                {
                    OZT_LicenceDef d = m_TrdDraft.Settings.Licences[i];
                    if (d.Id == licence && d.Name != "")
                        return d.Name;
                }
            }
            return cls;
        }
        string root = "";
        if (GetGame().ConfigIsExisting("CfgVehicles " + cls))
            root = "CfgVehicles";
        else if (GetGame().ConfigIsExisting("CfgWeapons " + cls))
            root = "CfgWeapons";
        else if (GetGame().ConfigIsExisting("CfgMagazines " + cls))
            root = "CfgMagazines";
        if (root == "")
            return cls;
        string shown;
        GetGame().ConfigGetText(root + " " + cls + " displayName", shown);
        // A name the game holds in one language only comes with a mark in front.
        if (shown.IndexOf("$UNT$") == 0)
            shown = shown.Substring(5, shown.Length() - 5);
        if (shown == "" || shown.IndexOf("STR_") == 0)
            return cls;
        return shown;
    }

    // What the server's load will say about a line, said before it does: the class,
    // the prices against the smallest note, a money loop within the line (a unit sold
    // and bought straight back, the line's biggest discount and its stock step
    // counted). `costs`: the least the line costs for the loop's side.
    protected void TrdGdCheck(OZT_Category cat, OZT_Goods e, out bool classOk, out bool unitOk, out bool loopOk, out bool loopAsked, out float costs)
    {
        classOk = TrdGdClassKnown(e.ClassName);
        int unit = TrdUnitOf(m_TrdDraft.Settings);
        int offBuy = e.Buy % unit;
        int offSell = e.Sell % unit;
        unitOk = true;
        if (offBuy != 0 || offSell != 0)
            unitOk = false;
        loopOk = true;
        loopAsked = false;
        costs = 0;
        if (e.Buy <= 0 || e.Sell <= 0)
            return;
        loopAsked = true;
        float most = 0;
        if (cat)
            most = TrdGdMostCut(cat.Discounts);
        float own = TrdGdMostCut(e.Discounts);
        if (own > most)
            most = own;
        float step = 1;
        if (e.Stock && e.Stock.Coefficient < 1)
            step = e.Stock.Coefficient;
        costs = e.Buy * step * (100 - most) / 100;
        if (e.Sell > costs)
            loopOk = false;
    }

    // The first thing a line's check finds, in the table's few words; "" when the
    // load will say nothing.
    protected string TrdGdRemark(OZT_Category cat, OZT_Goods e)
    {
        bool classOk;
        bool unitOk;
        bool loopOk;
        bool loopAsked;
        float costs;
        TrdGdCheck(cat, e, classOk, unitOk, loopOk, loopAsked, costs);
        if (!classOk)
            return TrdTr("#STR_OZT_VPP_GD_W_CLASS");
        if (!loopOk)
        {
            string loopWord = TrdTr("#STR_OZT_VPP_GD_W_LOOP");
            return loopWord + " " + TrdMoney(e.Sell) + " > " + TrdNum(costs);
        }
        if (!unitOk)
            return TrdTr("#STR_OZT_VPP_GD_W_UNIT");
        return "";
    }

    // ------------------------------------------------------------------ painting

    protected void TrdGdPaint()
    {
        if (!m_TrdDraft || !m_TrdDraft.Goods)
        {
            SayTrd("TrdGdSub", TrdTr("#STR_OZT_VPP_READING"));
            return;
        }
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        if (m_TrdGdCat >= cats.Count())
            m_TrdGdCat = cats.Count() - 1;
        if (m_TrdGdCat < 0)
            m_TrdGdCat = 0;
        TrdGdPaintCats();
        TrdGdPaintCat();
        TrdGdPaintLines();
        TrdGdPaintForm();
    }

    protected bool TrdGdMakeRows()
    {
        if (!m_TrdGdCatRows)
        {
            Widget catHost = TrdW("TrdGdCatsRows");
            if (catHost)
                m_TrdGdCatRows = new OZ_VppFlexRows(m_Flex, catHost, TRD_LAYOUTS + "goods_cat.layout", "OZT_GdCat", 230, 36);
        }
        if (!m_TrdGdRows)
        {
            Widget host = TrdW("TrdGdLinesRows");
            if (host)
                m_TrdGdRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "goods_row.layout", "OZT_GdRow", 779, 52);
        }
        if (!m_TrdGdCatRows || !m_TrdGdRows)
            return false;
        return true;
    }

    // The categories list and the page's counts, the fields left as they are.
    protected void TrdGdPaintCats()
    {
        if (!m_TrdDraft || !m_TrdDraft.Goods || !TrdGdMakeRows())
            return;
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        int lines = 0;
        string lockWord = TrdTr("#STR_OZT_VPP_GD_COND");
        m_TrdGdCatRows.Clear();
        for (int c = 0; c < cats.Count(); c++)
        {
            OZT_Category cat = cats[c];
            int n = 0;
            if (cat.Goods)
                n = cat.Goods.Count();
            lines = lines + n;
            Widget row = m_TrdGdCatRows.Add(c.ToString());
            if (!row)
                continue;
            string name = cat.Name;
            if (name == "")
                name = cat.Id;
            string lockText = "";
            if (cat.Requires && cat.Requires.Count() > 0)
                lockText = lockWord;
            RowText(row, "GdCatName", name, OZT_Metal.MUTED);
            RowText(row, "GdCatLock", lockText, OZT_Metal.MUTED);
            RowText(row, "GdCatCount", n.ToString(), OZT_Metal.MUTED);
        }
        m_TrdGdCatRows.Fit();
        TrdGdMarkCats();

        string countWord = TrdTr("#STR_OZT_VPP_GD_COUNT");
        string unitWord = TrdTr("#STR_OZT_VPP_GD_UNIT");
        string sub = countWord + " " + lines.ToString() + " / " + cats.Count().ToString();
        sub = sub + " · " + unitWord + " " + TrdUnitOf(m_TrdDraft.Settings).ToString();
        SayTrd("TrdGdSub", sub);
    }

    // The picked category's row is lit: the accent's soft ground and bar, bright name.
    protected void TrdGdMarkCats()
    {
        if (!m_TrdGdCatRows)
            return;
        Widget row;
        Widget part;
        TextWidget name;
        bool lit;
        for (int r = 0; r < m_TrdGdCatRows.Count(); r++)
        {
            row = m_TrdGdCatRows.Row(r);
            if (!row)
                continue;
            lit = m_TrdGdCatRows.Key(r).ToInt() == m_TrdGdCat;
            part = row.FindAnyWidget("OZT_GdCatBg");
            if (part)
                part.Show(lit);
            part = row.FindAnyWidget("OZT_GdCatBar");
            if (part)
                part.Show(lit);
            name = TextWidget.Cast(row.FindAnyWidget("GdCatName"));
            if (name)
            {
                if (lit)
                    name.SetColor(OZT_Metal.TEXT);
                else
                    name.SetColor(OZT_Metal.MUTED);
            }
        }
    }

    // The picked category's own fields and the table's head: its name, its id, how
    // many conditions lock it, the biggest discount it gives.
    protected void TrdGdPaintCat()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat)
        {
            TrdSet("TrdGdCatId", "");
            TrdSet("TrdGdCatName", "");
            m_TrdGdCatTitle = "";
            SayTrd("TrdGdCatHead", "");
            SayTrd("TrdGdCatSub", "");
            SayTrd("TrdGdWarnN", "");
            return;
        }
        TrdSet("TrdGdCatId", cat.Id);
        TrdSet("TrdGdCatName", cat.Name);
        TrdGdPaintCatHead();
    }

    protected void TrdGdPaintCatHead()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat)
            return;
        m_TrdGdCatTitle = cat.Name;
        if (m_TrdGdCatTitle == "")
            m_TrdGdCatTitle = cat.Id;
        SayTrd("TrdGdCatHead", m_TrdGdCatTitle);
        int conds = 0;
        if (cat.Requires)
            conds = cat.Requires.Count();
        string condsWord = TrdTr("#STR_OZT_VPP_GD_CONDS");
        string sub = "id " + cat.Id + " · " + condsWord + " " + conds.ToString();
        float cut = TrdGdMostCut(cat.Discounts);
        if (cut > 0)
        {
            string cutWord = TrdTr("#STR_OZT_VPP_GD_CUT_UPTO");
            sub = sub + " · " + cutWord + " " + TrdNum(cut) + " %";
        }
        SayTrd("TrdGdCatSub", sub);
        TrdGdFlowHeads();
    }

    // How many lines of the picked category the load will remark on.
    protected void TrdGdPaintWarnN()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        int warned = 0;
        if (cat && cat.Goods)
        {
            for (int i = 0; i < cat.Goods.Count(); i++)
            {
                if (TrdGdRemark(cat, cat.Goods[i]) != "")
                    warned++;
            }
        }
        if (warned == 0)
        {
            SayTrd("TrdGdWarnN", "");
            return;
        }
        string warnWord = TrdTr("#STR_OZT_VPP_GD_WARN_N");
        SayTrd("TrdGdWarnN", warnWord + " " + warned.ToString());
    }

    // The table: the picked category's lines the search keeps (by class or by the
    // name the players read), a row each.
    protected void TrdGdPaintLines()
    {
        if (!TrdGdMakeRows())
            return;
        m_TrdGdRows.Clear();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        string find = TrdLow(GetEdit("TrdGdFind").Trim());
        int hidden = 0;
        if (cat && cat.Goods)
        {
            for (int i = 0; i < cat.Goods.Count(); i++)
            {
                OZT_Goods e = cat.Goods[i];
                if (find != "" && i != m_TrdGdLine)
                {
                    bool hit = TrdLow(e.ClassName).IndexOf(find) != -1;
                    if (!hit)
                        hit = TrdLow(TrdGdNameOf(e.ClassName)).IndexOf(find) != -1;
                    if (!hit)
                        continue;
                }
                if (m_TrdGdRows.Count() >= TRD_GD_ROWS_MAX && i != m_TrdGdLine)
                {
                    hidden++;
                    continue;
                }
                Widget row = m_TrdGdRows.Add(i.ToString());
                if (row)
                    TrdGdFillRow(row, cat, e);
            }
        }
        m_TrdGdRows.Fit();
        TrdGdMarkRows();
        TrdGdPaintWarnN();
        if (hidden > 0)
        {
            string cut = TrdTr("#STR_OZT_VPP_LIST_CUT");
            int rowsMax = TRD_GD_ROWS_MAX;
            int rowsAll = cat.Goods.Count();
            Hint(cut + " " + rowsMax.ToString() + " / " + rowsAll.ToString());
        }
    }

    // One line of the book in a row: its name and class (with what comes with it, and
    // what the load will remark on), the prices, the rule, the stock block, its own
    // conditions and discount.
    protected void TrdGdFillRow(Widget row, OZT_Category cat, OZT_Goods e)
    {
        string cls = e.ClassName;
        if (cls == "")
            cls = "?";
        RowText(row, "GdRowName", TrdGdNameOf(e.ClassName), OZT_Metal.TEXT);

        // The remark first: the line is cut where the prices begin.
        string second = cls;
        if (e.With && e.With.Count() > 0)
            second = second + " + " + TrdJoin(e.With, " + ");
        int secondInk = OZT_Metal.MUTED;
        string remark = TrdGdRemark(cat, e);
        if (remark != "")
        {
            second = remark + " · " + second;
            secondInk = OZT_Metal.ALERT;
        }
        RowText(row, "GdRowCls", second, secondInk);

        string buy = "—";
        if (e.Buy > 0)
            buy = TrdMoney(e.Buy);
        string sell = "—";
        if (e.Sell > 0)
            sell = TrdMoney(e.Sell);
        RowText(row, "GdRowBuy", buy, OZT_Metal.ACCENT);
        RowText(row, "GdRowSell", sell, OZT_Metal.TEXT);
        RowText(row, "GdRowRule", TrdGdRuleShort(e.Quantity), OZT_Metal.MUTED);

        string stock = TrdTr("#STR_OZT_VPP_GD_UNLIMITED");
        if (e.Stock)
        {
            string floorText = "—";
            if (e.Stock.Floor > 0)
                floorText = TrdNum(e.Stock.Floor);
            stock = e.Stock.Max.ToString() + " · " + TrdNum(e.Stock.Coefficient);
            stock = stock + " · " + TrdNum(e.Stock.Destock) + " · " + floorText;
        }
        RowText(row, "GdRowStock", stock, OZT_Metal.TEXT);

        string req = "";
        if (e.Requires && e.Requires.Count() > 0)
        {
            string condsWord = TrdTr("#STR_OZT_VPP_GD_CONDS");
            req = condsWord + " " + e.Requires.Count().ToString();
        }
        float cut = TrdGdMostCut(e.Discounts);
        if (cut > 0)
        {
            if (req != "")
                req = req + " · ";
            req = req + "−" + TrdNum(cut) + " %";
        }
        if (req == "")
            req = "—";
        RowText(row, "GdRowReq", req, OZT_Metal.MUTED);
    }

    // The picked line's row lies on the accent's soft ground.
    protected void TrdGdMarkRows()
    {
        if (!m_TrdGdRows)
            return;
        Widget row;
        Widget bg;
        bool lit;
        for (int r = 0; r < m_TrdGdRows.Count(); r++)
        {
            row = m_TrdGdRows.Row(r);
            if (!row)
                continue;
            lit = m_TrdGdRows.Key(r).ToInt() == m_TrdGdLine;
            bg = row.FindAnyWidget("OZT_GdRowBg");
            if (bg)
                bg.Show(lit);
        }
    }

    // The picked line's row again, after an edit, without making the table again
    // under the cursor.
    protected void TrdGdPaintRow()
    {
        OZT_Goods e = TrdGdLineAt();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!e || !cat || !m_TrdGdRows)
            return;
        Widget row = m_TrdGdRows.Row(m_TrdGdRows.Find(m_TrdGdLine.ToString()));
        if (row)
            TrdGdFillRow(row, cat, e);
        TrdGdPaintWarnN();
    }

    // The line's form: its head, its fields, its parts, its check.
    protected void TrdGdPaintForm()
    {
        OZT_Goods e = TrdGdLineAt();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        bool picked = false;
        if (e)
            picked = true;
        TrdShow("TrdGdLnBox", picked);
        if (!picked)
        {
            m_TrdGdLineTitle = TrdTr("#STR_OZT_VPP_GD_PICK");
            TrdInk("TrdGdLnHead", m_TrdGdLineTitle, OZT_Metal.MUTED);
            SayTrd("TrdGdLnSub", "");
            TrdGdFlowHeads();
            return;
        }
        TrdGdPaintLineHead();
        TrdSet("TrdGdCls", e.ClassName);
        TrdSet("TrdGdBuy", e.Buy.ToString());
        TrdSet("TrdGdSell", e.Sell.ToString());
        TrdSet("TrdGdWithNew", "");
        TrdGdPaintStock();
        TrdGdPaintParts();
        TrdGdPaintCheck();
    }

    protected void TrdGdPaintLineHead()
    {
        OZT_Goods e = TrdGdLineAt();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!e)
            return;
        m_TrdGdLineTitle = TrdGdNameOf(e.ClassName);
        TrdInk("TrdGdLnHead", m_TrdGdLineTitle, OZT_Metal.TEXT);
        string sub = "";
        if (cat)
        {
            string inWord = TrdTr("#STR_OZT_VPP_GD_LINE_IN");
            sub = inWord + " «" + cat.Name + "»";
        }
        SayTrd("TrdGdLnSub", sub);
        TrdGdFlowHeads();
    }

    // The rule's select, the stock's switch and its four fields, and the prices the
    // stock's curve gives at two stocks.
    protected void TrdGdPaintStock()
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e)
            return;
        SayTrd("BtnTrdGdRuleText", TrdGdRuleWord(e.Quantity));
        bool stocked = false;
        if (e.Stock)
            stocked = true;
        TrdShow("BtnTrdGdStkMark", stocked);
        if (stocked)
        {
            TrdSet("TrdGdMax", e.Stock.Max.ToString());
            TrdSet("TrdGdCoef", TrdNum(e.Stock.Coefficient));
            TrdSet("TrdGdDest", TrdNum(e.Stock.Destock));
            TrdSet("TrdGdFloor", TrdNum(e.Stock.Floor));
        }
        else
        {
            TrdSet("TrdGdMax", "");
            TrdSet("TrdGdCoef", "");
            TrdSet("TrdGdDest", "");
            TrdSet("TrdGdFloor", "");
        }
        TrdGdPaintCurve();
    }

    // "stock 10: buy 1 580, sell 785" and "stock 50: buy 1 000, sell 500", a line
    // each: what one costs and brings while the line holds ten and fifty, by the
    // server's own curve and rounding, at a trader with no factor.
    protected void TrdGdPaintCurve()
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e || !e.Stock)
        {
            SayTrd("TrdGdCurve0", "");
            SayTrd("TrdGdCurve1", "");
            return;
        }
        float floorShare = e.Stock.Floor;
        if (floorShare <= 0 && m_TrdDraft.Settings)
            floorShare = m_TrdDraft.Settings.StockFloor;
        float coef = e.Stock.Coefficient;
        if (coef <= 0)
            coef = 1;
        int unit = TrdUnitOf(m_TrdDraft.Settings);
        SayTrd("TrdGdCurve0", TrdGdCurveAt(e, 10, coef, floorShare, unit));
        SayTrd("TrdGdCurve1", TrdGdCurveAt(e, 50, coef, floorShare, unit));
    }

    protected string TrdGdCurveAt(OZT_Goods e, int stock, float coef, float floorShare, int unit)
    {
        float share = OZT_Book.Curve(coef, stock - 1, floorShare);
        int buy = OZT_Config.RoundUpTo(e.Buy * share, unit);
        int sell = OZT_Config.RoundDownTo(e.Sell * share, unit);
        string word = TrdTr("#STR_OZT_VPP_GD_AT_STOCK");
        string buyWord = TrdTr("#STR_OZT_VPP_SK_BUY_W");
        string sellWord = TrdTr("#STR_OZT_VPP_SK_SELL_W");
        string text = word + " " + stock.ToString() + ": ";
        text = text + buyWord + " " + TrdMoney(buy);
        text = text + ", " + sellWord + " " + TrdMoney(sell);
        return text;
    }

    // ---- what comes with the line: a chip a part

    protected bool TrdGdMakeParts()
    {
        if (m_TrdGdPartChips)
            return true;
        Widget host = TrdW("TrdGdWithBody");
        if (!host)
            return false;
        m_TrdGdPartChips = new OZT_VppChips(m_Flex, host, TRD_LAYOUTS + "goods_part.layout", "OZT_GdPart");
        return true;
    }

    protected void TrdGdPaintParts()
    {
        if (!TrdGdMakeParts())
            return;
        m_TrdGdPartChips.Clear();
        OZT_Goods e = TrdGdLineAt();
        float s = UnitScale();
        int parts = 0;
        if (e && e.With)
        {
            for (int i = 0; i < e.With.Count(); i++)
            {
                Widget chip = m_TrdGdPartChips.Add(i.ToString());
                if (!chip)
                    continue;
                // The template is 127 x 26: 8 before the name, 35 around it, the
                // cross 23 from the right edge.
                int ink = OZT_Metal.TEXT;
                if (!OZT_Config.ClassKnown(e.With[i]))
                    ink = OZT_Metal.ALERT;
                RowText(chip, "GdPartName", e.With[i], ink);
                TextWidget nameW = TextWidget.Cast(chip.FindAnyWidget("GdPartName"));
                float tw = TrdTextW(nameW, e.With[i], 12);
                float w = tw + 35 * s;
                chip.SetSize(w, 26 * s);
                Widget bg = chip.FindAnyWidget("OZT_GdPartBg");
                if (bg)
                    bg.SetSize(w, 26 * s);
                if (nameW)
                    nameW.SetSize(tw + 2, 14 * s);
                Widget del = chip.FindAnyWidget("GdPartDel");
                if (del)
                    del.SetPos(w - 23 * s, 3 * s);
                m_TrdGdPartChips.SetWidth(m_TrdGdPartChips.Count() - 1, w);
                parts++;
            }
        }
        string none = "";
        if (parts == 0)
            none = TrdTr("#STR_OZT_VPP_GD_WITH_NONE");
        TrdInk("TrdGdWithNone", none, OZT_Metal.FAINT);
        TrdShow("TrdGdWithNone", parts == 0);
        TrdGdFlowParts();
    }

    protected void TrdGdFlowParts()
    {
        if (!m_TrdGdPartChips)
            return;
        Widget body = TrdW("TrdGdWithBody");
        Widget scroll = TrdW("TrdGdWithScroll");
        Widget none = TrdW("TrdGdWithNone");
        if (!body || !scroll)
            return;
        float s = UnitScale();
        float room;
        float view;
        scroll.GetSize(room, view);
        if (room <= 0)
            room = 297 * s;
        float need = m_TrdGdPartChips.Flow(room - 10 * s, 6 * s, 26 * s, null, 0);
        if (m_TrdGdPartChips.Count() == 0)
            need = 0;
        if (none)
            none.SetPos(0, 5 * s);
        if (need < view)
            need = view;
        body.SetSize(room, need);
        scroll.Update();
    }

    // The line's check, a line a finding: the class, the prices against the smallest
    // note, a money loop within the line.
    protected void TrdGdPaintCheck()
    {
        OZT_Goods e = TrdGdLineAt();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!e || !cat)
            return;
        bool classOk;
        bool unitOk;
        bool loopOk;
        bool loopAsked;
        float costs;
        TrdGdCheck(cat, e, classOk, unitOk, loopOk, loopAsked, costs);

        string text = TrdTr("#STR_OZT_VPP_GD_CHK_CLASS");
        if (!classOk)
            text = TrdTr("#STR_OZT_VPP_GD_BAD_CLASS");
        TrdGdCheckLine(0, classOk, text);

        string unit = TrdUnitOf(m_TrdDraft.Settings).ToString();
        text = TrdTr("#STR_OZT_VPP_GD_OK_UNIT");
        if (!unitOk)
            text = TrdTr("#STR_OZT_VPP_GD_BAD_UNIT");
        TrdGdCheckLine(1, unitOk, text + " " + unit);

        TrdShow("TrdGdChk2", loopAsked);
        TrdShow("TrdGdChk2Icon", loopAsked);
        if (!loopAsked)
            return;
        if (loopOk)
        {
            text = TrdTr("#STR_OZT_VPP_GD_OK_LOOP");
            text = text + " " + TrdMoney(e.Sell) + " ≤ " + TrdNum(costs);
        }
        else
        {
            text = TrdTr("#STR_OZT_VPP_GD_BAD_LOOP");
            text = text + " " + TrdMoney(e.Sell) + " > " + TrdNum(costs);
        }
        TrdGdCheckLine(2, loopOk, text);
    }

    protected void TrdGdCheckLine(int n, bool good, string text)
    {
        string name = "TrdGdChk" + n.ToString();
        if (good)
        {
            TrdInk(name, text, OZT_Metal.TEXT);
            TrdSign(TrdW(name + "Icon"), "check", OZT_Metal.OK);
            return;
        }
        TrdInk(name, text, OZT_Metal.ALERT);
        TrdSign(TrdW(name + "Icon"), "cross", OZT_Metal.ALERT);
    }

    // The heads' words stand one after another: the category's name and its facts
    // over the table, the line's name and its category over the form.
    protected void TrdGdFlowHeads()
    {
        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        float tw;
        TextWidget head = TextWidget.Cast(TrdW("TrdGdCatHead"));
        Widget sub = TrdW("TrdGdCatSub");
        Widget warn = TrdW("TrdGdWarnN");
        if (head && sub && warn)
        {
            // The facts live in a box that cuts them where the remarks begin.
            Widget box = sub.GetParent();
            float wx;
            float wy;
            tw = TrdTextW(head, m_TrdGdCatTitle, 18);
            head.GetPos(x, y);
            head.GetSize(w, h);
            head.SetSize(tw + 2, h);
            float at = x + tw + 10 * s;
            warn.GetPos(wx, wy);
            float room = wx - at - 10 * s;
            if (room < 0)
                room = 0;
            if (box)
            {
                box.GetPos(x, y);
                box.GetSize(w, h);
                box.SetPos(at, y);
                box.SetSize(room, h);
                sub.SetSize(room, h);
            }
        }

        head = TextWidget.Cast(TrdW("TrdGdLnHead"));
        sub = TrdW("TrdGdLnSub");
        if (head && sub)
        {
            tw = TrdTextW(head, m_TrdGdLineTitle, 18);
            // The form is 297 units wide inside its paddings.
            float most = 297 * s;
            if (tw > most)
                tw = most;
            head.GetPos(x, y);
            head.GetSize(w, h);
            head.SetSize(tw + 2, h);
            float lx = x + tw + 8 * s;
            sub.GetPos(x, y);
            sub.GetSize(w, h);
            sub.SetPos(lx, y);
            float left = 16 * s + most - lx;
            if (left < 0)
                left = 0;
            sub.SetSize(left, h);
        }
    }

    // ------------------------------------------------------------------ edits

    protected bool TrdGdIsField(string nm)
    {
        array<string> fields = {"TrdGdCls", "TrdGdBuy", "TrdGdSell", "TrdGdMax", "TrdGdCoef", "TrdGdDest", "TrdGdFloor"};
        return fields.Find(nm) != -1;
    }

    // A field of the line typed into: the draft's line follows it. A value equal to
    // the draft's is no edit (an OnChange the script's own fill may raise).
    protected void TrdGdEdit(string nm)
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e)
            return;
        string v = GetEdit(nm).Trim();
        int n = v.ToInt();
        float f = v.ToFloat();
        bool changed = false;
        if (nm == "TrdGdCls")
        {
            if (e.ClassName != v)
            {
                e.ClassName = v;
                changed = true;
                TrdGdPaintLineHead();
            }
        }
        else if (nm == "TrdGdBuy")
        {
            if (e.Buy != n)
            {
                e.Buy = n;
                changed = true;
            }
        }
        else if (nm == "TrdGdSell")
        {
            if (e.Sell != n)
            {
                e.Sell = n;
                changed = true;
            }
        }
        else if (e.Stock)
        {
            if (nm == "TrdGdMax" && e.Stock.Max != n)
            {
                e.Stock.Max = n;
                changed = true;
            }
            else if (nm == "TrdGdCoef" && e.Stock.Coefficient != f)
            {
                e.Stock.Coefficient = f;
                changed = true;
            }
            else if (nm == "TrdGdDest" && e.Stock.Destock != f)
            {
                e.Stock.Destock = f;
                changed = true;
            }
            else if (nm == "TrdGdFloor" && e.Stock.Floor != f)
            {
                e.Stock.Floor = f;
                changed = true;
            }
        }
        if (!changed)
            return;
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaintRow();
        TrdGdPaintCurve();
        TrdGdPaintCheck();
    }

    // A category's id or name typed in. The points that deal in the category follow
    // its id.
    protected void TrdGdCatEdit(string nm)
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat)
            return;
        string v = GetEdit(nm).Trim();
        if (nm == "TrdGdCatName")
        {
            if (cat.Name == v)
                return;
            cat.Name = v;
            TrdTouch(OZT_Const.CFG_GOODS);
            TrdGdPaintCats();
            TrdGdPaintCatHead();
            TrdGdPaintLineHead();
            return;
        }
        if (cat.Id == v)
            return;
        string was = cat.Id;
        cat.Id = v;
        TrdTouch(OZT_Const.CFG_GOODS);
        if (m_TrdDraft.Traders && was != "" && was != v)
        {
            bool moved = false;
            array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
            for (int i = 0; i < points.Count(); i++)
            {
                if (!points[i].Categories)
                    continue;
                int at = points[i].Categories.Find(was);
                if (at < 0)
                    continue;
                points[i].Categories.Set(at, v);
                moved = true;
            }
            if (moved)
                TrdTouch(OZT_Const.CFG_TRADERS);
        }
        if (!OZT_Ids.IsSafe(v))
            Hint(TrdTr("#STR_OZT_VPP_GD_BAD_ID"));
        TrdGdPaintCats();
        TrdGdPaintCatHead();
    }

    protected void TrdGdCatAdd()
    {
        if (!m_TrdDraft || !m_TrdDraft.Goods)
            return;
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        int n = cats.Count() + 1;
        string id = "category_" + n.ToString();
        while (TrdGdCatIndex(id) >= 0)
        {
            n++;
            id = "category_" + n.ToString();
        }
        OZT_Category cat = new OZT_Category();
        cat.Id = id;
        cat.Name = id;
        cat.Goods = new array<ref OZT_Goods>();
        cat.Requires = new array<ref OZT_Condition>();
        cat.Discounts = new array<ref OZT_Discount>();
        cats.Insert(cat);
        m_TrdGdCat = cats.Count() - 1;
        m_TrdGdLine = -1;
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaint();
    }

    protected int TrdGdCatIndex(string id)
    {
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        for (int i = 0; i < cats.Count(); i++)
        {
            if (cats[i].Id == id)
                return i;
        }
        return -1;
    }

    // A category leaves the draft with its lines; the points that name it are
    // said by the server's load after the apply.
    protected void TrdGdCatDel()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat)
            return;
        m_TrdDraft.Goods.Categories.RemoveOrdered(m_TrdGdCat);
        m_TrdGdCat = 0;
        m_TrdGdLine = -1;
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaint();
    }

    protected void TrdGdLineAdd()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat)
            return;
        if (!cat.Goods)
            cat.Goods = new array<ref OZT_Goods>();
        OZT_Goods e = new OZT_Goods();
        e.ClassName = "";
        e.With = new array<string>();
        e.Requires = new array<ref OZT_Condition>();
        e.Discounts = new array<ref OZT_Discount>();
        cat.Goods.Insert(e);
        m_TrdGdLine = cat.Goods.Count() - 1;
        TrdSet("TrdGdFind", "");
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaint();
        Hint(TrdTr("#STR_OZT_VPP_GD_TYPE_CLASS"));
    }

    protected void TrdGdLineDel()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        OZT_Goods e = TrdGdLineAt();
        if (!cat || !e)
            return;
        cat.Goods.RemoveOrdered(m_TrdGdLine);
        m_TrdGdLine = -1;
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaint();
    }

    protected void TrdGdRuleNext()
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e)
            return;
        if (e.Quantity == OZT_Const.RULE_EACH)
            e.Quantity = OZT_Const.RULE_UNIT;
        else if (e.Quantity == OZT_Const.RULE_UNIT)
            e.Quantity = OZT_Const.RULE_FRACTION;
        else
            e.Quantity = OZT_Const.RULE_EACH;
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaintStock();
        TrdGdPaintRow();
    }

    // Finite stock on (a block that says something: Max 0 with its Coefficient is
    // "only what players sell") or off (unlimited).
    protected void TrdGdStockToggle()
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e)
            return;
        if (e.Stock)
            e.Stock = null;
        else
        {
            OZT_GoodsStock st = new OZT_GoodsStock();
            st.Max = 0;
            st.Coefficient = 1;
            st.Destock = 0;
            st.Floor = 0;
            e.Stock = st;
        }
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaintStock();
        TrdGdPaintRow();
        TrdGdPaintCheck();
    }

    // The class typed beside the parts joins what comes with the line.
    protected void TrdGdPartAdd()
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e)
            return;
        string part = GetEdit("TrdGdWithNew").Trim();
        if (part == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_GD_PART_TYPE"));
            return;
        }
        if (!e.With)
            e.With = new array<string>();
        if (part == e.ClassName || e.With.Find(part) != -1)
        {
            Hint(TrdTr("#STR_OZT_VPP_GD_PART_HAS"));
            return;
        }
        e.With.Insert(part);
        TrdSet("TrdGdWithNew", "");
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaintParts();
        TrdGdPaintRow();
    }

    protected void TrdGdPartDel(int index)
    {
        OZT_Goods e = TrdGdLineAt();
        if (!e || !e.With || index < 0 || index >= e.With.Count())
            return;
        e.With.RemoveOrdered(index);
        TrdTouch(OZT_Const.CFG_GOODS);
        TrdGdPaintParts();
        TrdGdPaintRow();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill)
        {
            string nm = w.GetName();
            if (TrdGdIsField(nm))
            {
                TrdGdEdit(nm);
                return true;
            }
            if (nm == "TrdGdCatId" || nm == "TrdGdCatName")
            {
                TrdGdCatEdit(nm);
                return true;
            }
            if (nm == "TrdGdFind")
            {
                TrdGdPaintLines();
                return true;
            }
        }
        return super.OnChange(w, x, y, finished);
    }

    // A click inside one of the page's lists; false: not there.
    protected bool TrdGdListClick(Widget w)
    {
        int i;
        if (m_TrdGdCatRows)
        {
            i = m_TrdGdCatRows.IndexOf(w);
            if (i != -1)
            {
                m_TrdGdCat = m_TrdGdCatRows.Key(i).ToInt();
                m_TrdGdLine = -1;
                TrdGdMarkCats();
                TrdGdPaintCat();
                TrdGdPaintLines();
                TrdGdPaintForm();
                return true;
            }
        }
        if (m_TrdGdRows)
        {
            i = m_TrdGdRows.IndexOf(w);
            if (i != -1)
            {
                m_TrdGdLine = m_TrdGdRows.Key(i).ToInt();
                TrdGdMarkRows();
                TrdGdPaintForm();
                return true;
            }
        }
        if (m_TrdGdPartChips)
        {
            i = m_TrdGdPartChips.IndexOf(w);
            if (i != -1)
            {
                if (w.GetName() == "GdPartDel")
                    TrdGdPartDel(m_TrdGdPartChips.Key(i).ToInt());
                return true;
            }
        }
        return false;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            if (TrdGdListClick(w))
                return true;
            string nm = w.GetName();
            if (nm == "BtnTrdGdRule")
            {
                TrdGdRuleNext();
                return true;
            }
            if (nm == "BtnTrdGdStk")
            {
                TrdGdStockToggle();
                return true;
            }
            if (nm == "BtnTrdGdLnAdd")
            {
                TrdGdLineAdd();
                return true;
            }
            if (nm == "BtnTrdGdLnDel")
            {
                TrdGdLineDel();
                return true;
            }
            if (nm == "BtnTrdGdCatAdd")
            {
                TrdGdCatAdd();
                return true;
            }
            if (nm == "BtnTrdGdCatDel")
            {
                TrdGdCatDel();
                return true;
            }
            if (nm == "BtnTrdGdWithAdd")
            {
                TrdGdPartAdd();
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
