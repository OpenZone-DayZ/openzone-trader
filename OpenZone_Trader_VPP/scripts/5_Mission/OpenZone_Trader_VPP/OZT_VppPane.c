// The module TRADER of the OpenZone admin window (VPP rework 2026-11-12, design
// §5.6; its pages converted from the mockup boards 2026-11-13, guide
// docs/specs/2026-11-13-openzone-vpp-page-guide.md). This file is the first of the
// trader's layers of OZ_VppAdminMenu, the module's frame: it creates and registers the
// pages, routes their answers and their events, and holds what the pages share -- the
// draft of the trader's three files (OZT_VppDraft), the server's problems after a
// load, the gate providers' choices, and the draft's bar.
//
// Every page is a layout of its own, converted from its board (ui/css2layout.json:
// ozt_vpp_<key>.layout, root PaneTrd<Key>), and a layer of its own in the file
// OZT_VppPane<Page>.c, whose name sorts after this one: the files of a module folder
// compile in alphabetical order, and a layer that overrides a hook must come after the
// layer that declares it. A page's layer overrides the hooks below (TrdShown,
// TrdLayout, TrdOnAnswer, TrdRepaint) for its own key and calls super for the rest.
//
// THE DRAFT'S BAR exists once (ozt_vpp_draft.layout: what is unsaved, the server's
// problems, discard, apply). The frame lays it over the bottom of the open page, for
// every page TrdHasBar names, TRD_BAR_H units high, from TrdBarLeft units of the
// page's left edge; the problems panel (ozt_vpp_probs.layout) opens above it, as wide
// as the page. A page keeps its own bottom TRD_BAR_H units free and needs no bar of
// its own; a button of a page named Btn<Prefix>Apply or Btn<Prefix>Discard (the
// prefixes of TrdBarClick) asks the same questions as the bar's.
//
// Every name this module adds to the window's class starts with Trd / m_Trd: every
// glue mods the same OZ_VppAdminMenu, and a second declaration of one name in
// another glue is a compile error of the whole module. Confirm-strip actions it
// owns start with "trd:".
//
// The pages talk to the section `traders` of the trader mod (OZT_Admin) and to the
// core's `config` section for the three files (cfg_get / cfg_set). The admin's
// rights are the core dispatcher's to check; the pages only ask.
//
// Guards: NO_GUI -- the server compiles Mission without the UI; AVPPAdminTools
// and OpenZone_VPP -- names of CfgMods classes, defined by the engine.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

// Chips: small boxes made from a row template into one frame and placed by script, each
// as wide as its caller says, in lines (the categories of a point, the parts of a goods
// line, the things a trader wears). The caller fills a chip's parts and gives its width;
// Flow puts the chips in lines. What OZ_VppFlexRows is for rows as wide as their list.
class OZT_VppChips
{
    protected OZ_VppFlex m_Flex;
    protected Widget m_Host;
    protected string m_Layout;
    protected string m_RootName;
    protected ref array<Widget> m_Chips;
    protected ref array<string> m_Keys;
    protected ref array<float> m_Widths;

    void OZT_VppChips(OZ_VppFlex flex, Widget host, string layout, string rootName)
    {
        m_Flex = flex;
        m_Host = host;
        m_Layout = layout;
        m_RootName = rootName;
        m_Chips = new array<Widget>();
        m_Keys = new array<string>();
        m_Widths = new array<float>();
    }

    Widget Host()
    {
        return m_Host;
    }

    int Count()
    {
        return m_Chips.Count();
    }

    Widget Chip(int i)
    {
        if (i < 0 || i >= m_Chips.Count())
            return null;
        return m_Chips[i];
    }

    string Key(int i)
    {
        if (i < 0 || i >= m_Keys.Count())
            return "";
        return m_Keys[i];
    }

    // The chip that holds w: w itself or anything inside a chip; -1: no chip of these.
    int IndexOf(Widget w)
    {
        int i;
        while (w && w != m_Host)
        {
            i = m_Chips.Find(w);
            if (i != -1)
                return i;
            w = w.GetParent();
        }
        return -1;
    }

    // A new chip at the end; null when the template did not load.
    Widget Add(string key)
    {
        if (!m_Host)
            return null;
        Widget chip = OZ_VppFlex.Create(m_Layout, m_Host, m_RootName);
        if (!chip)
            return null;
        m_Chips.Insert(chip);
        m_Keys.Insert(key);
        m_Widths.Insert(0);
        return chip;
    }

    // The width Flow counts for a chip, in pixels.
    void SetWidth(int i, float w)
    {
        if (i >= 0 && i < m_Widths.Count())
            m_Widths.Set(i, w);
    }

    void Clear()
    {
        int i;
        for (i = 0; i < m_Chips.Count(); i++)
        {
            if (m_Chips[i])
            {
                if (m_Flex)
                    m_Flex.Forget(m_Chips[i]);
                m_Chips[i].Unlink();
            }
        }
        m_Chips.Clear();
        m_Keys.Clear();
        m_Widths.Clear();
    }

    // The chips in lines of `lineH` pixels inside `room` pixels, `gap` between chips and
    // between lines; `tail`, when given, stands after the last chip (a button that adds
    // one), `tailW` wide. Returns the height the lines take.
    float Flow(float room, float gap, float lineH, Widget tail, float tailW)
    {
        float x = 0;
        float y = 0;
        float w;
        int i;
        for (i = 0; i < m_Chips.Count(); i++)
        {
            w = m_Widths[i];
            if (x > 0 && x + w > room)
            {
                x = 0;
                y = y + lineH + gap;
            }
            if (m_Chips[i])
                m_Chips[i].SetPos(x, y);
            x = x + w + gap;
        }
        if (tail)
        {
            if (x > 0 && x + tailW > room)
            {
                x = 0;
                y = y + lineH + gap;
            }
            tail.SetPos(x, y);
        }
        return y + lineH;
    }
}

modded class OZ_VppAdminMenu
{
    static const string TRD_MOD     = "trader";
    static const string TRD_LAYOUTS = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_";

    // The draft's bar and the problems panel as their layouts declare them, in layout
    // units (TraderDraftBar.html, TraderProblems.html; a problem's row is 1312 x 44).
    static const float TRD_BAR_H   = 61;
    static const float TRD_PROBS_H = 230;

    // The pages' roots by key ("points", ...) and the key of each rail id; the
    // content area the shell gave the panes, in pixels (OnWindowResized). Fields are
    // declared bare and set in OnCreate: every glue derives from this class.
    protected ref map<string, Widget> m_TrdPanes;
    protected ref map<string, string> m_TrdKeys;
    protected float m_TrdW;
    protected float m_TrdH;

    // The draft of the three files; the script writing fields itself (an
    // OnChange that follows is not an edit).
    protected ref OZT_VppDraft m_TrdDraft;
    protected bool m_TrdFill;

    // The draft's bar and the problems panel over the open page; the server's last
    // `problems`, whether its panel is open, whether the answer on its way follows an
    // apply (then problems open the panel), and the last apply's line.
    protected Widget m_TrdBar;
    protected Widget m_TrdProbsPanel;
    protected ref OZ_VppFlexRows m_TrdProbRows;
    protected ref OZT_AdminProblems m_TrdProbs;
    protected bool m_TrdProbsOpen;
    protected bool m_TrdProbsAfterApply;
    protected string m_TrdNote;

    // The server's last `gates`: the providers, their kinds and choices.
    protected ref OZT_AdminGates m_TrdGates;
    protected bool m_TrdGatesAsked;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;

        m_TrdPanes = new map<string, Widget>();
        m_TrdKeys = new map<string, string>();
        m_TrdDraft = new OZT_VppDraft();
        m_TrdW = 0;
        m_TrdH = 0;
        m_TrdFill = false;
        m_TrdProbsOpen = false;
        m_TrdProbsAfterApply = false;
        m_TrdNote = "";
        m_TrdGatesAsked = false;

        array<string> keys = new array<string>();
        array<string> labels = new array<string>();
        TrdPageList(keys, labels);
        for (int i = 0; i < keys.Count(); i++)
            TrdAddPage(TRD_MOD, "#STR_OZT_VPP_MOD", TRD_MOD + "." + keys[i], keys[i], labels[i]);

        TrdMakeBar();

        OZ_ClientState.AdminWatch().Insert(this.OnTraderAnswer);
        TrdPaintBars();
    }

    void ~OZ_VppAdminMenu()
    {
        OZ_ClientState.AdminWatch().Remove(this.OnTraderAnswer);
    }

    // The module's pages in the rail's order, with their captions.
    protected void TrdPageList(array<string> keys, array<string> labels)
    {
        keys.Insert("points");
        labels.Insert("#STR_OZT_VPP_PAGE_POINTS");
        keys.Insert("goods");
        labels.Insert("#STR_OZT_VPP_PAGE_GOODS");
        keys.Insert("gates");
        labels.Insert("#STR_OZT_VPP_PAGE_GATES");
        keys.Insert("stock");
        labels.Insert("#STR_OZT_VPP_PAGE_STOCK");
        keys.Insert("licences");
        labels.Insert("#STR_OZT_VPP_PAGE_LICENCES");
        keys.Insert("money");
        labels.Insert("#STR_OZT_VPP_PAGE_MONEY");
        keys.Insert("import");
        labels.Insert("#STR_OZT_VPP_PAGE_IMPORT");
    }

    // The root's name of a page's layout, as ui/css2layout.json has it: the engine
    // has handed one path another file's widgets, and OZ_VppFlex.Create checks the
    // name. "": a key this list does not know is created unchecked.
    protected string TrdRootName(string key)
    {
        if (key == "points")
            return "PaneTrdPoints";
        if (key == "goods")
            return "PaneTrdGoods";
        if (key == "gates")
            return "PaneTrdGates";
        if (key == "stock")
            return "PaneTrdStock";
        if (key == "licences")
            return "PaneTrdLicences";
        if (key == "money")
            return "PaneTrdMoney";
        if (key == "import")
            return "PaneTrdImport";
        if (key == "rtrade")
            return "PaneTrdRTrade";
        return "";
    }

    // One page: its own layout under the window, registered on the rail.
    protected Widget TrdAddPage(string module, string moduleLabel, string id, string key, string label)
    {
        string path = TRD_LAYOUTS + key + ".layout";
        Widget pane = OZ_VppFlex.Create(path, M_SUB_WIDGET, TrdRootName(key));
        if (!pane)
        {
            OZT_Log.Error("trader vpp: a page's layout did not load: " + path);
            return null;
        }
        m_TrdPanes.Set(key, pane);
        m_TrdKeys.Set(id, key);
        RegisterPage(module, moduleLabel, id, label, pane, "");
        return pane;
    }

    // Is a page of ours on screen, and which: "" when none.
    protected string TrdShownKey()
    {
        string key = "";
        if (m_TrdKeys)
            m_TrdKeys.Find(CurrentPane(), key);
        return key;
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);
        string key;
        if (m_TrdKeys && m_TrdKeys.Find(id, key))
            TrdShown(key);
        // The bar follows the page: shown on a page that has it, hidden on any other,
        // another module's too.
        TrdPlaceBar();
    }

    // A page of ours came on screen. Each page's layer takes its own key; a page
    // with the draft's bar asks for the server's problems.
    protected void TrdShown(string key)
    {
        // The line of an apply that is over is yesterday's news on the next visit.
        if (m_TrdDraft && m_TrdDraft.Changed() == 0 && m_TrdDraft.Sending() == "")
            m_TrdNote = "";
        TrdPaintBars();
        if (TrdHasBar(key))
            TrdAskProblems();
    }

    // The pages the draft's bar lies on: the ones that edit the draft.
    protected bool TrdHasBar(string key)
    {
        array<string> bars = {"points", "goods", "gates", "licences", "money"};
        return bars.Find(key) != -1;
    }

    // Where the bar begins on a page, in layout units from the page's left edge: on
    // Points it lies under the card, beside the list (the list's column is 381 wide).
    protected float TrdBarLeft(string key)
    {
        if (key == "points")
            return 381;
        return 0;
    }

    // The window was resized: the bar follows the content area, and every page that
    // places something by script (the rows of its lists, its chips, a chart) does it
    // again in TrdLayout. w x h: the content area in pixels.
    override void OnWindowResized(float w, float h)
    {
        super.OnWindowResized(w, h);
        m_TrdW = w;
        m_TrdH = h;
        if (M_SUB_WIDGET && m_TrdPanes)
        {
            TrdPlaceBar();
            TrdLayout(w, h);
        }
    }

    protected void TrdLayout(float w, float h)
    {
        if (m_TrdProbRows && m_TrdProbsOpen)
            m_TrdProbRows.Fit();
    }

    // The confirm strip of the window stands above the draft's bar.
    override float ConfirmLift()
    {
        float lift = super.ConfirmLift();
        string key = TrdShownKey();
        if (m_TrdBar && key != "" && TrdHasBar(key) && lift < TRD_BAR_H)
            lift = TRD_BAR_H;
        return lift;
    }

    // A page's content changed under it (a file arrived, a discard, an apply):
    // the shown page paints itself again.
    protected void TrdRepaint(string key)
    {
    }

    // ------------------------------------------------------------------ helpers

    protected Widget TrdW(string name)
    {
        if (!M_SUB_WIDGET)
            return null;
        return M_SUB_WIDGET.FindAnyWidget(name);
    }

    // Pixels, as exact-size widgets take them.
    protected void TrdSize(string name, float w, float h)
    {
        Widget x = TrdW(name);
        if (x)
            x.SetSize(w, h);
    }

    protected void TrdPlace(string name, float x, float y, float w, float h)
    {
        Widget it = TrdW(name);
        if (!it)
            return;
        it.SetPos(x, y);
        it.SetSize(w, h);
    }

    // A page head's rule followed the window's width while the pages were laid out by
    // script; a converted page has no such widget and this does nothing.
    protected void TrdLayHead(string prefix, float w)
    {
        TrdSize(prefix + "HeadRule", w, UnitScale());
    }

    protected void TrdShow(string name, bool on)
    {
        Widget it = TrdW(name);
        if (it)
            it.Show(on);
    }

    // A pill the converter drew in parts (an edge and a ground, each a middle and two
    // round ends, an icon, the words) made as wide as its words plus padR and stood at x
    // of its parent: a part that reaches into the pill's right half stretches, a part
    // that starts there moves, the rest stays. No wider than room, and the words are
    // cut at their width when they do not fit. A text not drawn yet answers no width:
    // nothing moves until it does. Returns the new width, 0 when nothing was done.
    protected float TrdHugPill(Widget pill, TextWidget words, float x, float room, float padR)
    {
        if (!pill || !words)
            return 0;
        int tw = OZ_VppFlex.TextWidth(words);
        if (tw <= 0)
            return 0;
        float px;
        float py;
        float pw;
        float ph;
        pill.GetPos(px, py);
        pill.GetSize(pw, ph);
        if (pw <= 0)
            return 0;
        float wx;
        float wy;
        words.GetPos(wx, wy);
        float w = wx + tw + 2 + padR;
        if (w > room)
            w = room;
        float least = wx + padR + 1;
        if (w < least)
            w = least;
        float dw = w - pw;
        float half = pw / 2;
        float cx;
        float cy;
        float cw;
        float ch;
        Widget c = pill.GetChildren();
        while (c)
        {
            c.GetPos(cx, cy);
            c.GetSize(cw, ch);
            if (cx > half)
                c.SetPos(cx + dw, cy);
            else if (cx + cw > half)
                c.SetSize(cw + dw, ch);
            c = c.GetSibling();
        }
        pill.SetPos(x, py);
        pill.SetSize(w, ph);
        return w;
    }

    // A string of the trader's stringtable in the client's language. Take it into
    // a local before it meets another: two translations in one expression lose
    // one of them (dayz-modding, mod-structure.md).
    protected string TrdTr(string key)
    {
        return Widget.TranslateString(key);
    }

    protected void SayTrd(string name, string text)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (t)
            t.SetText(text);
    }

    protected void TrdInk(string name, string text, int color)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(color);
    }

    // A field written by the script: the OnChange it may raise is not an edit.
    protected void TrdSet(string name, string text)
    {
        m_TrdFill = true;
        SetEdit(name, text);
        m_TrdFill = false;
    }

    // A text's width in pixels as it is drawn. A page that is not on screen has not
    // been laid out and measures nothing: then an estimate from the letters, `px` the
    // text's size in layout units (the page is painted again when it is shown).
    protected float TrdTextW(TextWidget t, string text, float px)
    {
        if (!t)
            return 0;
        float w = OZ_VppFlex.TextWidth(t);
        if (w > 0)
            return w;
        float letters = text.LengthUtf8();
        return letters * px * 0.55 * UnitScale();
    }

    // A chip as wide as its word: the box (and its ground, when given) gets the
    // text's width and `pad` pixels on each side, the text stands `pad` in. `text` and
    // `px` are for TrdTextW. Returns the box's new width in pixels.
    protected float TrdHug(Widget box, Widget ground, TextWidget t, string text, float px, float pad)
    {
        if (!box || !t)
            return 0;
        float tw = TrdTextW(t, text, px);
        float w;
        float h;
        float x;
        float y;
        box.GetSize(w, h);
        w = tw + pad * 2;
        box.SetSize(w, h);
        if (ground)
            ground.SetSize(w, h);
        t.GetPos(x, y);
        t.SetPos(pad, y);
        t.GetSize(x, h);
        t.SetSize(tw + 2, h);
        return w;
    }

    // A sign of the window's icon set on an image widget, in a colour: "check",
    // "cross", "dot", "ring".
    protected void TrdSign(Widget w, string image, int color)
    {
        ImageWidget sign = ImageWidget.Cast(w);
        if (!sign)
            return;
        sign.LoadImageFile(0, "set:ozv_ui image:" + image);
        sign.SetImage(0);
        sign.SetColor(color);
    }

    // The same, the sign made `size` pixels square about the middle of where it
    // stands: the signs of the icon set are drawn at different sizes (a check larger
    // than a dot), and one widget shows now one, now another. A widget the engine has
    // not laid out yet keeps its size.
    protected void TrdSignSized(Widget w, string image, int color, float size)
    {
        if (!w)
            return;
        float x;
        float y;
        float sw;
        float sh;
        w.GetPos(x, y);
        w.GetSize(sw, sh);
        if (sw > 0 && sh > 0)
        {
            w.SetPos(x + (sw - size) / 2, y + (sh - size) / 2);
            w.SetSize(size, size);
        }
        TrdSign(w, image, color);
    }

    // A pill of a filter row on the page's ground: lit (the accent's soft ground,
    // bright words) or not (a frame, muted words). Its parts are <name>Edge, <name>Bg
    // and <name>Text.
    protected void TrdPaintPill(string name, bool lit)
    {
        Widget t = TrdW(name + "Text");
        if (lit)
        {
            OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Edge", OZT_Metal.PICK);
            OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Bg", OZT_Metal.PICK);
            if (t)
                t.SetColor(OZT_Metal.TEXT);
            return;
        }
        OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Edge", OZT_Metal.EDGE);
        OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Bg", OZT_Metal.PANEL);
        if (t)
            t.SetColor(OZT_Metal.MUTED);
    }

    // A segment of a switch inside a box: lit or not. Its parts are <name>Bg and
    // <name>Text.
    protected void TrdPaintSeg(string name, bool lit)
    {
        Widget t = TrdW(name + "Text");
        if (lit)
        {
            OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Bg", OZT_Metal.PICK_HEAD);
            if (t)
                t.SetColor(OZT_Metal.TEXT);
            return;
        }
        OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Bg", OZT_Metal.HEAD);
        if (t)
            t.SetColor(OZT_Metal.MUTED);
    }

    // A listbox from rows: `inks` colours rows (0: the layout's own), `picked` is
    // selected and kept in view. The core's m_Repaint swallows the selection
    // events SelectRow may raise. (A converted page has row templates instead; this
    // serves a page that still holds a TextListboxWidget.)
    protected void TrdFill(string name, array<string> rows, array<int> inks, int picked)
    {
        TextListboxWidget lb = TextListboxWidget.Cast(TrdW(name));
        if (!lb)
            return;
        m_Repaint = true;
        lb.ClearItems();
        for (int i = 0; i < rows.Count(); i++)
        {
            lb.AddItem(rows[i], null, 0);
            if (inks && i < inks.Count() && inks[i] != 0)
                lb.SetItemColor(i, 0, inks[i]);
        }
        if (picked >= 0 && picked < rows.Count())
        {
            lb.SelectRow(picked);
            lb.EnsureVisible(picked);
        }
        m_Repaint = false;
    }

    // The verb of an op: "move:npc" -> "move".
    protected string TrdVerb(string op)
    {
        int colon = op.IndexOf(":");
        if (colon < 0)
            return op;
        return op.Substring(0, colon);
    }

    protected string TrdJoin(array<string> items, string sep)
    {
        string all = "";
        if (!items)
            return all;
        for (int i = 0; i < items.Count(); i++)
        {
            if (i > 0)
                all = all + sep;
            all = all + items[i];
        }
        return all;
    }

    // A float as the admin types it: at most three decimals, no trailing zeros.
    protected string TrdNum(float f)
    {
        int whole = Math.Round(f * 1000);
        float back = whole / 1000.0;
        return back.ToString();
    }

    // An amount of money as the pages write it: digits in threes, "12 500".
    protected string TrdMoney(int n)
    {
        string digits = n.ToString();
        int len = digits.Length();
        if (n < 0 || len < 4)
            return digits;
        string grouped = "";
        int left;
        for (int i = 0; i < len; i++)
        {
            left = len - i;
            if (i > 0 && left % 3 == 0)
                grouped = grouped + " ";
            grouped = grouped + digits.Get(i);
        }
        return grouped;
    }

    // Lower case for a filter: the core window's OZ_VppFold, because the engine's ToLower
    // turns every Cyrillic byte into a space (a Cyrillic word then matched any word as long).
    protected string TrdLow(string s)
    {
        return OZ_VppFold.Lower(s);
    }

    // The smallest amount that can change hands under these settings: what the
    // server's OZT_Config.Unit() answers once it has checked the notes -- the
    // smallest note the game knows with item money, 1 with any other provider.
    protected int TrdUnitOf(OZT_Settings s)
    {
        if (!s || !s.Notes || s.Provider != OZT_Const.PROVIDER_ITEMS)
            return 1;
        int unit = 0;
        for (int i = 0; i < s.Notes.Count(); i++)
        {
            OZT_Note n = s.Notes[i];
            if (!n || n.Value <= 0)
                continue;
            if (!OZT_Config.ClassKnown(n.ClassName))
                continue;
            if (GetGame().ConfigIsExisting("CfgMagazines " + n.ClassName))
                continue;
            if (unit == 0 || n.Value < unit)
                unit = n.Value;
        }
        if (unit <= 0)
            return 1;
        return unit;
    }

    // A condition kind as the admin reads it; a kind of another mod as it is.
    protected string TrdCondKindWord(string kind)
    {
        if (kind == "faction")
            return TrdTr("#STR_OZT_VPP_CK_FACTION");
        if (kind == "post")
            return TrdTr("#STR_OZT_VPP_CK_POST");
        if (kind == "leader")
            return TrdTr("#STR_OZT_VPP_CK_LEADER");
        if (kind == "licence")
            return TrdTr("#STR_OZT_VPP_CK_LICENCE");
        if (kind == "research")
            return TrdTr("#STR_OZT_VPP_CK_RESEARCH");
        return kind;
    }

    // "research: Name (id) | Name (id)", "not licence: ...": a condition in words,
    // each alternative by the label its provider gives it when it has one.
    protected string TrdCondWords(OZT_Condition c)
    {
        if (!c)
            return "";
        string kind = TrdCondKindWord(c.Kind);
        string words = kind + ": " + TrdArgWords(c.Kind, c.Arg);
        if (c.Not == 1)
        {
            string notWord = TrdTr("#STR_OZT_VPP_NOT");
            words = notWord + " " + words;
        }
        return words;
    }

    protected string TrdArgWords(string kind, string arg)
    {
        array<string> alts = new array<string>();
        arg.Split("|", alts);
        string words = "";
        for (int i = 0; i < alts.Count(); i++)
        {
            string one = alts[i].Trim();
            if (one == "")
                continue;
            if (words != "")
                words = words + " | ";
            string label = TrdChoiceLabel(kind, one);
            if (label != "" && label != one)
                words = words + label + " (" + one + ")";
            else
                words = words + one;
        }
        if (words == "")
            words = "-";
        return words;
    }

    // ------------------------------------------------------------------ gates

    protected void TrdAskGates()
    {
        m_TrdGatesAsked = true;
        Ask(OZT_Const.SECTION_ADMIN, "gates", "{}");
    }

    protected void TrdNeedGates()
    {
        if (!m_TrdGates && !m_TrdGatesAsked)
            TrdAskGates();
    }

    // The server's gates have arrived (m_TrdGates). A layer that waits for them to
    // decide something -- Research > Trade appears only beside the research gate --
    // hears it here.
    protected void TrdGatesKnown()
    {
    }

    // The kind's entry of the last `gates`, or null.
    protected OZT_AdminGateKind TrdGateKind(string kind)
    {
        if (!m_TrdGates)
            return null;
        for (int f = 0; f < m_TrdGates.Families.Count(); f++)
        {
            OZT_AdminGateFamily fam = m_TrdGates.Families[f];
            for (int k = 0; k < fam.Kinds.Count(); k++)
            {
                if (fam.Kinds[k].Kind == kind)
                    return fam.Kinds[k];
            }
        }
        return null;
    }

    // The family that answers a kind ("" when no provider has it).
    protected string TrdFamilyOf(string kind)
    {
        if (!m_TrdGates)
            return "";
        for (int f = 0; f < m_TrdGates.Families.Count(); f++)
        {
            OZT_AdminGateFamily fam = m_TrdGates.Families[f];
            for (int k = 0; k < fam.Kinds.Count(); k++)
            {
                if (fam.Kinds[k].Kind == kind)
                    return fam.Family;
            }
        }
        return "";
    }

    protected string TrdChoiceLabel(string kind, string value)
    {
        OZT_AdminGateKind gk = TrdGateKind(kind);
        if (!gk)
            return "";
        int at = gk.Values.Find(value);
        if (at < 0 || at >= gk.Labels.Count())
            return "";
        return gk.Labels[at];
    }

    // Every kind the providers answer, the known ones first when `gates` has not
    // come yet.
    protected void TrdKinds(array<string> kinds)
    {
        kinds.Clear();
        if (m_TrdGates)
        {
            for (int f = 0; f < m_TrdGates.Families.Count(); f++)
            {
                OZT_AdminGateFamily fam = m_TrdGates.Families[f];
                for (int k = 0; k < fam.Kinds.Count(); k++)
                    kinds.Insert(fam.Kinds[k].Kind);
            }
        }
        array<string> known = {"faction", "post", "leader", "licence", "research"};
        for (int i = 0; i < known.Count(); i++)
        {
            if (kinds.Find(known[i]) < 0)
                kinds.Insert(known[i]);
        }
    }

    // Every family the providers name, the known ones when `gates` has not come.
    protected void TrdFamilies(array<string> families)
    {
        families.Clear();
        if (m_TrdGates)
        {
            for (int f = 0; f < m_TrdGates.Families.Count(); f++)
                families.Insert(m_TrdGates.Families[f].Family);
        }
        array<string> known = {"factions", "licences", "research"};
        for (int i = 0; i < known.Count(); i++)
        {
            if (families.Find(known[i]) < 0)
                families.Insert(known[i]);
        }
    }

    // ------------------------------------------------------------------ the draft

    // A file the shown page needs: asked once, the answer goes to the draft.
    protected void TrdNeedDoc(string name)
    {
        if (!m_TrdDraft || m_TrdDraft.Loaded(name) || m_TrdDraft.Asked(name))
            return;
        m_TrdDraft.MarkAsked(name, true);
        AskCfg(name);
    }

    // The server's file changed (an op, an apply, an import): read it again
    // unless the draft has changed it.
    protected void TrdReread(string name)
    {
        if (!m_TrdDraft || m_TrdDraft.Dirty(name))
            return;
        m_TrdDraft.MarkAsked(name, true);
        AskCfg(name);
    }

    protected void TrdTouch(string name)
    {
        if (!m_TrdDraft)
            return;
        m_TrdDraft.Touch(name);
        TrdPaintBars();
    }

    // Ops that rewrite a file on the server wait while the draft has changed it:
    // an apply would write the draft's older copy over the op's work.
    protected bool TrdDraftHolds(string name)
    {
        if (!m_TrdDraft || !m_TrdDraft.Dirty(name))
            return false;
        Hint(TrdTr("#STR_OZT_VPP_DRAFT_FIRST"));
        return true;
    }

    override void OnCfgText(string name, string body)
    {
        super.OnCfgText(name, body);
        if (!m_TrdDraft || !m_TrdDraft.Owns(name))
            return;
        string err;
        if (!m_TrdDraft.Take(name, body, err) && err != "")
        {
            string bad = TrdTr("#STR_OZT_VPP_BAD_FILE");
            Hint(bad + " " + name + ": " + err);
        }
        TrdPaintBars();
        TrdRepaint(TrdShownKey());
    }

    // The files a changed draft holds, in the admin's words.
    protected string TrdChangedWords()
    {
        array<string> names = m_TrdDraft.Names();
        string words = "";
        for (int i = 0; i < names.Count(); i++)
        {
            if (!m_TrdDraft.Dirty(names[i]))
                continue;
            if (words != "")
                words = words + ", ";
            words = words + TrdFileWord(names[i]);
        }
        return words;
    }

    protected string TrdFileWord(string name)
    {
        if (name == OZT_Const.CFG_SETTINGS)
            return TrdTr("#STR_OZT_VPP_FILE_SETTINGS");
        if (name == OZT_Const.CFG_GOODS)
            return TrdTr("#STR_OZT_VPP_FILE_GOODS");
        return TrdTr("#STR_OZT_VPP_FILE_TRADERS");
    }

    protected void TrdApplyAsk()
    {
        if (!m_TrdDraft || m_TrdDraft.Changed() == 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_NOTHING_CHANGED"));
            return;
        }
        if (m_TrdDraft.Sending() != "")
        {
            Hint(TrdTr("#STR_OZT_VPP_APPLYING"));
            return;
        }
        string ask = TrdTr("#STR_OZT_VPP_APPLY_ASK");
        string files = TrdChangedWords();
        Confirm(ask + " " + files + "?", "#STR_OZT_VPP_YES_APPLY", "trd:apply");
    }

    protected void TrdDiscardAsk()
    {
        if (!m_TrdDraft || m_TrdDraft.Changed() == 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_NOTHING_CHANGED"));
            return;
        }
        string ask = TrdTr("#STR_OZT_VPP_DISCARD_ASK");
        string files = TrdChangedWords();
        Confirm(ask + " " + files + "?", "#STR_OZT_VPP_YES_DISCARD", "trd:discard");
    }

    // The next changed file to the server; after the last, the problems its load
    // reported: they open the panel when there are any.
    protected void TrdApplyNext()
    {
        string name = m_TrdDraft.Next();
        if (name == "")
        {
            string done = TrdTr("#STR_OZT_VPP_APPLIED");
            m_TrdNote = done + " " + OZ_VppJournal.Clock();
            m_TrdProbsAfterApply = true;
            TrdAskProblems();
            TrdPaintBars();
            return;
        }
        SendCfg(name, m_TrdDraft.Body(name));
        TrdPaintBars();
    }

    // The core's config answers: an applied file, or a file that could not be read.
    protected void TrdOnConfig(string op, bool ok, string json, string error)
    {
        if (op.IndexOf("cfg_set:") == 0)
        {
            string sent = op.Substring(8, op.Length() - 8);
            if (sent != m_TrdDraft.Sending())
            {
                // Applied elsewhere (the core's file page): read it again.
                if (ok && m_TrdDraft.Owns(sent))
                    TrdReread(sent);
                return;
            }
            if (!ok)
            {
                m_TrdDraft.Refused();
                m_TrdNote = Words(error);
                m_TrdProbsAfterApply = true;
                TrdAskProblems();
                TrdPaintBars();
                return;
            }
            m_TrdDraft.Applied(sent);
            TrdReread(sent);
            TrdApplyNext();
            TrdRepaint(TrdShownKey());
            return;
        }
        if (op.IndexOf("cfg_get:") == 0 && !ok)
        {
            string asked = op.Substring(8, op.Length() - 8);
            if (!m_TrdDraft.Owns(asked))
                return;
            m_TrdDraft.MarkAsked(asked, false);
            string unread = TrdTr("#STR_OZT_VPP_BAD_FILE");
            m_TrdNote = unread + " " + asked + ": " + Words(error);
            TrdPaintBars();
        }
    }

    // ------------------------------------------------------------------ the bar

    // The bar and the problems panel: made once, kept in the window's content frame
    // beside the pages and above them (a page's root has priority 100, the window's
    // confirm strip 200), shown by TrdPlaceBar.
    protected void TrdMakeBar()
    {
        m_TrdBar = OZ_VppFlex.Create(TRD_LAYOUTS + "draft.layout", M_SUB_WIDGET, "OZT_VppDraft");
        if (m_TrdBar)
        {
            if (m_Content)
                m_Content.AddChild(m_TrdBar);
            m_TrdBar.Show(false);
            m_TrdBar.SetSort(150);
        }
        else
        {
            OZT_Log.Error("trader vpp: the draft's bar did not load; apply and discard have no buttons");
        }

        m_TrdProbsPanel = OZ_VppFlex.Create(TRD_LAYOUTS + "probs.layout", M_SUB_WIDGET, "OZT_VppProbs");
        if (m_TrdProbsPanel)
        {
            if (m_Content)
                m_Content.AddChild(m_TrdProbsPanel);
            m_TrdProbsPanel.Show(false);
            m_TrdProbsPanel.SetSort(160);
        }
        else
        {
            OZT_Log.Error("trader vpp: the problems panel did not load");
        }
    }

    // The bar over the bottom of the shown page, as wide as the page from TrdBarLeft
    // on; hidden on a page without it. The problems panel above it while it is open,
    // as wide as the page.
    protected void TrdPlaceBar()
    {
        if (!m_TrdBar || !m_Flex)
            return;
        string key = TrdShownKey();
        bool on = key != "" && TrdHasBar(key);
        bool panel = on && m_TrdProbsOpen;
        m_TrdBar.Show(on);
        if (m_TrdProbsPanel)
            m_TrdProbsPanel.Show(panel);
        if (!on)
            return;

        float s = UnitScale();
        float cw = m_WinW - (DEF_W - CONTENT_W) * s;
        float ch = m_WinH - (DEF_H - CONTENT_H) * s;
        float left = TrdBarLeft(key) * s;
        float w = cw - left;
        m_Flex.Resize(m_TrdBar, w, TRD_BAR_H * s, PAGE_W * s, TRD_BAR_H * s);
        m_TrdBar.SetPos(left, ch - TRD_BAR_H * s);
        if (m_TrdProbsPanel && panel)
        {
            // The panel is as wide as the page whatever the bar's width: a problem is a
            // long line.
            m_Flex.Resize(m_TrdProbsPanel, cw, TRD_PROBS_H * s, PAGE_W * s, TRD_PROBS_H * s);
            m_TrdProbsPanel.SetPos(0, ch - (TRD_BAR_H + TRD_PROBS_H) * s);
            if (m_TrdProbRows)
                m_TrdProbRows.Fit();
        }
    }

    protected void TrdAskProblems()
    {
        Ask(OZT_Const.SECTION_ADMIN, "problems", "{}");
    }

    // The bar's words: the draft's state with the last apply's line, and the
    // server's problems. The pages call this after every edit of the draft.
    protected void TrdPaintBars()
    {
        if (!m_TrdDraft || !m_TrdBar)
            return;

        string state;
        int ink = OZT_Metal.MUTED;
        if (m_TrdDraft.Sending() != "")
        {
            state = TrdTr("#STR_OZT_VPP_APPLYING");
            ink = OZT_Metal.ALERT;
        }
        else if (m_TrdDraft.Changed() == 0)
        {
            state = TrdTr("#STR_OZT_VPP_NO_CHANGES");
        }
        else
        {
            string unsaved = TrdTr("#STR_OZT_VPP_UNSAVED");
            string files = TrdChangedWords();
            state = unsaved + " " + files;
            ink = OZT_Metal.ALERT;
        }
        if (m_TrdNote != "")
            state = state + "  ·  " + m_TrdNote;
        TrdInk("TrdBarState", state, ink);

        string probs;
        int probInk = OZT_Metal.MUTED;
        if (!m_TrdProbs)
            probs = TrdTr("#STR_OZT_VPP_PROBS_UNKNOWN");
        else if (m_TrdProbs.Count == 0)
        {
            probs = TrdTr("#STR_OZT_VPP_PROBS_NONE");
            probInk = OZT_Metal.OK;
        }
        else
        {
            string many = TrdTr("#STR_OZT_VPP_PROBS_N");
            probs = many + " " + m_TrdProbs.Count.ToString() + " ›";
            probInk = OZT_Metal.ALERT;
        }
        TrdInk("BtnTrdProbsText", probs, probInk);
        TrdPlaceBar();
    }

    // The problems panel's rows: a problem a row, then how many more the server
    // keeps, and the ready line's counters in the panel's head.
    protected void TrdFillProbs()
    {
        if (!m_TrdProbsPanel || !m_Flex)
            return;
        if (!m_TrdProbRows)
        {
            Widget host = m_TrdProbsPanel.FindAnyWidget("TrdProbsRows");
            if (!host)
                return;
            m_TrdProbRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "prob_row.layout", "OZT_ProbRow", 1312, 44);
        }

        m_TrdProbRows.Clear();
        string counters = "";
        if (!m_TrdProbs)
        {
            TrdProbRow(TrdTr("#STR_OZT_VPP_PROBS_UNKNOWN"), OZT_Metal.MUTED);
        }
        else
        {
            for (int i = 0; i < m_TrdProbs.Lines.Count(); i++)
                TrdProbRow(m_TrdProbs.Lines[i], OZT_Metal.ALERT);
            if (m_TrdProbs.Cut == 1)
            {
                string more = TrdTr("#STR_OZT_VPP_PROBS_MORE");
                int left = m_TrdProbs.Count - m_TrdProbs.Lines.Count();
                TrdProbRow(more + " " + left.ToString(), OZT_Metal.MUTED);
            }
            if (m_TrdProbRows.Count() == 0)
                TrdProbRow(TrdTr("#STR_OZT_VPP_PROBS_NONE"), OZT_Metal.OK);
            counters = m_TrdProbs.Counters;
        }
        m_TrdProbRows.Fit();
        TextWidget head = TextWidget.Cast(m_TrdProbsPanel.FindAnyWidget("TrdProbsCounters"));
        if (head)
            head.SetText(counters);
    }

    protected void TrdProbRow(string text, int color)
    {
        Widget row = m_TrdProbRows.Add(m_TrdProbRows.Count().ToString());
        if (row)
            RowText(row, "ProbText", text, color);
    }

    protected void TrdOpenProbs(bool open)
    {
        m_TrdProbsOpen = open;
        TrdPlaceBar();
        if (open)
            TrdFillProbs();
    }

    // A click on the bar, on the problems panel, or on a page's own button of the
    // draft: such a button is named after the page's prefix, Btn<Prefix>Apply,
    // Btn<Prefix>Discard, Btn<Prefix>Probs (the names of the bars the pages had
    // before the one bar).
    protected bool TrdBarClick(string nm)
    {
        if (nm.IndexOf("BtnTrd") != 0)
            return false;

        string what = "";
        if (nm == "BtnTrdApply" || nm == "BtnTrdDiscard" || nm == "BtnTrdProbs" || nm == "BtnTrdProbsClose")
        {
            what = nm.Substring(6, nm.Length() - 6);
        }
        else
        {
            array<string> pages = {"TrdPt", "TrdGd", "TrdGt", "TrdLc", "TrdMn"};
            array<string> verbs = {"Apply", "Discard", "Probs", "ProbsClose"};
            string full;
            for (int i = 0; i < pages.Count(); i++)
            {
                for (int v = 0; v < verbs.Count(); v++)
                {
                    full = "Btn" + pages[i] + verbs[v];
                    if (nm == full)
                        what = verbs[v];
                }
            }
        }
        if (what == "")
            return false;

        if (what == "Apply")
        {
            TrdApplyAsk();
            return true;
        }
        if (what == "Discard")
        {
            TrdDiscardAsk();
            return true;
        }
        if (what == "ProbsClose")
        {
            TrdOpenProbs(false);
            return true;
        }
        // The problems button opens the panel and asks the server again, or closes it.
        if (m_TrdProbsOpen)
        {
            TrdOpenProbs(false);
            return true;
        }
        TrdOpenProbs(true);
        TrdAskProblems();
        return true;
    }

    // ------------------------------------------------------------------ answers

    void OnTraderAnswer(string section, string op, bool ok, string json, string error)
    {
        if (!M_SUB_WIDGET)
            return;
        if (section == OZ_AdminSect.CONFIG)
        {
            TrdOnConfig(op, ok, json, error);
            return;
        }
        if (section != OZT_Const.SECTION_ADMIN)
            return;
        TrdOnAnswer(op, ok, json, error);
    }

    // An answer of the section `traders`. A failure is in the window's journal
    // already, with the server's words; the pages only follow what changed. The
    // later layers take their ops and call super.
    protected void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        string err;
        if (op == "problems")
        {
            bool after = m_TrdProbsAfterApply;
            m_TrdProbsAfterApply = false;
            if (!ok)
                return;
            OZT_AdminProblems probs = new OZT_AdminProblems();
            if (JsonFileLoader<OZT_AdminProblems>.LoadData(json, probs, err))
            {
                m_TrdProbs = probs.Copy();
                if (after && m_TrdProbs.Count > 0)
                    m_TrdProbsOpen = true;
                TrdPaintBars();
                if (m_TrdProbsOpen)
                    TrdFillProbs();
            }
            return;
        }
        if (op == "gates")
        {
            m_TrdGatesAsked = false;
            if (!ok)
                return;
            OZT_AdminGates gates = new OZT_AdminGates();
            if (JsonFileLoader<OZT_AdminGates>.LoadData(json, gates, err))
            {
                m_TrdGates = gates.Copy();
                TrdGatesKnown();
                TrdRepaint(TrdShownKey());
            }
            return;
        }
    }

    // The section's reads are asked all the time and are not news for the
    // window's journal (its failures still reach it).
    override bool IsReadOp(string op)
    {
        array<string> reads = {"stock_list", "licence_list", "problems", "gates", "gate_holders"};
        if (reads.Find(op) != -1)
            return true;
        return super.IsReadOp(op);
    }

    // ------------------------------------------------------------------ gestures

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET && m_TrdDraft && TrdBarClick(w.GetName()))
            return true;
        return super.OnClick(w, x, y, button);
    }

    // The confirm strip's "yes" for the draft; the rest go on.
    override void OnConfirmed(string action)
    {
        if (action == "trd:apply")
        {
            TrdApplyNext();
            return;
        }
        if (action == "trd:discard")
        {
            m_TrdDraft.Discard();
            m_TrdNote = TrdTr("#STR_OZT_VPP_DISCARDED");
            TrdPaintBars();
            TrdRepaint(TrdShownKey());
            return;
        }
        super.OnConfirmed(action);
    }
}

#endif
#endif
#endif
