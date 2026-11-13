// Trader > Conditions and discounts (design §5.6 and §8.2; mockup TraderGates): who
// sees goods and at what price. On the left the categories of the price book, their
// lines that have conditions (or all of them) and the points; in the middle the picked
// target's lock and discounts, a condition a row; on the right the picker of a
// condition's values; above them the gate families switched off for the server (the
// settings' GatesOff) and on or off at a point (GatesOn, GatesOff). Everything is
// edited in the draft of the trader's three files and saved by the draft's bar, which
// the module's frame lays over the page's foot.
//
// The page is converted from the mockup (ui/css2layout.json, page `gates`): the three
// lists are row templates, and what follows a text's length is placed by script after
// the rows are made (TrdGtFlowCond and its kin).
//
// The picker offers what the provider of the condition's kind names (`gates`): factions,
// licences, and the research tree's nodes when the research gate is loaded -- this page
// knows the trader alone. A provider's label of the shape "name - branch (owners) -
// T<tier>" (OZT_ResearchGate.Choices writes it: change one, change both) is read into an
// owner and a branch to filter by; any other label is a plain name. A kind with no
// choices is a free word, typed into the Arg field. Who holds a value now comes from
// `gate_holders`, asked one at a time and kept (TrdGtHoldWant).
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

// What the four converted pages of this file's author do with the named parts of a
// layout, under any root (the window, or a row made from a template): show, write,
// colour, and make a drawn box as wide as its words. A text widget cuts its text at its
// own width, so whatever a script writes into a tight place is sized here.
class OZT_VppKit
{
    static void PartShow(Widget root, string name, bool on)
    {
        if (!root)
            return;
        Widget w = root.FindAnyWidget(name);
        if (w)
            w.Show(on);
    }

    static void PartSay(Widget root, string name, string text)
    {
        if (!root)
            return;
        TextWidget t = TextWidget.Cast(root.FindAnyWidget(name));
        if (t)
            t.SetText(text);
    }

    static void PartInk(Widget root, string name, string text, int color)
    {
        if (!root)
            return;
        TextWidget t = TextWidget.Cast(root.FindAnyWidget(name));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(color);
    }

    // The colour of a text or of a sign.
    static void PartTint(Widget root, string name, int color)
    {
        if (!root)
            return;
        Widget w = root.FindAnyWidget(name);
        if (w)
            w.SetColor(color);
    }

    // A box's frame and ground (a pill's round ends with them).
    static void PartBox(Widget root, string name, int edge, int ground)
    {
        OZ_VppFlex.Paint(root, name + "Edge", edge);
        OZ_VppFlex.Paint(root, name + "Bg", ground);
    }

    // A pill of a filter row on the page's ground: lit, or plain with words of `restInk`.
    static void PillPaint(Widget root, string name, bool lit, int restInk)
    {
        if (lit)
        {
            OZT_VppKit.PartBox(root, name, OZT_Metal.PICK, OZT_Metal.PICK);
            OZT_VppKit.PartTint(root, name + "Text", OZT_Metal.TEXT);
            return;
        }
        OZT_VppKit.PartBox(root, name, OZT_Metal.EDGE, OZT_Metal.PANEL);
        OZT_VppKit.PartTint(root, name + "Text", restInk);
    }

    // A segment of a switch on a box's ground: lit or plain.
    static void SegPaint(Widget root, string name, bool lit)
    {
        if (lit)
        {
            OZ_VppFlex.Paint(root, name + "Bg", OZT_Metal.PICK_HEAD);
            OZT_VppKit.PartTint(root, name + "Text", OZT_Metal.TEXT);
            return;
        }
        OZ_VppFlex.Paint(root, name + "Bg", OZT_Metal.HEAD);
        OZT_VppKit.PartTint(root, name + "Text", OZT_Metal.MUTED);
    }

    // A checkbox as its state says: the mark shown while it is on, the box in the accent
    // or plain. The box is a button holding its frame (the larger panel), its fill and
    // the mark <tick>Mark.
    static void TickMark(Widget root, string tick, bool on)
    {
        if (!root)
            return;
        Widget box = root.FindAnyWidget(tick);
        if (!box)
            return;
        Widget mark = box.FindAnyWidget(tick + "Mark");
        if (mark)
            mark.Show(on);
        int edge = OZT_Metal.FAINT;
        int fill = OZT_Metal.CELL;
        if (on)
        {
            edge = OZT_Metal.ACCENT;
            fill = OZT_Metal.ACCENT;
        }
        float bw;
        float bh;
        float w;
        float h;
        box.GetSize(bw, bh);
        Widget c = box.GetChildren();
        while (c)
        {
            if (c != mark)
            {
                c.GetSize(w, h);
                if (w >= bw - 0.5)
                    c.SetColor(edge);
                else
                    c.SetColor(fill);
            }
            c = c.GetSibling();
        }
    }

    // A sign of the window's icon set on a named image, in a colour: "check", "cross",
    // "dot", "ring".
    static void SignSet(Widget root, string name, string image, int color)
    {
        if (!root)
            return;
        ImageWidget sign = ImageWidget.Cast(root.FindAnyWidget(name));
        if (!sign)
            return;
        sign.LoadImageFile(0, "set:ozv_ui image:" + image);
        sign.SetImage(0);
        sign.SetColor(color);
    }

    // A named text's width in pixels as it is drawn now; 0 for no text, and for a text
    // the engine has not laid out yet.
    static float TextW(Widget root, string name)
    {
        if (!root)
            return 0;
        TextWidget t = TextWidget.Cast(root.FindAnyWidget(name));
        if (!t)
            return 0;
        return OZ_VppFlex.TextWidth(t);
    }

    // A named text as wide as its words. Returns the words' width in pixels; a text that
    // measures nothing (empty, or not laid out yet) keeps its box.
    static float FitText(Widget root, string name)
    {
        if (!root)
            return 0;
        TextWidget t = TextWidget.Cast(root.FindAnyWidget(name));
        if (!t)
            return 0;
        float w;
        float h;
        int tw = OZ_VppFlex.TextWidth(t);
        if (tw <= 0)
            return 0;
        t.GetSize(w, h);
        t.SetSize(tw + 2, h);
        return tw;
    }

    // A named text as wide as its words, for a text that is never empty: returns the room
    // it takes in its line -- its words' width, or its box's own while the engine has
    // not measured the words yet (then the layout's place stands for a frame).
    static float FitWord(Widget root, string name)
    {
        float tw = OZT_VppKit.FitText(root, name);
        if (tw > 0)
            return tw;
        return OZT_VppKit.WidthOf(root, name);
    }

    // A named widget's width in pixels now.
    static float WidthOf(Widget root, string name)
    {
        if (!root)
            return 0;
        Widget w = root.FindAnyWidget(name);
        if (!w)
            return 0;
        float ww;
        float hh;
        w.GetSize(ww, hh);
        return ww;
    }

    // A drawn part of a box made `d` pixels wider: its middle grows, a pill's right end
    // (the frame that cuts the round sprite) moves.
    static void GrowPart(Widget root, string part, float d)
    {
        float x;
        float y;
        Widget mid = root.FindAnyWidget(part);
        if (mid)
        {
            mid.GetSize(x, y);
            mid.SetSize(x + d, y);
        }
        Widget end = root.FindAnyWidget(part + "R");
        if (!end)
            return;
        Widget cut = end.GetParent();
        if (!cut)
            return;
        cut.GetPos(x, y);
        cut.SetPos(x + d, y);
    }

    // A named box and what it draws made `w` pixels wide.
    static void WidenBox(Widget root, string name, float w)
    {
        if (!root)
            return;
        Widget box = root.FindAnyWidget(name);
        if (!box)
            return;
        float bw;
        float bh;
        box.GetSize(bw, bh);
        box.SetSize(w, bh);
        OZT_VppKit.GrowPart(root, name + "Edge", w - bw);
        OZT_VppKit.GrowPart(root, name + "Bg", w - bw);
    }

    // A box as wide as its one text, `pad` pixels on each side. Returns the box's width;
    // a text that measures nothing yet leaves the box as it is.
    static float HugBox(Widget root, string name, string textName, float pad)
    {
        if (!root)
            return 0;
        TextWidget t = TextWidget.Cast(root.FindAnyWidget(textName));
        if (!t)
            return 0;
        float x;
        float y;
        int tw = OZ_VppFlex.TextWidth(t);
        if (tw <= 0)
            return OZT_VppKit.WidthOf(root, name);
        float full = tw + pad * 2;
        OZT_VppKit.WidenBox(root, name, full);
        t.GetPos(x, y);
        t.SetPos(pad, y);
        t.GetSize(x, y);
        t.SetSize(tw + 2, y);
        return full;
    }

    // A box of two texts, the second (smaller) `gap` pixels after the first; an empty
    // second takes no room. Returns the box's width.
    static float HugPair(Widget root, string name, string firstName, string secondName, float pad, float gap)
    {
        if (!root)
            return 0;
        TextWidget a = TextWidget.Cast(root.FindAnyWidget(firstName));
        TextWidget b = TextWidget.Cast(root.FindAnyWidget(secondName));
        if (!a)
            return 0;
        float x;
        float y;
        int aw = OZ_VppFlex.TextWidth(a);
        if (aw <= 0)
            return OZT_VppKit.WidthOf(root, name);
        a.GetPos(x, y);
        a.SetPos(pad, y);
        a.GetSize(x, y);
        a.SetSize(aw + 2, y);
        float full = pad + aw;
        int bw = 0;
        if (b)
            bw = OZ_VppFlex.TextWidth(b);
        if (b && bw > 0)
        {
            b.GetPos(x, y);
            b.SetPos(full + gap, y);
            b.GetSize(x, y);
            b.SetSize(bw + 2, y);
            full = full + gap + bw;
        }
        full = full + pad;
        OZT_VppKit.WidenBox(root, name, full);
        return full;
    }

    // A widget moved along its row, its height kept.
    static void MoveX(Widget w, float x)
    {
        if (!w)
            return;
        float ox;
        float oy;
        w.GetPos(ox, oy);
        w.SetPos(x, oy);
    }

    static void SetWidth(Widget w, float width)
    {
        if (!w)
            return;
        float ow;
        float oh;
        w.GetSize(ow, oh);
        if (width < 0)
            width = 0;
        w.SetSize(width, oh);
    }

    // How far a named list is scrolled down, and the same place given back to it after
    // its rows were made again.
    static float ScrollAt(Widget root, string name)
    {
        if (!root)
            return 0;
        ScrollWidget sc = ScrollWidget.Cast(root.FindAnyWidget(name));
        if (!sc)
            return 0;
        return sc.GetVScrollPos();
    }

    static void ScrollTo(Widget root, string name, float at)
    {
        if (!root)
            return;
        ScrollWidget sc = ScrollWidget.Cast(root.FindAnyWidget(name));
        if (!sc)
            return;
        sc.Update();
        sc.VScrollToPos(at);
    }

    // Where a widget ends on its row: its place plus its width.
    static float EndOf(Widget w)
    {
        if (!w)
            return 0;
        float x;
        float y;
        float ww;
        float hh;
        w.GetPos(x, y);
        w.GetSize(ww, hh);
        return x + ww;
    }

    // "a|b |c" -> the values, trimmed, the empty ones left out.
    static void Alts(string arg, array<string> into)
    {
        into.Clear();
        array<string> parts = new array<string>();
        arg.Split("|", parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            string one = parts[i].Trim();
            if (one != "" && into.Find(one) == -1)
                into.Insert(one);
        }
    }

    static string Joined(array<string> items, string sep)
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

    // Lower case for a filter: the core window's OZ_VppFold, because the engine's ToLower
    // turns every Cyrillic byte into a space (a Cyrillic word then matched any word as long).
    static string Low(string s)
    {
        return OZ_VppFold.Lower(s);
    }

    // Does a text hold a filter's words? `low` is in lower case already.
    static bool Holds(string text, string low)
    {
        return OZ_VppFold.Lower(text).IndexOf(low) != -1;
    }

    // Only digits, and at least one?
    static bool Digits(string s)
    {
        int n = s.Length();
        if (n == 0)
            return false;
        for (int i = 0; i < n; i++)
        {
            int c = s.Get(i).ToAscii();
            if (c < 48 || c > 57)
                return false;
        }
        return true;
    }

    // A provider's label of the shape "name - branch (owner, owner) - T<tier>" taken
    // apart (the research gate writes its nodes so); false, and the label as the name,
    // for any other. The cuts fall on ASCII signs, so byte offsets are safe.
    static bool LabelParts(string label, out string name, out string branch, out string owners, out string tier)
    {
        name = label;
        branch = "";
        owners = "";
        tier = "";
        int t = label.LastIndexOf(" - T");
        if (t <= 0)
            return false;
        string tail = label.Substring(t + 4, label.Length() - t - 4);
        if (!OZT_VppKit.Digits(tail))
            return false;
        string head = label.Substring(0, t);
        int b = head.LastIndexOf(" - ");
        if (b <= 0)
            return false;
        name = head.Substring(0, b);
        branch = head.Substring(b + 3, head.Length() - b - 3);
        tier = tail;
        int n = branch.Length();
        if (n > 3 && branch.Substring(n - 1, 1) == ")")
        {
            int open = branch.LastIndexOf(" (");
            if (open > 0)
            {
                owners = branch.Substring(open + 2, n - open - 3);
                branch = branch.Substring(0, open);
            }
        }
        return true;
    }
}

modded class OZ_VppAdminMenu
{
    static const string TRD_GT_L = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_gates_";

    // The row templates of the middle list, in the order of m_TrdGtMid.
    static const int TRD_GT_SEC   = 0;
    static const int TRD_GT_COND  = 1;
    static const int TRD_GT_ADD   = 2;
    static const int TRD_GT_LFOOT = 3;
    static const int TRD_GT_DISC  = 4;
    static const int TRD_GT_INH   = 5;
    static const int TRD_GT_DADD  = 6;
    static const int TRD_GT_DFOOT = 7;
    static const int TRD_GT_LOOP  = 8;

    // The most rows the left list and the picker make; a search narrows them.
    static const int TRD_GT_LIST_MAX = 300;
    static const int TRD_GT_PICK_MAX = 200;

    // The picked target: "" none, "c|<category>", "l|<category>|<class>", "p|<point>".
    // Whether the list shows every line or those with conditions; the targets edited
    // since the draft was last clean; the point and the first family of the strip.
    protected string m_TrdGtTgt;
    protected bool   m_TrdGtAll;
    protected ref array<string> m_TrdGtTouched;
    protected string m_TrdGtFamAt;
    protected int    m_TrdGtFamFrom;

    // The left list: groups, lines and lines with a warning, three templates in one list.
    protected ref OZ_VppFlexRows m_TrdGtGroups;
    protected ref OZ_VppFlexRows m_TrdGtLines;
    protected ref OZ_VppFlexRows m_TrdGtWarns;

    // The middle list: a FlexRows per template (TRD_GT_SEC ..), all in one list. The
    // kinds the add row's buttons stand for, and where that row's window of kinds begins;
    // the target the rows were last made for.
    protected ref array<ref OZ_VppFlexRows> m_TrdGtMid;
    protected ref array<string> m_TrdGtAddKinds;
    protected int m_TrdGtKindFrom;
    protected string m_TrdGtMidOf;

    // The picker: whether a condition is open in it, and which -- the list (0 the lock,
    // 1 a discount's When, 2 a discount's By), the discount (-1: a new one), the
    // condition (-1: a new one); the values chosen; the owner and the branch the list is
    // narrowed to; its three lists and the chips of what is chosen.
    protected bool m_TrdGtPkOn;
    protected int  m_TrdGtPkSec;
    protected int  m_TrdGtPkDisc;
    protected int  m_TrdGtPkCond;
    protected ref array<string> m_TrdGtPkVals;
    protected string m_TrdGtPkOwner;
    protected string m_TrdGtPkBranch;
    protected ref OZ_VppFlexRows m_TrdGtPkOwnRows;
    protected ref OZ_VppFlexRows m_TrdGtPkBrRows;
    protected ref OZ_VppFlexRows m_TrdGtPkValRows;
    protected ref OZT_VppChips m_TrdGtPkChips;

    // Who holds what (`gate_holders`), by "kind|arg": 0 asked, 1 known, 2 nobody answers
    // the kind; the words to show; the questions waiting, one in flight at a time (which,
    // and since when).
    protected ref array<string> m_TrdGtHoldKey;
    protected ref array<int>    m_TrdGtHoldState;
    protected ref array<string> m_TrdGtHoldSaid;
    protected ref array<string> m_TrdGtHoldQ;
    protected bool m_TrdGtHoldBusy;
    protected string m_TrdGtHoldSent;
    protected int m_TrdGtHoldAt;

    // The conditions copied from a target: its lock and its discounts.
    protected ref array<ref OZT_Condition> m_TrdGtClipLock;
    protected ref array<ref OZT_Discount>  m_TrdGtClipDisc;

    // The script is writing a field itself. The frames left on which what the page
    // places by its words is placed again (TrdGtReflow), and whether that is going on:
    // a placing pass writes no edit box.
    protected bool m_TrdGtPut;
    protected int  m_TrdGtFlowDue;
    protected bool m_TrdGtFlowing;

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "gates")
            return;
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdNeedGates();
        TrdGtPaint();
        TrdGtPlaceTitle();
        m_TrdGtFlowDue = 3;
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "gates")
            TrdGtPaint();
    }

    // The window changed its size: the lists follow it. The left list and the picker keep
    // their widths, the middle one stretches; a resize puts a row's stretched parts back
    // where its layout has them, so what stands by its words is placed again.
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (!M_SUB_WIDGET || TrdShownKey() != "gates")
            return;
        TrdGtPlaceTitle();
        if (!TrdGtLoaded())
            return;
        int i;
        if (m_TrdGtGroups)
        {
            m_TrdGtGroups.Fit();
            m_TrdGtLines.Fit();
            m_TrdGtWarns.Fit();
        }
        if (m_TrdGtMid)
        {
            for (i = 0; i < m_TrdGtMid.Count(); i++)
                m_TrdGtMid[i].Fit();
        }
        if (m_TrdGtPkOwnRows)
            m_TrdGtPkOwnRows.Fit();
        if (m_TrdGtPkBrRows)
            m_TrdGtPkBrRows.Fit();
        if (m_TrdGtPkValRows)
            m_TrdGtPkValRows.Fit();
        TrdGtReflow();
        m_TrdGtFlowDue = 3;
    }

    // Every frame while the window is open. What the page places by its words is placed
    // again on the frames after a paint: a row made on this frame may not measure its
    // texts until the engine has laid it out (the core's own pages do the same).
    override void Tick()
    {
        super.Tick();
        if (m_TrdGtFlowDue <= 0)
            return;
        m_TrdGtFlowDue--;
        if (!M_SUB_WIDGET || TrdShownKey() != "gates")
            return;
        TrdGtPlaceTitle();
        if (TrdGtLoaded())
            TrdGtReflow();
    }

    // The page's title as wide as its words, and the chip of the glue 12 units after it,
    // as wide as its own words, up to 12 units before the search box: the layout drew both
    // for the Ukrainian title, and the English one is longer.
    protected void TrdGtPlaceTitle()
    {
        TextWidget title = TextWidget.Cast(TrdW("TrdGtTitle"));
        TextWidget words = TextWidget.Cast(TrdW("TrdGtChipText"));
        Widget chip = TrdW("TrdGtChip");
        Widget find = TrdW("TrdGtFindBox");
        if (!title || !words || !chip || !find)
            return;
        int tw = OZ_VppFlex.TextWidth(title);
        if (tw <= 0)
            return;
        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        title.GetPos(x, y);
        title.GetSize(w, h);
        title.SetSize(tw + 2, h);
        float at = x + tw + 12 * s;
        float fx;
        float fy;
        find.GetPos(fx, fy);
        float room = fx - 12 * s - at;
        TrdHugPill(chip, words, at, room, 10 * s);
    }

    // Everything of the page that stands by the length of its words, placed: nothing is
    // made or read here, only measured and moved.
    protected void TrdGtReflow()
    {
        m_TrdGtFlowing = true;
        TrdGtPlaceTitle();
        TrdGtPlaceFams();
        TrdGtPlacePills();
        TrdGtPlaceList();
        TrdGtPlaceHead();
        if (m_TrdGtMid && m_TrdGtTgt != "")
            TrdGtFillMid();
        TrdGtPkPlaceValues();
        TrdGtPkPlaceChosen();
        m_TrdGtFlowing = false;
    }

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        super.TrdOnAnswer(op, ok, json, error);
        if (op != "gate_holders")
            return;
        m_TrdGtHoldBusy = false;
        if (!ok)
        {
            // The server refused, and the journal has its words: the questions that wait
            // would be refused alike. They are dropped, and may all be asked again.
            TrdGtHoldForgetOne(m_TrdGtHoldSent);
            TrdGtHoldDrop();
            return;
        }
        OZT_AdminGateHolders parsed = new OZT_AdminGateHolders();
        string err;
        if (JsonFileLoader<OZT_AdminGateHolders>.LoadData(json, parsed, err))
        {
            OZT_AdminGateHolders h = parsed.Copy();
            TrdOnHolders(h);
        }
        TrdGtHoldPump();
    }

    // Who holds a value has come: kept, and the page that shows it paints it. A later
    // layer (Research > Trade) takes the answer for its own view too.
    protected void TrdOnHolders(OZT_AdminGateHolders h)
    {
        TrdGtHoldKeep(h);
        if (TrdShownKey() == "gates")
            TrdGtPkPaintState(h.Kind, h.Arg);
    }

    // ------------------------------------------------------------------ who holds what

    protected void TrdGtHoldInit()
    {
        if (m_TrdGtHoldKey)
            return;
        m_TrdGtHoldKey = new array<string>();
        m_TrdGtHoldState = new array<int>();
        m_TrdGtHoldSaid = new array<string>();
        m_TrdGtHoldQ = new array<string>();
    }

    protected void TrdGtHoldKeep(OZT_AdminGateHolders h)
    {
        TrdGtHoldInit();
        string key = h.Kind + "|" + h.Arg;
        int at = m_TrdGtHoldKey.Find(key);
        if (at == -1)
        {
            at = m_TrdGtHoldKey.Insert(key);
            m_TrdGtHoldState.Insert(0);
            m_TrdGtHoldSaid.Insert("");
        }
        string said = OZT_VppKit.Joined(h.Who, ", ");
        if (h.Cut == 1)
            said = said + " ...";
        int state = 1;
        if (h.Answers != 1)
            state = 2;
        m_TrdGtHoldState.Set(at, state);
        m_TrdGtHoldSaid.Set(at, said);
    }

    // Ask who holds `kind arg` unless it is known or asked already. The condition's
    // alternatives travel in `arg` as the file has them ("a|b": any of them).
    protected void TrdGtHoldWant(string kind, string arg)
    {
        TrdGtHoldInit();
        if (kind == "" || arg == "")
            return;
        string key = kind + "|" + arg;
        if (m_TrdGtHoldKey.Find(key) != -1)
            return;
        m_TrdGtHoldKey.Insert(key);
        m_TrdGtHoldState.Insert(0);
        m_TrdGtHoldSaid.Insert("");
        m_TrdGtHoldQ.Insert(key);
        TrdGtHoldPump();
    }

    // The next question to the server, one in flight at a time. An answer that has not
    // come in ten seconds is given up: its question may be asked again.
    protected void TrdGtHoldPump()
    {
        if (!m_TrdGtHoldQ || m_TrdGtHoldQ.Count() == 0)
            return;
        int now = GetGame().GetTime();
        if (m_TrdGtHoldBusy)
        {
            if (now - m_TrdGtHoldAt < 10000)
                return;
            TrdGtHoldForgetOne(m_TrdGtHoldSent);
        }
        string key = m_TrdGtHoldQ[0];
        m_TrdGtHoldQ.RemoveOrdered(0);
        int bar = key.IndexOf("|");
        if (bar <= 0)
            return;
        OZT_AdminGateAsk ask = new OZT_AdminGateAsk();
        ask.Kind = key.Substring(0, bar);
        ask.Arg = key.Substring(bar + 1, key.Length() - bar - 1);
        string body;
        string err;
        JsonFileLoader<OZT_AdminGateAsk>.MakeData(ask, body, err, false);
        m_TrdGtHoldBusy = true;
        m_TrdGtHoldSent = key;
        m_TrdGtHoldAt = now;
        Ask(OZT_Const.SECTION_ADMIN, "gate_holders", body);
    }

    protected void TrdGtHoldForgetOne(string key)
    {
        if (!m_TrdGtHoldKey || key == "")
            return;
        int at = m_TrdGtHoldKey.Find(key);
        if (at == -1 || m_TrdGtHoldState[at] != 0)
            return;
        m_TrdGtHoldKey.RemoveOrdered(at);
        m_TrdGtHoldState.RemoveOrdered(at);
        m_TrdGtHoldSaid.RemoveOrdered(at);
    }

    // The questions not sent yet are forgotten -- what they asked about is off the
    // screen -- and may be asked again.
    protected void TrdGtHoldDrop()
    {
        if (!m_TrdGtHoldQ)
            return;
        for (int i = 0; i < m_TrdGtHoldQ.Count(); i++)
            TrdGtHoldForgetOne(m_TrdGtHoldQ[i]);
        m_TrdGtHoldQ.Clear();
    }

    // 0 not known yet, 1 known, 2 nobody answers the kind; `said` the holders' names
    // ("" when nobody holds it).
    protected int TrdGtHoldOf(string kind, string arg, out string said)
    {
        said = "";
        if (!m_TrdGtHoldKey)
            return 0;
        int at = m_TrdGtHoldKey.Find(kind + "|" + arg);
        if (at == -1)
            return 0;
        said = m_TrdGtHoldSaid[at];
        return m_TrdGtHoldState[at];
    }

    // Everything asked again from now on (a refresh).
    protected void TrdGtHoldForget()
    {
        if (!m_TrdGtHoldKey)
            return;
        m_TrdGtHoldKey.Clear();
        m_TrdGtHoldState.Clear();
        m_TrdGtHoldSaid.Clear();
        m_TrdGtHoldQ.Clear();
    }

    // ------------------------------------------------------------------ what the files hold

    protected bool TrdGtLoaded()
    {
        if (!m_TrdDraft)
            return false;
        bool goods = m_TrdDraft.Loaded(OZT_Const.CFG_GOODS);
        bool points = m_TrdDraft.Loaded(OZT_Const.CFG_TRADERS);
        bool settings = m_TrdDraft.Loaded(OZT_Const.CFG_SETTINGS);
        return goods && points && settings;
    }

    protected OZT_Category TrdGtCatById(string id)
    {
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        for (int i = 0; i < cats.Count(); i++)
        {
            if (cats[i].Id == id)
                return cats[i];
        }
        return null;
    }

    protected OZT_Goods TrdGtLineOf(OZT_Category cat, string cls)
    {
        if (!cat || !cat.Goods)
            return null;
        for (int i = 0; i < cat.Goods.Count(); i++)
        {
            if (cat.Goods[i].ClassName == cls)
                return cat.Goods[i];
        }
        return null;
    }

    protected OZT_Point TrdGtPointById(string id)
    {
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            if (points[i].Id == id)
                return points[i];
        }
        return null;
    }

    // The picked target taken apart: its kind ("c", "l", "p", "" none), the category's
    // or the point's id, the line's class.
    protected void TrdGtTgtParts(out string kind, out string a, out string b)
    {
        kind = "";
        a = "";
        b = "";
        if (m_TrdGtTgt == "")
            return;
        array<string> p = new array<string>();
        m_TrdGtTgt.Split("|", p);
        if (p.Count() > 0)
            kind = p[0];
        if (p.Count() > 1)
            a = p[1];
        if (p.Count() > 2)
            b = p[2];
    }

    // Does the picked target still stand in the files? A discard or a fresh read may
    // have taken it away: then nothing is picked.
    protected void TrdGtCheckTgt()
    {
        string kind;
        string a;
        string b;
        TrdGtTgtParts(kind, a, b);
        bool there = false;
        if (kind == "c" && TrdGtCatById(a))
            there = true;
        if (kind == "l" && TrdGtLineOf(TrdGtCatById(a), b))
            there = true;
        if (kind == "p" && TrdGtPointById(a))
            there = true;
        if (!there && m_TrdGtTgt != "")
        {
            m_TrdGtTgt = "";
            TrdGtPkClose();
        }
    }

    // The file the picked target lives in.
    protected string TrdGtFile()
    {
        if (m_TrdGtTgt.IndexOf("p|") == 0)
            return OZT_Const.CFG_TRADERS;
        return OZT_Const.CFG_GOODS;
    }

    // The picked target's lock: a category's or a line's Requires, a point's Access;
    // made when the file had none and `make` says so. Null when nothing is picked.
    protected array<ref OZT_Condition> TrdGtLock(bool make)
    {
        string kind;
        string a;
        string b;
        TrdGtTgtParts(kind, a, b);
        if (kind == "c")
        {
            OZT_Category cat = TrdGtCatById(a);
            if (!cat)
                return null;
            if (!cat.Requires && make)
                cat.Requires = new array<ref OZT_Condition>();
            return cat.Requires;
        }
        if (kind == "l")
        {
            OZT_Goods e = TrdGtLineOf(TrdGtCatById(a), b);
            if (!e)
                return null;
            if (!e.Requires && make)
                e.Requires = new array<ref OZT_Condition>();
            return e.Requires;
        }
        if (kind == "p")
        {
            OZT_Point p = TrdGtPointById(a);
            if (!p)
                return null;
            if (!p.Access && make)
                p.Access = new array<ref OZT_Condition>();
            return p.Access;
        }
        return null;
    }

    // The picked target's discounts: a category's or a line's. Null otherwise.
    protected array<ref OZT_Discount> TrdGtDiscs(bool make)
    {
        string kind;
        string a;
        string b;
        TrdGtTgtParts(kind, a, b);
        if (kind == "c")
        {
            OZT_Category cat = TrdGtCatById(a);
            if (!cat)
                return null;
            if (!cat.Discounts && make)
                cat.Discounts = new array<ref OZT_Discount>();
            return cat.Discounts;
        }
        if (kind == "l")
        {
            OZT_Goods e = TrdGtLineOf(TrdGtCatById(a), b);
            if (!e)
                return null;
            if (!e.Discounts && make)
                e.Discounts = new array<ref OZT_Discount>();
            return e.Discounts;
        }
        return null;
    }

    protected int TrdGtCount(array<ref OZT_Condition> conds)
    {
        if (!conds)
            return 0;
        return conds.Count();
    }

    protected int TrdGtDiscCount(array<ref OZT_Discount> discs)
    {
        if (!discs)
            return 0;
        return discs.Count();
    }

    // The most a list of discounts can ever give, in percent; `several` when it takes
    // more than one fixed discount to say so (then the list reads "up to").
    protected float TrdGtMost(array<ref OZT_Discount> discs, out bool several)
    {
        several = false;
        float most = 0;
        if (!discs)
            return 0;
        for (int i = 0; i < discs.Count(); i++)
        {
            OZT_Discount d = discs[i];
            if (!d)
                continue;
            if (d.By)
                several = true;
            if (d.Most() > most)
                most = d.Most();
        }
        if (discs.Count() > 1)
            several = true;
        if (most > 100)
            most = 100;
        return most;
    }

    // "-15 %", or "up to -20 %".
    protected string TrdGtCutWords(array<ref OZT_Discount> discs)
    {
        bool several;
        float most = TrdGtMost(discs, several);
        string words = "−" + TrdNum(most) + " %";
        if (several)
        {
            string upTo = TrdTr("#STR_OZT_VPP_GT_UP_TO");
            words = upTo + " " + words;
        }
        return words;
    }

    // A family as the admin reads it in a list ("factions" -> "фракції"); another mod's
    // family by its own word.
    protected string TrdGtFamWord(string family)
    {
        if (family == "factions")
            return TrdTr("#STR_OZT_VPP_GT_FAML_FACTIONS");
        if (family == "licences")
            return TrdTr("#STR_OZT_VPP_GT_FAML_LICENCES");
        if (family == "research")
            return TrdTr("#STR_OZT_VPP_GT_FAML_RESEARCH");
        return family;
    }

    // ... and as a caption ("Фракції").
    protected string TrdGtFamName(string family)
    {
        if (family == "factions")
            return TrdTr("#STR_OZT_VPP_GT_FAMN_FACTIONS");
        if (family == "licences")
            return TrdTr("#STR_OZT_VPP_GT_FAMN_LICENCES");
        if (family == "research")
            return TrdTr("#STR_OZT_VPP_GT_FAMN_RESEARCH");
        return family;
    }

    // A condition kind as a chip names it; a kind of another mod as it is.
    protected string TrdGtKindName(string kind)
    {
        if (kind == "faction")
            return TrdTr("#STR_OZT_VPP_GT_K_FACTION");
        if (kind == "post")
            return TrdTr("#STR_OZT_VPP_GT_K_POST");
        if (kind == "leader")
            return TrdTr("#STR_OZT_VPP_GT_K_LEADER");
        if (kind == "licence")
            return TrdTr("#STR_OZT_UI_LICENCE");
        if (kind == "research")
            return TrdTr("#STR_OZT_VPP_GT_K_RESEARCH");
        return kind;
    }

    // The words of the families a list of conditions leans on, each once, into `into`:
    // the family of a kind somebody answers, the kind itself when nobody does. Returns
    // whether every condition of the list is of a kind nobody answers (such a lock passes).
    protected bool TrdGtFamsOf(array<ref OZT_Condition> conds, array<string> into)
    {
        bool allLoose = true;
        if (!conds)
            return false;
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c)
                continue;
            string family = TrdFamilyOf(c.Kind);
            string word = c.Kind;
            if (family != "")
            {
                word = TrdGtFamWord(family);
                allLoose = false;
            }
            if (into.Find(word) == -1)
                into.Insert(word);
        }
        if (conds.Count() == 0 || !m_TrdGates)
            return false;
        return allLoose;
    }

    protected void TrdGtFamsOfDiscs(array<ref OZT_Discount> discs, array<string> into)
    {
        if (!discs)
            return;
        for (int i = 0; i < discs.Count(); i++)
        {
            OZT_Discount d = discs[i];
            if (!d)
                continue;
            TrdGtFamsOf(d.When, into);
            if (d.By)
            {
                array<ref OZT_Condition> by = new array<ref OZT_Condition>();
                by.Insert(d.By);
                TrdGtFamsOf(by, into);
            }
        }
    }

    // A value of a kind that names its choices in full, which is not among them: the
    // lock with it never opens. "" when the list has no such value, or cannot tell.
    protected string TrdGtStray(array<ref OZT_Condition> conds)
    {
        if (!conds)
            return "";
        array<string> alts = new array<string>();
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c)
                continue;
            OZT_AdminGateKind gk = TrdGateKind(c.Kind);
            if (!gk || gk.Cut == 1 || gk.Values.Count() == 0)
                continue;
            OZT_VppKit.Alts(c.Arg, alts);
            for (int a = 0; a < alts.Count(); a++)
            {
                string value = alts[a];
                // a faction may name its axis: "org:<id>", "base:<id>"
                if (c.Kind == "faction" && value.IndexOf("org:") == 0)
                    value = value.Substring(4, value.Length() - 4);
                else if (c.Kind == "faction" && value.IndexOf("base:") == 0)
                    value = value.Substring(5, value.Length() - 5);
                if (gk.Values.Find(value) == -1)
                    return alts[a];
            }
        }
        return "";
    }

    protected string TrdGtStrayOfDiscs(array<ref OZT_Discount> discs)
    {
        if (!discs)
            return "";
        for (int i = 0; i < discs.Count(); i++)
        {
            OZT_Discount d = discs[i];
            if (!d)
                continue;
            string stray = TrdGtStray(d.When);
            if (stray != "")
                return stray;
            if (d.By)
            {
                array<ref OZT_Condition> by = new array<ref OZT_Condition>();
                by.Insert(d.By);
                stray = TrdGtStray(by);
                if (stray != "")
                    return stray;
            }
        }
        return "";
    }

    // Do a list's conditions hold the search's words (kind, value, or a value's label)?
    protected bool TrdGtCondsHave(array<ref OZT_Condition> conds, string low)
    {
        if (!conds)
            return false;
        array<string> alts = new array<string>();
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c)
                continue;
            if (OZT_VppKit.Holds(c.Kind, low) || OZT_VppKit.Holds(c.Arg, low))
                return true;
            OZT_VppKit.Alts(c.Arg, alts);
            for (int a = 0; a < alts.Count(); a++)
            {
                if (OZT_VppKit.Holds(TrdChoiceLabel(c.Kind, alts[a]), low))
                    return true;
            }
        }
        return false;
    }

    protected bool TrdGtDiscsHave(array<ref OZT_Discount> discs, string low)
    {
        if (!discs)
            return false;
        for (int i = 0; i < discs.Count(); i++)
        {
            OZT_Discount d = discs[i];
            if (!d)
                continue;
            if (TrdGtCondsHave(d.When, low))
                return true;
            if (d.By)
            {
                array<ref OZT_Condition> by = new array<ref OZT_Condition>();
                by.Insert(d.By);
                if (TrdGtCondsHave(by, low))
                    return true;
            }
        }
        return false;
    }

    // ------------------------------------------------------------------ edits are remembered

    // An edit of the picked target: its file is the draft's now, and the target is
    // marked as changed until the draft is clean again.
    protected void TrdGtTouched()
    {
        if (!m_TrdGtTouched)
            m_TrdGtTouched = new array<string>();
        if (m_TrdGtTgt != "" && m_TrdGtTouched.Find(m_TrdGtTgt) == -1)
            m_TrdGtTouched.Insert(m_TrdGtTgt);
        TrdTouch(TrdGtFile());
    }

    protected bool TrdGtIsTouched(string key)
    {
        if (!m_TrdGtTouched)
            return false;
        return m_TrdGtTouched.Find(key) != -1;
    }

    // ------------------------------------------------------------------ painting

    protected void TrdGtPaint()
    {
        if (!M_SUB_WIDGET)
            return;
        if (!TrdGtLoaded())
        {
            SayTrd("TrdGtSub", "#STR_OZT_VPP_READING");
            return;
        }
        SayTrd("TrdGtSub", "#STR_OZT_VPP_GT_SUB");
        // A clean draft has no changed targets.
        if (m_TrdGtTouched && m_TrdDraft.Changed() == 0)
            m_TrdGtTouched.Clear();
        TrdGtCheckTgt();
        TrdGtPaintFams();
        TrdGtPaintList();
        TrdGtPaintHead();
        TrdGtPaintConds();
        TrdGtPaintPicker();
        m_TrdGtFlowDue = 3;
    }

    // ------------------------------------------------------------------ the families' strip

    // The point the strip's switches belong to: the picked one, else the first.
    protected OZT_Point TrdGtFamPoint()
    {
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        if (points.Count() == 0)
            return null;
        OZT_Point p = TrdGtPointById(m_TrdGtFamAt);
        if (p)
            return p;
        m_TrdGtFamAt = points[0].Id;
        return points[0];
    }

    protected bool TrdGtServerOff(string family)
    {
        array<string> off = m_TrdDraft.Settings.GatesOff;
        if (!off)
            return false;
        return off.Find(family) != -1;
    }

    // Three families at a time: each with the server's switch and the point's own
    // (as the server, on here, off here); the slots stand one after another by the
    // length of their words.
    protected void TrdGtPaintFams()
    {
        array<string> families = new array<string>();
        TrdFamilies(families);
        if (m_TrdGtFamFrom >= families.Count() || families.Count() <= 3)
            m_TrdGtFamFrom = 0;

        OZT_Point p = TrdGtFamPoint();
        string caption = TrdTr("#STR_OZT_VPP_GT_NO_POINTS");
        if (p)
            caption = p.Name + " (" + p.Id + ")";
        SayTrd("TrdGtPointText", caption);

        string onWord = TrdTr("#STR_OZT_VPP_GT_FAM_ON");
        string offWord = TrdTr("#STR_OZT_VPP_GT_FAM_OFF");
        string serverWord = TrdTr("#STR_OZT_VPP_RS_SERVER");
        Widget slot;
        for (int n = 0; n < 3; n++)
        {
            string id = "TrdGtFam" + n.ToString();
            slot = M_SUB_WIDGET.FindAnyWidget(id);
            if (!slot)
                continue;
            int at = m_TrdGtFamFrom + n;
            if (at >= families.Count())
            {
                slot.Show(false);
                continue;
            }
            slot.Show(true);
            string fam = families[at];
            bool srvOff = TrdGtServerOff(fam);
            SayTrd(id + "Name", TrdGtFamName(fam));
            SayTrd(id + "Word", fam + " · " + serverWord);
            if (srvOff)
                TrdInk(id + "SrvText", offWord, OZT_Metal.ALERT);
            else
                TrdInk(id + "SrvText", onWord, OZT_Metal.OK);

            bool here = false;
            bool hereOn = false;
            if (p && p.GatesOn && p.GatesOn.Find(fam) != -1)
            {
                here = true;
                hereOn = true;
            }
            else if (p && p.GatesOff && p.GatesOff.Find(fam) != -1)
                here = true;
            bool asServer = false;
            if (p && !here)
                asServer = true;
            bool isOn = here && hereOn;
            bool isOff = here && !hereOn;
            OZT_VppKit.SegPaint(M_SUB_WIDGET, "Btn" + id + "As", asServer);
            OZT_VppKit.SegPaint(M_SUB_WIDGET, "Btn" + id + "On", isOn);
            OZT_VppKit.SegPaint(M_SUB_WIDGET, "Btn" + id + "Off", isOff);
        }

        // More families than slots: the button steps through them.
        Widget more = M_SUB_WIDGET.FindAnyWidget("BtnTrdGtFamMore");
        if (more)
        {
            more.Show(families.Count() > 3);
            int rest = families.Count() - 3;
            string moreWord = TrdTr("#STR_OZT_VPP_RS_MORE");
            SayTrd("BtnTrdGtFamMoreText", moreWord + " " + rest.ToString() + " ›");
        }
        TrdGtPlaceFams();
        m_TrdGtFlowDue = 3;

        // Who answers today, and where a family is switched off.
        string answer = TrdTr("#STR_OZT_VPP_GT_ANSWER");
        string who = "";
        if (!m_TrdGates)
            who = "...";
        else
        {
            for (int f = 0; f < m_TrdGates.Families.Count(); f++)
            {
                if (m_TrdGates.Families[f].Answers != 1)
                    continue;
                if (who != "")
                    who = who + ", ";
                who = who + m_TrdGates.Families[f].Family;
            }
            if (who == "")
                who = TrdTr("#STR_OZT_VPP_GT_ANSWER_NONE");
        }
        TrdInk("TrdGtAnswer", answer + " " + who, OZT_Metal.MUTED);
        TrdGtPaintOffLine();
    }

    // The slots of the strip one after another: in each the name, the family's word and
    // the server's switch stand by their words' lengths, and a slot is as wide as its
    // widest line (its three segments are 184 units).
    protected void TrdGtPlaceFams()
    {
        float s = UnitScale();
        float x = 0;
        float y = 0;
        float w = 0;
        float h = 0;
        float top = 0;
        float tw = 0;
        string id;
        Widget slot;
        Widget srv;
        Widget first = M_SUB_WIDGET.FindAnyWidget("TrdGtFam0");
        if (!first)
            return;
        first.GetPos(x, y);
        for (int n = 0; n < 3; n++)
        {
            id = "TrdGtFam" + n.ToString();
            slot = M_SUB_WIDGET.FindAnyWidget(id);
            if (!slot || !slot.IsVisible())
                continue;
            tw = OZT_VppKit.FitWord(slot, id + "Name");
            top = tw + 6 * s;
            OZT_VppKit.MoveX(slot.FindAnyWidget(id + "Word"), top);
            tw = OZT_VppKit.FitWord(slot, id + "Word");
            top = top + tw + 6 * s;
            srv = slot.FindAnyWidget("Btn" + id + "Srv");
            OZT_VppKit.MoveX(srv, top);
            tw = OZT_VppKit.FitWord(slot, id + "SrvText");
            OZT_VppKit.SetWidth(srv, tw + 2);
            top = top + tw + 2;
            if (top < 186 * s)
                top = 186 * s;
            slot.GetSize(w, h);
            slot.SetSize(top, h);
            slot.SetPos(x, y);
            x = x + top + 22 * s;
        }
        Widget more = M_SUB_WIDGET.FindAnyWidget("BtnTrdGtFamMore");
        if (more && more.IsVisible())
        {
            OZT_VppKit.HugBox(M_SUB_WIDGET, "BtnTrdGtFamMore", "BtnTrdGtFamMoreText", 8 * s);
            OZT_VppKit.MoveX(more, x);
        }
    }

    protected void TrdGtPaintOffLine()
    {
        array<string> off = m_TrdDraft.Settings.GatesOff;
        if (off && off.Count() > 0)
        {
            string srvHead = TrdTr("#STR_OZT_VPP_GT_OFF_SRV");
            TrdInk("TrdGtWarn", srvHead + " " + OZT_VppKit.Joined(off, ", "), OZT_Metal.ALERT);
            return;
        }
        int n = 0;
        string firstOne = "";
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            OZT_Point pt = points[i];
            if (!pt.GatesOff)
                continue;
            for (int k = 0; k < pt.GatesOff.Count(); k++)
            {
                n++;
                if (firstOne == "")
                    firstOne = pt.Name + ": " + pt.GatesOff[k];
            }
        }
        if (n == 0)
        {
            TrdInk("TrdGtWarn", "#STR_OZT_VPP_GT_OFF_NONE", OZT_Metal.MUTED);
            return;
        }
        string head = TrdTr("#STR_OZT_VPP_GT_OFF_AT");
        TrdInk("TrdGtWarn", head + " " + n.ToString() + " · " + firstOne, OZT_Metal.ALERT);
    }

    // The server's switch of a family: off for every point, or on again.
    protected void TrdGtFamServer(int slot)
    {
        array<string> families = new array<string>();
        TrdFamilies(families);
        int at = m_TrdGtFamFrom + slot;
        if (at < 0 || at >= families.Count())
            return;
        array<string> off = m_TrdDraft.Settings.GatesOff;
        if (!off)
        {
            m_TrdDraft.Settings.GatesOff = new array<string>();
            off = m_TrdDraft.Settings.GatesOff;
        }
        int had = off.Find(families[at]);
        if (had != -1)
            off.RemoveOrdered(had);
        else
            off.Insert(families[at]);
        TrdTouch(OZT_Const.CFG_SETTINGS);
        TrdGtPaintFams();
    }

    // A family at the strip's point: 0 as the server, 1 on here, 2 off here.
    protected void TrdGtFamHere(int slot, int state)
    {
        array<string> families = new array<string>();
        TrdFamilies(families);
        int at = m_TrdGtFamFrom + slot;
        OZT_Point p = TrdGtFamPoint();
        if (at < 0 || at >= families.Count())
            return;
        if (!p)
        {
            Hint("#STR_OZT_VPP_GT_NO_POINTS");
            return;
        }
        string fam = families[at];
        if (!p.GatesOn)
            p.GatesOn = new array<string>();
        if (!p.GatesOff)
            p.GatesOff = new array<string>();
        int on = p.GatesOn.Find(fam);
        if (on != -1)
            p.GatesOn.RemoveOrdered(on);
        int off = p.GatesOff.Find(fam);
        if (off != -1)
            p.GatesOff.RemoveOrdered(off);
        if (state == 1)
            p.GatesOn.Insert(fam);
        else if (state == 2)
            p.GatesOff.Insert(fam);
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdGtPaintFams();
    }

    // The strip's point one on.
    protected void TrdGtFamNextPoint()
    {
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        if (points.Count() == 0)
        {
            Hint("#STR_OZT_VPP_GT_NO_POINTS");
            return;
        }
        int at = 0;
        for (int i = 0; i < points.Count(); i++)
        {
            if (points[i].Id == m_TrdGtFamAt)
                at = i + 1;
        }
        if (at >= points.Count())
            at = 0;
        m_TrdGtFamAt = points[at].Id;
        TrdGtPaintFams();
    }

    // ------------------------------------------------------------------ the left list

    protected void TrdGtMakeLists()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdGtListRows");
        if (!host || m_TrdGtGroups)
            return;
        m_TrdGtGroups = new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "group.layout", "OZT_GtGroup", 279, 28);
        m_TrdGtLines = new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "line.layout", "OZT_GtLine", 279, 50);
        m_TrdGtWarns = new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "linew.layout", "OZT_GtLineW", 279, 68);
    }

    // The categories with their lines, then the points. A search keeps the lines whose
    // class or conditions hold its words, and the categories and points named by it.
    // The list stays where the admin scrolled it: an edit rebuilds it.
    protected void TrdGtPaintList()
    {
        TrdGtMakeLists();
        if (!m_TrdGtGroups)
            return;
        float scrolled = OZT_VppKit.ScrollAt(M_SUB_WIDGET, "TrdGtListScroll");
        m_TrdGtGroups.Clear();
        m_TrdGtLines.Clear();
        m_TrdGtWarns.Clear();

        string find = OZT_VppKit.Low(GetEdit("TrdGtFind").Trim());
        int made = 0;
        int hidden = 0;
        int withConds = 0;
        int allLines = 0;
        int c;
        int g;
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        array<int> keep = new array<int>();
        for (c = 0; c < cats.Count(); c++)
        {
            OZT_Category cat = cats[c];
            keep.Clear();
            bool catNamed = find == "" || OZT_VppKit.Holds(cat.Name, find) || OZT_VppKit.Holds(cat.Id, find);
            if (cat.Goods)
            {
                for (g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_Goods e = cat.Goods[g];
                    allLines++;
                    bool has = TrdGtCount(e.Requires) > 0 || TrdGtDiscCount(e.Discounts) > 0;
                    if (has)
                        withConds++;
                    string key = "l|" + cat.Id + "|" + e.ClassName;
                    bool show = m_TrdGtAll || has || key == m_TrdGtTgt;
                    if (show && find != "" && !catNamed)
                    {
                        show = OZT_VppKit.Holds(e.ClassName, find);
                        if (!show)
                            show = TrdGtCondsHave(e.Requires, find);
                        if (!show)
                            show = TrdGtDiscsHave(e.Discounts, find);
                    }
                    if (show)
                        keep.Insert(g);
                }
            }
            bool catHas = TrdGtCondsHave(cat.Requires, find);
            if (!catHas)
                catHas = TrdGtDiscsHave(cat.Discounts, find);
            if (find != "" && !catNamed && !catHas && keep.Count() == 0)
                continue;
            TrdGtAddGroup(cat);
            for (g = 0; g < keep.Count(); g++)
            {
                if (made >= TRD_GT_LIST_MAX)
                {
                    hidden++;
                    continue;
                }
                TrdGtAddLine(cat, cat.Goods[keep[g]]);
                made++;
            }
        }
        if (hidden > 0)
        {
            string moreLines = TrdTr("#STR_OZT_VPP_GT_LIST_MORE");
            TrdGtAddNote(moreLines + " " + hidden.ToString());
        }

        // The points: who may trade at each.
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        bool headed = false;
        for (c = 0; c < points.Count(); c++)
        {
            OZT_Point pt = points[c];
            bool ptNamed = find == "" || OZT_VppKit.Holds(pt.Name, find) || OZT_VppKit.Holds(pt.Id, find);
            if (!ptNamed && !TrdGtCondsHave(pt.Access, find))
                continue;
            if (!headed)
            {
                headed = true;
                TrdGtAddPointsHead(points.Count());
            }
            TrdGtAddPoint(pt);
        }

        m_TrdGtGroups.Fit();
        m_TrdGtLines.Fit();
        m_TrdGtWarns.Fit();
        TrdGtPlaceList();
        OZT_VppKit.ScrollTo(M_SUB_WIDGET, "TrdGtListScroll", scrolled);

        string withWord = TrdTr("#STR_OZT_VPP_GT_F_WITH");
        string allWord = TrdTr("#STR_OZT_VPP_GT_F_ALL");
        SayTrd("BtnTrdGtWithText", withWord + " · " + withConds.ToString());
        SayTrd("BtnTrdGtAllText", allWord + " · " + allLines.ToString());
        OZT_VppKit.PillPaint(M_SUB_WIDGET, "BtnTrdGtWith", !m_TrdGtAll, OZT_Metal.MUTED);
        OZT_VppKit.PillPaint(M_SUB_WIDGET, "BtnTrdGtAll", m_TrdGtAll, OZT_Metal.MUTED);
        TrdGtPlacePills();
        m_TrdGtFlowDue = 3;
    }

    // The two filters over the list as wide as their words (each carries a count), the
    // second after the first.
    protected void TrdGtPlacePills()
    {
        float s = UnitScale();
        Widget first = M_SUB_WIDGET.FindAnyWidget("BtnTrdGtWith");
        Widget second = M_SUB_WIDGET.FindAnyWidget("BtnTrdGtAll");
        if (!first || !second)
            return;
        float w = OZT_VppKit.HugBox(M_SUB_WIDGET, "BtnTrdGtWith", "BtnTrdGtWithText", 12 * s);
        OZT_VppKit.HugBox(M_SUB_WIDGET, "BtnTrdGtAll", "BtnTrdGtAllText", 12 * s);
        float x;
        float y;
        first.GetPos(x, y);
        OZT_VppKit.MoveX(second, x + w + 6 * s);
    }

    // Every row of the list placed by its words.
    protected void TrdGtPlaceList()
    {
        if (!m_TrdGtGroups)
            return;
        int i;
        for (i = 0; i < m_TrdGtGroups.Count(); i++)
            TrdGtPlaceGroup(m_TrdGtGroups.Row(i));
        for (i = 0; i < m_TrdGtLines.Count(); i++)
            TrdGtPlaceLine(m_TrdGtLines.Row(i));
        for (i = 0; i < m_TrdGtWarns.Count(); i++)
            TrdGtPlaceLine(m_TrdGtWarns.Row(i));
    }

    // A category's row: its name, how many lines it holds, and what its own lock and
    // discounts are in a badge -- a lock nobody answers is drawn hollow: it passes.
    protected void TrdGtAddGroup(OZT_Category cat)
    {
        string key = "c|" + cat.Id;
        Widget row = m_TrdGtGroups.Add(key);
        if (!row)
            return;
        string name = cat.Name;
        if (name == "")
            name = cat.Id;
        int lines = 0;
        if (cat.Goods)
            lines = cat.Goods.Count();

        int locks = TrdGtCount(cat.Requires);
        int discs = TrdGtDiscCount(cat.Discounts);
        array<string> fams = new array<string>();
        string lockWord = TrdTr("#STR_OZT_VPP_GT_LOCK_N");
        string badge = "";
        int ink = OZT_Metal.TEXT;
        bool hollow = false;
        if (discs > 0)
        {
            TrdGtFamsOfDiscs(cat.Discounts, fams);
            badge = TrdGtCutWords(cat.Discounts);
            ink = OZT_Metal.ACCENT;
            if (locks > 0)
                badge = lockWord + " · " + badge;
            else if (fams.Count() > 0)
                badge = badge + " · " + OZT_VppKit.Joined(fams, ", ");
        }
        else if (locks > 0)
        {
            bool loose = TrdGtFamsOf(cat.Requires, fams);
            badge = lockWord + " · " + OZT_VppKit.Joined(fams, ", ");
            if (loose)
            {
                string passes = TrdTr("#STR_OZT_VPP_GT_PASSES");
                badge = badge + " — " + passes;
                ink = OZT_Metal.MUTED;
                hollow = true;
            }
        }
        TrdGtSetGroup(row, name, lines.ToString(), badge, ink, hollow);
        TrdGtMarkGroup(row, key == m_TrdGtTgt);
    }

    // A group row's words: its name, a count, and a badge ("" for none) in its colours.
    protected void TrdGtSetGroup(Widget row, string name, string count, string badge, int ink, bool hollow)
    {
        RowText(row, "GrpName", name, OZT_Metal.TEXT);
        RowText(row, "GrpCount", count, OZT_Metal.MUTED);
        OZT_VppKit.PartShow(row, "GrpBadge", badge != "");
        if (badge == "")
            return;
        RowText(row, "GrpBadgeText", badge, ink);
        if (hollow)
            OZT_VppKit.PartBox(row, "GrpBadge", OZT_Metal.EDGE, OZT_Metal.PANEL);
        else
            OZT_VppKit.PartBox(row, "GrpBadge", OZT_Metal.RAISED, OZT_Metal.RAISED);
    }

    // The badge at the row's right end as wide as its words, the count before it, the
    // name in what is left.
    protected void TrdGtPlaceGroup(Widget row)
    {
        if (!row)
            return;
        float s = UnitScale();
        float rw;
        float rh;
        float bw;
        float cw;
        float ch;
        float nx;
        float ny;
        float tw;
        row.GetSize(rw, rh);
        float right = rw - 9 * s;
        Widget box = row.FindAnyWidget("GrpBadge");
        if (box && box.IsVisible())
        {
            bw = OZT_VppKit.HugBox(row, "GrpBadge", "GrpBadgeText", 7 * s);
            // A long badge leaves the name its first letters at least.
            if (bw > rw - 90 * s)
            {
                bw = rw - 90 * s;
                OZT_VppKit.WidenBox(row, "GrpBadge", bw);
                OZT_VppKit.SetWidth(row.FindAnyWidget("GrpBadgeText"), bw - 14 * s);
            }
            OZT_VppKit.MoveX(box, right - bw);
            right = right - bw - 8 * s;
        }
        // The count is written from the right: its box ends where the badge begins.
        Widget count = row.FindAnyWidget("GrpCount");
        if (count)
        {
            count.GetSize(cw, ch);
            OZT_VppKit.MoveX(count, right - cw);
            tw = OZT_VppKit.TextW(row, "GrpCount");
            if (tw > 0)
                right = right - tw - 8 * s;
        }
        Widget name = row.FindAnyWidget("GrpName");
        if (!name)
            return;
        Widget cell = name.GetParent();
        if (!cell || cell == row)
            return;
        cell.GetPos(nx, ny);
        OZT_VppKit.SetWidth(cell, right - nx);
        OZT_VppKit.SetWidth(name, right - nx);
    }

    protected void TrdGtMarkGroup(Widget row, bool picked)
    {
        if (picked)
            OZT_VppKit.PartBox(row, "OZT_GtGroup", OZT_Metal.ACCENT, OZT_Metal.PICK);
        else
            OZT_VppKit.PartBox(row, "OZT_GtGroup", OZT_Metal.PANEL, OZT_Metal.PANEL);
    }

    // The caption over the points, and a note at the list's cut: rows of the group
    // template that pick nothing (their key is empty).
    protected void TrdGtAddPointsHead(int n)
    {
        Widget row = m_TrdGtGroups.Add("");
        if (!row)
            return;
        TrdGtSetGroup(row, "#STR_OZT_VPP_PAGE_POINTS", n.ToString(), "", OZT_Metal.TEXT, false);
        TrdGtMarkGroup(row, false);
    }

    protected void TrdGtAddNote(string text)
    {
        Widget row = m_TrdGtGroups.Add("");
        if (!row)
            return;
        TrdGtSetGroup(row, text, "", "", OZT_Metal.TEXT, false);
        OZT_VppKit.PartTint(row, "GrpName", OZT_Metal.MUTED);
        TrdGtMarkGroup(row, false);
    }

    // A line's row: its class, whether the draft changed it, its lock and its best
    // discount in pills, the families they lean on; with a warning under them when a
    // condition names a value its provider does not have.
    protected void TrdGtAddLine(OZT_Category cat, OZT_Goods e)
    {
        string key = "l|" + cat.Id + "|" + e.ClassName;
        string stray = TrdGtStray(e.Requires);
        if (stray == "")
            stray = TrdGtStrayOfDiscs(e.Discounts);
        Widget row;
        string root = "OZT_GtLine";
        if (stray != "")
        {
            row = m_TrdGtWarns.Add(key);
            root = "OZT_GtLineW";
        }
        else
            row = m_TrdGtLines.Add(key);
        if (!row)
            return;

        array<string> fams = new array<string>();
        TrdGtFamsOf(e.Requires, fams);
        TrdGtFamsOfDiscs(e.Discounts, fams);
        string lock = "";
        int locks = TrdGtCount(e.Requires);
        if (locks > 0)
        {
            string lockWord = TrdTr("#STR_OZT_VPP_GT_LOCK_N");
            lock = lockWord + " · " + locks.ToString();
        }
        string cut = "";
        if (TrdGtDiscCount(e.Discounts) > 0)
            cut = TrdGtCutWords(e.Discounts);
        string more = "";
        if (TrdGtIsTouched(key))
            more = TrdTr("#STR_OZT_VPP_GT_CHANGED_W");
        TrdGtSetLine(row, e.ClassName, more, OZT_Metal.ALERT, lock, cut, OZT_VppKit.Joined(fams, " · "), stray);
        TrdGtMarkLine(row, root, key == m_TrdGtTgt);
    }

    // A point's row, in the line's template: its name and id, how many conditions its
    // access has, the families they lean on.
    protected void TrdGtAddPoint(OZT_Point pt)
    {
        string key = "p|" + pt.Id;
        string stray = TrdGtStray(pt.Access);
        Widget row;
        string root = "OZT_GtLine";
        if (stray != "")
        {
            row = m_TrdGtWarns.Add(key);
            root = "OZT_GtLineW";
        }
        else
            row = m_TrdGtLines.Add(key);
        if (!row)
            return;
        array<string> fams = new array<string>();
        TrdGtFamsOf(pt.Access, fams);
        string lock = "";
        int n = TrdGtCount(pt.Access);
        if (n > 0)
        {
            string accessWord = TrdTr("#STR_OZT_VPP_RS_ACCESS");
            lock = accessWord + " · " + n.ToString();
        }
        int moreInk = OZT_Metal.MUTED;
        string more = pt.Id;
        if (TrdGtIsTouched(key))
        {
            more = TrdTr("#STR_OZT_VPP_GT_CHANGED_W");
            moreInk = OZT_Metal.ALERT;
        }
        string name = pt.Name;
        if (name == "")
            name = pt.Id;
        TrdGtSetLine(row, name, more, moreInk, lock, "", OZT_VppKit.Joined(fams, " · "), stray);
        TrdGtMarkLine(row, root, key == m_TrdGtTgt);
    }

    // A line row's words: its name, the mark at its right end, the two pills ("" hides
    // one), the families, and the value a warning names ("" in a row without one).
    protected void TrdGtSetLine(Widget row, string name, string more, int moreInk, string lock, string cut, string fams, string stray)
    {
        RowText(row, "LnClass", name, OZT_Metal.TEXT);
        RowText(row, "LnMore", more, moreInk);
        OZT_VppKit.PartShow(row, "LnLock", lock != "");
        OZT_VppKit.PartSay(row, "LnLockText", lock);
        OZT_VppKit.PartShow(row, "LnCut", cut != "");
        OZT_VppKit.PartSay(row, "LnCutText", cut);
        OZT_VppKit.PartSay(row, "LnFam", fams);
        if (stray == "")
            return;
        string noValue = TrdTr("#STR_OZT_VPP_GT_NO_VALUE");
        RowText(row, "LnWarn", noValue + " " + stray, OZT_Metal.ALERT);
    }

    // The two pills as wide as their words, one after the other, the families' words in
    // what is left of the row; the name in what the mark at the right end leaves.
    protected void TrdGtPlaceLine(Widget row)
    {
        if (!row)
            return;
        float s = UnitScale();
        float rw;
        float rh;
        float w;
        row.GetSize(rw, rh);
        float x = 17 * s;
        Widget box = row.FindAnyWidget("LnLock");
        if (box && box.IsVisible())
        {
            w = OZT_VppKit.HugBox(row, "LnLock", "LnLockText", 6 * s);
            OZT_VppKit.MoveX(box, x);
            x = x + w + 5 * s;
        }
        box = row.FindAnyWidget("LnCut");
        if (box && box.IsVisible())
        {
            w = OZT_VppKit.HugBox(row, "LnCut", "LnCutText", 6 * s);
            OZT_VppKit.MoveX(box, x);
            x = x + w + 5 * s;
        }
        float room = rw - 11 * s - x;
        Widget fam = row.FindAnyWidget("LnFam");
        Widget cell;
        if (fam)
        {
            cell = fam.GetParent();
            if (cell && cell != row)
            {
                OZT_VppKit.MoveX(cell, x);
                OZT_VppKit.SetWidth(cell, room);
            }
            OZT_VppKit.SetWidth(fam, room);
        }
        Widget name = row.FindAnyWidget("LnClass");
        if (!name)
            return;
        float right = rw - 11 * s;
        float mw = OZT_VppKit.TextW(row, "LnMore");
        if (mw > 0)
            right = right - mw - 8 * s;
        cell = name.GetParent();
        if (cell && cell != row)
            OZT_VppKit.SetWidth(cell, right - 17 * s);
        OZT_VppKit.SetWidth(name, right - 17 * s);
    }

    protected void TrdGtMarkLine(Widget row, string root, bool picked)
    {
        if (picked)
            OZT_VppKit.PartBox(row, root, OZT_Metal.ACCENT, OZT_Metal.PICK);
        else
            OZT_VppKit.PartBox(row, root, OZT_Metal.HEAD, OZT_Metal.HEAD);
    }

    // The picked row lit in all three kinds of rows, the rest plain.
    protected void TrdGtMarkList()
    {
        int i;
        if (!m_TrdGtGroups)
            return;
        for (i = 0; i < m_TrdGtGroups.Count(); i++)
            TrdGtMarkGroup(m_TrdGtGroups.Row(i), m_TrdGtGroups.Key(i) == m_TrdGtTgt && m_TrdGtTgt != "");
        for (i = 0; i < m_TrdGtLines.Count(); i++)
            TrdGtMarkLine(m_TrdGtLines.Row(i), "OZT_GtLine", m_TrdGtLines.Key(i) == m_TrdGtTgt);
        for (i = 0; i < m_TrdGtWarns.Count(); i++)
            TrdGtMarkLine(m_TrdGtWarns.Row(i), "OZT_GtLineW", m_TrdGtWarns.Key(i) == m_TrdGtTgt);
    }

    // A row of the list picked: its conditions in the middle. A condition open in the
    // picker belonged to the target before: it closes.
    protected void TrdGtPick(string key)
    {
        if (key == "" || key == m_TrdGtTgt)
            return;
        m_TrdGtTgt = key;
        TrdGtPkClose();
        if (key.IndexOf("p|") == 0)
            m_TrdGtFamAt = key.Substring(2, key.Length() - 2);
        TrdGtMarkList();
        TrdGtPaintFams();
        TrdGtPaintHead();
        TrdGtPaintConds();
        TrdGtPaintPicker();
    }

    // Another page asks for a target (Research > Trade leads here): the page is shown
    // with it picked.
    protected void TrdGtOpenAt(string key)
    {
        if (key != "")
        {
            m_TrdGtTgt = key;
            m_TrdGtAll = true;
            TrdGtPkClose();
        }
        ShowPane(TRD_MOD + ".gates");
    }

    // ------------------------------------------------------------------ the middle: head

    // The target's name, what it is beside it (the sub follows the name's length), and
    // whether the draft changed it.
    protected void TrdGtPaintHead()
    {
        string kind;
        string a;
        string b;
        TrdGtTgtParts(kind, a, b);
        string head = TrdTr("#STR_OZT_VPP_GT_PICK");
        string sub = "";
        if (kind == "c")
        {
            OZT_Category cat = TrdGtCatById(a);
            if (cat)
            {
                head = cat.Name + "  (" + cat.Id + ")";
                sub = TrdTr("#STR_OZT_VPP_GT_CAT_H");
            }
        }
        else if (kind == "l")
        {
            OZT_Category lineCat = TrdGtCatById(a);
            OZT_Goods e = TrdGtLineOf(lineCat, b);
            if (e)
            {
                head = e.ClassName;
                string lineWord = TrdTr("#STR_OZT_VPP_USE_LINE");
                string inWord = TrdTr("#STR_OZT_VPP_IN");
                string buyWord = TrdTr("#STR_OZT_VPP_GT_BUY");
                string sellWord = TrdTr("#STR_OZT_VPP_GT_SELL");
                sub = lineWord + " " + inWord + " «" + lineCat.Name + "»";
                sub = sub + " · " + buyWord + " " + e.Buy.ToString();
                sub = sub + " · " + sellWord + " " + e.Sell.ToString();
                if (e.Stock)
                {
                    string stockWord = TrdTr("#STR_OZT_VPP_GT_STOCK_TO");
                    sub = sub + " · " + stockWord + " " + e.Stock.Max.ToString();
                }
            }
        }
        else if (kind == "p")
        {
            OZT_Point p = TrdGtPointById(a);
            if (p)
            {
                head = p.Name + "  (" + p.Id + ")";
                sub = TrdTr("#STR_OZT_VPP_GT_POINT_H");
            }
        }
        SayTrd("TrdGtHead", head);
        SayTrd("TrdGtTargetSub", sub);
        TrdShow("TrdGtChanged", TrdGtIsTouched(m_TrdGtTgt));
        TrdGtPlaceHead();
        m_TrdGtFlowDue = 3;
    }

    // The target's name as wide as its words; what it is stands 10 units after it and
    // ends before the mark of a change (without the mark, where the mark would end).
    protected void TrdGtPlaceHead()
    {
        float s = UnitScale();
        Widget name = M_SUB_WIDGET.FindAnyWidget("TrdGtHead");
        Widget subText = M_SUB_WIDGET.FindAnyWidget("TrdGtTargetSub");
        Widget mark = M_SUB_WIDGET.FindAnyWidget("TrdGtChanged");
        if (!name || !subText || !mark)
            return;
        float x;
        float y;
        float mx;
        float my;
        float mw;
        float mh;
        name.GetPos(x, y);
        mark.GetPos(mx, my);
        mark.GetSize(mw, mh);
        float right = mx - 10 * s;
        if (!mark.IsVisible())
            right = mx + mw;
        float tw = OZT_VppKit.FitWord(M_SUB_WIDGET, "TrdGtHead");
        if (tw > right - x)
        {
            tw = right - x;
            OZT_VppKit.SetWidth(name, tw);
        }
        float from = x + tw + 10 * s;
        float room = right - from;
        Widget cell = subText.GetParent();
        if (cell && cell != name.GetParent())
        {
            OZT_VppKit.MoveX(cell, from);
            OZT_VppKit.SetWidth(cell, room);
        }
        else
            OZT_VppKit.MoveX(subText, from);
        OZT_VppKit.SetWidth(subText, room);
    }

    // ------------------------------------------------------------------ the middle: rows

    protected void TrdGtMakeMid()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdGtCondsRows");
        if (!host || m_TrdGtMid)
            return;
        m_TrdGtMid = new array<ref OZ_VppFlexRows>();
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "sec.layout", "OZT_GtSec", 604, 35));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "cond.layout", "OZT_GtCond", 604, 42));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "add.layout", "OZT_GtAdd", 604, 34));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "lockfoot.layout", "OZT_GtLockFoot", 604, 46));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "disc.layout", "OZT_GtDisc", 604, 42));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "inh.layout", "OZT_GtInh", 604, 42));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "discadd.layout", "OZT_GtDiscAdd", 604, 34));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "discfoot.layout", "OZT_GtDiscFoot", 604, 28));
        m_TrdGtMid.Insert(new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "loop.layout", "OZT_GtLoop", 604, 78));
    }

    // The picked target's lock and discounts, slice by slice: a section's head, a
    // condition a row, the row that adds one, the notes. The rows of all the templates
    // go into one list in the order they are made here; they are made, sized to the list
    // and only then filled and placed by their words (TrdGtFillMid). An edit makes them
    // again and the list stays where it was scrolled; another target starts from its top.
    protected void TrdGtPaintConds()
    {
        TrdGtMakeMid();
        if (!m_TrdGtMid)
            return;
        float scrolled = 0;
        if (m_TrdGtMidOf == m_TrdGtTgt)
            scrolled = OZT_VppKit.ScrollAt(M_SUB_WIDGET, "TrdGtCondsScroll");
        m_TrdGtMidOf = m_TrdGtTgt;
        int i;
        for (i = 0; i < m_TrdGtMid.Count(); i++)
            m_TrdGtMid[i].Clear();

        string kind;
        string a;
        string b;
        TrdGtTgtParts(kind, a, b);
        if (kind == "")
            return;

        array<ref OZT_Condition> lock = TrdGtLock(false);
        array<ref OZT_Discount> discs = TrdGtDiscs(false);
        OZT_Category cat = null;
        if (kind != "p")
            cat = TrdGtCatById(a);

        // The lock.
        if (kind == "p")
            m_TrdGtMid[TRD_GT_SEC].Add("#STR_OZT_VPP_GT_SEC_ACCESS");
        else
            m_TrdGtMid[TRD_GT_SEC].Add("#STR_OZT_VPP_GT_SEC_LOCK");
        if (lock)
        {
            for (i = 0; i < lock.Count(); i++)
                m_TrdGtMid[TRD_GT_COND].Add(i.ToString());
        }
        m_TrdGtMid[TRD_GT_ADD].Add("");
        m_TrdGtMid[TRD_GT_LFOOT].Add("");

        // The discounts: a row per condition a discount waits for, or one row for a
        // discount that waits for none or is computed.
        int k;
        if (kind != "p")
        {
            m_TrdGtMid[TRD_GT_SEC].Add("#STR_OZT_VPP_GT_SEC_DISC");
            if (discs)
            {
                for (i = 0; i < discs.Count(); i++)
                {
                    OZT_Discount d = discs[i];
                    if (!d)
                        continue;
                    if (d.By)
                        m_TrdGtMid[TRD_GT_DISC].Add(i.ToString() + "|-2");
                    else if (!d.When || d.When.Count() == 0)
                        m_TrdGtMid[TRD_GT_DISC].Add(i.ToString() + "|-3");
                    else
                    {
                        for (k = 0; k < d.When.Count(); k++)
                            m_TrdGtMid[TRD_GT_DISC].Add(i.ToString() + "|" + k.ToString());
                    }
                }
            }
            // A line also gets its category's discounts: shown, edited at the category.
            if (kind == "l" && cat && cat.Discounts)
            {
                for (i = 0; i < cat.Discounts.Count(); i++)
                    m_TrdGtMid[TRD_GT_INH].Add(i.ToString());
            }
            m_TrdGtMid[TRD_GT_DADD].Add("");
            m_TrdGtMid[TRD_GT_DFOOT].Add("");
            if (kind == "l")
                m_TrdGtMid[TRD_GT_LOOP].Add("");
        }

        for (i = 0; i < m_TrdGtMid.Count(); i++)
            m_TrdGtMid[i].Fit();
        TrdGtFillMid();
        OZT_VppKit.ScrollTo(M_SUB_WIDGET, "TrdGtCondsScroll", scrolled);
        m_TrdGtFlowDue = 3;
    }

    // Every row of the middle list filled and placed (after Fit: the rows have their
    // width). Run again on the frames after a paint and after a resize: it makes nothing.
    protected void TrdGtFillMid()
    {
        if (!m_TrdGtMid)
            return;
        string kind;
        string a;
        string b;
        TrdGtTgtParts(kind, a, b);
        array<ref OZT_Condition> lock = TrdGtLock(false);
        array<ref OZT_Discount> discs = TrdGtDiscs(false);
        OZT_Category cat = null;
        if (kind == "c" || kind == "l")
            cat = TrdGtCatById(a);
        int i;
        Widget row;
        OZ_VppFlexRows rows = m_TrdGtMid[TRD_GT_SEC];
        for (i = 0; i < rows.Count(); i++)
            TrdGtFillSec(rows.Row(i), rows.Key(i));

        rows = m_TrdGtMid[TRD_GT_COND];
        for (i = 0; i < rows.Count(); i++)
        {
            if (!lock || i >= lock.Count())
                continue;
            row = rows.Row(i);
            bool open = m_TrdGtPkOn && m_TrdGtPkSec == 0 && m_TrdGtPkCond == i;
            TrdGtFillCond(row, "Cond", lock[i], open);
            TrdGtFlowCond(row, "Cond", "BtnCondDel", lock[i], -1);
        }

        rows = m_TrdGtMid[TRD_GT_ADD];
        if (rows.Count() > 0)
            TrdGtFillAdd(rows.Row(0));

        rows = m_TrdGtMid[TRD_GT_LFOOT];
        if (rows.Count() > 0)
            TrdGtFillLockFoot(rows.Row(0), kind, cat);

        rows = m_TrdGtMid[TRD_GT_DISC];
        for (i = 0; i < rows.Count(); i++)
            TrdGtFillDisc(rows.Row(i), rows.Key(i));

        rows = m_TrdGtMid[TRD_GT_INH];
        for (i = 0; i < rows.Count(); i++)
        {
            if (cat && cat.Discounts && i < cat.Discounts.Count())
                TrdGtFillInh(rows.Row(i), cat, cat.Discounts[i]);
        }

        rows = m_TrdGtMid[TRD_GT_LOOP];
        if (rows.Count() > 0 && cat)
            TrdGtFillLoop(rows.Row(0), cat, TrdGtLineOf(cat, b));
    }

    // A section's head: its title on the frame's line, the line going on after it.
    protected void TrdGtFillSec(Widget row, string title)
    {
        if (!row)
            return;
        float s = UnitScale();
        OZT_VppKit.PartSay(row, "SecTitle", title);
        Widget t = row.FindAnyWidget("SecTitle");
        Widget line = row.FindAnyWidget("SecLine");
        if (!t || !line)
            return;
        float x;
        float y;
        float rw;
        float rh;
        float tw = OZT_VppKit.FitWord(row, "SecTitle");
        t.GetPos(x, y);
        row.GetSize(rw, rh);
        float from = x + tw + 8 * s;
        OZT_VppKit.MoveX(line, from);
        OZT_VppKit.SetWidth(line, rw - from);
        OZT_VppKit.SetWidth(row.FindAnyWidget("SecLineBg"), rw - from);
    }

    // A condition's parts: its kind, whether it is turned round, its first two values
    // (the provider's name for each, its id after it). `prefix` is "Cond" in a lock's
    // row, "Disc" in a discount's. TrdGtFlowCond places them and counts the rest.
    protected void TrdGtFillCond(Widget row, string prefix, OZT_Condition c, bool open)
    {
        if (!row || !c)
            return;
        OZT_VppKit.PartSay(row, prefix + "KindText", TrdGtKindName(c.Kind));
        TrdGtPaintNot(row, "Btn" + prefix + "Not", c.Not == 1);

        array<string> alts = new array<string>();
        OZT_VppKit.Alts(c.Arg, alts);
        string name;
        string id;
        TrdGtValueWords(c.Kind, "", name, id);
        if (alts.Count() > 0)
            TrdGtValueWords(c.Kind, alts[0], name, id);
        OZT_VppKit.PartSay(row, prefix + "V0Name", name);
        OZT_VppKit.PartSay(row, prefix + "V0Id", id);
        if (alts.Count() > 1)
        {
            TrdGtValueWords(c.Kind, alts[1], name, id);
            OZT_VppKit.PartSay(row, prefix + "V1Name", name);
            OZT_VppKit.PartSay(row, prefix + "V1Id", id);
            OZT_VppKit.PartSay(row, prefix + "Or", "#STR_OZT_VPP_RS_OR");
        }

        // The row whose condition is open in the picker: framed in the accent.
        if (open)
            OZT_VppKit.PartBox(row, prefix + "Box", OZT_Metal.ACCENT, OZT_Metal.CELL);
        else
            OZT_VppKit.PartBox(row, prefix + "Box", OZT_Metal.CELL, OZT_Metal.CELL);
        OZT_VppKit.PartShow(row, prefix + "BoxBar", open);
    }

    // A value as a chip writes it: the name its provider gives it and its id after it,
    // or the value alone; "—" for no value at all (a leader of any faction).
    protected void TrdGtValueWords(string kind, string value, out string name, out string id)
    {
        name = "—";
        id = "";
        if (value == "")
            return;
        name = value;
        string label = TrdChoiceLabel(kind, value);
        if (label == "" || label == value)
            return;
        string branch;
        string owners;
        string tier;
        OZT_VppKit.LabelParts(label, name, branch, owners, tier);
        id = value;
    }

    // The "not" of a row: plain, or lit in the colour of a refusal.
    protected void TrdGtPaintNot(Widget row, string button, bool on)
    {
        if (on)
        {
            OZT_VppKit.PartBox(row, button, OZT_Metal.BAD, 0xFF21100F);
            OZT_VppKit.PartTint(row, button + "Text", OZT_Metal.BAD);
            return;
        }
        OZT_VppKit.PartBox(row, button, OZT_Metal.EDGE, OZT_Metal.CELL);
        OZT_VppKit.PartTint(row, button + "Text", OZT_Metal.MUTED);
    }

    // A condition's row placed by its words: the kind's chip as wide as its word (at
    // `kindX` pixels of the row's box; below zero: where the layout has it), the "not"
    // after it, the values in what is left before `stop` (the row's next button). A
    // second value that does not fit gives way to the count of the ones not shown.
    protected void TrdGtFlowCond(Widget row, string prefix, string stop, OZT_Condition c, float kindX)
    {
        if (!row || !c)
            return;
        float s = UnitScale();
        Widget kind = row.FindAnyWidget(prefix + "Kind");
        Widget notBtn = row.FindAnyWidget("Btn" + prefix + "Not");
        Widget vals = row.FindAnyWidget("Btn" + prefix + "Vals");
        Widget end = row.FindAnyWidget(stop);
        if (!kind || !notBtn || !vals || !end)
            return;
        float x;
        float y;
        float ex;
        float ey;
        kind.GetPos(x, y);
        if (kindX >= 0)
        {
            x = kindX;
            OZT_VppKit.MoveX(kind, x);
        }
        float kw = OZT_VppKit.HugBox(row, prefix + "Kind", prefix + "KindText", 8 * s);
        x = x + kw + 7 * s;
        OZT_VppKit.MoveX(notBtn, x);
        x = x + 28 * s + 7 * s;
        end.GetPos(ex, ey);
        float room = ex - 7 * s - x;
        OZT_VppKit.MoveX(vals, x);
        OZT_VppKit.SetWidth(vals, room);

        // Inside the values' button.
        Widget v0 = row.FindAnyWidget(prefix + "V0");
        Widget v1 = row.FindAnyWidget(prefix + "V1");
        Widget orWord = row.FindAnyWidget(prefix + "Or");
        Widget more = row.FindAnyWidget(prefix + "More");
        if (!v0)
            return;
        array<string> alts = new array<string>();
        OZT_VppKit.Alts(c.Arg, alts);
        int unseen = alts.Count() - 2;
        if (unseen < 0)
            unseen = 0;
        bool two = alts.Count() > 1;
        if (v1)
            v1.Show(two);
        if (orWord)
            orWord.Show(two);

        float at = OZT_VppKit.HugPair(row, prefix + "V0", prefix + "V0Name", prefix + "V0Id", 8 * s, 6 * s);
        OZT_VppKit.MoveX(v0, 0);
        // One value longer than the row: its chip ends with the row, its id gives way.
        if (at > room && room > 40 * s)
        {
            OZT_VppKit.PartSay(row, prefix + "V0Id", "");
            OZT_VppKit.WidenBox(row, prefix + "V0", room);
            OZT_VppKit.SetWidth(row.FindAnyWidget(prefix + "V0Name"), room - 16 * s);
            at = room;
        }
        if (two && v1 && orWord)
        {
            float ow = OZT_VppKit.FitWord(row, prefix + "Or");
            float w1 = OZT_VppKit.HugPair(row, prefix + "V1", prefix + "V1Name", prefix + "V1Id", 8 * s, 6 * s);
            float need = at + 6 * s + ow + 6 * s + w1;
            float tail = 0;
            if (unseen > 0)
                tail = 32 * s;
            if (need + tail > room)
            {
                // No room for the second value: it joins the count.
                v1.Show(false);
                orWord.Show(false);
                unseen++;
            }
            else
            {
                OZT_VppKit.MoveX(orWord, at + 6 * s);
                OZT_VppKit.MoveX(v1, at + 6 * s + ow + 6 * s);
                at = need;
            }
        }
        string moreText = "";
        if (unseen > 0)
            moreText = "+" + unseen.ToString();
        OZT_VppKit.PartSay(row, prefix + "More", moreText);
        OZT_VppKit.FitText(row, prefix + "More");
        if (more)
            OZT_VppKit.MoveX(more, at + 6 * s);
    }

    // The row that adds a condition: a button per kind the providers answer (and the
    // five the trader ships words for), six at a time; a kind nobody answers is faint.
    // The buttons stand after the row's caption, each as wide as its word.
    protected void TrdGtFillAdd(Widget row)
    {
        if (!row)
            return;
        float s = UnitScale();
        if (!m_TrdGtAddKinds)
            m_TrdGtAddKinds = new array<string>();
        m_TrdGtAddKinds.Clear();
        array<string> kinds = new array<string>();
        TrdKinds(kinds);
        int shown = 6;
        bool paged = kinds.Count() > 6;
        if (paged)
            shown = 5;
        if (m_TrdGtKindFrom >= kinds.Count() || !paged)
            m_TrdGtKindFrom = 0;

        float x = 0;
        float y = 0;
        Widget first = row.FindAnyWidget("BtnAddKind0");
        if (first)
            first.GetPos(x, y);
        Widget cap = row.FindAnyWidget("AddCap");
        if (cap)
        {
            cap.GetPos(x, y);
            x = x + OZT_VppKit.FitWord(row, "AddCap") + 6 * s;
        }
        for (int n = 0; n < 6; n++)
        {
            string name = "BtnAddKind" + n.ToString();
            Widget btn = row.FindAnyWidget(name);
            if (!btn)
                continue;
            string kind = "";
            string caption = "";
            int ink = OZT_Metal.TEXT;
            if (n < shown && m_TrdGtKindFrom + n < kinds.Count())
            {
                kind = kinds[m_TrdGtKindFrom + n];
                caption = TrdGtKindName(kind);
                if (m_TrdGates && TrdFamilyOf(kind) == "")
                    ink = OZT_Metal.FAINT;
            }
            else if (paged && n == 5)
            {
                kind = ">";
                caption = TrdTr("#STR_OZT_VPP_GT_KIND_MORE");
                ink = OZT_Metal.MUTED;
            }
            m_TrdGtAddKinds.Insert(kind);
            btn.Show(kind != "");
            if (kind == "")
                continue;
            RowText(row, name + "Text", caption, ink);
            float w = OZT_VppKit.HugBox(row, name, name + "Text", 10 * s);
            OZT_VppKit.MoveX(btn, x);
            x = x + w + 6 * s;
        }
    }

    // Under a lock: what else holds for the target. A line has its category's lock on
    // top of its own.
    protected void TrdGtFillLockFoot(Widget row, string kind, OZT_Category cat)
    {
        if (!row)
            return;
        string words = "";
        if (kind == "c")
            words = TrdTr("#STR_OZT_VPP_GT_LOCK_OF_CAT");
        else if (kind == "p")
            words = TrdTr("#STR_OZT_VPP_GT_ACCESS_NOTE");
        else if (cat)
        {
            string from = TrdTr("#STR_OZT_VPP_GT_FROM_CAT");
            words = from + " «" + cat.Name + "»: ";
            if (TrdGtCount(cat.Requires) == 0)
            {
                string noLocks = TrdTr("#STR_OZT_VPP_GT_NO_LOCKS");
                words = words + noLocks;
            }
            else
            {
                string all = "";
                for (int i = 0; i < cat.Requires.Count(); i++)
                {
                    if (all != "")
                        all = all + "; ";
                    all = all + TrdCondWords(cat.Requires[i]);
                }
                words = words + all;
            }
        }
        OZT_VppKit.PartSay(row, "LfInherit", words);
    }

    // The discount and the condition of a discount's row, by its key
    // "<discount>|<condition>" (-2: computed by the provider of By, -3: waits for none).
    protected OZT_Discount TrdGtDiscOf(string key, out int cond)
    {
        cond = -3;
        array<ref OZT_Discount> discs = TrdGtDiscs(false);
        int bar = key.IndexOf("|");
        if (!discs || bar <= 0)
            return null;
        int d = key.Substring(0, bar).ToInt();
        cond = key.Substring(bar + 1, key.Length() - bar - 1).ToInt();
        if (d < 0 || d >= discs.Count())
            return null;
        return discs[d];
    }

    protected OZT_Condition TrdGtDiscCond(OZT_Discount d, int cond)
    {
        if (!d)
            return null;
        if (cond == -2)
            return d.By;
        if (cond < 0 || !d.When || cond >= d.When.Count())
            return null;
        return d.When[cond];
    }

    // A discount's row. Its first condition's row carries the percent (a computed
    // discount its cap, after the sign "at most"); a further condition's row says "and".
    // The word before the condition is as wide as it is in this language, and the
    // condition's parts stand after it.
    protected void TrdGtFillDisc(Widget row, string key)
    {
        if (!row)
            return;
        float s = UnitScale();
        int cond;
        OZT_Discount d = TrdGtDiscOf(key, cond);
        if (!d)
            return;
        int bar = key.IndexOf("|");
        int at = key.Substring(0, bar).ToInt();
        OZT_Condition c = TrdGtDiscCond(d, cond);

        bool first = cond <= 0;
        OZT_VppKit.PartShow(row, "DiscPctBox", first);
        string when = TrdTr("#STR_OZT_VPP_GT_AND");
        string sign = "−";
        float pct = d.Percent;
        if (first)
        {
            when = TrdTr("#STR_OZT_VPP_GT_WHEN");
            if (cond == -2)
            {
                sign = "≤";
                pct = d.Max;
                when = TrdTr("#STR_OZT_VPP_GT_BY");
            }
            else if (cond == -3)
                when = TrdTr("#STR_OZT_VPP_GT_ALWAYS");
            OZT_VppKit.PartSay(row, "DiscSign", sign);
            // A placing pass leaves the field alone: the admin may be typing in it.
            EditBoxWidget box = EditBoxWidget.Cast(row.FindAnyWidget("DiscPct"));
            if (box && !m_TrdGtFlowing)
            {
                m_TrdGtPut = true;
                box.SetText(TrdNum(pct));
                m_TrdGtPut = false;
            }
        }
        OZT_VppKit.PartSay(row, "DiscWhen", when);

        // A discount that waits for nothing has no condition to show: only the buttons
        // that add one and remove it. A computed discount has one condition and no "and".
        bool has = c != null;
        OZT_VppKit.PartShow(row, "DiscKind", has);
        OZT_VppKit.PartShow(row, "BtnDiscNot", has);
        OZT_VppKit.PartShow(row, "BtnDiscVals", has);
        OZT_VppKit.PartShow(row, "BtnDiscAnd", cond != -2);
        bool open = false;
        if (m_TrdGtPkOn && m_TrdGtPkDisc == at)
        {
            if (m_TrdGtPkSec == 1 && m_TrdGtPkCond == cond)
                open = true;
            if (m_TrdGtPkSec == 2 && cond == -2)
                open = true;
        }

        // The word, then the condition: not nearer than the layout has its chip.
        Widget word = row.FindAnyWidget("DiscWhen");
        Widget chip = row.FindAnyWidget("DiscKind");
        float x = 0;
        float y = 0;
        float kx = 0;
        float ky = 0;
        if (word)
            word.GetPos(x, y);
        if (chip)
            chip.GetPos(kx, ky);
        float after = x + OZT_VppKit.FitWord(row, "DiscWhen") + 7 * s;
        if (after < 142 * s)
            after = 142 * s;
        if (!has)
        {
            OZT_VppKit.PartBox(row, "DiscBox", OZT_Metal.CELL, OZT_Metal.CELL);
            return;
        }
        TrdGtFillCond(row, "Disc", c, open);
        if (cond == -2)
            TrdGtFlowCond(row, "Disc", "BtnDiscDel", c, after);
        else
            TrdGtFlowCond(row, "Disc", "BtnDiscAnd", c, after);
    }

    // A discount of the line's category: it holds for the line too, and is edited at
    // the category.
    protected void TrdGtFillInh(Widget row, OZT_Category cat, OZT_Discount d)
    {
        if (!row || !d)
            return;
        string pct = "−" + TrdNum(d.Percent) + " %";
        string words = "";
        if (d.By)
        {
            string byWord = TrdTr("#STR_OZT_VPP_GT_BY");
            pct = "≤ " + TrdNum(d.Max) + " %";
            words = byWord + " " + TrdCondWords(d.By);
        }
        else if (!d.When || d.When.Count() == 0)
            words = TrdTr("#STR_OZT_VPP_GT_ALWAYS");
        else
        {
            string whenWord = TrdTr("#STR_OZT_VPP_GT_WHEN");
            string andWord = TrdTr("#STR_OZT_VPP_GT_AND");
            for (int i = 0; i < d.When.Count(); i++)
            {
                if (i == 0)
                    words = whenWord + " ";
                else
                    words = words + " " + andWord + " ";
                words = words + TrdCondWords(d.When[i]);
            }
        }
        // The category's name is cut to leave the words after it their room.
        string catName = cat.Name;
        if (catName.LengthUtf8() > 18)
            catName = catName.SubstringUtf8(0, 17) + "…";
        string from = TrdTr("#STR_OZT_VPP_GT_INH_FROM");
        string readOnly = TrdTr("#STR_OZT_VPP_GT_READ_ONLY");
        OZT_VppKit.PartSay(row, "InhPct", pct);
        OZT_VppKit.PartSay(row, "InhText", words);
        OZT_VppKit.PartSay(row, "InhFrom", from + " «" + catName + "» · " + readOnly);
    }

    // The money check for a line, as the server's load makes it (OZT_Config.CheckLoops)
    // save the rounding to the smallest note: the most a point pays for one in the best
    // condition against the least a player pays where it is cheapest, with the largest
    // discount the line or its category can give and the step of a stocked line. The
    // server's check is the judge (the draft's bar shows its findings); this one says
    // the same of what the draft holds now, before it is applied.
    protected void TrdGtFillLoop(Widget row, OZT_Category cat, OZT_Goods e)
    {
        if (!row || !e)
            return;
        OZT_VppKit.PartSay(row, "LoopNote", "#STR_OZT_VPP_GT_LOOP_NOTE");
        if (e.Buy <= 0 || e.Sell <= 0)
        {
            RowText(row, "LoopHead", "#STR_OZT_VPP_GT_LOOP_OFF", OZT_Metal.MUTED);
            OZT_VppKit.PartShow(row, "LoopSign", false);
            return;
        }
        bool several;
        float most = TrdGtMost(e.Discounts, several);
        float catMost = TrdGtMost(cat.Discounts, several);
        if (catMost > most)
            most = catMost;
        float buyFactor = 0;
        float sellFactor = 0;
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            OZT_Point p = points[i];
            if (!p.Categories || p.Categories.Find(cat.Id) == -1)
                continue;
            if (buyFactor == 0 || p.BuyFactor < buyFactor)
                buyFactor = p.BuyFactor;
            if (p.SellFactor > sellFactor)
                sellFactor = p.SellFactor;
        }
        if (buyFactor <= 0)
            buyFactor = 1;
        if (sellFactor <= 0)
            sellFactor = 1;
        float step = 1;
        if (e.Stock && e.Stock.Coefficient < 1)
            step = e.Stock.Coefficient;
        float best = 0;
        OZT_ConditionFactors shares = m_TrdDraft.Settings.Condition;
        if (shares)
        {
            for (int level = 0; level < 4; level++)
            {
                if (shares.Of(level) > best)
                    best = shares.Of(level);
            }
        }
        if (best <= 0)
            best = 1;
        float least = e.Buy * buyFactor * step * (100 - most) / 100;
        float top = e.Sell * sellFactor * best;
        bool ok = top <= least + 0.001;

        string buyWord = TrdTr("#STR_OZT_VPP_GT_BUY");
        string head = TrdTr("#STR_OZT_VPP_GT_LOOP_OK");
        string sign = "≤";
        if (!ok)
        {
            head = TrdTr("#STR_OZT_VPP_GT_LOOP_BAD");
            sign = ">";
        }
        head = head + " " + e.Sell.ToString();
        if (sellFactor != 1)
            head = head + " × " + TrdNum(sellFactor);
        if (best != 1)
            head = head + " × " + TrdNum(best);
        if (sellFactor != 1 || best != 1)
            head = head + " = " + TrdNum(top);
        head = head + " " + sign + " " + buyWord + " " + e.Buy.ToString();
        if (buyFactor != 1)
            head = head + " × " + TrdNum(buyFactor);
        if (step != 1)
            head = head + " × " + TrdNum(step);
        if (most > 0)
            head = head + " × (1 − " + TrdNum(most) + " %)";
        head = head + " = " + TrdNum(least);
        if (ok)
        {
            RowText(row, "LoopHead", head, OZT_Metal.TEXT);
            OZT_VppKit.SignSet(row, "LoopSign", "check", OZT_Metal.OK);
        }
        else
        {
            RowText(row, "LoopHead", head, OZT_Metal.ALERT);
            OZT_VppKit.SignSet(row, "LoopSign", "cross", OZT_Metal.BAD);
        }
        OZT_VppKit.PartShow(row, "LoopSign", true);
    }

    // After an edit of the target: the middle again, the list's badges, the head's mark.
    protected void TrdGtAfterEdit()
    {
        TrdGtPaintConds();
        TrdGtPaintList();
        TrdGtPaintHead();
    }

    // ------------------------------------------------------------------ the picker

    protected void TrdGtPkInit()
    {
        if (!m_TrdGtPkVals)
            m_TrdGtPkVals = new array<string>();
    }

    protected void TrdGtPkClose()
    {
        TrdGtPkInit();
        m_TrdGtPkOn = false;
        m_TrdGtPkVals.Clear();
        m_TrdGtPkOwner = "";
        m_TrdGtPkBranch = "";
    }

    // A condition into the picker: `sec` 0 the lock, 1 a discount's When, 2 a discount's
    // By; `disc` and `cond` say which (-1: a new one). An open one of another row goes
    // unsaved, as a cancel.
    protected void TrdGtPkOpen(int sec, int disc, int cond, string kind, string arg)
    {
        TrdGtPkInit();
        m_TrdGtPkOn = true;
        m_TrdGtPkSec = sec;
        m_TrdGtPkDisc = disc;
        m_TrdGtPkCond = cond;
        m_TrdGtPkOwner = "";
        m_TrdGtPkBranch = "";
        OZT_VppKit.Alts(arg, m_TrdGtPkVals);
        m_TrdGtPut = true;
        SetEdit("TrdGtKind", kind);
        SetEdit("TrdGtPickFind", "");
        m_TrdGtPut = false;
        TrdGtFillMid();
        TrdGtPaintPicker();
    }

    protected string TrdGtPkKind()
    {
        return GetEdit("TrdGtKind").Trim();
    }

    // The picker's head, its status line, its three lists and what is chosen.
    protected void TrdGtPaintPicker()
    {
        TrdGtPkInit();
        string kind = TrdGtPkKind();
        string title = TrdTr("#STR_OZT_VPP_GT_PK_IDLE");
        if (m_TrdGtPkOn)
        {
            string what = TrdTr("#STR_OZT_VPP_GT_LOCK");
            if (m_TrdGtTgt.IndexOf("p|") == 0)
                what = TrdTr("#STR_OZT_VPP_PT_ACCESS");
            if (m_TrdGtPkSec != 0)
                what = TrdTr("#STR_OZT_VPP_GT_PK_DISC");
            string tkind;
            string ta;
            string tb;
            TrdGtTgtParts(tkind, ta, tb);
            string whose = ta;
            if (tkind == "l")
                whose = tb;
            title = what + " " + whose;
            if (m_TrdGtPkCond == -1 && !(m_TrdGtPkSec == 2 && m_TrdGtPkDisc != -1))
            {
                string newWord = TrdTr("#STR_OZT_VPP_GT_PK_NEW");
                title = title + " · " + newWord;
            }
        }
        SayTrd("TrdGtPkTitle", title);

        OZT_AdminGateKind gk = TrdGateKind(kind);
        string sub;
        if (kind == "")
            sub = TrdTr("#STR_OZT_VPP_GT_NEED_KIND");
        else if (!m_TrdGates)
            sub = TrdTr("#STR_OZT_VPP_GT_ASKING");
        else if (TrdFamilyOf(kind) == "")
            sub = TrdTr("#STR_OZT_VPP_GT_PK_NOBODY");
        else if (!gk || gk.Values.Count() == 0)
            sub = TrdTr("#STR_OZT_VPP_GT_FREE");
        else
        {
            string count = TrdTr("#STR_OZT_VPP_GT_CHOICES");
            sub = TrdCondKindWord(kind) + ": " + count + " " + gk.Values.Count().ToString();
            if (gk.Cut == 1)
                sub = sub + "+";
        }
        SayTrd("TrdGtPickSub", sub);

        // Done and cancel are lit while a condition is open.
        if (m_TrdGtPkOn)
        {
            OZT_VppKit.PartBox(M_SUB_WIDGET, "BtnTrdGtPkDone", OZT_Metal.ACCENT, OZT_Metal.ACCENT);
            OZT_VppKit.PartTint(M_SUB_WIDGET, "BtnTrdGtPkDoneText", 0xFF140A02);
            OZT_VppKit.PartBox(M_SUB_WIDGET, "BtnTrdGtPkCancel", OZT_Metal.EDGE, OZT_Metal.RAISED);
            OZT_VppKit.PartTint(M_SUB_WIDGET, "BtnTrdGtPkCancelText", OZT_Metal.TEXT);
        }
        else
        {
            OZT_VppKit.PartBox(M_SUB_WIDGET, "BtnTrdGtPkDone", OZT_Metal.EDGE, OZT_Metal.HEAD);
            OZT_VppKit.PartTint(M_SUB_WIDGET, "BtnTrdGtPkDoneText", OZT_Metal.FAINT);
            OZT_VppKit.PartBox(M_SUB_WIDGET, "BtnTrdGtPkCancel", OZT_Metal.EDGE, OZT_Metal.HEAD);
            OZT_VppKit.PartTint(M_SUB_WIDGET, "BtnTrdGtPkCancelText", OZT_Metal.FAINT);
        }

        TrdGtPkPaintGroups();
        TrdGtPkPaintValues();
        TrdGtPkPaintChosen();
    }

    // A value's owner words, branch and tier, when its provider's label carries them.
    protected bool TrdGtPkParts(OZT_AdminGateKind gk, int i, out string name, out string branch, out string owners, out string tier)
    {
        string label = "";
        if (i < gk.Labels.Count())
            label = gk.Labels[i];
        if (label == "")
            label = gk.Values[i];
        return OZT_VppKit.LabelParts(label, name, branch, owners, tier);
    }

    // Is a value under the owner and the branch the lists are narrowed to?
    protected bool TrdGtPkUnder(string owners, string branch)
    {
        if (m_TrdGtPkOwner != "")
        {
            array<string> own = new array<string>();
            owners.Split(",", own);
            bool mine = false;
            for (int i = 0; i < own.Count(); i++)
            {
                if (own[i].Trim() == m_TrdGtPkOwner)
                    mine = true;
            }
            if (!mine)
                return false;
        }
        if (m_TrdGtPkBranch != "" && branch != m_TrdGtPkBranch)
            return false;
        return true;
    }

    // The owners and the branches the kind's values come under (the research tree's
    // nodes do), each with how many of the chosen values it holds; the first row of
    // each is "all".
    protected void TrdGtPkPaintGroups()
    {
        Widget ownHost = M_SUB_WIDGET.FindAnyWidget("TrdGtPkOwnersRows");
        Widget brHost = M_SUB_WIDGET.FindAnyWidget("TrdGtPkBranchesRows");
        if (!ownHost || !brHost)
            return;
        if (!m_TrdGtPkOwnRows)
            m_TrdGtPkOwnRows = new OZ_VppFlexRows(m_Flex, ownHost, TRD_GT_L + "owner.layout", "OZT_GtOwner", 112, 30);
        if (!m_TrdGtPkBrRows)
            m_TrdGtPkBrRows = new OZ_VppFlexRows(m_Flex, brHost, TRD_GT_L + "branch.layout", "OZT_GtBranch", 112, 42);
        m_TrdGtPkOwnRows.Clear();
        m_TrdGtPkBrRows.Clear();

        OZT_AdminGateKind gk = TrdGateKind(TrdGtPkKind());
        array<string> owners = new array<string>();
        array<int> ownerPicked = new array<int>();
        array<string> branches = new array<string>();
        array<int> branchNodes = new array<int>();
        array<string> own = new array<string>();
        string name;
        string branch;
        string ownerWords;
        string tier;
        int i;
        int k;
        int at;
        if (gk)
        {
            for (i = 0; i < gk.Values.Count(); i++)
            {
                if (!TrdGtPkParts(gk, i, name, branch, ownerWords, tier))
                    continue;
                bool chosen = m_TrdGtPkVals.Find(gk.Values[i]) != -1;
                own.Clear();
                ownerWords.Split(",", own);
                bool under = m_TrdGtPkOwner == "";
                for (k = 0; k < own.Count(); k++)
                {
                    string one = own[k].Trim();
                    if (one == "")
                        continue;
                    at = owners.Find(one);
                    if (at == -1)
                    {
                        at = owners.Insert(one);
                        ownerPicked.Insert(0);
                    }
                    if (chosen)
                        ownerPicked.Set(at, ownerPicked[at] + 1);
                    if (one == m_TrdGtPkOwner)
                        under = true;
                }
                if (!under || branch == "")
                    continue;
                at = branches.Find(branch);
                if (at == -1)
                {
                    at = branches.Insert(branch);
                    branchNodes.Insert(0);
                }
                branchNodes.Set(at, branchNodes[at] + 1);
            }
        }

        Widget row;
        string allWord = TrdTr("#STR_OZT_VPP_RS_OWNER_ALL");
        if (owners.Count() > 0)
        {
            row = m_TrdGtPkOwnRows.Add("");
            if (row)
                TrdGtPkOwnerRow(row, allWord, 0, m_TrdGtPkOwner == "");
            for (i = 0; i < owners.Count(); i++)
            {
                row = m_TrdGtPkOwnRows.Add(owners[i]);
                if (row)
                    TrdGtPkOwnerRow(row, owners[i], ownerPicked[i], owners[i] == m_TrdGtPkOwner);
            }
        }
        string nodesWord = TrdTr("#STR_OZT_VPP_GT_PK_NODES_N");
        if (branches.Count() > 0)
        {
            for (i = 0; i < branches.Count(); i++)
            {
                row = m_TrdGtPkBrRows.Add(branches[i]);
                if (!row)
                    continue;
                bool lit = branches[i] == m_TrdGtPkBranch;
                int ink = OZT_Metal.MUTED;
                if (lit)
                    ink = OZT_Metal.TEXT;
                RowText(row, "BrName", branches[i], ink);
                RowText(row, "BrSub", nodesWord + " " + branchNodes[i].ToString(), OZT_Metal.MUTED);
                TrdGtPkLit(row, "OZT_GtBranch", lit);
            }
        }
        m_TrdGtPkOwnRows.Fit();
        m_TrdGtPkBrRows.Fit();

        // The values' caption says what the right column of the list is.
        if (owners.Count() > 0)
            SayTrd("TrdGtPkValsHead", "#STR_OZT_VPP_GT_PK_NODES");
        else
            SayTrd("TrdGtPkValsHead", "#STR_OZT_VPP_GT_PK_VALS");
    }

    protected void TrdGtPkOwnerRow(Widget row, string name, int picked, bool lit)
    {
        int ink = OZT_Metal.MUTED;
        if (lit)
            ink = OZT_Metal.TEXT;
        RowText(row, "OwName", name, ink);
        string count = "";
        if (picked > 0)
            count = picked.ToString();
        RowText(row, "OwCount", count, OZT_Metal.ACCENT);
        TrdGtPkLit(row, "OZT_GtOwner", lit);
    }

    // A row of the picker's left lists lit: the accent's soft ground and its bar.
    protected void TrdGtPkLit(Widget row, string root, bool lit)
    {
        if (lit)
        {
            OZ_VppFlex.Paint(row, root + "Bg", OZT_Metal.PICK_HEAD);
            OZ_VppFlex.Paint(row, root + "Bar", OZT_Metal.ACCENT);
            return;
        }
        OZ_VppFlex.Paint(row, root + "Bg", OZT_Metal.HEAD);
        OZ_VppFlex.Paint(row, root + "Bar", OZT_Metal.HEAD);
    }

    // The values of the kind under the search, the owner and the branch: each with its
    // tick, its name, its id (and tier) and who holds it now. The questions about values
    // no longer listed are dropped; the listed ones are asked one at a time.
    protected void TrdGtPkPaintValues()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdGtPickRows");
        if (!host)
            return;
        if (!m_TrdGtPkValRows)
            m_TrdGtPkValRows = new OZ_VppFlexRows(m_Flex, host, TRD_GT_L + "val.layout", "OZT_GtVal", 265, 34);
        m_TrdGtPkValRows.Clear();
        TrdGtHoldDrop();
        string kind = TrdGtPkKind();
        OZT_AdminGateKind gk = TrdGateKind(kind);
        if (!gk)
        {
            m_TrdGtPkValRows.Fit();
            return;
        }
        string find = OZT_VppKit.Low(GetEdit("TrdGtPickFind").Trim());
        string tierWord = TrdTr("#STR_OZT_VPP_RS_TIER");
        string name;
        string branch;
        string owners;
        string tier;
        Widget row;
        int hidden = 0;
        for (int i = 0; i < gk.Values.Count(); i++)
        {
            string value = gk.Values[i];
            bool shaped = TrdGtPkParts(gk, i, name, branch, owners, tier);
            if (shaped && !TrdGtPkUnder(owners, branch))
                continue;
            if (!shaped && (m_TrdGtPkOwner != "" || m_TrdGtPkBranch != ""))
                continue;
            if (find != "" && !OZT_VppKit.Holds(name, find) && !OZT_VppKit.Holds(value, find))
                continue;
            if (m_TrdGtPkValRows.Count() >= TRD_GT_PICK_MAX)
            {
                hidden++;
                continue;
            }
            row = m_TrdGtPkValRows.Add(value);
            if (!row)
                continue;
            string sub = value;
            if (shaped)
                sub = value + " · " + tierWord + " " + tier;
            RowText(row, "PkName", name, OZT_Metal.TEXT);
            RowText(row, "PkSub", sub, OZT_Metal.MUTED);
            TrdGtPkMarkValue(row, m_TrdGtPkVals.Find(value) != -1);
            TrdGtPkState(row, kind, value);
            TrdGtHoldWant(kind, value);
        }
        m_TrdGtPkValRows.Fit();
        TrdGtPkPlaceValues();
        m_TrdGtFlowDue = 3;
        if (hidden > 0)
        {
            string moreWord = TrdTr("#STR_OZT_VPP_RS_MORE");
            string had = TrdTr("#STR_OZT_VPP_GT_CHOICES");
            SayTrd("TrdGtPickSub", had + " " + gk.Values.Count().ToString() + " · " + moreWord + " " + hidden.ToString());
        }
    }

    protected void TrdGtPkMarkValue(Widget row, bool chosen)
    {
        OZT_VppKit.TickMark(row, "PkTick", chosen);
        if (chosen)
            OZ_VppFlex.Paint(row, "OZT_GtValBg", OZT_Metal.PICK_HEAD);
        else
            OZ_VppFlex.Paint(row, "OZT_GtValBg", OZT_Metal.HEAD);
    }

    // Who holds a value, at the row's right end: the names after a tick. A provider that
    // names nobody may not be able to tell, and then nothing is said -- but research
    // can: a node nobody completed says so.
    protected void TrdGtPkState(Widget row, string kind, string value)
    {
        if (!row)
            return;
        string said;
        int state = TrdGtHoldOf(kind, value, said);
        bool held = state == 1 && said != "";
        OZT_VppKit.PartShow(row, "PkSign", held);
        string words = "";
        int ink = OZT_Metal.MUTED;
        if (held)
        {
            words = said;
            ink = OZT_Metal.OK;
        }
        else if (state == 1 && kind == "research")
            words = TrdTr("#STR_OZT_VPP_RS_NOBODY");
        RowText(row, "PkState", words, ink);
    }

    // The state written from the row's right end, as wide as its words (two fifths of
    // the row at most), its tick before its first letter; the value's name and id in
    // what is left.
    protected void TrdGtPkPlaceState(Widget row)
    {
        if (!row)
            return;
        float s = UnitScale();
        float rw;
        float rh;
        float nx;
        float ny;
        row.GetSize(rw, rh);
        float right = rw - 8 * s;
        float left = right;
        Widget state = row.FindAnyWidget("PkState");
        Widget sign = row.FindAnyWidget("PkSign");
        float tw = OZT_VppKit.FitText(row, "PkState");
        float most = rw * 0.4;
        if (tw > most)
        {
            tw = most;
            OZT_VppKit.SetWidth(state, tw);
        }
        if (state && tw > 0)
        {
            left = right - tw;
            OZT_VppKit.MoveX(state, left);
        }
        if (sign && sign.IsVisible())
        {
            left = left - 15 * s;
            OZT_VppKit.MoveX(sign, left);
        }
        Widget name = row.FindAnyWidget("PkName");
        if (!name)
            return;
        Widget cell = name.GetParent();
        if (!cell || cell == row)
            return;
        cell.GetPos(nx, ny);
        float room = left - 6 * s - nx;
        OZT_VppKit.SetWidth(cell, room);
        OZT_VppKit.SetWidth(name, room);
        OZT_VppKit.SetWidth(row.FindAnyWidget("PkSub"), room);
    }

    protected void TrdGtPkPlaceValues()
    {
        if (!m_TrdGtPkValRows)
            return;
        for (int i = 0; i < m_TrdGtPkValRows.Count(); i++)
            TrdGtPkPlaceState(m_TrdGtPkValRows.Row(i));
    }

    // Who holds a value has come: its row on screen says it.
    protected void TrdGtPkPaintState(string kind, string value)
    {
        if (!m_TrdGtPkValRows || kind != TrdGtPkKind())
            return;
        int at = m_TrdGtPkValRows.Find(value);
        if (at == -1)
            return;
        Widget row = m_TrdGtPkValRows.Row(at);
        TrdGtPkState(row, kind, value);
        TrdGtPkPlaceState(row);
    }

    // What is chosen: a chip a value -- its name and, beside it, its owner when its
    // provider names one, else its id -- in two lines; the Arg as the file will have it.
    protected void TrdGtPkPaintChosen()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdGtPkSel");
        if (!host)
            return;
        if (!m_TrdGtPkChips)
            m_TrdGtPkChips = new OZT_VppChips(m_Flex, host, TRD_GT_L + "sel.layout", "OZT_GtSel");
        m_TrdGtPkChips.Clear();
        string kind = TrdGtPkKind();
        string name;
        string branch;
        string owners;
        string tier;
        string label;
        string beside;
        Widget chip;
        for (int i = 0; i < m_TrdGtPkVals.Count(); i++)
        {
            chip = m_TrdGtPkChips.Add(m_TrdGtPkVals[i]);
            if (!chip)
                continue;
            name = m_TrdGtPkVals[i];
            beside = "";
            label = TrdChoiceLabel(kind, m_TrdGtPkVals[i]);
            if (label != "" && label != m_TrdGtPkVals[i])
            {
                beside = m_TrdGtPkVals[i];
                if (OZT_VppKit.LabelParts(label, name, branch, owners, tier) && owners != "")
                    beside = owners;
            }
            OZT_VppKit.PartSay(chip, "SelName", name);
            OZT_VppKit.PartSay(chip, "SelOwner", beside);
        }
        TrdGtPkPlaceChosen();
        m_TrdGtFlowDue = 3;

        m_TrdGtPut = true;
        SetEdit("TrdGtArg", OZT_VppKit.Joined(m_TrdGtPkVals, "|"));
        m_TrdGtPut = false;
    }

    // Every chip as wide as its words and its own button, then the chips in lines; a
    // chip longer than the frame is cut at its name.
    protected void TrdGtPkPlaceChosen()
    {
        if (!m_TrdGtPkChips)
            return;
        float s = UnitScale();
        float full = 387 * s;
        float nameW;
        float besideW;
        float w;
        float h;
        float x;
        Widget chip;
        Widget name;
        for (int i = 0; i < m_TrdGtPkChips.Count(); i++)
        {
            chip = m_TrdGtPkChips.Chip(i);
            if (!chip)
                continue;
            name = chip.FindAnyWidget("SelName");
            nameW = OZT_VppKit.FitWord(chip, "SelName");
            besideW = OZT_VppKit.FitText(chip, "SelOwner");
            w = 8 * s + nameW + 4 * s + 22 * s + 2 * s;
            if (besideW > 0)
                w = w + 6 * s + besideW;
            if (w > full)
            {
                nameW = nameW - (w - full);
                if (nameW < 20 * s)
                    nameW = 20 * s;
                OZT_VppKit.SetWidth(name, nameW);
                w = full;
            }
            OZT_VppKit.MoveX(name, 8 * s);
            x = 8 * s + nameW;
            if (besideW > 0)
            {
                OZT_VppKit.MoveX(chip.FindAnyWidget("SelOwner"), x + 6 * s);
                x = x + 6 * s + besideW;
            }
            OZT_VppKit.MoveX(chip.FindAnyWidget("BtnSelDel"), x + 4 * s);
            chip.GetSize(x, h);
            chip.SetSize(w, h);
            OZT_VppKit.SetWidth(chip.FindAnyWidget("OZT_GtSelBg"), w);
            m_TrdGtPkChips.SetWidth(i, w);
        }
        m_TrdGtPkChips.Flow(full, 6 * s, 26 * s, null, 0);
    }

    // A value chosen or let go in the picker's list.
    protected void TrdGtPkToggle(string value)
    {
        if (!m_TrdGtPkOn)
        {
            Hint("#STR_OZT_VPP_GT_PK_IDLE");
            return;
        }
        int at = m_TrdGtPkVals.Find(value);
        if (at != -1)
            m_TrdGtPkVals.RemoveOrdered(at);
        else
            m_TrdGtPkVals.Insert(value);
        if (m_TrdGtPkValRows)
        {
            int row = m_TrdGtPkValRows.Find(value);
            if (row != -1)
                TrdGtPkMarkValue(m_TrdGtPkValRows.Row(row), at == -1);
        }
        TrdGtPkPaintGroups();
        TrdGtPkPaintChosen();
    }

    // The Arg field typed into: its words are the values (a kind with no choices is
    // written so, and any value may be).
    protected void TrdGtPkTyped()
    {
        if (!m_TrdGtPkOn)
            return;
        OZT_VppKit.Alts(GetEdit("TrdGtArg"), m_TrdGtPkVals);
        if (m_TrdGtPkValRows)
        {
            for (int i = 0; i < m_TrdGtPkValRows.Count(); i++)
                TrdGtPkMarkValue(m_TrdGtPkValRows.Row(i), m_TrdGtPkVals.Find(m_TrdGtPkValRows.Key(i)) != -1);
        }
    }

    // The kind one step on or back through the kinds the providers answer.
    protected void TrdGtKindStep(int step)
    {
        array<string> kinds = new array<string>();
        TrdKinds(kinds);
        if (kinds.Count() == 0)
            return;
        int at = kinds.Find(TrdGtPkKind()) + step;
        if (at < 0)
            at = kinds.Count() - 1;
        if (at >= kinds.Count())
            at = 0;
        m_TrdGtPut = true;
        SetEdit("TrdGtKind", kinds[at]);
        m_TrdGtPut = false;
        TrdGtPkKindChanged();
    }

    // Another kind: its own values, and none of the old kind's chosen.
    protected void TrdGtPkKindChanged()
    {
        TrdGtPkInit();
        m_TrdGtPkVals.Clear();
        m_TrdGtPkOwner = "";
        m_TrdGtPkBranch = "";
        TrdGtPaintPicker();
    }

    // "Done": the open condition into the draft -- a changed one in its place, a new
    // one at the end of its list, a computed discount as a discount of its own.
    protected void TrdGtPkDone()
    {
        if (!m_TrdGtPkOn)
        {
            Hint("#STR_OZT_VPP_GT_PK_IDLE");
            return;
        }
        string kind = TrdGtPkKind();
        if (kind == "")
        {
            Hint("#STR_OZT_VPP_GT_NEED_KIND");
            return;
        }
        string arg = OZT_VppKit.Joined(m_TrdGtPkVals, "|");
        OZT_Condition c;
        if (m_TrdGtPkSec == 0)
        {
            array<ref OZT_Condition> lock = TrdGtLock(true);
            if (!lock)
                return;
            if (m_TrdGtPkCond >= 0 && m_TrdGtPkCond < lock.Count())
                c = lock[m_TrdGtPkCond];
            else
            {
                c = new OZT_Condition();
                lock.Insert(c);
            }
        }
        else
        {
            array<ref OZT_Discount> discs = TrdGtDiscs(true);
            if (!discs)
            {
                Hint("#STR_OZT_VPP_GT_NO_DISC_HERE");
                return;
            }
            OZT_Discount d;
            if (m_TrdGtPkDisc >= 0 && m_TrdGtPkDisc < discs.Count())
                d = discs[m_TrdGtPkDisc];
            else
            {
                d = new OZT_Discount();
                d.When = new array<ref OZT_Condition>();
                discs.Insert(d);
            }
            if (m_TrdGtPkSec == 2)
            {
                if (!d.By)
                    d.By = new OZT_Condition();
                c = d.By;
            }
            else
            {
                if (!d.When)
                    d.When = new array<ref OZT_Condition>();
                if (m_TrdGtPkCond >= 0 && m_TrdGtPkCond < d.When.Count())
                    c = d.When[m_TrdGtPkCond];
                else
                {
                    c = new OZT_Condition();
                    d.When.Insert(c);
                }
            }
        }
        c.Kind = kind;
        c.Arg = arg;
        TrdGtPkClose();
        TrdGtTouched();
        TrdGtAfterEdit();
        TrdGtPaintPicker();
    }

    protected void TrdGtPkCancel()
    {
        TrdGtPkClose();
        TrdGtFillMid();
        TrdGtPaintPicker();
    }

    // ------------------------------------------------------------------ edits of rows

    // The condition of a lock's row or of a discount's, by the row a button sits in.
    protected OZT_Condition TrdGtRowCond(Widget w, out int sec, out int disc, out int cond)
    {
        sec = 0;
        disc = -1;
        cond = -1;
        if (!m_TrdGtMid)
            return null;
        int i = m_TrdGtMid[TRD_GT_COND].IndexOf(w);
        if (i != -1)
        {
            array<ref OZT_Condition> lock = TrdGtLock(false);
            cond = i;
            if (!lock || i >= lock.Count())
                return null;
            return lock[i];
        }
        i = m_TrdGtMid[TRD_GT_DISC].IndexOf(w);
        if (i == -1)
            return null;
        string key = m_TrdGtMid[TRD_GT_DISC].Key(i);
        OZT_Discount d = TrdGtDiscOf(key, cond);
        disc = key.Substring(0, key.IndexOf("|")).ToInt();
        sec = 1;
        if (cond == -2)
            sec = 2;
        return TrdGtDiscCond(d, cond);
    }

    // A row's values pressed: its condition opens in the picker.
    protected void TrdGtRowOpen(Widget w)
    {
        int sec;
        int disc;
        int cond;
        OZT_Condition c = TrdGtRowCond(w, sec, disc, cond);
        if (!c)
            return;
        TrdGtPkOpen(sec, disc, cond, c.Kind, c.Arg);
    }

    // A row's "not": the condition turned round, or back.
    protected void TrdGtRowNot(Widget w)
    {
        int sec;
        int disc;
        int cond;
        OZT_Condition c = TrdGtRowCond(w, sec, disc, cond);
        if (!c)
            return;
        if (c.Not == 1)
            c.Not = 0;
        else
            c.Not = 1;
        TrdGtTouched();
        TrdGtFillMid();
        TrdGtPaintList();
        TrdGtPaintHead();
    }

    // A lock's row out.
    protected void TrdGtCondDel(Widget w)
    {
        int i = m_TrdGtMid[TRD_GT_COND].IndexOf(w);
        array<ref OZT_Condition> lock = TrdGtLock(false);
        if (i == -1 || !lock || i >= lock.Count())
            return;
        lock.RemoveOrdered(i);
        TrdGtPkClose();
        TrdGtTouched();
        TrdGtAfterEdit();
        TrdGtPaintPicker();
    }

    // A discount's row out: a condition of several goes alone, the last one takes the
    // discount with it.
    protected void TrdGtDiscDel(Widget w)
    {
        int i = m_TrdGtMid[TRD_GT_DISC].IndexOf(w);
        if (i == -1)
            return;
        string key = m_TrdGtMid[TRD_GT_DISC].Key(i);
        int cond;
        OZT_Discount d = TrdGtDiscOf(key, cond);
        array<ref OZT_Discount> discs = TrdGtDiscs(false);
        if (!d || !discs)
            return;
        int at = key.Substring(0, key.IndexOf("|")).ToInt();
        bool whole = cond < 0 || !d.When || d.When.Count() <= 1;
        if (whole)
            discs.RemoveOrdered(at);
        else
            d.When.RemoveOrdered(cond);
        TrdGtPkClose();
        TrdGtTouched();
        TrdGtAfterEdit();
        TrdGtPaintPicker();
    }

    // "+ and": one more condition the row's discount waits for, picked in the picker.
    protected void TrdGtDiscAnd(Widget w)
    {
        int i = m_TrdGtMid[TRD_GT_DISC].IndexOf(w);
        if (i == -1)
            return;
        string key = m_TrdGtMid[TRD_GT_DISC].Key(i);
        int cond;
        OZT_Discount d = TrdGtDiscOf(key, cond);
        if (!d || d.By)
        {
            Hint("#STR_OZT_VPP_GT_PICK_DISC");
            return;
        }
        int at = key.Substring(0, key.IndexOf("|")).ToInt();
        TrdGtPkOpen(1, at, -1, TrdGtFirstKind(), "");
    }

    // The kind a new condition starts with: research when its gate answers, else the
    // first kind somebody answers.
    protected string TrdGtFirstKind()
    {
        if (TrdFamilyOf("research") != "")
            return "research";
        array<string> kinds = new array<string>();
        TrdKinds(kinds);
        if (kinds.Count() > 0)
            return kinds[0];
        return "";
    }

    // A discount's percent typed into (a computed one's cap): the draft follows.
    protected void TrdGtDiscTyped(Widget w)
    {
        int i = m_TrdGtMid[TRD_GT_DISC].IndexOf(w);
        EditBoxWidget box = EditBoxWidget.Cast(w);
        if (i == -1 || !box)
            return;
        int cond;
        OZT_Discount d = TrdGtDiscOf(m_TrdGtMid[TRD_GT_DISC].Key(i), cond);
        if (!d)
            return;
        float v = box.GetText().Trim().ToFloat();
        if (v < 0)
            v = 0;
        if (v > 100)
            v = 100;
        if (cond == -2)
        {
            if (d.Max == v)
                return;
            d.Max = v;
        }
        else
        {
            if (d.Percent == v)
                return;
            d.Percent = v;
        }
        // The field is left as it is typed; the row is written again when the page is.
        TrdGtTouched();
        TrdShow("TrdGtChanged", true);
        TrdGtPlaceHead();
    }

    // "+ discount on a condition": a discount of nothing yet, its first condition asked
    // for in the picker (a discount of 0 % gives nothing until its percent is typed).
    protected void TrdGtDiscAdd()
    {
        array<ref OZT_Discount> discs = TrdGtDiscs(true);
        if (!discs)
        {
            Hint("#STR_OZT_VPP_GT_NO_DISC_HERE");
            return;
        }
        OZT_Discount d = new OZT_Discount();
        d.When = new array<ref OZT_Condition>();
        discs.Insert(d);
        TrdGtTouched();
        TrdGtAfterEdit();
        TrdGtPkOpen(1, discs.Count() - 1, -1, TrdGtFirstKind(), "");
    }

    // "+ discount a mod computes": made when its condition is picked (it cannot be
    // without one).
    protected void TrdGtDiscAddBy()
    {
        if (!TrdGtDiscs(true))
        {
            Hint("#STR_OZT_VPP_GT_NO_DISC_HERE");
            return;
        }
        TrdGtPkOpen(2, -1, -1, TrdGtFirstKind(), "");
    }

    // A kind's button of the lock's add row: a new condition of that kind, picked in
    // the picker; the row's last button steps to more kinds when there are many.
    protected void TrdGtAddKind(int n)
    {
        if (!m_TrdGtAddKinds || n < 0 || n >= m_TrdGtAddKinds.Count())
            return;
        string kind = m_TrdGtAddKinds[n];
        if (kind == ">")
        {
            m_TrdGtKindFrom = m_TrdGtKindFrom + 5;
            if (m_TrdGtMid[TRD_GT_ADD].Count() > 0)
                TrdGtFillAdd(m_TrdGtMid[TRD_GT_ADD].Row(0));
            return;
        }
        if (kind == "")
            return;
        if (m_TrdGates && TrdFamilyOf(kind) == "")
            Hint("#STR_OZT_VPP_GT_NOBODY");
        TrdGtPkOpen(0, -1, -1, kind, "");
    }

    // ------------------------------------------------------------------ copy and paste

    protected void TrdGtCopy()
    {
        array<ref OZT_Condition> lock = TrdGtLock(false);
        array<ref OZT_Discount> discs = TrdGtDiscs(false);
        if (m_TrdGtTgt == "")
        {
            Hint("#STR_OZT_VPP_GT_NEED_TARGET");
            return;
        }
        m_TrdGtClipLock = new array<ref OZT_Condition>();
        m_TrdGtClipDisc = new array<ref OZT_Discount>();
        OZT_Condition.CopyAll(lock, m_TrdGtClipLock);
        OZT_Discount.CopyAll(discs, m_TrdGtClipDisc);
        int n = m_TrdGtClipLock.Count() + m_TrdGtClipDisc.Count();
        string said = TrdTr("#STR_OZT_VPP_GT_COPIED");
        Hint(said + " " + n.ToString());
    }

    protected bool TrdGtSameCond(OZT_Condition a, OZT_Condition b)
    {
        return a.Kind == b.Kind && a.Arg == b.Arg && a.Not == b.Not;
    }

    // The copied conditions into the picked target, those it does not have already: the
    // lock's into its lock, the discounts into its discounts (a point has none).
    protected void TrdGtPaste()
    {
        if (m_TrdGtTgt == "")
        {
            Hint("#STR_OZT_VPP_GT_NEED_TARGET");
            return;
        }
        if (!m_TrdGtClipLock || m_TrdGtClipLock.Count() + m_TrdGtClipDisc.Count() == 0)
        {
            Hint("#STR_OZT_VPP_GT_CLIP_EMPTY");
            return;
        }
        int added = 0;
        int i;
        int k;
        array<ref OZT_Condition> lock = TrdGtLock(true);
        if (lock)
        {
            for (i = 0; i < m_TrdGtClipLock.Count(); i++)
            {
                bool had = false;
                for (k = 0; k < lock.Count(); k++)
                {
                    if (TrdGtSameCond(lock[k], m_TrdGtClipLock[i]))
                        had = true;
                }
                if (had)
                    continue;
                lock.Insert(m_TrdGtClipLock[i].Copy());
                added++;
            }
        }
        array<ref OZT_Discount> discs = TrdGtDiscs(true);
        if (discs)
        {
            for (i = 0; i < m_TrdGtClipDisc.Count(); i++)
            {
                discs.Insert(m_TrdGtClipDisc[i].Copy());
                added++;
            }
        }
        string said = TrdTr("#STR_OZT_VPP_GT_PASTED");
        Hint(said + " " + added.ToString());
        if (added == 0)
            return;
        TrdGtPkClose();
        TrdGtTouched();
        TrdGtAfterEdit();
        TrdGtPaintPicker();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdGtPut && TrdGtChanged(w, finished))
            return true;
        return super.OnChange(w, x, y, finished);
    }

    protected bool TrdGtChanged(Widget w, bool finished)
    {
        if (!TrdGtOwns(w))
            return false;
        string nm = w.GetName();
        if (nm == "TrdGtFind")
        {
            if (TrdGtLoaded())
                TrdGtPaintList();
            return true;
        }
        if (nm == "TrdGtPickFind")
        {
            TrdGtPkPaintValues();
            return true;
        }
        if (nm == "TrdGtKind")
        {
            TrdGtPkKindChanged();
            return true;
        }
        if (nm == "TrdGtArg")
        {
            TrdGtPkTyped();
            if (finished)
                TrdGtPkPaintChosen();
            return true;
        }
        if (nm == "DiscPct" && m_TrdGtMid && TrdGtLoaded())
        {
            TrdGtDiscTyped(w);
            if (finished)
                TrdGtPaintList();
            return true;
        }
        return false;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET && TrdGtClick(w))
            return true;
        return super.OnClick(w, x, y, button);
    }

    // Is a widget one of this page's own: anything under the page's root? (The rows'
    // buttons carry short names, and another page may name a button of its own alike.)
    protected bool TrdGtOwns(Widget w)
    {
        if (!m_TrdPanes)
            return false;
        Widget pane = m_TrdPanes.Get("gates");
        if (!pane)
            return false;
        Widget at = w;
        while (at)
        {
            if (at == pane)
                return true;
            at = at.GetParent();
        }
        return false;
    }

    protected bool TrdGtClick(Widget w)
    {
        if (!TrdGtOwns(w))
            return false;
        string nm = w.GetName();
        if (nm == "BtnTrdGtRefresh")
        {
            TrdGtHoldForget();
            TrdAskGates();
            TrdReread(OZT_Const.CFG_SETTINGS);
            TrdReread(OZT_Const.CFG_GOODS);
            TrdReread(OZT_Const.CFG_TRADERS);
            return true;
        }
        // An edit box takes its click itself.
        if (EditBoxWidget.Cast(w))
            return false;
        if (!TrdGtLoaded())
        {
            Hint("#STR_OZT_VPP_READING");
            return true;
        }
        int i;

        // The head and the families' strip.
        if (nm == "BtnTrdGtCopy")
        {
            TrdGtCopy();
            return true;
        }
        if (nm == "BtnTrdGtPaste")
        {
            TrdGtPaste();
            return true;
        }
        if (nm == "TrdGtPoint")
        {
            TrdGtFamNextPoint();
            return true;
        }
        if (nm == "BtnTrdGtFamMore")
        {
            m_TrdGtFamFrom = m_TrdGtFamFrom + 3;
            TrdGtPaintFams();
            return true;
        }
        if (nm.IndexOf("BtnTrdGtFam") == 0 && nm.Length() > 12)
        {
            int slot = nm.Substring(11, 1).ToInt();
            string verb = nm.Substring(12, nm.Length() - 12);
            if (verb == "Srv")
                TrdGtFamServer(slot);
            else if (verb == "As")
                TrdGtFamHere(slot, 0);
            else if (verb == "On")
                TrdGtFamHere(slot, 1);
            else if (verb == "Off")
                TrdGtFamHere(slot, 2);
            return true;
        }

        // The left list and its two filters.
        if (nm == "BtnTrdGtWith" || nm == "BtnTrdGtAll")
        {
            bool all = nm == "BtnTrdGtAll";
            m_TrdGtAll = all;
            TrdGtPaintList();
            return true;
        }
        if (m_TrdGtGroups)
        {
            i = m_TrdGtGroups.IndexOf(w);
            if (i != -1)
            {
                TrdGtPick(m_TrdGtGroups.Key(i));
                return true;
            }
            i = m_TrdGtLines.IndexOf(w);
            if (i != -1)
            {
                TrdGtPick(m_TrdGtLines.Key(i));
                return true;
            }
            i = m_TrdGtWarns.IndexOf(w);
            if (i != -1)
            {
                TrdGtPick(m_TrdGtWarns.Key(i));
                return true;
            }
        }

        // The middle list's rows.
        if (nm == "BtnCondVals" || nm == "BtnDiscVals")
        {
            TrdGtRowOpen(w);
            return true;
        }
        if (nm == "BtnCondNot" || nm == "BtnDiscNot")
        {
            TrdGtRowNot(w);
            return true;
        }
        if (nm == "BtnCondDel")
        {
            TrdGtCondDel(w);
            return true;
        }
        if (nm == "BtnDiscDel")
        {
            TrdGtDiscDel(w);
            return true;
        }
        if (nm == "BtnDiscAnd")
        {
            TrdGtDiscAnd(w);
            return true;
        }
        if (nm == "BtnDiscAddWhen")
        {
            TrdGtDiscAdd();
            return true;
        }
        if (nm == "BtnDiscAddBy")
        {
            TrdGtDiscAddBy();
            return true;
        }
        if (nm.IndexOf("BtnAddKind") == 0 && nm.Length() == 11)
        {
            TrdGtAddKind(nm.Substring(10, 1).ToInt());
            return true;
        }

        // The picker.
        if (nm == "BtnTrdGtPkDone")
        {
            TrdGtPkDone();
            return true;
        }
        if (nm == "BtnTrdGtPkCancel" || nm == "BtnTrdGtPkClose")
        {
            TrdGtPkCancel();
            return true;
        }
        if (nm == "BtnTrdGtKindPrev")
        {
            TrdGtKindStep(-1);
            return true;
        }
        if (nm == "BtnTrdGtKindNext")
        {
            TrdGtKindStep(1);
            return true;
        }
        if (nm == "BtnSelDel" && m_TrdGtPkChips)
        {
            i = m_TrdGtPkChips.IndexOf(w);
            if (i != -1)
                TrdGtPkToggle(m_TrdGtPkChips.Key(i));
            return true;
        }
        if (m_TrdGtPkValRows)
        {
            i = m_TrdGtPkValRows.IndexOf(w);
            if (i != -1)
            {
                TrdGtPkToggle(m_TrdGtPkValRows.Key(i));
                return true;
            }
        }
        if (m_TrdGtPkOwnRows)
        {
            i = m_TrdGtPkOwnRows.IndexOf(w);
            if (i != -1)
            {
                m_TrdGtPkOwner = m_TrdGtPkOwnRows.Key(i);
                m_TrdGtPkBranch = "";
                TrdGtPkPaintGroups();
                TrdGtPkPaintValues();
                return true;
            }
        }
        if (m_TrdGtPkBrRows)
        {
            i = m_TrdGtPkBrRows.IndexOf(w);
            if (i != -1)
            {
                string branch = m_TrdGtPkBrRows.Key(i);
                if (branch == m_TrdGtPkBranch)
                    branch = "";
                m_TrdGtPkBranch = branch;
                TrdGtPkPaintGroups();
                TrdGtPkPaintValues();
                return true;
            }
        }
        return false;
    }
}

#endif
#endif
#endif
