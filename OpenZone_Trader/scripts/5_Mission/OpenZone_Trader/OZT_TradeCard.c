// The item card of the trade screen (design §3.5): what the tile under a
// resting cursor is. Name, weight and condition as the game's own inspect
// window words them, the sum that applies here, the game's description, and a
// few lines: size, cargo, the stack, what the same trader asks or pays for it,
// and why it cannot be sold.
//
// Everything the card says about the thing comes from the thing: its name and
// description from the item, never from our files. Every sum is the server's.
//
// A window of its own on the workspace, over the screen and its pictures
// (why: the note of ui/OpenZone_Trader/ozt_trade_qty.json). It ignores the
// pointer whole. Its height is its content's: it is laid out off the screen,
// measured two frames later and then put beside the tile.

class OZT_Card
{
    static const string LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_card.layout";
    static const string ROOT   = "OZT_TradeCard";
    static const int ROWS = 5;

    protected Widget m_Root;
    protected Widget m_Bg;
    protected Widget m_Body;
    protected bool m_Shown;
    protected int m_Settle;
    protected int m_Used;

    // The tile's rectangle on the screen, in pixels.
    protected float m_AtX;
    protected float m_AtY;
    protected float m_AtW;
    protected float m_AtH;

    void OZT_Card()
    {
        m_Root = OZT_Ui.Make(LAYOUT, null, ROOT);
        if (!m_Root)
            return;
        m_Root.Show(false);
        m_Bg = m_Root.FindAnyWidget("CardBg");
        m_Body = m_Root.FindAnyWidget("CardBody");
        // Spacers take the pointer unless told otherwise, and nothing of the
        // card may: the tile under the cursor must stay under the cursor.
        if (m_Body)
            m_Body.SetFlags(WidgetFlags.IGNOREPOINTER);
        for (int r = 0; r < ROWS; r++)
        {
            Widget line = m_Root.FindAnyWidget("CardRow" + r.ToString());
            if (line)
                line.SetFlags(WidgetFlags.IGNOREPOINTER);
        }
    }

    // The window is not a part of the screen's layout: the engine does not
    // delete it with the menu.
    void Destroy()
    {
        if (m_Root)
            m_Root.Unlink();
        m_Root = null;
        m_Bg = null;
        m_Body = null;
        m_Shown = false;
    }

    bool IsShown()
    {
        return m_Shown;
    }

    void Hide()
    {
        if (!m_Shown)
            return;
        m_Shown = false;
        m_Settle = 0;
        if (m_Root)
            m_Root.Show(false);
    }

    protected void Line(string key, string value)
    {
        if (m_Used >= ROWS)
            return;
        string n = m_Used.ToString();
        OZT_Ui.Say(m_Root, "CardKey" + n, key);
        OZT_Ui.Say(m_Root, "CardVal" + n, value);
        OZT_Ui.Show(m_Root, "CardRow" + n, true);
        m_Used++;
    }

    // `g` is the trader's entry for the thing's class (null when he neither
    // sells nor takes it); `comes` names what the goods come with, "" when
    // nothing does.
    void Open(OZT_Tile t, OZT_WireGoods g, string comes)
    {
        if (!m_Root || !m_Body || !t || !t.Item || !t.Root)
            return;
        EntityAI item = t.Item;
        bool goods = t.Kind == OZT_Tile.SHELF || t.Kind == OZT_Tile.BUY;
        bool byUnit = false;
        if (g && g.Rule == OZT_Const.RULE_UNIT)
            byUnit = true;

        // A licence reads its own name, not the name of what draws it.
        string title = OZT_Ui.NameOf(item);
        if (g && g.Name != "")
            title = g.Name;
        OZT_Ui.Say(m_Root, "CardName", title);
        // The game's own words and colours for weight and condition.
        InspectMenuNew.UpdateItemInfoWeight(m_Root, item);
        InspectMenuNew.UpdateItemInfoDamage(m_Root, item);

        string label = OZT_Ui.Words("STR_OZT_UI_PAYS");
        string price = "";
        if (t.Kind == OZT_Tile.SHELF)
        {
            label = OZT_Ui.Words("STR_OZT_UI_PRICE");
            if (byUnit)
                label = OZT_Ui.Words("STR_OZT_UI_PER_UNIT");
        }
        else if (t.Kind == OZT_Tile.BUY || t.Kind == OZT_Tile.SELL)
        {
            label = OZT_Ui.Words("STR_OZT_UI_TOTAL");
        }
        if (t.Price > 0)
            price = OZT_Ui.Sum(t.Price);
        else if (t.Price < 0)
            price = "...";
        else
            price = "-";
        OZT_Ui.Say(m_Root, "CardPriceLabel", label);
        OZT_Ui.Say(m_Root, "CardPrice", price);

        string about = "";
        InventoryItem ii = InventoryItem.Cast(item);
        if (ii)
            about = ii.GetTooltip();
        RichTextWidget desc = RichTextWidget.Cast(m_Root.FindAnyWidget("CardDesc"));
        if (desc)
        {
            desc.SetText(about);
            desc.Show(about != "");
        }
        OZT_Ui.Show(m_Root, "CardGap", about != "");

        m_Used = 0;
        for (int r = 0; r < ROWS; r++)
            OZT_Ui.Show(m_Root, "CardRow" + r.ToString(), false);

        if (ii)
        {
            int w;
            int h;
            GetGame().GetInventoryItemSize(ii, w, h);
            Line(OZT_Ui.Words("STR_OZT_UI_SIZE"), w.ToString() + " x " + h.ToString());
        }
        CargoBase cargo = item.GetInventory().GetCargo();
        if (cargo)
        {
            int cw = cargo.GetWidth();
            int ch = cargo.GetHeight();
            Line(OZT_Ui.Words("STR_OZT_UI_HOLDS"), cw.ToString() + " x " + ch.ToString());
        }
        if (t.Kind == OZT_Tile.SHELF && byUnit && OZT_Stack.Countable(item))
        {
            int full = OZT_Stack.Max(item);
            Line(OZT_Ui.Words("STR_OZT_UI_FULL_STACK"), full.ToString());
        }
        if (t.Kind == OZT_Tile.OWN && t.Count > 0)
            Line(OZT_Ui.Words("STR_OZT_UI_IN_STACK"), t.Count.ToString());
        if ((t.Kind == OZT_Tile.BUY || t.Kind == OZT_Tile.SELL) && t.Count > 0)
            Line(OZT_Ui.Words("STR_OZT_UI_COUNT"), t.Count.ToString());
        // What the price of the goods holds besides the goods.
        if (goods && comes != "")
            Line(OZT_Ui.Words("STR_OZT_UI_COMES_WITH"), comes);
        // A licence the player holds (design §11.2).
        if (t.Owned)
            Line(OZT_Ui.Words("STR_OZT_UI_LICENCE"), OZT_Ui.Words("STR_OZT_UI_LICENCE_HELD"));
        // What a stock line holds (design §11.1).
        if (t.Kind == OZT_Tile.SHELF && g && g.Stock >= 0)
            Line(OZT_Ui.Words("STR_OZT_UI_IN_STOCK"), g.Stock.ToString());
        // The other side of the same trader's book.
        if (goods && g && g.Sell > 0)
            Line(OZT_Ui.Words("STR_OZT_UI_PAYS_NEW"), OZT_Ui.Sum(g.Sell));
        if (!goods && g && g.Buy > 0)
            Line(OZT_Ui.Words("STR_OZT_UI_SELLS_FOR"), OZT_Ui.Sum(g.Buy));

        string why = "";
        if (!goods && t.Why == 1)
            why = OZT_Ui.Words("STR_OZT_UI_WHY_NOT_TAKEN");
        else if (!goods && t.Why == 2)
            why = OZT_Ui.Words("STR_OZT_UI_WHY_NOT_EMPTY");
        RichTextWidget whyText = RichTextWidget.Cast(m_Root.FindAnyWidget("CardWhy"));
        if (whyText)
        {
            whyText.SetText(why);
            whyText.Show(why != "");
        }

        t.Root.GetScreenPos(m_AtX, m_AtY);
        t.Root.GetScreenSize(m_AtW, m_AtH);
        // Off the screen until it has a size.
        m_Root.SetPos(-4000, 0);
        m_Root.Show(true);
        m_Body.Update();
        m_Shown = true;
        m_Settle = 2;
    }

    // Each frame while the card is up: the engine lays the content out, then
    // the card takes its height and its place.
    void Frame()
    {
        if (!m_Shown || m_Settle <= 0)
            return;
        m_Settle--;
        if (m_Settle > 0)
            return;
        Settle();
    }

    protected void Settle()
    {
        if (!m_Root || !m_Body)
            return;
        float px = OZT_Ui.Px();
        float bw;
        float bh;
        m_Body.GetScreenSize(bw, bh);
        float w = Math.Round(400 * px);
        float h = Math.Round(134 * px + bh);
        m_Root.SetSize(w, h);
        if (m_Bg)
        {
            m_Bg.SetPos(1, 1);
            m_Bg.SetSize(w - 2, h - 2);
        }

        int sw;
        int sh;
        GetScreenSize(sw, sh);
        // To the right of the tile, or to the left where the screen ends.
        float x = m_AtX + m_AtW + 10;
        if (x + w > sw - 8)
            x = m_AtX - w - 10;
        if (x < 8)
            x = 8;
        float y = m_AtY;
        if (y + h > sh - 8)
            y = sh - 8 - h;
        if (y < 8)
            y = 8;
        m_Root.SetPos(Math.Round(x), Math.Round(y));
    }
}
