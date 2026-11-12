// Trader > Goods (design §5.6 and §8; mockup TraderGoods): the price book -- its
// categories and their lines -- edited in the draft of the goods file and applied
// by the draft's bar (cfg_set trader_goods). A line's price, quantity rule, stock
// block and the parts that come with it; a category's id and name. The locks and
// discounts of both are the conditions page's.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The category picked, the line picked in it (-1: none), and which line each
    // row of the filtered list shows.
    protected int m_TrdGdCat = 0;
    protected int m_TrdGdLine = -1;
    protected ref array<int> m_TrdGdRows;

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

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdGd", w);
        float bottom = h - (16 + 56) * s;
        float catsH = bottom - (96 + 132) * s;
        if (catsH < 60 * s)
            catsH = 60 * s;
        TrdSize("TrdGdCats", 230 * s, catsH);
        TrdMove("TrdGdCatForm", 24 * s, bottom - 126 * s);
        float midW = w - (270 + 16 + 330 + 24) * s;
        if (midW < 200 * s)
            midW = 200 * s;
        float linesH = bottom - 130 * s;
        if (linesH < 60 * s)
            linesH = 60 * s;
        TrdSize("TrdGdLines", midW, linesH);
        TrdPlace("TrdGdForm", (270 + 16) * s + midW, 76 * s, 330 * s, bottom - 76 * s);
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

    // The smallest note: every price is a multiple of it with item money.
    protected int TrdGdUnit()
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Settings.Notes)
            return 1;
        array<ref OZT_Note> notes = m_TrdDraft.Settings.Notes;
        int unit = 0;
        for (int i = 0; i < notes.Count(); i++)
        {
            if (notes[i].Value > 0 && (unit == 0 || notes[i].Value < unit))
                unit = notes[i].Value;
        }
        if (unit <= 0)
            return 1;
        return unit;
    }

    // The quantity rule in words.
    protected string TrdGdRuleWord(string rule)
    {
        if (rule == OZT_Const.RULE_UNIT)
            return TrdTr("#STR_OZT_VPP_GD_RULE_UNIT");
        if (rule == OZT_Const.RULE_FRACTION)
            return TrdTr("#STR_OZT_VPP_GD_RULE_FRACTION");
        return TrdTr("#STR_OZT_VPP_GD_RULE_EACH");
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

    // The categories list and the page's counts, the fields left as they are.
    protected void TrdGdPaintCats()
    {
        if (!m_TrdDraft || !m_TrdDraft.Goods)
            return;
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        int lines = 0;
        string lockWord = TrdTr("#STR_OZT_VPP_GD_LOCKED");
        array<string> rows = new array<string>();
        for (int c = 0; c < cats.Count(); c++)
        {
            OZT_Category cat = cats[c];
            int n = 0;
            if (cat.Goods)
                n = cat.Goods.Count();
            lines = lines + n;
            string row = cat.Name + "  (" + cat.Id + ")  " + n.ToString();
            if (cat.Requires && cat.Requires.Count() > 0)
                row = row + "  " + lockWord;
            float cut = TrdGdMostCut(cat.Discounts);
            if (cut > 0)
                row = row + "  -" + TrdNum(cut) + "%";
            rows.Insert(row);
        }
        TrdFill("TrdGdCats", rows, null, m_TrdGdCat);

        string countWord = TrdTr("#STR_OZT_VPP_GD_COUNT");
        string unitWord = TrdTr("#STR_OZT_VPP_GD_UNIT");
        string sub = countWord + " " + lines.ToString() + " / " + cats.Count().ToString();
        sub = sub + ".  " + unitWord + " " + TrdGdUnit().ToString();
        SayTrd("TrdGdSub", sub);
    }

    protected void TrdGdPaintCat()
    {
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!cat)
        {
            TrdSet("TrdGdCatId", "");
            TrdSet("TrdGdCatName", "");
            return;
        }
        TrdSet("TrdGdCatId", cat.Id);
        TrdSet("TrdGdCatName", cat.Name);
    }

    // One line of the book in a row: class, prices, rule, stock, lock, discount.
    protected string TrdGdRow(OZT_Goods e)
    {
        string cls = e.ClassName;
        if (cls == "")
            cls = "?";
        string row = cls + "   " + e.Buy.ToString() + " / " + e.Sell.ToString();
        string rule = TrdGdRuleWord(e.Quantity);
        row = row + "   " + rule;
        if (e.Stock)
        {
            string stock = e.Stock.Max.ToString() + " x" + TrdNum(e.Stock.Coefficient);
            row = row + "   [" + stock + "]";
        }
        if (e.Requires && e.Requires.Count() > 0)
        {
            string lockWord = TrdTr("#STR_OZT_VPP_GD_LOCKED");
            row = row + "  " + lockWord;
        }
        float cut = TrdGdMostCut(e.Discounts);
        if (cut > 0)
            row = row + "  -" + TrdNum(cut) + "%";
        return row;
    }

    protected void TrdGdPaintLines()
    {
        if (!m_TrdGdRows)
            m_TrdGdRows = new array<int>();
        m_TrdGdRows.Clear();
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        int picked = -1;
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        string find = TrdLow(GetEdit("TrdGdFind").Trim());
        if (cat && cat.Goods)
        {
            for (int i = 0; i < cat.Goods.Count(); i++)
            {
                OZT_Goods e = cat.Goods[i];
                if (find != "" && TrdLow(e.ClassName).IndexOf(find) < 0)
                    continue;
                if (i == m_TrdGdLine)
                    picked = rows.Count();
                m_TrdGdRows.Insert(i);
                rows.Insert(TrdGdRow(e));
                int ink = 0;
                if (!TrdGdClassKnown(e.ClassName))
                    ink = OZT_Metal.ALERT;
                inks.Insert(ink);
            }
        }
        TrdFill("TrdGdLines", rows, inks, picked);
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
        if (GetGame().ConfigIsExisting("CfgVehicles " + cls))
            return true;
        if (GetGame().ConfigIsExisting("CfgWeapons " + cls))
            return true;
        return GetGame().ConfigIsExisting("CfgMagazines " + cls);
    }

    protected void TrdGdPaintForm()
    {
        OZT_Goods e = TrdGdLineAt();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!e)
        {
            SayTrd("TrdGdLnHead", TrdTr("#STR_OZT_VPP_GD_PICK"));
            SayTrd("TrdGdLnSub", "");
            SayTrd("TrdGdCheck", "");
            TrdSet("TrdGdCls", "");
            TrdSet("TrdGdBuy", "");
            TrdSet("TrdGdSell", "");
            TrdSet("TrdGdMax", "");
            TrdSet("TrdGdCoef", "");
            TrdSet("TrdGdDest", "");
            TrdSet("TrdGdFloor", "");
            TrdSet("TrdGdWith", "");
            SayTrd("BtnTrdGdRuleText", "-");
            SayTrd("BtnTrdGdStkText", "-");
            return;
        }
        string head = e.ClassName;
        if (head == "")
            head = "?";
        SayTrd("TrdGdLnHead", head);
        string inWord = TrdTr("#STR_OZT_VPP_IN");
        if (cat)
            SayTrd("TrdGdLnSub", inWord + " " + cat.Name + " (" + cat.Id + ")");
        TrdSet("TrdGdCls", e.ClassName);
        TrdSet("TrdGdBuy", e.Buy.ToString());
        TrdSet("TrdGdSell", e.Sell.ToString());
        TrdSet("TrdGdWith", TrdJoin(e.With, ","));
        SayTrd("BtnTrdGdRuleText", TrdGdRuleWord(e.Quantity));
        string stockWord = TrdTr("#STR_OZT_VPP_GD_STOCK_OFF");
        if (e.Stock)
        {
            stockWord = TrdTr("#STR_OZT_VPP_GD_STOCK_ON");
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
        SayTrd("BtnTrdGdStkText", stockWord);
        TrdGdPaintCheck();
    }

    // What the server's load will say about the line, said before it does: the
    // class, the prices against the smallest note, a money loop within the line.
    protected void TrdGdPaintCheck()
    {
        OZT_Goods e = TrdGdLineAt();
        OZT_Category cat = TrdGdCatAt(m_TrdGdCat);
        if (!e || !cat)
            return;
        bool bad = false;
        string text = "";
        if (TrdGdClassKnown(e.ClassName))
            text = TrdTr("#STR_OZT_VPP_GD_CHK_CLASS");
        else
        {
            text = TrdTr("#STR_OZT_VPP_GD_CHK_NO_CLASS");
            bad = true;
        }
        int unit = TrdGdUnit();
        bool fits = e.Buy % unit == 0 && e.Sell % unit == 0;
        string prices;
        if (fits)
            prices = TrdTr("#STR_OZT_VPP_GD_CHK_UNIT");
        else
        {
            prices = TrdTr("#STR_OZT_VPP_GD_CHK_NO_UNIT");
            bad = true;
        }
        text = text + "\n" + prices + " " + unit.ToString();
        if (e.Buy > 0 && e.Sell > 0)
        {
            float most = TrdGdMostCut(cat.Discounts);
            float own = TrdGdMostCut(e.Discounts);
            if (own > most)
                most = own;
            float step = 1;
            if (e.Stock && e.Stock.Coefficient < 1)
                step = e.Stock.Coefficient;
            float costs = e.Buy * step * (100 - most) / 100;
            string loop;
            if (e.Sell > costs)
            {
                loop = TrdTr("#STR_OZT_VPP_GD_CHK_LOOP");
                bad = true;
            }
            else
                loop = TrdTr("#STR_OZT_VPP_GD_CHK_NO_LOOP");
            text = text + "\n" + loop + " " + e.Sell.ToString() + " / " + TrdNum(costs);
        }
        int ink = OZT_Metal.MUTED;
        if (bad)
            ink = OZT_Metal.ALERT;
        TrdInk("TrdGdCheck", text, ink);
    }

    // ------------------------------------------------------------------ edits

    protected bool TrdGdIsField(string nm)
    {
        array<string> fields = {"TrdGdCls", "TrdGdBuy", "TrdGdSell", "TrdGdMax", "TrdGdCoef", "TrdGdDest", "TrdGdFloor", "TrdGdWith"};
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
                SayTrd("TrdGdLnHead", v);
                changed = true;
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
        else if (nm == "TrdGdWith")
        {
            if (!e.With)
                e.With = new array<string>();
            array<string> parts = new array<string>();
            TrdSplit(v, parts);
            if (TrdJoin(parts, ",") != TrdJoin(e.With, ","))
            {
                TrdSplit(v, e.With);
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
        TrdGdPaintCheck();
    }

    // The picked line's row again, without rebuilding the list under the cursor.
    protected void TrdGdPaintRow()
    {
        OZT_Goods e = TrdGdLineAt();
        TextListboxWidget lb = TextListboxWidget.Cast(TrdW("TrdGdLines"));
        if (!e || !lb || !m_TrdGdRows)
            return;
        int row = m_TrdGdRows.Find(m_TrdGdLine);
        if (row < 0)
            return;
        lb.SetItem(row, TrdGdRow(e), null, 0);
    }

    // A category's id typed in: the points that deal in it follow it.
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
        TrdGdPaintForm();
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
        TrdGdPaintForm();
        TrdGdPaintRow();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && !m_Repaint)
        {
            string nm = w.GetName();
            if (nm == "TrdGdCats")
            {
                if (row < 0)
                    return true;
                m_TrdGdCat = row;
                m_TrdGdLine = -1;
                TrdGdPaintCat();
                TrdGdPaintLines();
                TrdGdPaintForm();
                return true;
            }
            if (nm == "TrdGdLines")
            {
                if (m_TrdGdRows && row >= 0 && row < m_TrdGdRows.Count())
                {
                    m_TrdGdLine = m_TrdGdRows[row];
                    TrdGdPaintForm();
                }
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

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
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
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
