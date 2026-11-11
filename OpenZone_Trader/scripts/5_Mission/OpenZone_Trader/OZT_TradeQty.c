// The quantity dialog of the trade screen: how many units of a stack are sold,
// or how many are bought of goods priced by the unit (design §3.4: a slider,
// minus, plus, all).
//
// A bought total is the price of one times the count, the server's own rule
// for a buy. A sold total is the server's to say: for the whole stack `open`
// has said it, and for a part the dialog asks the screen to ask the server
// (a quote of that one line) a moment after the count stops moving.

//
// The dialog is a window of its own on the workspace, over the screen (why:
// the note of ui/OpenZone_Trader/ozt_trade_qty.json), so it hears its own
// events as the handler of its root, and the screen deletes it when it closes.

class OZT_QtyDialog : ScriptedWidgetEventHandler
{
    static const string LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_qty.layout";
    static const string ROOT   = "OZT_TradeQty";

    // How long the count has to rest before the server is asked for a total.
    static const float ASK_AFTER = 0.3;

    protected OZT_TradeMenu m_Menu;
    protected Widget m_Root;
    protected Widget m_Veil;
    protected SliderWidget m_Slider;
    protected ItemPreviewWidget m_Pic;

    protected bool m_Shown;
    protected bool m_Selling;
    // Notes at the change tab: given and taken, not sold and bought.
    protected bool m_Change;
    protected string m_Type;
    protected EntityAI m_Item;
    protected int m_Low;
    protected int m_High;
    protected int m_Most;
    protected int m_Count;
    protected int m_Each;
    protected int m_Whole;

    // What the dialog shows as the total; below zero while it is not known.
    protected int m_Total;
    protected float m_AskIn;

    void OZT_QtyDialog(OZT_TradeMenu menu)
    {
        m_Menu = menu;
        m_AskIn = -1;
        m_Root = OZT_Ui.Make(LAYOUT, null, ROOT);
        if (!m_Root)
            return;
        m_Root.Show(false);
        m_Root.SetHandler(this);
        m_Veil = m_Root;
        m_Slider = SliderWidget.Cast(m_Root.FindAnyWidget("QtySlider"));
        m_Pic = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("QtyPic"));
    }

    // The window is not a part of the screen's layout: the engine does not
    // delete it with the menu.
    void Destroy()
    {
        Shut();
        if (m_Root)
        {
            m_Root.SetHandler(null);
            m_Root.Unlink();
        }
        m_Root = null;
        m_Veil = null;
        m_Slider = null;
        m_Pic = null;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        return Clicked(w);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        return Slid(w);
    }

    bool IsOpen()
    {
        return m_Shown;
    }

    // Units of goods priced by the unit: `count` to begin with, `most` at most.
    void ToBuy(string type, EntityAI stand, int each, int most, int count, bool change)
    {
        m_Selling = false;
        m_Change = change;
        m_Type = type;
        m_Item = stand;
        m_Low = 0;
        m_High = 0;
        m_Each = each;
        m_Whole = 0;
        Begin(most, count);
    }

    // Units of a stack the player carries: `count` of them to begin with.
    void ToSell(EntityAI item, int low, int high, int units, int whole, int count, bool change)
    {
        m_Selling = true;
        m_Change = change;
        m_Type = item.GetType();
        m_Item = item;
        m_Low = low;
        m_High = high;
        m_Each = 0;
        m_Whole = whole;
        Begin(units, count);
    }

    protected void Begin(int most, int count)
    {
        if (!m_Veil || !m_Slider)
            return;
        m_Most = most;
        if (m_Most < 1)
            m_Most = 1;
        m_Slider.SetMinMax(1, m_Most);
        m_Slider.SetStep(1);
        if (m_Pic)
        {
            m_Pic.SetItem(m_Item);
            if (m_Item)
                m_Pic.SetView(m_Item.GetViewIndex());
        }
        string title = OZT_Ui.Words("STR_OZT_UI_HOW_MANY_BUY");
        string go = OZT_Ui.Words("STR_OZT_UI_OK_BUY");
        string unit = OZT_Ui.Words1("STR_OZT_UI_EACH", OZT_Ui.Sum(m_Each));
        if (m_Selling)
        {
            title = OZT_Ui.Words("STR_OZT_UI_HOW_MANY_SELL");
            go = OZT_Ui.Words("STR_OZT_UI_OK_SELL");
            unit = OZT_Ui.Words1("STR_OZT_UI_WHOLE", OZT_Ui.Sum(m_Whole));
        }
        if (m_Change && m_Selling)
        {
            title = OZT_Ui.Words("STR_OZT_UI_HOW_MANY_GIVE");
            go = OZT_Ui.Words("STR_OZT_UI_OK_GIVE");
        }
        else if (m_Change)
        {
            title = OZT_Ui.Words("STR_OZT_UI_HOW_MANY_TAKE");
            go = OZT_Ui.Words("STR_OZT_UI_OK_TAKE");
        }
        OZT_Ui.Say(m_Root, "QtyTitle", title);
        OZT_Ui.Say(m_Root, "QtyOkText", go);
        OZT_Ui.Say(m_Root, "QtyName", OZT_Ui.NameOf(m_Item));
        OZT_Ui.Say(m_Root, "QtyUnit", unit);
        m_Shown = true;
        m_Veil.Show(true);
        SetCount(count);
    }

    void Shut()
    {
        m_Shown = false;
        m_AskIn = -1;
        if (m_Pic)
            m_Pic.SetItem(null);
        if (m_Veil)
            m_Veil.Show(false);
    }

    protected void SetCount(int count)
    {
        if (count < 1)
            count = 1;
        if (count > m_Most)
            count = m_Most;
        m_Count = count;
        if (m_Slider && Math.Round(m_Slider.GetCurrent()) != count)
            m_Slider.SetCurrent(count);

        m_AskIn = -1;
        if (!m_Selling)
            m_Total = m_Each * count;
        else if (count >= m_Most)
            m_Total = m_Whole;
        else
        {
            m_Total = -1;
            m_AskIn = ASK_AFTER;
        }
        PaintCount();
    }

    protected void PaintCount()
    {
        OZT_Ui.Say(m_Root, "QtyCount", m_Count.ToString());
        if (m_Total < 0)
            OZT_Ui.Say(m_Root, "QtyTotal", "...");
        else
            OZT_Ui.Say(m_Root, "QtyTotal", OZT_Ui.Sum(m_Total));
    }

    // The server's sum for a part of the stack: taken only if it is for the
    // count the dialog still shows.
    void Priced(int low, int high, int count, int price)
    {
        if (!m_Shown || !m_Selling)
            return;
        if (low != m_Low || high != m_High || count != m_Count)
            return;
        m_Total = price;
        PaintCount();
    }

    void Frame(float dt)
    {
        if (!m_Shown || m_AskIn < 0)
            return;
        m_AskIn = m_AskIn - dt;
        if (m_AskIn >= 0)
            return;
        m_AskIn = -1;
        m_Menu.QtyAsk(m_Low, m_High, m_Count);
    }

    // The slider moved.
    bool Slid(Widget w)
    {
        if (!m_Shown || w != m_Slider)
            return false;
        int count = Math.Round(m_Slider.GetCurrent());
        if (count != m_Count)
            SetCount(count);
        return true;
    }

    // A click while the dialog is open: true when it was the dialog's own.
    bool Clicked(Widget w)
    {
        if (!m_Shown || !w)
            return false;
        string name = w.GetName();
        if (name == "QtyMinus")
            SetCount(m_Count - 1);
        else if (name == "QtyPlus")
            SetCount(m_Count + 1);
        else if (name == "QtyAll")
            SetCount(m_Most);
        else if (name == "QtyOk")
            Accept();
        else if (name == "QtyCancel")
            Shut();
        else
            return false;
        return true;
    }

    void Accept()
    {
        if (!m_Shown)
            return;
        bool selling = m_Selling;
        string type = m_Type;
        int low = m_Low;
        int high = m_High;
        int count = m_Count;
        Shut();
        m_Menu.QtyChosen(selling, type, low, high, count);
    }
}
