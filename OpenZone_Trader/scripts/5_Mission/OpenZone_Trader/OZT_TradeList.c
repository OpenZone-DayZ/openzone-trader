// The price list of the trade screen (design §3.5): per category, what the
// trader sells and for how much, and what he pays for a pristine thing. It is
// built from the data the screen already holds, the answer of `open`: goods
// locked for the player are not in that answer, so they are not in the list.
//
// A window of its own on the workspace, as the quantity dialog is and for the
// same reason; it hears its own events as the handler of its root, and the
// screen deletes it when it closes. Its pictures are the shelf's stand-ins.

class OZT_PriceList : ScriptedWidgetEventHandler
{
    static const string LAYOUT     = "OpenZone_Trader/gui/layouts/ozt_trade_list.layout";
    static const string ROOT       = "OZT_TradeList";
    static const string ROW_LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_list_row.layout";
    static const string ROW_ROOT   = "OZT_TradeListRow";
    static const string TAB_LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_tab.layout";
    static const string TAB_ROOT   = "OZT_TradeTab";

    protected Widget m_Root;
    protected Widget m_Rows;
    protected ScrollWidget m_Scroll;
    protected bool m_Shown;

    protected OZT_OpenRes m_Data;
    protected OZT_Shelf m_Shelf;

    // Tabs: "" for ALL, then the trader's categories.
    protected ref array<string> m_TabIds;
    protected ref array<Widget> m_Tabs;
    protected int m_Tab;

    protected ref array<Widget> m_Made;
    protected ref array<ItemPreviewWidget> m_Pics;

    void OZT_PriceList()
    {
        m_TabIds = new array<string>();
        m_Tabs = new array<Widget>();
        m_Made = new array<Widget>();
        m_Pics = new array<ItemPreviewWidget>();
        m_Root = OZT_Ui.Make(LAYOUT, null, ROOT);
        if (!m_Root)
            return;
        m_Root.Show(false);
        m_Root.SetHandler(this);
        m_Rows = m_Root.FindAnyWidget("ListRows");
        m_Scroll = ScrollWidget.Cast(m_Root.FindAnyWidget("ListScroll"));
    }

    void Destroy()
    {
        Shut();
        ClearRows();
        if (m_Root)
        {
            m_Root.SetHandler(null);
            m_Root.Unlink();
        }
        m_Root = null;
        m_Rows = null;
        m_Scroll = null;
    }

    bool IsOpen()
    {
        return m_Shown;
    }

    // `data` is the last answer of `open`; `shelf` gives the pictures.
    void Open(OZT_OpenRes data, OZT_Shelf shelf)
    {
        if (!m_Root || !m_Rows || !data || !shelf)
            return;
        m_Data = data;
        m_Shelf = shelf;
        OZT_Ui.Say(m_Root, "ListTrader", data.Name);

        // What wear does to the sum the trader pays, in whole percent.
        string note = OZT_Ui.Words("STR_OZT_UI_LIST_NOTE");
        note.Replace("%1", Percent(data.Condition.Worn));
        note.Replace("%2", Percent(data.Condition.Damaged));
        note.Replace("%3", Percent(data.Condition.BadlyDamaged));
        RichTextWidget foot = RichTextWidget.Cast(m_Root.FindAnyWidget("ListNote"));
        if (foot)
            foot.SetText(note);

        MakeTabs();
        m_Shown = true;
        m_Root.Show(true);
        Fill();
    }

    void Shut()
    {
        m_Shown = false;
        if (m_Root)
            m_Root.Show(false);
    }

    protected string Percent(float factor)
    {
        int whole = Math.Round(factor * 100);
        return whole.ToString();
    }

    protected void MakeTabs()
    {
        for (int old = 0; old < m_Tabs.Count(); old++)
        {
            if (m_Tabs[old])
                m_Tabs[old].Unlink();
        }
        m_Tabs.Clear();
        m_TabIds.Clear();
        Widget host = m_Root.FindAnyWidget("ListTabs");
        if (!host)
            return;

        array<string> names = new array<string>();
        m_TabIds.Insert("");
        names.Insert(OZT_Ui.Words("STR_OZT_UI_ALL"));
        for (int c = 0; c < m_Data.Categories.Count(); c++)
        {
            // The change tab's notes are money, not goods with a price.
            if (m_Data.Categories[c].Id == OZT_Const.CATEGORY_CHANGE)
                continue;
            m_TabIds.Insert(m_Data.Categories[c].Id);
            names.Insert(m_Data.Categories[c].Name);
        }
        if (m_Tab >= m_TabIds.Count())
            m_Tab = 0;

        float px = OZT_Ui.Px();
        float gap = Math.Round(4 * px);
        float all = Math.Round(860 * px);
        int n = names.Count();
        // A tab is never wider than a fifth of the strip.
        float w = Math.Floor((all - gap * (n - 1)) / n);
        float most = Math.Floor(all / 5);
        if (w > most)
            w = most;
        float h = Math.Round(44 * px);
        for (int i = 0; i < n; i++)
        {
            Widget tab = OZT_Ui.Make(TAB_LAYOUT, host, TAB_ROOT);
            if (!tab)
                continue;
            tab.SetUserID(i);
            tab.SetPos(i * (w + gap), 0);
            tab.SetSize(w, h);
            OZT_Ui.Say(tab, "TabText", names[i]);
            m_Tabs.Insert(tab);
        }
        PaintTabs();
    }

    protected void PaintTabs()
    {
        for (int i = 0; i < m_Tabs.Count(); i++)
        {
            Widget tab = m_Tabs[i];
            if (!tab)
                continue;
            bool on = tab.GetUserID() == m_Tab;
            OZT_Ui.Show(tab, "TabLine", on);
            if (on)
            {
                OZT_Ui.Tint(tab, "TabText", OZT_Look.Text());
                OZT_Ui.Tint(tab, "TabBg", OZT_Look.TileLit());
            }
            else
            {
                OZT_Ui.Tint(tab, "TabText", OZT_Look.Muted());
                OZT_Ui.Tint(tab, "TabBg", OZT_Look.Raised());
            }
        }
    }

    protected void ClearRows()
    {
        for (int p = 0; p < m_Pics.Count(); p++)
        {
            if (m_Pics[p])
                m_Pics[p].SetItem(null);
        }
        m_Pics.Clear();
        for (int i = 0; i < m_Made.Count(); i++)
        {
            if (m_Made[i])
                m_Made[i].Unlink();
        }
        m_Made.Clear();
    }

    // One line per goods of the category in view: everything the trader
    // sells, and everything he pays for, sold here or not.
    protected void Fill()
    {
        ClearRows();
        if (!m_Data || !m_Rows)
            return;
        string cat = m_TabIds[m_Tab];
        string none = "-";
        string perUnit = OZT_Ui.Words("STR_OZT_UI_PER_UNIT_NOTE");
        for (int i = 0; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (cat != "" && g.Category != cat)
                continue;
            if (g.Category == OZT_Const.CATEGORY_CHANGE)
                continue;
            EntityAI stand = m_Shelf.Of(g.ClassName);
            if (!stand)
                continue;
            Widget row = OZT_Ui.Make(ROW_LAYOUT, m_Rows, ROW_ROOT);
            if (!row)
                continue;
            m_Made.Insert(row);
            ItemPreviewWidget pic = ItemPreviewWidget.Cast(row.FindAnyWidget("RowPic"));
            if (pic)
            {
                pic.SetItem(stand);
                pic.SetView(stand.GetViewIndex());
                m_Pics.Insert(pic);
            }
            OZT_Ui.Say(row, "RowName", OZT_Ui.NameOf(stand));
            string note = "";
            if (g.Rule == OZT_Const.RULE_UNIT)
                note = perUnit;
            OZT_Ui.Say(row, "RowNote", note);
            if (g.Buy > 0)
                OZT_Ui.Say(row, "RowSells", OZT_Ui.Sum(g.Buy));
            else
                OZT_Ui.Say(row, "RowSells", none);
            if (g.Sell > 0)
                OZT_Ui.Say(row, "RowPays", OZT_Ui.Sum(g.Sell));
            else
                OZT_Ui.Say(row, "RowPays", none);
        }
        m_Rows.Update();
        if (m_Scroll)
        {
            m_Scroll.Update();
            m_Scroll.VScrollToPos01(0);
        }
    }

    protected int TabOf(Widget w)
    {
        while (w)
        {
            int at = m_Tabs.Find(w);
            if (at > -1)
                return m_Tabs[at].GetUserID();
            w = w.GetParent();
        }
        return -1;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (m_Shown && w && w.GetName() == "ListClose")
        {
            Shut();
            return true;
        }
        return false;
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (!m_Shown || button != MouseState.LEFT)
            return false;
        int tab = TabOf(w);
        if (tab < 0)
            return false;
        if (tab != m_Tab)
        {
            m_Tab = tab;
            PaintTabs();
            Fill();
        }
        return true;
    }
}
