// Trader > Money (design §5.6; mockup TraderMoney): what holds for every trader of
// the server -- the settings file's money (cash, cash then the TraderPlus bank, the
// bank, or another provider's id), its banknotes and wallets, the share of the
// price a worn thing brings, the stock at a server start, and the deal rules --
// edited in the draft of the trader's files and saved by the draft's bar, which the
// module's frame lays over the page's foot, or by the button of the page's head (the
// frame answers it: Btn<TrdMn>Apply). A point's own money is the points page's.
//
// The page is converted from the mockup (ui/css2layout.json, page `money`). The
// banknotes are a list of row templates; the four shares are sliders (a track, a
// fill and a knob the script moves: the pointer is followed while its button is
// held, TrdMnFollowSliders); the switches are checkboxes and segments painted by
// their state.
//
// A layer of OZ_VppAdminMenu after OZT_VppPaneGates.c (it takes OZT_VppKit from it).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_MN_NOTE = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_money_note.layout";

    // The banknotes' rows and the class of the one picked ("" none); the slider the
    // pointer holds (1..4, 0 none) and whether its button was down a frame ago; the
    // script writing a field itself.
    protected ref OZ_VppFlexRows m_TrdMnRows;
    protected string m_TrdMnNote;
    protected int    m_TrdMnDrag;
    protected bool   m_TrdMnDown;
    protected bool   m_TrdMnPut;

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "money")
            return;
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdMnPaint();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "money")
            TrdMnPaint();
    }

    // The window changed its size: the list follows it, and the knobs and fills stand by
    // their values again (a resize moves them by the sample's shares).
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (!M_SUB_WIDGET || TrdShownKey() != "money")
            return;
        if (m_TrdMnRows)
            m_TrdMnRows.Fit();
        TrdMnPaintSliders();
    }

    // Every frame while the window is open: the sliders follow the pointer.
    override void Tick()
    {
        super.Tick();
        if (M_SUB_WIDGET && TrdShownKey() == "money")
            TrdMnFollowSliders();
    }

    protected OZT_Settings TrdMnSet()
    {
        if (!m_TrdDraft || !m_TrdDraft.Loaded(OZT_Const.CFG_SETTINGS))
            return null;
        return m_TrdDraft.Settings;
    }

    // A field written by the script: the OnChange it may raise is not an edit.
    protected void TrdMnSetField(string name, string text)
    {
        m_TrdMnPut = true;
        SetEdit(name, text);
        m_TrdMnPut = false;
    }

    // An edit was made to the settings: the draft knows (and its bar says so).
    protected void TrdMnTouched()
    {
        TrdTouch(OZT_Const.CFG_SETTINGS);
    }

    // A comma list typed by the admin, trimmed, the empty ones left out.
    protected void TrdMnSplit(string text, array<string> into)
    {
        into.Clear();
        array<string> parts = new array<string>();
        text.Split(",", parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            string one = parts[i].Trim();
            if (one != "")
                into.Insert(one);
        }
    }

    // ------------------------------------------------------------------ painting

    protected void TrdMnPaint()
    {
        if (!M_SUB_WIDGET)
            return;
        OZT_Settings st = TrdMnSet();
        if (!st)
        {
            SayTrd("TrdMnSub", "#STR_OZT_VPP_READING");
            return;
        }
        SayTrd("TrdMnSub", "#STR_OZT_VPP_MN_SUB");
        TrdMnSetField("TrdMnProv", st.Provider);
        TrdMnSetField("TrdMnHolders", TrdJoin(st.Holders, ","));
        TrdMnSetField("TrdMnFloor", TrdNum(st.StockFloor));
        TrdMnSetField("TrdMnCooldown", st.CooldownMs.ToString());
        TrdMnPaintNotes();
        TrdMnPaintSliders();
        TrdMnPaintSwitches();
    }

    // The banknotes, a row each, the picked one lit; each row's own button removes it.
    protected void TrdMnPaintNotes()
    {
        OZT_Settings st = TrdMnSet();
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdMnNotesRows");
        if (!st || !host)
            return;
        if (!m_TrdMnRows)
            m_TrdMnRows = new OZ_VppFlexRows(m_Flex, host, TRD_MN_NOTE, "OZT_MnNote", 614, 28);
        m_TrdMnRows.Clear();
        Widget row;
        for (int i = 0; i < st.Notes.Count(); i++)
        {
            row = m_TrdMnRows.Add(st.Notes[i].ClassName);
            if (!row)
                continue;
            OZT_VppKit.PartInk(row, "NoteCls", st.Notes[i].ClassName, OZT_Metal.TEXT);
            OZT_VppKit.PartInk(row, "NoteVal", st.Notes[i].Value.ToString(), OZT_Metal.TEXT);
        }
        m_TrdMnRows.Fit();
        TrdMnMarkNotes();
        TrdMnPaintNotesHint();
    }

    protected void TrdMnMarkNotes()
    {
        if (!m_TrdMnRows)
            return;
        for (int i = 0; i < m_TrdMnRows.Count(); i++)
        {
            if (m_TrdMnNote != "" && m_TrdMnRows.Key(i) == m_TrdMnNote)
                OZ_VppFlex.Paint(m_TrdMnRows.Row(i), "OZT_MnNoteBg", OZT_Metal.PICK_HEAD);
            else
                OZ_VppFlex.Paint(m_TrdMnRows.Row(i), "OZT_MnNoteBg", OZT_Metal.HEAD);
        }
    }

    // Change is exact only when every value divides by the smallest: the line under the
    // notes names those that do not.
    protected void TrdMnPaintNotesHint()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        int least = 0;
        int i;
        for (i = 0; i < st.Notes.Count(); i++)
        {
            if (st.Notes[i].Value > 0 && (least == 0 || st.Notes[i].Value < least))
                least = st.Notes[i].Value;
        }
        string odd = "";
        if (least > 0)
        {
            for (i = 0; i < st.Notes.Count(); i++)
            {
                int left = st.Notes[i].Value % least;
                if (left == 0)
                    continue;
                if (odd != "")
                    odd = odd + ", ";
                odd = odd + st.Notes[i].ClassName + " = " + st.Notes[i].Value.ToString();
            }
        }
        if (odd == "")
        {
            TrdInk("TrdMnNotesHint", "#STR_OZT_VPP_MN_NOTES_H", OZT_Metal.MUTED);
            return;
        }
        string head = TrdTr("#STR_OZT_VPP_MN_NOTES_BAD");
        TrdInk("TrdMnNotesHint", head + " (" + least.ToString() + "): " + odd, OZT_Metal.ALERT);
    }

    // The share of the price by condition, pristine first.
    protected float TrdMnShare(OZT_Settings st, int n)
    {
        if (n == 0)
            return st.Condition.Pristine;
        if (n == 1)
            return st.Condition.Worn;
        if (n == 2)
            return st.Condition.Damaged;
        return st.Condition.BadlyDamaged;
    }

    protected void TrdMnPaintSliders()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        for (int n = 0; n < 4; n++)
            TrdMnPaintSlider(n, TrdMnShare(st, n));
    }

    // One slider at a share (0..1): its knob along the track, its fill up to the knob,
    // its value in percent beside it.
    protected void TrdMnPaintSlider(int n, float share)
    {
        string name = "TrdMnCond" + n.ToString();
        Widget box = M_SUB_WIDGET.FindAnyWidget(name);
        Widget fill = M_SUB_WIDGET.FindAnyWidget(name + "Fill");
        Widget knob = M_SUB_WIDGET.FindAnyWidget(name + "Knob");
        if (!box || !fill || !knob)
            return;
        float w;
        float h;
        float kw;
        float kh;
        float x;
        float y;
        box.GetSize(w, h);
        knob.GetSize(kw, kh);
        knob.GetPos(x, y);
        float part = share;
        if (part < 0)
            part = 0;
        if (part > 1)
            part = 1;
        knob.SetPos((w - kw) * part, y);
        fill.GetSize(x, h);
        fill.SetSize(w * part, h);
        int pct = Math.Round(share * 100);
        TrdInk(name + "Val", pct.ToString() + " %", OZT_Metal.MUTED);
    }

    // The pointer's button held over a slider moves it, a percent a step, until the
    // button is let go. Only a press that begins over a slider of this page, inside the
    // page's scrolled view, takes one.
    protected void TrdMnFollowSliders()
    {
        bool down = false;
        if (GetMouseState(MouseState.LEFT) & 0x80000000)
            down = true;
        bool was = m_TrdMnDown;
        m_TrdMnDown = down;
        if (!down)
        {
            m_TrdMnDrag = 0;
            return;
        }
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;

        int mx;
        int my;
        GetMousePos(mx, my);
        float sx;
        float sy;
        float sw;
        float sh;
        Widget box;
        if (m_TrdMnDrag == 0)
        {
            if (was)
                return;
            Widget view = M_SUB_WIDGET.FindAnyWidget("TrdMnBodyScroll");
            if (!view || !TrdMnOverWindow())
                return;
            view.GetScreenPos(sx, sy);
            view.GetScreenSize(sw, sh);
            if (mx < sx || mx > sx + sw || my < sy || my > sy + sh)
                return;
            for (int n = 0; n < 4; n++)
            {
                box = M_SUB_WIDGET.FindAnyWidget("TrdMnCond" + n.ToString());
                if (!box)
                    continue;
                box.GetScreenPos(sx, sy);
                box.GetScreenSize(sw, sh);
                if (mx >= sx - 8 && mx <= sx + sw + 8 && my >= sy - 5 && my <= sy + sh + 5)
                    m_TrdMnDrag = n + 1;
            }
            if (m_TrdMnDrag == 0)
                return;
        }

        int at = m_TrdMnDrag - 1;
        box = M_SUB_WIDGET.FindAnyWidget("TrdMnCond" + at.ToString());
        Widget knob = M_SUB_WIDGET.FindAnyWidget("TrdMnCond" + at.ToString() + "Knob");
        if (!box || !knob)
            return;
        float kw;
        float kh;
        box.GetScreenPos(sx, sy);
        box.GetScreenSize(sw, sh);
        knob.GetScreenSize(kw, kh);
        float room = sw - kw;
        if (room <= 0)
            return;
        float part = (mx - sx - kw * 0.5) / room;
        if (part < 0)
            part = 0;
        if (part > 1)
            part = 1;
        int pct = Math.Round(part * 100);
        float share = pct / 100.0;
        int had = Math.Round(TrdMnShare(st, at) * 100);
        if (pct == had)
            return;
        if (at == 0)
            st.Condition.Pristine = share;
        else if (at == 1)
            st.Condition.Worn = share;
        else if (at == 2)
            st.Condition.Damaged = share;
        else
            st.Condition.BadlyDamaged = share;
        TrdMnPaintSlider(at, share);
        TrdMnTouched();
    }

    // Is the pointer over this window (and not over another window lying on it)?
    protected bool TrdMnOverWindow()
    {
        Widget under = GetWidgetUnderCursor();
        while (under)
        {
            if (under == M_SUB_WIDGET)
                return true;
            under = under.GetParent();
        }
        return false;
    }

    // One of three ways to pay lit: its frame in the accent, its mark filled.
    protected void TrdMnLight(string button, bool on)
    {
        if (on)
        {
            OZT_VppKit.PartBox(M_SUB_WIDGET, button, OZT_Metal.ACCENT, OZT_Metal.PICK_HEAD);
            OZT_VppKit.PartBox(M_SUB_WIDGET, button + "Dot", OZT_Metal.ACCENT, OZT_Metal.TEXT);
            return;
        }
        OZT_VppKit.PartBox(M_SUB_WIDGET, button, OZT_Metal.EDGE, OZT_Metal.CELL);
        OZT_VppKit.PartBox(M_SUB_WIDGET, button + "Dot", OZT_Metal.MUTED, OZT_Metal.CELL);
    }

    protected void TrdMnPaintSwitches()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        TrdMnLight("BtnTrdMnItems", st.Provider == OZT_Const.PROVIDER_ITEMS);
        TrdMnLight("BtnTrdMnBoth", st.Provider == "items+traderplus");
        TrdMnLight("BtnTrdMnBank", st.Provider == "traderplus");
        OZT_VppKit.SegPaint(M_SUB_WIDGET, "BtnTrdMnStart0", st.StartStock == 0);
        OZT_VppKit.SegPaint(M_SUB_WIDGET, "BtnTrdMnStart1", st.StartStock == 1);
        OZT_VppKit.SegPaint(M_SUB_WIDGET, "BtnTrdMnStart2", st.StartStock == 2);
        OZT_VppKit.TickMark(M_SUB_WIDGET, "TrdMnDestockTick", st.DestockAtStart == 1);
        OZT_VppKit.TickMark(M_SUB_WIDGET, "TrdMnPristineTick", st.StockOnlyPristine == 1);
        OZT_VppKit.TickMark(M_SUB_WIDGET, "TrdMnWearTick", st.WearBought == 1);
    }

    // ------------------------------------------------------------------ edits

    protected bool TrdMnIsField(string nm)
    {
        array<string> fields = {"TrdMnProv", "TrdMnHolders", "TrdMnFloor", "TrdMnCooldown"};
        return fields.Find(nm) != -1;
    }

    // A field typed into: the draft's settings follow it. A value equal to the
    // draft's is no edit.
    protected void TrdMnEdit(string nm)
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        string v = GetEdit(nm).Trim();
        float f = v.ToFloat();
        int n = v.ToInt();
        bool changed = true;
        if (nm == "TrdMnProv")
        {
            if (st.Provider == v)
                changed = false;
            st.Provider = v;
            TrdMnPaintSwitches();
        }
        else if (nm == "TrdMnHolders")
        {
            array<string> parts = new array<string>();
            TrdMnSplit(v, parts);
            if (TrdJoin(parts, ",") == TrdJoin(st.Holders, ","))
                changed = false;
            TrdMnSplit(v, st.Holders);
        }
        else if (nm == "TrdMnFloor")
        {
            if (st.StockFloor == f)
                changed = false;
            st.StockFloor = f;
        }
        else if (nm == "TrdMnCooldown")
        {
            if (st.CooldownMs == n)
                changed = false;
            st.CooldownMs = n;
        }
        if (changed)
            TrdMnTouched();
    }

    protected void TrdMnProvider(string provider)
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        st.Provider = provider;
        TrdMnSetField("TrdMnProv", provider);
        TrdMnPaintSwitches();
        TrdMnTouched();
    }

    // A note of the class in the fields: its value changed, or a new note in its
    // place by value.
    protected void TrdMnNoteSet()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        string cls = GetEdit("TrdMnNoteCls").Trim();
        int worth = GetEdit("TrdMnNoteVal").Trim().ToInt();
        if (cls == "" || worth <= 0)
        {
            Hint("#STR_OZT_VPP_MN_NOTE_NEED");
            return;
        }
        m_TrdMnNote = cls;
        for (int i = 0; i < st.Notes.Count(); i++)
        {
            if (st.Notes[i].ClassName == cls)
            {
                st.Notes[i].Value = worth;
                TrdMnTouched();
                TrdMnPaintNotes();
                return;
            }
        }
        OZT_Note n = new OZT_Note();
        n.ClassName = cls;
        n.Value = worth;
        int at = 0;
        while (at < st.Notes.Count() && st.Notes[at].Value <= worth)
            at++;
        st.Notes.InsertAt(n, at);
        TrdMnTouched();
        TrdMnPaintNotes();
    }

    // The note of a row out of the list.
    protected void TrdMnNoteDel(int at)
    {
        OZT_Settings st = TrdMnSet();
        if (!st || at < 0 || at >= st.Notes.Count())
            return;
        if (st.Notes[at].ClassName == m_TrdMnNote)
            m_TrdMnNote = "";
        st.Notes.RemoveOrdered(at);
        TrdMnTouched();
        TrdMnPaintNotes();
    }

    // A row picked: the two fields take its note.
    protected void TrdMnNotePick(int at)
    {
        OZT_Settings st = TrdMnSet();
        if (!st || at < 0 || at >= st.Notes.Count())
            return;
        m_TrdMnNote = st.Notes[at].ClassName;
        TrdMnSetField("TrdMnNoteCls", st.Notes[at].ClassName);
        TrdMnSetField("TrdMnNoteVal", st.Notes[at].Value.ToString());
        TrdMnMarkNotes();
    }

    protected void TrdMnFlip(string what)
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        if (what == "destock")
        {
            if (st.DestockAtStart == 1)
                st.DestockAtStart = 0;
            else
                st.DestockAtStart = 1;
        }
        else if (what == "pristine")
        {
            if (st.StockOnlyPristine == 1)
                st.StockOnlyPristine = 0;
            else
                st.StockOnlyPristine = 1;
        }
        else if (what == "wear")
        {
            if (st.WearBought == 1)
                st.WearBought = 0;
            else
                st.WearBought = 1;
        }
        TrdMnPaintSwitches();
        TrdMnTouched();
    }

    protected void TrdMnStart(int start)
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        st.StartStock = start;
        TrdMnPaintSwitches();
        TrdMnTouched();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdMnPut && TrdMnIsField(w.GetName()))
        {
            TrdMnEdit(w.GetName());
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET && TrdMnClick(w))
            return true;
        return super.OnClick(w, x, y, button);
    }

    protected bool TrdMnClick(Widget w)
    {
        string nm = w.GetName();
        // The head's "save and reload" is the draft's apply: the frame answers it.
        if (nm == "BtnTrdMnApply")
            return false;

        int at = -1;
        if (m_TrdMnRows && (nm == "BtnNotePick" || nm == "BtnNoteDel"))
            at = m_TrdMnRows.IndexOf(w);
        bool mine = at != -1 || nm.IndexOf("BtnTrdMn") == 0 || nm.IndexOf("TrdMn") == 0;
        if (!mine)
            return false;
        if (!TrdMnSet())
        {
            Hint("#STR_OZT_VPP_READING");
            return true;
        }
        if (at != -1)
        {
            if (nm == "BtnNoteDel")
                TrdMnNoteDel(at);
            else
                TrdMnNotePick(at);
            return true;
        }
        if (nm == "BtnTrdMnItems")
        {
            TrdMnProvider(OZT_Const.PROVIDER_ITEMS);
            return true;
        }
        if (nm == "BtnTrdMnBoth")
        {
            TrdMnProvider("items+traderplus");
            return true;
        }
        if (nm == "BtnTrdMnBank")
        {
            TrdMnProvider("traderplus");
            return true;
        }
        if (nm == "BtnTrdMnNoteSet")
        {
            TrdMnNoteSet();
            return true;
        }
        if (nm == "BtnTrdMnStart0")
        {
            TrdMnStart(0);
            return true;
        }
        if (nm == "BtnTrdMnStart1")
        {
            TrdMnStart(1);
            return true;
        }
        if (nm == "BtnTrdMnStart2")
        {
            TrdMnStart(2);
            return true;
        }
        if (nm == "BtnTrdMnDestock" || nm == "TrdMnDestockTick")
        {
            TrdMnFlip("destock");
            return true;
        }
        if (nm == "BtnTrdMnPristine" || nm == "TrdMnPristineTick")
        {
            TrdMnFlip("pristine");
            return true;
        }
        if (nm == "BtnTrdMnWear" || nm == "TrdMnWearTick")
        {
            TrdMnFlip("wear");
            return true;
        }
        // A slider's knob: the pointer is followed frame by frame, the click itself is nothing.
        if (nm.IndexOf("TrdMnCond") == 0)
            return true;
        return false;
    }
}

#endif
#endif
#endif
