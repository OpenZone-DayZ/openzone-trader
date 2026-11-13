// Trader > Stock (design §5.6 and §11.1; board TraderStock, layout
// ozt_vpp_stock.layout): the stock of each point by condition (`stock_list`: per point
// and category, the stocked lines it deals by, with the count in each condition and the
// Max) and the count of one line set by hand (`stock_set`, the bridge verb `stock` as
// an admin op): a step with a row's own buttons, any number in the form. Points that
// name one stock share one shelf: setting a line at one sets it at all.
//
// The table also says what one of a line costs and brings now, and the side panel draws
// how both move with the stock. The numbers are made here, from the three files as the
// server last gave them (not the draft's edits) and the server's own curve and rounding
// (OZT_Book.Curve, OZT_Config.RoundUpTo / RoundDownTo), for a player with no discount:
// OZT_Book.BuyForAt and SellUnits for one unit.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

// What a stocked line's prices are made of at one point: the line of the goods file
// the point deals by, the point's factors, the settings' notes, floor and condition
// shares.
class OZT_VppStockNums
{
    int Buy;
    int Sell;
    string Rule;
    float Coef;
    float Floor;
    float BuyFactor;
    float SellFactor;
    int NoteUnit;
    int OnlyPristine;
    ref array<float> Shares;

    void OZT_VppStockNums()
    {
        Shares = new array<float>();
    }

    // The share of the goods' sell price a thing in a condition brings (the
    // settings' factors, pristine too).
    float SellShare(int level)
    {
        if (level < 0 || level >= Shares.Count())
            return 0;
        return Shares[level];
    }

    // The share of the goods' buy price a line of a condition asks: a pristine line
    // the price itself, a worn one the settings' factor of it (OZT_Book.LevelShare).
    float BuyShare(int level)
    {
        if (level <= 0)
            return 1;
        return SellShare(level);
    }

    // What one costs from the line of `level` while it holds `stock`: the curve at
    // the stock the unit leaves, each unit rounded up to a note. A line with nothing
    // in it: what its first would cost.
    int BuyAt(int level, int stock)
    {
        if (Buy <= 0)
            return 0;
        int left = stock - 1;
        if (left < 0)
            left = 0;
        float price = Buy * BuyFactor * BuyShare(level);
        price = price * OZT_Book.Curve(Coef, left, Floor);
        return OZT_Config.RoundUpTo(price, NoteUnit);
    }

    // What the trader pays for one more thing in the condition `level` while the
    // line it goes into holds `stock`: the curve at the stock the unit makes, rounded
    // down to a note.
    int SellAt(int level, int stock)
    {
        if (Sell <= 0)
            return 0;
        int made = stock;
        if (made < 1)
            made = 1;
        float price = Sell * SellFactor * SellShare(level);
        price = price * OZT_Book.Curve(Coef, made - 1, Floor);
        return OZT_Config.RoundDownTo(price, NoteUnit);
    }
}

modded class OZ_VppAdminMenu
{
    // The table holds no more rows than this; the search narrows it.
    static const int TRD_SK_ROWS_MAX = 300;

    // The section's last stock list, and the three files as the server last gave
    // them (the prices are made of those).
    protected ref OZT_AdminStockList m_TrdSk;
    protected ref OZT_Settings    m_TrdSkSettings;
    protected ref OZT_GoodsFile   m_TrdSkGoods;
    protected ref OZT_TradersFile m_TrdSkTraders;

    // What is picked, by name, so a fresh list keeps it: the point, and the row (its
    // category, class and condition; "" class: no row). The condition the form sets.
    protected string m_TrdSkKeepPoint;
    protected string m_TrdSkKeepCat;
    protected string m_TrdSkKeepClass;
    protected int    m_TrdSkKeepLevel;
    protected int    m_TrdSkLevel;

    // The table's head as last written (its words are placed by their widths).
    protected string m_TrdSkTitle;

    // The rows of the points, and of the table: its captions (a category) and its
    // lines (a class in a condition; key "category|line|level" of the picked point).
    protected ref OZ_VppFlexRows m_TrdSkPointRows;
    protected ref OZ_VppFlexRows m_TrdSkCatRows;
    protected ref OZ_VppFlexRows m_TrdSkRows;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        m_TrdSkKeepPoint = "";
        m_TrdSkKeepCat = "";
        m_TrdSkKeepClass = "";
        m_TrdSkKeepLevel = 0;
        m_TrdSkLevel = 0;
        m_TrdSkTitle = "";
    }

    void ~OZ_VppAdminMenu()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.TrdSkDrawChart);
    }

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "stock")
            return;
        Ask(OZT_Const.SECTION_ADMIN, "stock_list", "{}");
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdSkPaint();
        // The canvas has no size before the page's first layout pass: once more then.
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TrdSkDrawChart, 150, false);
    }

    // A file arrived or the draft went back to it: the prices are made again.
    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "stock")
            TrdSkPaint();
    }

    // The window was resized: the rows follow their lists, the chips their words, the
    // chart its canvas.
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (m_TrdSkPointRows)
            m_TrdSkPointRows.Fit();
        if (m_TrdSkCatRows)
            m_TrdSkCatRows.Fit();
        if (m_TrdSkRows)
        {
            m_TrdSkRows.Fit();
            TrdSkPlaceRows();
        }
        if (TrdShownKey() != "stock")
            return;
        TrdSkFlowHead();
        TrdSkDrawChart();
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
        TrdSkPaint();
    }

    // ------------------------------------------------------------------ reading

    // The picked point: the one kept by name, else the first.
    protected OZT_AdminStockPoint TrdSkPointAt()
    {
        if (!m_TrdSk || m_TrdSk.Points.Count() == 0)
            return null;
        for (int i = 0; i < m_TrdSk.Points.Count(); i++)
        {
            if (m_TrdSk.Points[i].Id == m_TrdSkKeepPoint)
                return m_TrdSk.Points[i];
        }
        return m_TrdSk.Points[0];
    }

    // The picked row's category and line at the picked point; null when no row is
    // picked or a fresh list no longer has it.
    protected OZT_AdminStockCat TrdSkCatAt()
    {
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (!p || m_TrdSkKeepClass == "")
            return null;
        for (int c = 0; c < p.Cats.Count(); c++)
        {
            if (p.Cats[c].Id == m_TrdSkKeepCat)
                return p.Cats[c];
        }
        return null;
    }

    protected OZT_AdminStockLine TrdSkLineAt()
    {
        OZT_AdminStockCat cat = TrdSkCatAt();
        if (!cat)
            return null;
        for (int l = 0; l < cat.Lines.Count(); l++)
        {
            if (cat.Lines[l].ClassName == m_TrdSkKeepClass)
                return cat.Lines[l];
        }
        return null;
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

    // The line a thing sold in a condition goes into: its own, or the pristine one
    // when the settings say so or the goods are counted by the unit (OZT_Book).
    protected int TrdSkInto(OZT_VppStockNums nums, int level)
    {
        if (nums.OnlyPristine == 1 || nums.Rule == OZT_Const.RULE_UNIT)
            return 0;
        return level;
    }

    // What the prices of a class in a category at a point are made of, from the files
    // in force; null while they have not come, or when they no longer have the line.
    protected OZT_VppStockNums TrdSkNumsOf(string pointId, string catId, string cls)
    {
        if (!m_TrdSkSettings || !m_TrdSkGoods || !m_TrdSkTraders)
            return null;
        OZT_Point point = null;
        int i;
        for (i = 0; i < m_TrdSkTraders.Traders.Count(); i++)
        {
            if (m_TrdSkTraders.Traders[i].Id == pointId)
                point = m_TrdSkTraders.Traders[i];
        }
        OZT_Goods line = null;
        for (i = 0; i < m_TrdSkGoods.Categories.Count(); i++)
        {
            OZT_Category cat = m_TrdSkGoods.Categories[i];
            if (cat.Id != catId || !cat.Goods)
                continue;
            // The category's first line of the class is the one the load keeps.
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                if (!line && cat.Goods[g].ClassName == cls)
                    line = cat.Goods[g];
            }
        }
        if (!point || !line || !line.Stock)
            return null;

        OZT_VppStockNums nums = new OZT_VppStockNums();
        nums.NoteUnit = TrdUnitOf(m_TrdSkSettings);
        // A price that is no multiple of the smallest note is made one by the load, in
        // the trader's favour (OZT_Config.Fitted).
        nums.Buy = line.Buy;
        nums.Sell = line.Sell;
        int off;
        if (nums.NoteUnit > 1 && nums.Buy > 0)
        {
            off = nums.Buy % nums.NoteUnit;
            if (off != 0)
                nums.Buy = OZT_Config.RoundUpTo(nums.Buy, nums.NoteUnit);
        }
        if (nums.NoteUnit > 1 && nums.Sell > 0)
        {
            off = nums.Sell % nums.NoteUnit;
            if (off != 0)
                nums.Sell = OZT_Config.RoundDownTo(nums.Sell, nums.NoteUnit);
        }
        nums.Rule = line.Quantity;
        nums.Coef = line.Stock.Coefficient;
        if (nums.Coef <= 0)
            nums.Coef = 1;
        nums.Floor = line.Stock.Floor;
        if (nums.Floor <= 0)
            nums.Floor = m_TrdSkSettings.StockFloor;
        nums.BuyFactor = point.BuyFactor;
        nums.SellFactor = point.SellFactor;
        nums.OnlyPristine = m_TrdSkSettings.StockOnlyPristine;
        for (int lv = 0; lv < OZT_Const.LEVEL_RUINED; lv++)
        {
            float share = 0;
            if (m_TrdSkSettings.Condition)
                share = m_TrdSkSettings.Condition.Of(lv);
            nums.Shares.Insert(share);
        }
        return nums;
    }

    // A price in the table: "2 375", "10 / unit" for goods counted by the unit, a dash
    // for a side that is off.
    protected string TrdSkPriceText(int price, string rule)
    {
        if (price <= 0)
            return "—";
        if (rule != OZT_Const.RULE_UNIT)
            return TrdMoney(price);
        string per = TrdTr("#STR_OZT_VPP_SK_PER_UNIT");
        return TrdMoney(price) + " " + per;
    }

    // ------------------------------------------------------------------ painting

    protected bool TrdSkMakeRows()
    {
        if (!m_TrdSkPointRows)
        {
            Widget pointHost = TrdW("TrdSkPointsRows");
            if (pointHost)
                m_TrdSkPointRows = new OZ_VppFlexRows(m_Flex, pointHost, TRD_LAYOUTS + "stock_point.layout", "OZT_SkPoint", 230, 49);
        }
        if (!m_TrdSkRows)
        {
            // Both kinds of rows go into the table's one spacer, in the order they are
            // added.
            Widget host = TrdW("TrdSkLinesRows");
            if (host)
            {
                m_TrdSkCatRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "stock_cat.layout", "OZT_SkCat", 669, 28);
                m_TrdSkRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "stock_row.layout", "OZT_SkRow", 669, 39);
            }
        }
        if (!m_TrdSkPointRows || !m_TrdSkCatRows || !m_TrdSkRows)
            return false;
        return true;
    }

    protected void TrdSkPaint()
    {
        // The files in force: the draft's own objects while it has not changed them, a
        // fresh reading of what the server gave otherwise.
        if (m_TrdDraft)
        {
            m_TrdSkSettings = m_TrdDraft.SettingsAsRead();
            m_TrdSkGoods = m_TrdDraft.GoodsAsRead();
            m_TrdSkTraders = m_TrdDraft.TradersAsRead();
        }
        if (!m_TrdSk || !TrdSkMakeRows())
            return;

        string stockWord = TrdTr("#STR_OZT_VPP_SK_STOCK");
        string sharedWord = TrdTr("#STR_OZT_VPP_SK_SHARED");
        OZT_AdminStockPoint picked = TrdSkPointAt();
        m_TrdSkPointRows.Clear();
        for (int i = 0; i < m_TrdSk.Points.Count(); i++)
        {
            OZT_AdminStockPoint p = m_TrdSk.Points[i];
            Widget row = m_TrdSkPointRows.Add(p.Id);
            if (!row)
                continue;
            string name = p.Name;
            if (name == "")
                name = p.Id;
            string sub = stockWord + " " + p.Stock;
            if (TrdSkSharers(p.Stock) > 1)
                sub = sub + " · " + sharedWord;
            bool lit = p == picked;
            RowText(row, "SkPointName", name, OZT_Metal.TEXT);
            RowText(row, "SkPointSub", sub, OZT_Metal.MUTED);
            Widget part = row.FindAnyWidget("OZT_SkPointBg");
            if (part)
                part.Show(lit);
            part = row.FindAnyWidget("OZT_SkPointBar");
            if (part)
                part.Show(lit);
        }
        m_TrdSkPointRows.Fit();

        TrdSkPaintLines();
        TrdSkPaintSide();
        TrdSkPaintStart();
    }

    // How many points sell from a stock.
    protected int TrdSkSharers(string stock)
    {
        int n = 0;
        for (int i = 0; i < m_TrdSk.Points.Count(); i++)
        {
            if (m_TrdSk.Points[i].Stock == stock)
                n++;
        }
        return n;
    }

    // The table of the picked point: a caption a category, then its lines the search
    // keeps -- the pristine line of each class always, a worn one while it holds
    // something.
    protected void TrdSkPaintLines()
    {
        if (!m_TrdSk || !TrdSkMakeRows())
            return;
        m_TrdSkCatRows.Clear();
        m_TrdSkRows.Clear();
        OZT_AdminStockPoint p = TrdSkPointAt();
        string find = TrdLow(GetEdit("TrdSkFind").Trim());
        int lines = 0;
        int hidden = 0;
        if (p)
        {
            for (int c = 0; c < p.Cats.Count(); c++)
            {
                OZT_AdminStockCat cat = p.Cats[c];
                bool captioned = false;
                for (int l = 0; l < cat.Lines.Count(); l++)
                {
                    OZT_AdminStockLine line = cat.Lines[l];
                    lines++;
                    if (find != "" && TrdLow(line.ClassName).IndexOf(find) == -1)
                        continue;
                    if (m_TrdSkRows.Count() >= TRD_SK_ROWS_MAX)
                    {
                        hidden++;
                        continue;
                    }
                    if (!captioned)
                    {
                        captioned = true;
                        TrdSkCaption(cat.Name + " · " + cat.Id);
                    }
                    OZT_VppStockNums nums = TrdSkNumsOf(p.Id, cat.Id, line.ClassName);
                    for (int lv = 0; lv < OZT_Const.LEVEL_RUINED; lv++)
                    {
                        if (lv > 0 && TrdSkCount(line, lv) == 0)
                            continue;
                        string key = c.ToString() + "|" + l.ToString() + "|" + lv.ToString();
                        Widget row = m_TrdSkRows.Add(key);
                        if (row)
                            TrdSkFillRow(row, line, lv, nums);
                    }
                }
            }
        }
        if (lines == 0)
            TrdSkCaption(TrdTr("#STR_OZT_VPP_SK_NONE"));
        m_TrdSkCatRows.Fit();
        m_TrdSkRows.Fit();
        TrdSkPlaceRows();
        TrdSkMarkRows();

        // The table's head: the stock, who sells from it, how many lines.
        string title = "";
        string sub = "";
        if (p)
        {
            title = p.Stock;
            string sells = TrdTr("#STR_OZT_VPP_SK_SELLS");
            string linesWord = TrdTr("#STR_OZT_VPP_SK_LINES_N");
            sub = sells + " " + TrdSkSellers(p.Stock);
            sub = sub + " · " + linesWord + ": " + lines.ToString();
            if (hidden > 0)
            {
                string cut = TrdTr("#STR_OZT_VPP_LIST_CUT");
                int rowsMax = TRD_SK_ROWS_MAX;
                sub = sub + " · " + cut + " " + rowsMax.ToString();
            }
        }
        m_TrdSkTitle = title;
        SayTrd("TrdSkStockHead", title);
        SayTrd("TrdSkStockSub", sub);
        TrdSkFlowHead();
    }

    // The names of the points that sell from a stock.
    protected string TrdSkSellers(string stock)
    {
        string names = "";
        for (int i = 0; i < m_TrdSk.Points.Count(); i++)
        {
            OZT_AdminStockPoint p = m_TrdSk.Points[i];
            if (p.Stock != stock)
                continue;
            if (names != "")
                names = names + ", ";
            if (p.Name != "")
                names = names + p.Name;
            else
                names = names + p.Id;
        }
        return names;
    }

    protected void TrdSkCaption(string text)
    {
        Widget row = m_TrdSkCatRows.Add(m_TrdSkCatRows.Count().ToString());
        if (row)
            RowText(row, "SkCatName", text, OZT_Metal.MUTED);
    }

    // One class in one condition: its count, its Max beside the pristine line, and
    // what one costs and brings now.
    protected void TrdSkFillRow(Widget row, OZT_AdminStockLine line, int level, OZT_VppStockNums nums)
    {
        // The count; with the pristine line's, the Max it starts from ("280 / 300")
        // while both fit between the row's buttons.
        int count = TrdSkCount(line, level);
        string countText = count.ToString();
        if (level == 0 && line.Max > 0)
        {
            string withMax = countText + " / " + line.Max.ToString();
            if (withMax.Length() <= 11)
                countText = withMax;
        }
        RowText(row, "SkRowCls", line.ClassName, OZT_Metal.TEXT);
        RowText(row, "SkRowCount", countText, OZT_Metal.TEXT);

        int ink = OZT_Metal.OK;
        int ground = OZT_Metal.LEVEL_0;
        if (level == 1)
        {
            ink = OZT_Metal.MUTED;
            ground = OZT_Metal.LEVEL_1;
        }
        else if (level == 2)
        {
            ink = OZT_Metal.ALERT;
            ground = OZT_Metal.LEVEL_2;
        }
        else if (level == 3)
        {
            ink = OZT_Metal.BAD;
            ground = OZT_Metal.LEVEL_3;
        }
        RowText(row, "SkRowLevelText", TrdLevelWord(level), ink);
        OZ_VppFlex.Paint(row, "SkRowLevelBg", ground);

        string buy = "…";
        string sell = "…";
        if (nums)
        {
            buy = TrdSkPriceText(nums.BuyAt(level, count), nums.Rule);
            int into = TrdSkInto(nums, level);
            sell = TrdSkPriceText(nums.SellAt(level, TrdSkCount(line, into)), nums.Rule);
        }
        RowText(row, "SkRowBuy", buy, OZT_Metal.ACCENT);
        RowText(row, "SkRowSell", sell, OZT_Metal.TEXT);
    }

    // The condition's chip of every row hugs its word.
    protected void TrdSkPlaceRows()
    {
        if (!m_TrdSkRows)
            return;
        float s = UnitScale();
        for (int i = 0; i < m_TrdSkRows.Count(); i++)
        {
            Widget row = m_TrdSkRows.Row(i);
            if (!row)
                continue;
            array<string> at = new array<string>();
            m_TrdSkRows.Key(i).Split("|", at);
            int level = 0;
            if (at.Count() == 3)
                level = at[2].ToInt();
            Widget chip = row.FindAnyWidget("SkRowLevel");
            Widget chipBg = row.FindAnyWidget("SkRowLevelBg");
            TextWidget chipText = TextWidget.Cast(row.FindAnyWidget("SkRowLevelText"));
            TrdHug(chip, chipBg, chipText, TrdLevelWord(level), 12, 8 * s);
        }
    }

    // The category, the line and the condition a row of the table stands for (its key
    // against the picked point); false when the list has moved on.
    protected bool TrdSkRowOf(string key, out OZT_AdminStockCat cat, out OZT_AdminStockLine line, out int level)
    {
        cat = null;
        line = null;
        level = 0;
        OZT_AdminStockPoint p = TrdSkPointAt();
        array<string> at = new array<string>();
        key.Split("|", at);
        if (!p || at.Count() != 3)
            return false;
        int c = at[0].ToInt();
        int l = at[1].ToInt();
        level = at[2].ToInt();
        if (c < 0 || c >= p.Cats.Count())
            return false;
        cat = p.Cats[c];
        if (l < 0 || l >= cat.Lines.Count())
            return false;
        line = cat.Lines[l];
        return true;
    }

    // The picked row lies on the accent's soft ground; the rest on the page's.
    protected void TrdSkMarkRows()
    {
        if (!m_TrdSkRows)
            return;
        OZT_AdminStockCat cat;
        OZT_AdminStockLine line;
        int level;
        Widget row;
        bool lit;
        for (int r = 0; r < m_TrdSkRows.Count(); r++)
        {
            row = m_TrdSkRows.Row(r);
            if (!row)
                continue;
            lit = false;
            if (m_TrdSkKeepClass != "" && TrdSkRowOf(m_TrdSkRows.Key(r), cat, line, level))
            {
                if (cat.Id == m_TrdSkKeepCat && line.ClassName == m_TrdSkKeepClass && level == m_TrdSkKeepLevel)
                    lit = true;
            }
            if (lit)
                OZ_VppFlex.Paint(row, "OZT_SkRowBg", OZT_Metal.PICK);
            else
                OZ_VppFlex.Paint(row, "OZT_SkRowBg", OZT_Metal.PANEL);
        }
    }

    // The table's head: the stock's name, then who sells from it.
    protected void TrdSkFlowHead()
    {
        TextWidget head = TextWidget.Cast(TrdW("TrdSkStockHead"));
        Widget sub = TrdW("TrdSkStockSub");
        Widget findBox = TrdW("TrdSkFind");
        if (!head || !sub)
            return;
        Widget box = sub.GetParent();
        if (!box)
            return;
        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        float fx = 0;
        float fy = 0;
        float tw = TrdTextW(head, m_TrdSkTitle, 18);
        head.GetPos(x, y);
        head.GetSize(w, h);
        head.SetSize(tw + 2, h);
        float at = x + tw + 10 * s;
        // The search box stands in a frame of its own, 11 units in.
        if (findBox && findBox.GetParent())
            findBox.GetParent().GetPos(fx, fy);
        float room = fx - at - 10 * s;
        if (room < 0)
            room = 0;
        box.GetPos(x, y);
        box.GetSize(w, h);
        box.SetPos(at, y);
        box.SetSize(room, h);
        sub.SetSize(room, h);
    }

    // The side panel: the picked row's line in words, its curve, three stocks of it,
    // and the form that sets a count.
    protected void TrdSkPaintSide()
    {
        SayTrd("BtnTrdSkLevelText", TrdLevelWord(m_TrdSkLevel));
        OZT_AdminStockPoint p = TrdSkPointAt();
        OZT_AdminStockCat cat = TrdSkCatAt();
        OZT_AdminStockLine line = TrdSkLineAt();
        if (!p || !cat || !line)
        {
            SayTrd("TrdSkHead", TrdTr("#STR_OZT_VPP_SK_PICK"));
            SayTrd("TrdSkLnSub", "");
            SayTrd("TrdSkLnNums", "");
            SayTrd("TrdSkMaxInfo", "");
            TrdSet("TrdSkCount", "");
            TrdSkTile(0, "", null, 0, 0);
            TrdSkTile(1, "", null, 0, 0);
            TrdSkTile(2, "", null, 0, 0);
            TrdSkDrawChart();
            return;
        }

        // "AKM: price by stock"; a class too long for the panel stands alone.
        string curveWord = TrdTr("#STR_OZT_VPP_SK_CURVE");
        string headText = line.ClassName + ": " + curveWord;
        TextWidget headW = TextWidget.Cast(TrdW("TrdSkHead"));
        if (headW)
        {
            float headRoom;
            float headH;
            headW.SetText(headText);
            headW.GetSize(headRoom, headH);
            if (headRoom > 0 && TrdTextW(headW, headText, 18) > headRoom)
                headW.SetText(line.ClassName);
        }
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

        string inWord = TrdTr("#STR_OZT_VPP_IN");
        string sub = inWord + " «" + cat.Name + "» · " + TrdLevelWord(m_TrdSkKeepLevel);
        SayTrd("TrdSkLnSub", sub);
        OZT_VppStockNums nums = TrdSkNumsOf(p.Id, cat.Id, line.ClassName);
        if (!nums)
        {
            SayTrd("TrdSkLnNums", TrdTr("#STR_OZT_VPP_READING"));
            TrdSkTile(0, "", null, 0, 0);
            TrdSkTile(1, "", null, 0, 0);
            TrdSkTile(2, "", null, 0, 0);
            TrdSkDrawChart();
            return;
        }
        string buyWord = TrdTr("#STR_OZT_VPP_SK_BUY_W");
        string sellWord = TrdTr("#STR_OZT_VPP_SK_SELL_W");
        string coefWord = TrdTr("#STR_OZT_VPP_SK_COEF_W");
        string floorWord = TrdTr("#STR_OZT_VPP_SK_FLOOR_W");
        sub = buyWord + " " + TrdMoney(nums.Buy);
        sub = sub + " · " + sellWord + " " + TrdMoney(nums.Sell);
        sub = sub + " · " + coefWord + " " + TrdNum(nums.Coef);
        if (nums.Floor > 0)
            sub = sub + " · " + floorWord + " " + TrdNum(nums.Floor);
        SayTrd("TrdSkLnNums", sub);

        // Three stocks of the line: what it holds now, ten, and where the floor begins
        // (fifty when it has none).
        int level = m_TrdSkKeepLevel;
        int now = TrdSkCount(line, level);
        string stockWord = TrdTr("#STR_OZT_VPP_GD_AT_STOCK");
        TrdSkTile(0, stockWord + " " + now.ToString(), nums, level, now);
        TrdSkTile(1, stockWord + " 10", nums, level, 10);
        int from = TrdSkFloorFrom(nums);
        if (from > 0)
        {
            string fromWord = TrdTr("#STR_OZT_VPP_SK_FLOOR_FROM");
            TrdSkTile(2, fromWord + " " + from.ToString(), nums, level, from);
        }
        else
        {
            TrdSkTile(2, stockWord + " 50", nums, level, 50);
        }
        TrdSkDrawChart();
    }

    // A tile under the chart: a stock, then what one costs there (the accent, as the
    // chart's line) and what one brings. No numbers: an empty tile.
    protected void TrdSkTile(int n, string cap, OZT_VppStockNums nums, int level, int stock)
    {
        string name = "TrdSkTile" + n.ToString();
        string buy = "";
        string sell = "";
        if (nums)
        {
            buy = TrdSkPriceText(nums.BuyAt(level, stock), "");
            sell = TrdSkPriceText(nums.SellAt(level, stock), "");
        }
        SayTrd(name + "Cap", cap);
        SayTrd(name + "Buy", buy);
        SayTrd(name + "Sell", sell);
    }

    // The stock from which the curve lies on the floor; 0: no floor, or a curve that
    // does not fall.
    protected int TrdSkFloorFrom(OZT_VppStockNums nums)
    {
        if (nums.Floor <= 0 || nums.Coef >= 1)
            return 0;
        float share = 1;
        for (int s = 1; s <= 2000; s++)
        {
            if (share <= nums.Floor)
                return s;
            share = share * nums.Coef;
        }
        return 0;
    }

    // What a server start does to the stock, from the settings.
    protected void TrdSkPaintStart()
    {
        if (!m_TrdSk)
            return;
        string refill = "#STR_OZT_VPP_MN_START_0";
        if (m_TrdSk.StartStock == 1)
            refill = "#STR_OZT_VPP_MN_START_1";
        else if (m_TrdSk.StartStock == 2)
            refill = "#STR_OZT_VPP_MN_START_2";
        string destock = "#STR_OZT_VPP_MN_DESTOCK_NO";
        if (m_TrdSk.DestockAtStart == 1)
            destock = "#STR_OZT_VPP_MN_DESTOCK_YES";
        string pristine = "#STR_OZT_VPP_MN_PRISTINE_NO";
        if (m_TrdSk.OnlyPristine == 1)
            pristine = "#STR_OZT_VPP_MN_PRISTINE_YES";
        SayTrd("TrdSkStart0", refill);
        SayTrd("TrdSkStart1", destock);
        SayTrd("TrdSkStart2", pristine);
        string floorWord = TrdTr("#STR_OZT_VPP_MN_FLOOR");
        SayTrd("TrdSkStart3", floorWord + " " + TrdNum(m_TrdSk.Floor));
        string note = TrdTr("#STR_OZT_VPP_SK_START_H");
        if (m_TrdSk.Cut == 1)
        {
            string cut = TrdTr("#STR_OZT_VPP_SK_CUT");
            note = note + " " + cut;
        }
        SayTrd("TrdSkStartNote", note);
    }

    // ------------------------------------------------------------------ the chart

    // The picked row's prices over its stock: what one costs (the accent) and what
    // one brings (the text's colour), the floor in dashes, the stock it holds now. The
    // mockup's drawing is 388 x 230 units: the plot from 44 to 380 across, from 20
    // down to 200.
    void TrdSkDrawChart()
    {
        if (!M_SUB_WIDGET)
            return;
        CanvasWidget cv = CanvasWidget.Cast(TrdW("TrdSkChart"));
        if (!cv)
            return;
        cv.Clear();

        OZT_AdminStockPoint p = TrdSkPointAt();
        OZT_AdminStockCat cat = TrdSkCatAt();
        OZT_AdminStockLine line = TrdSkLineAt();
        OZT_VppStockNums nums = null;
        if (p && cat && line)
            nums = TrdSkNumsOf(p.Id, cat.Id, line.ClassName);
        bool drawn = false;
        if (nums)
            drawn = true;
        TrdShow("TrdSkAxMax", drawn);
        TrdShow("TrdSkAx0", drawn);
        TrdShow("TrdSkAxX0", drawn);
        TrdShow("TrdSkAxXMid", drawn);
        TrdShow("TrdSkAxXMax", drawn);
        TrdShow("TrdSkAxFloor", false);
        TrdShow("TrdSkNow", false);
        if (!drawn)
            return;

        float s = UnitScale();
        float x0 = 44 * s;
        float x1 = 380 * s;
        float yBase = 200 * s;
        float yTop = 20 * s;
        int level = m_TrdSkKeepLevel;
        int now = TrdSkCount(line, level);

        // The stocks the chart covers: sixty, or enough for the line's Max and what it
        // holds, in tens.
        int span = 60;
        if (line.Max * 2 > span)
            span = line.Max * 2;
        if (now + 10 > span)
            span = now + 10;
        int tens = (span + 9) / 10;
        span = tens * 10;
        if (span > 2000)
            span = 2000;

        // The tallest price is the first unit's.
        int top = nums.BuyAt(level, 1);
        int topSell = nums.SellAt(level, 1);
        if (topSell > top)
            top = topSell;
        if (top <= 0)
            return;

        int edge = OZT_Metal.EDGE;
        cv.DrawLine(x0, yBase, x1, yBase, 1, edge);
        cv.DrawLine(x0, yTop, x0, yBase, 1, edge);

        float perStock = (x1 - x0) / (span - 1);
        float perCoin = (yBase - yTop) / top;

        // The floor: dashes at what one costs on it.
        if (nums.Floor > 0 && nums.Coef < 1)
        {
            int floorBuy = nums.BuyAt(level, 2000);
            float fy = yBase - floorBuy * perCoin;
            float dash = x0;
            while (dash < x1)
            {
                float dashEnd = dash + 4 * s;
                if (dashEnd > x1)
                    dashEnd = x1;
                cv.DrawLine(dash, fy, dashEnd, fy, 1, OZT_Metal.MUTED);
                dash = dash + 8 * s;
            }
            TrdShow("TrdSkAxFloor", true);
            TrdSkLabel("TrdSkAxFloor", floorBuy.ToString(), 40 * s, 2, fy - 7 * s);
        }

        // The two curves, at most 120 steps across.
        int steps = span - 1;
        if (steps > 120)
            steps = 120;
        float lastX = 0;
        float lastBuy = 0;
        float lastSell = 0;
        for (int i = 0; i <= steps; i++)
        {
            float part = i;
            int stock = 1 + Math.Round(part * (span - 1) / steps);
            float px = x0 + (stock - 1) * perStock;
            float buyY = yBase - nums.BuyAt(level, stock) * perCoin;
            float sellY = yBase - nums.SellAt(level, stock) * perCoin;
            if (i > 0)
            {
                cv.DrawLine(lastX, lastBuy, px, buyY, 3 * s, OZT_Metal.ACCENT);
                cv.DrawLine(lastX, lastSell, px, sellY, 2 * s, OZT_Metal.TEXT);
            }
            lastX = px;
            lastBuy = buyY;
            lastSell = sellY;
        }

        // The stock the line holds now.
        if (now >= 1)
        {
            int shown = now;
            if (shown > span)
                shown = span;
            float nowX = x0 + (shown - 1) * perStock;
            cv.DrawLine(nowX, yTop, nowX, yBase, 1, OZT_Metal.ALERT);
            TextWidget nowLabel = TextWidget.Cast(TrdW("TrdSkNow"));
            if (nowLabel)
            {
                string nowWord = TrdTr("#STR_OZT_VPP_SK_NOW");
                string nowText = nowWord + " " + now.ToString();
                nowLabel.Show(true);
                nowLabel.SetText(nowText);
                // The label keeps inside the plot: left of the line near its right edge.
                float nowW = TrdTextW(nowLabel, nowText, 12);
                if (nowX + 6 * s + nowW > x1)
                    TrdSkLabel("TrdSkNow", nowText, nowX - 4 * s, 2, 22 * s);
                else
                    TrdSkLabel("TrdSkNow", nowText, nowX + 4 * s, 0, 22 * s);
            }
        }

        // The axes' numbers: the tallest price left of the axis' top, the stocks under
        // its ends and its middle.
        int half = span / 2;
        TrdSkLabel("TrdSkAxMax", top.ToString(), 40 * s, 2, -1);
        SayTrd("TrdSkAx0", "0");
        SayTrd("TrdSkAxX0", "1");
        TrdSkLabel("TrdSkAxXMid", half.ToString(), 212 * s, 1, -1);
        TrdSkLabel("TrdSkAxXMax", span.ToString(), 380 * s, 2, -1);
    }

    // A label of the chart as wide as its words (a text widget cuts what is wider than
    // itself): `anchor` 0 -- its left edge at x, 1 -- its middle, 2 -- its right edge;
    // y below zero keeps the layout's. Pixels inside the plot.
    protected void TrdSkLabel(string name, string text, float x, int anchor, float y)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (!t)
            return;
        float w;
        float h;
        float px;
        float py;
        t.SetText(text);
        t.GetSize(w, h);
        t.GetPos(px, py);
        if (y >= 0)
            py = y;
        if (h <= 0)
            h = 13 * UnitScale();
        float tw = TrdTextW(t, text, 12) + 2;
        float left = x;
        if (anchor == 1)
            left = x - tw / 2;
        else if (anchor == 2)
            left = x - tw;
        if (left < 0)
            left = 0;
        t.SetSize(tw, h);
        t.SetPos(left, py);
    }

    // ------------------------------------------------------------------ the op

    // The count of the picked point's line in a condition, set on the server.
    protected void TrdSkSend(string catId, string cls, int level, int count)
    {
        OZT_AdminStockPoint p = TrdSkPointAt();
        if (!p)
            return;
        if (count < 0)
            count = 0;
        OZT_AdminStockSet req = new OZT_AdminStockSet();
        req.Point = p.Id;
        req.Category = catId;
        req.ClassName = cls;
        req.Level = level;
        req.Count = count;
        string body;
        string err;
        JsonFileLoader<OZT_AdminStockSet>.MakeData(req, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "stock_set", body);
    }

    // The form: the number typed, for the picked line in the form's condition.
    protected void TrdSkSet()
    {
        OZT_AdminStockCat cat = TrdSkCatAt();
        OZT_AdminStockLine line = TrdSkLineAt();
        if (!cat || !line)
        {
            Hint(TrdTr("#STR_OZT_VPP_SK_PICK"));
            return;
        }
        int count = GetEdit("TrdSkCount").Trim().ToInt();
        TrdSkSend(cat.Id, line.ClassName, m_TrdSkLevel, count);
    }

    // A row picked: the side panel shows its line, the form takes its condition.
    protected void TrdSkPick(string key)
    {
        OZT_AdminStockPoint p = TrdSkPointAt();
        OZT_AdminStockCat cat;
        OZT_AdminStockLine line;
        int level;
        if (!p || !TrdSkRowOf(key, cat, line, level))
            return;
        m_TrdSkKeepPoint = p.Id;
        m_TrdSkKeepCat = cat.Id;
        m_TrdSkKeepClass = line.ClassName;
        m_TrdSkKeepLevel = level;
        m_TrdSkLevel = level;
        TrdSkMarkRows();
        TrdSkPaintSide();
    }

    // One of a row's two buttons: the row's count a step down or up, at once.
    protected void TrdSkStep(string key, int delta)
    {
        OZT_AdminStockCat cat;
        OZT_AdminStockLine line;
        int level;
        if (!TrdSkRowOf(key, cat, line, level))
            return;
        TrdSkPick(key);
        TrdSkSend(cat.Id, line.ClassName, level, TrdSkCount(line, level) + delta);
    }

    // ------------------------------------------------------------------ gestures

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill && w.GetName() == "TrdSkFind")
        {
            TrdSkPaintLines();
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    // A click inside one of the page's lists; false: not there.
    protected bool TrdSkListClick(Widget w)
    {
        int i;
        if (m_TrdSkPointRows)
        {
            i = m_TrdSkPointRows.IndexOf(w);
            if (i != -1)
            {
                m_TrdSkKeepPoint = m_TrdSkPointRows.Key(i);
                m_TrdSkKeepCat = "";
                m_TrdSkKeepClass = "";
                m_TrdSkKeepLevel = 0;
                TrdSkPaint();
                return true;
            }
        }
        if (m_TrdSkRows)
        {
            i = m_TrdSkRows.IndexOf(w);
            if (i != -1)
            {
                string nm = w.GetName();
                if (nm == "SkRowMinus")
                    TrdSkStep(m_TrdSkRows.Key(i), -1);
                else if (nm == "SkRowPlus")
                    TrdSkStep(m_TrdSkRows.Key(i), 1);
                else
                    TrdSkPick(m_TrdSkRows.Key(i));
                return true;
            }
        }
        return false;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            if (TrdSkListClick(w))
                return true;
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
                SayTrd("BtnTrdSkLevelText", TrdLevelWord(m_TrdSkLevel));
                OZT_AdminStockLine line = TrdSkLineAt();
                if (line)
                    TrdSet("TrdSkCount", TrdSkCount(line, m_TrdSkLevel).ToString());
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
