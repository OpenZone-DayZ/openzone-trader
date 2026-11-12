// Tiles and the grids they lie in.
//
// A tile is a thing's picture with its marks. Four grids hold them: the
// trader's shelf, the player's own things and the two carts. A grid is twelve
// cells across; a tile takes as many cells as the thing takes in the
// inventory (the owner, 2026-11-11), and tiles are packed first fit, row by
// row.

class OZT_Tile
{
    // What a tile stands for.
    static const int SHELF = 1;   // goods of the trader
    static const int OWN   = 2;   // a thing the player carries
    static const int BUY   = 3;   // a line of the BUY cart
    static const int SELL  = 4;   // a line of the SELL cart
    static const int PART  = 5;   // a part in the parts strip

    static const string LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_tile.layout";
    static const string ROOT   = "OZT_TradeTile";

    int Kind;
    string Type;

    // The entity the picture is drawn from: the player's own thing, or the
    // shelf's stand-in for a class.
    EntityAI Item;

    // The player's own thing, as the server names it.
    int Low;
    int High;

    // What the tile shows. Price below zero: asked, not answered yet.
    int Price;
    int Count;

    // 0: can be sold. 1: the trader does not take it. 2: not empty (in the
    // strip: kept). 3: bought in this deal. 4: rides on a part that stays; on
    // the shelf: sold out.
    int Why;

    // The goods' condition on the shelf and in the BUY cart: a stock line's
    // level (design §11.1), 0 pristine.
    int Level;

    // A stocked line that holds nothing now: its picture pales and its count
    // reads a red zero.
    bool SoldOut;

    // A licence the player holds (design §11.2): the green corner.
    bool Owned;

    // A part's price the player pays (rounded up on the tile), and the
    // number of parts a BUY line fits to its thing.
    bool Pays;
    int Parts;

    // A line of a cart, or the place in the list the tile was made from.
    int Index;

    // Cells.
    int W;
    int H;
    int Col;
    int Row;

    Widget Root;
    protected Widget m_Edge;
    protected Widget m_Bg;
    protected ItemPreviewWidget m_Pic;
    protected Widget m_No;
    protected Widget m_Pip;
    protected TextWidget m_PriceText;
    protected TextWidget m_QtyText;
    protected TextWidget m_QtyTop;
    protected bool m_Picked;

    // The thing the parts strip shows: lit as the selection is while a part
    // of it is picked in the strip (the owner, 2026-11-12: the selection fell
    // off and the thing being changed was lost from sight).
    bool Held;

    void OZT_Tile(int kind, string type, EntityAI item)
    {
        Kind = kind;
        Type = type;
        Item = item;
        W = 1;
        H = 1;
        ReadFootprint();
    }

    // The inventory size, never wider than the grid.
    protected void ReadFootprint()
    {
        InventoryItem ii = InventoryItem.Cast(Item);
        if (!ii)
            return;
        int w;
        int h;
        GetGame().GetInventoryItemSize(ii, w, h);
        W = w;
        H = h;
        if (W < 1)
            W = 1;
        if (H < 1)
            H = 1;
        if (W > OZT_TileGrid.COLS)
            W = OZT_TileGrid.COLS;
        if (H > 15)
            H = 15;
    }

    bool Build(Widget parent)
    {
        Root = OZT_Ui.Make(LAYOUT, parent, ROOT);
        if (!Root)
            return false;
        m_Edge = Root.FindAnyWidget("TileEdge");
        m_Bg = Root.FindAnyWidget("TileBg");
        m_Pic = ItemPreviewWidget.Cast(Root.FindAnyWidget("TilePic"));
        m_No = Root.FindAnyWidget("TileNo");
        m_Pip = Root.FindAnyWidget("TilePip");
        m_PriceText = TextWidget.Cast(Root.FindAnyWidget("TilePrice"));
        m_QtyText = TextWidget.Cast(Root.FindAnyWidget("TileQty"));
        m_QtyTop = TextWidget.Cast(Root.FindAnyWidget("TileQtyTop"));
        if (m_Pic && Item)
        {
            m_Pic.SetItem(Item);
            m_Pic.SetView(Item.GetViewIndex());
            m_Pic.SetModelOrientation("0 0 0");
        }
        PaintMarks();
        return true;
    }

    // Where the grid put it, in whole pixels: `cell` is one cell's side.
    void Place(float cell)
    {
        if (!Root)
            return;
        float w = W * cell;
        float h = H * cell;
        Root.SetPos(Col * cell, Row * cell);
        Root.SetSize(w, h);
        if (m_Bg)
        {
            m_Bg.SetPos(1, 1);
            m_Bg.SetSize(w - 2, h - 2);
        }
    }

    // The whole of a holder of the given size, in pixels: a slot's tile.
    void Fill(float w, float h)
    {
        if (!Root)
            return;
        W = 2;
        H = 1;
        Root.SetPos(0, 0);
        Root.SetSize(w, h);
        if (m_Bg)
        {
            m_Bg.SetPos(1, 1);
            m_Bg.SetSize(w - 2, h - 2);
        }
        PaintMarks();
    }

    void PaintMarks()
    {
        if (!Root)
            return;
        if (m_PriceText)
        {
            // Short on every tile, so that a small thing's large price fits;
            // the card has the exact sum. What the player pays is rounded up,
            // what the player is paid down.
            bool pays = Kind == SHELF || Kind == BUY || Pays;
            if (Price > 0)
                m_PriceText.SetText(OZT_Ui.Short(Price, pays));
            else if (Price < 0)
                m_PriceText.SetText("...");
            else
                m_PriceText.SetText("");
            int priceTint = OZT_Look.Gold();
            if (SoldOut)
                priceTint = OZT_Look.Muted();
            m_PriceText.SetColor(priceTint);
        }
        // The count: beside the price where the tile is wide enough for both,
        // above it where it is not.
        bool wide = W > 1;
        bool counted = Count > 0 || SoldOut;
        string many = Count.ToString();
        int tint = OZT_Look.Text();
        if (SoldOut)
        {
            many = "0";
            tint = OZT_Look.Bad();
        }
        if (m_QtyText)
        {
            m_QtyText.Show(counted && wide);
            if (counted)
            {
                m_QtyText.SetText(many);
                m_QtyText.SetColor(tint);
            }
        }
        if (m_Pic)
        {
            float alpha = 1;
            if (SoldOut)
                alpha = 0.35;
            m_Pic.SetAlpha(alpha);
        }
        if (m_QtyTop)
        {
            bool top = counted && !wide;
            string word = "";
            if (top)
                word = many;
            else if (Parts > 0)
                word = "+" + Parts.ToString();
            m_QtyTop.Show(word != "");
            m_QtyTop.SetText(word);
            int topTint = OZT_Look.Text();
            if (top)
                topTint = tint;
            m_QtyTop.SetColor(topTint);
        }
        if (m_No)
        {
            // A sold-out line wears the red corner too: on the shelf it can
            // only mean "not to be had now".
            m_No.Show(Why == 1 || Why == 2 || SoldOut || Owned);
            if (Owned)
                m_No.SetColor(OZT_Look.Good());
            else if (Why == 2)
                m_No.SetColor(OZT_Look.Alert());
            else
                m_No.SetColor(OZT_Look.Bad());
        }
        if (m_Pip)
        {
            int level = Level;
            if (Item && Kind != SHELF && Kind != BUY)
                level = Item.GetHealthLevel();
            m_Pip.Show(level > 0);
            if (level > 0)
                m_Pip.SetColor(OZT_Look.Condition(level));
        }
        SetPicked(m_Picked);
    }

    // The selected tile wears the accent on its border. A thing the trader
    // does not take lies on dark red, one with something inside on dark amber.
    void SetPicked(bool on)
    {
        m_Picked = on;
        int edge = OZT_Look.EdgeDim();
        int bed = OZT_Look.Tile();
        if (Why == 1 && Kind != SHELF && Kind != BUY)
        {
            edge = OZT_Look.NoEdge();
            bed = OZT_Look.NoTile();
        }
        else if (Why == 2 && Kind != SHELF && Kind != BUY)
        {
            edge = OZT_Look.FullEdge();
            bed = OZT_Look.FullTile();
        }
        else if (Why == 3)
        {
            edge = OZT_Look.Accent();
        }
        else if (Why == 4)
        {
            bed = OZT_Look.Dark();
        }
        if (on || Held)
        {
            edge = OZT_Look.Accent();
            if (Why == 0)
                bed = OZT_Look.TileLit();
        }
        if (m_Edge)
            m_Edge.SetColor(edge);
        if (m_Bg)
            m_Bg.SetColor(bed);
    }

    bool IsPicked()
    {
        return m_Picked;
    }

    void Unbuild()
    {
        if (m_Pic)
            m_Pic.SetItem(null);
        if (Root)
            Root.Unlink();
        Root = null;
    }
}

class OZT_TileGrid
{
    static const int COLS = 12;
    static const float CELL_UNITS = 44;

    protected ScrollWidget m_Scroll;
    protected Widget m_Content;
    protected CanvasWidget m_Lines;
    protected ref array<ref OZT_Tile> m_Tiles;
    protected int m_MinRows;
    protected int m_Rows;
    protected float m_Cell;

    // `prefix` names the scroll, its content and its canvas in the layout:
    // <prefix>Scroll, <prefix>Content, <prefix>Lines.
    void OZT_TileGrid(Widget root, string prefix, int minRows)
    {
        m_Scroll = ScrollWidget.Cast(root.FindAnyWidget(prefix + "Scroll"));
        m_Content = root.FindAnyWidget(prefix + "Content");
        m_Lines = CanvasWidget.Cast(root.FindAnyWidget(prefix + "Lines"));
        m_Tiles = new array<ref OZT_Tile>();
        m_MinRows = minRows;
        m_Rows = minRows;
        m_Cell = Math.Floor(CELL_UNITS * OZT_Ui.Px());
        if (m_Cell < 8)
            m_Cell = 8;
    }

    bool IsReady()
    {
        return m_Scroll && m_Content;
    }

    // The scroll from `top` to `bottom`, pixels of its panel: the tab strip
    // above it took what it needed.
    void Reframe(float top, float bottom)
    {
        if (!m_Scroll || bottom <= top)
            return;
        float x;
        float y;
        m_Scroll.GetPos(x, y);
        float w;
        float h;
        m_Scroll.GetSize(w, h);
        m_Scroll.SetPos(x, top);
        m_Scroll.SetSize(w, bottom - top);
    }

    float Cell()
    {
        return m_Cell;
    }

    int Count()
    {
        return m_Tiles.Count();
    }

    OZT_Tile Get(int i)
    {
        return m_Tiles[i];
    }

    void Clear()
    {
        for (int i = 0; i < m_Tiles.Count(); i++)
            m_Tiles[i].Unbuild();
        m_Tiles.Clear();
    }

    // Adds a tile and makes its widgets; false when the layout did not load.
    bool Add(OZT_Tile t)
    {
        if (!m_Content || !t.Build(m_Content))
            return false;
        m_Tiles.Insert(t);
        return true;
    }

    // The tile a widget belongs to: the widget itself or anything inside it.
    OZT_Tile Of(Widget w)
    {
        while (w)
        {
            for (int i = 0; i < m_Tiles.Count(); i++)
            {
                if (m_Tiles[i].Root == w)
                    return m_Tiles[i];
            }
            if (w == m_Content)
                return null;
            w = w.GetParent();
        }
        return null;
    }

    // Is the widget this grid's, or anything in it?
    bool Holds(Widget w)
    {
        while (w)
        {
            if (w == m_Scroll)
                return true;
            w = w.GetParent();
        }
        return false;
    }

    // Packs the tiles first fit in the order they were added, places them,
    // sizes the content to its rows and draws the cells.
    void Arrange()
    {
        if (!IsReady())
            return;

        // One int per row, a bit per column.
        array<int> rows = new array<int>();
        int used = 0;
        for (int i = 0; i < m_Tiles.Count(); i++)
        {
            OZT_Tile t = m_Tiles[i];
            bool placed = false;
            int mask = ((1 << t.W) - 1);
            for (int y = 0; y < 400 && !placed; y++)
            {
                for (int x = 0; x + t.W <= COLS; x++)
                {
                    int shifted = mask << x;
                    bool fits = true;
                    for (int j = 0; j < t.H; j++)
                    {
                        int at = y + j;
                        if (at < rows.Count() && (rows[at] & shifted) != 0)
                        {
                            fits = false;
                            break;
                        }
                    }
                    if (!fits)
                        continue;
                    for (int k = 0; k < t.H; k++)
                    {
                        int row = y + k;
                        while (rows.Count() <= row)
                            rows.Insert(0);
                        rows[row] = rows[row] | shifted;
                    }
                    t.Col = x;
                    t.Row = y;
                    placed = true;
                    if (y + t.H > used)
                        used = y + t.H;
                    break;
                }
            }
            t.Place(m_Cell);
        }

        m_Rows = used;
        if (m_Rows < m_MinRows)
            m_Rows = m_MinRows;
        m_Content.SetSize(COLS * m_Cell, m_Rows * m_Cell);
        m_Content.Update();
        m_Scroll.Update();
        DrawCells();
    }

    protected void DrawCells()
    {
        if (!m_Lines)
            return;
        m_Lines.Clear();
        float w = COLS * m_Cell;
        float h = m_Rows * m_Cell;
        int color = OZT_Look.GridLine();
        for (int x = 0; x <= COLS; x++)
        {
            float px = x * m_Cell;
            m_Lines.DrawLine(px, 0, px, h, 1, color);
        }
        for (int y = 0; y <= m_Rows; y++)
        {
            float py = y * m_Cell;
            m_Lines.DrawLine(0, py, w, py, 1, color);
        }
    }
}
