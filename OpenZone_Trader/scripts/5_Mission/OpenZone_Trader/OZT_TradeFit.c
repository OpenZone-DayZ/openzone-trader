// The trade screen on a screen narrower than its body.
//
// The body is a block of exact layout units (ozt_trade.json: 1840 x 1032), and
// the engine turns a unit into H / 1080 pixels whatever the screen's shape
// (dayz-modding gui-layouts.md §23a, measured 2026-11-18). Narrower than 16:9
// the block was wider than the screen: 62 px a side were cut at 1920x1200 and
// 222 px at 1600x1200, the shelf's first column and the last column of the
// player's things among them.
//
// THE COLUMNS KEEP THEIR WIDTHS -- the grids are twelve cells of the
// inventory's own size -- AND THE ROOM BETWEEN THEM GIVES WAY. The middle
// column moves left by half of what is missing and the right one by all of
// it; the body and the lines that span it narrow by as much. The gaps go down
// to GAP_MIN units between the panels' art and no further: a screen narrower
// than that (4:3, 5:4) is still cut, and nothing is shrunk (the owner,
// 2026-11-18). A screen as wide as 16:9 or wider is not touched at all.
//
// Which column a widget belongs to is read off the layout, not listed: every
// direct child of the body goes by its centre, and one as wide as the body
// spans it. A panel added to the layout follows without a change here.
class OZT_TradeFit
{
    // Units of the layout: the least room left between two columns' art (the
    // art stands three units out of its panel, so the panels keep sixteen),
    // and the room kept between the body and the screen's edge while there
    // is room to keep.
    static const float GAP_MIN = 10;
    static const float EDGE = 8;

    static const int PART_LEFT = 0;
    static const int PART_MIDDLE = 1;
    static const int PART_RIGHT = 2;
    static const int PART_SPAN = 3;

    protected Widget m_Body;
    // The layout's own numbers, in pixels, taken once: every fit starts from
    // them, so a second fit never squeezes a squeezed body.
    protected float m_BodyW;
    protected float m_BodyH;
    protected ref array<Widget> m_Kids;
    protected ref array<int> m_Parts;
    protected ref array<float> m_X;
    protected ref array<float> m_Y;
    protected ref array<float> m_W;
    protected ref array<float> m_H;
    protected float m_GapLeft;
    protected float m_GapRight;
    // The screen the last fit was made for (0 before the first), and the
    // pixels that fit took out of the gaps.
    protected int m_ScreenW;
    protected int m_ScreenH;
    protected float m_Cut;

    void OZT_TradeFit(Widget body)
    {
        m_Body = body;
        m_Kids = new array<Widget>();
        m_Parts = new array<int>();
        m_X = new array<float>();
        m_Y = new array<float>();
        m_W = new array<float>();
        m_H = new array<float>();
        m_Cut = 0;
    }

    // The pixels the gaps gave, for the stand's checks.
    float Cut()
    {
        return m_Cut;
    }

    // From the menu's Init and once a frame after it: fits on the first frame
    // the layout can be measured. A window resized under the open screen shut
    // the screen on the stand (2026-11-18, twice, once with no fit to make),
    // so the next opening is what fits the new size; a size that changes
    // while the screen stays open is fitted all the same.
    void WatchScreen()
    {
        if (!m_Body)
            return;
        int sw;
        int sh;
        GetScreenSize(sw, sh);
        if (sw == m_ScreenW && sh == m_ScreenH)
            return;
        if (m_Kids.Count() == 0)
        {
            if (!TakeDesign())
                return;
        }
        m_ScreenW = sw;
        m_ScreenH = sh;
        SqueezeColumns(sw, sh);
    }

    // The body and its children as the layout placed them.
    protected bool TakeDesign()
    {
        float bw;
        float bh;
        m_Body.GetSize(bw, bh);
        // A widget measured before its first layout pass reads zero
        // (gui-layouts.md §23a): the next frame asks again.
        if (bw < 1)
            return false;
        float leftEnd = 0;
        float middleStart = bw;
        float middleEnd = 0;
        float rightStart = bw;
        Widget kid = m_Body.GetChildren();
        while (kid)
        {
            float x;
            float y;
            float w;
            float h;
            kid.GetPos(x, y);
            kid.GetSize(w, h);
            int part = PartOf(x, w, bw);
            m_Kids.Insert(kid);
            m_Parts.Insert(part);
            m_X.Insert(x);
            m_Y.Insert(y);
            m_W.Insert(w);
            m_H.Insert(h);
            float end = x + w;
            if (part == PART_LEFT && end > leftEnd)
                leftEnd = end;
            if (part == PART_MIDDLE && x < middleStart)
                middleStart = x;
            if (part == PART_MIDDLE && end > middleEnd)
                middleEnd = end;
            if (part == PART_RIGHT && x < rightStart)
                rightStart = x;
            kid = kid.GetSibling();
        }
        if (m_Kids.Count() == 0)
            return false;
        m_BodyW = bw;
        m_BodyH = bh;
        m_GapLeft = middleStart - leftEnd;
        m_GapRight = rightStart - middleEnd;
        return true;
    }

    protected static int PartOf(float x, float w, float bw)
    {
        if (w >= bw * 0.9)
            return PART_SPAN;
        float centre = x + w / 2;
        if (centre < bw / 3)
            return PART_LEFT;
        if (centre > bw * 2 / 3)
            return PART_RIGHT;
        return PART_MIDDLE;
    }

    protected void SqueezeColumns(int sw, int sh)
    {
        float px = OZT_Ui.Px();
        float least = GAP_MIN * px;
        float fromLeft = m_GapLeft - least;
        if (fromLeft < 0)
            fromLeft = 0;
        float fromRight = m_GapRight - least;
        if (fromRight < 0)
            fromRight = 0;

        // What the body lacks on this screen with its edges kept; the gaps
        // give it, half each, as far as each goes.
        float missing = m_BodyW + 2 * EDGE * px - sw;
        float cut = 0;
        if (missing > 0)
            cut = Math.Round(missing);
        float most = Math.Floor(fromLeft + fromRight);
        if (cut > most)
            cut = most;
        float cutLeft = Math.Round(cut / 2);
        if (cutLeft > fromLeft)
            cutLeft = Math.Floor(fromLeft);
        if (cut - cutLeft > fromRight)
            cutLeft = cut - Math.Floor(fromRight);
        if (cut == m_Cut)
            return;

        for (int i = 0; i < m_Kids.Count(); i++)
        {
            Widget kid = m_Kids[i];
            if (!kid)
                continue;
            int part = m_Parts[i];
            float x = m_X[i];
            if (part == PART_MIDDLE)
                x = x - cutLeft;
            else if (part == PART_RIGHT)
                x = x - cut;
            kid.SetPos(x, m_Y[i]);
            if (part == PART_SPAN)
                kid.SetSize(m_W[i] - cut, m_H[i]);
        }
        m_Body.SetSize(m_BodyW - cut, m_BodyH);
        m_Cut = cut;

        string said = "trade screen " + sw.ToString() + "x" + sh.ToString();
        said = said + ": the body is " + m_BodyW.ToString() + " px";
        said = said + ", the gaps between the columns give " + cut.ToString();
        said = said + " px (left " + cutLeft.ToString() + ", right ";
        float cutRight = cut - cutLeft;
        said = said + cutRight.ToString() + ")";
        OZT_Log.Dbg(said);
    }
}
