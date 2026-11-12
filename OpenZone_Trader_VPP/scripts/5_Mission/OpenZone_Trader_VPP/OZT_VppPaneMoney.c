// Trader > Money (design §5.6; mockup TraderMoney): what holds for every trader of
// the server -- the settings file's money (cash, cash then the TraderPlus bank, the
// bank, or another provider's id), its banknotes and wallets, the share of the
// price a worn thing brings, the stock at a server start, and the deal rules --
// edited in the draft and applied by the draft's bar (cfg_set trader_settings). A
// point's own money is the points page's.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The note picked on the list (-1: none).
    protected int m_TrdMnNote = -1;

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

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        TrdLayHead("TrdMn", w);
    }

    protected OZT_Settings TrdMnSet()
    {
        if (!m_TrdDraft)
            return null;
        return m_TrdDraft.Settings;
    }

    // ------------------------------------------------------------------ painting

    protected void TrdMnPaint()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
        {
            SayTrd("TrdMnSub", TrdTr("#STR_OZT_VPP_READING"));
            return;
        }
        SayTrd("TrdMnSub", TrdTr("#STR_OZT_VPP_MN_SUB"));
        TrdSet("TrdMnProv", st.Provider);
        TrdSet("TrdMnHolders", TrdJoin(st.Holders, ","));
        TrdSet("TrdMnCond0", TrdNum(st.Condition.Pristine));
        TrdSet("TrdMnCond1", TrdNum(st.Condition.Worn));
        TrdSet("TrdMnCond2", TrdNum(st.Condition.Damaged));
        TrdSet("TrdMnCond3", TrdNum(st.Condition.BadlyDamaged));
        TrdSet("TrdMnFloor", TrdNum(st.StockFloor));
        TrdSet("TrdMnCooldown", st.CooldownMs.ToString());
        TrdMnPaintNotes();
        TrdMnPaintSwitches();
    }

    protected void TrdMnPaintNotes()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        array<string> rows = new array<string>();
        for (int i = 0; i < st.Notes.Count(); i++)
            rows.Insert(st.Notes[i].ClassName + "   =   " + st.Notes[i].Value.ToString());
        if (m_TrdMnNote >= rows.Count())
            m_TrdMnNote = -1;
        TrdFill("TrdMnNotes", rows, null, m_TrdMnNote);
    }

    // A choice of several lit in the accent colour, the others muted.
    protected void TrdMnLight(string button, bool on)
    {
        TextWidget t = TextWidget.Cast(TrdW(button + "Text"));
        if (!t)
            return;
        if (on)
            t.SetColor(OZT_Metal.ACCENT);
        else
            t.SetColor(OZT_Metal.MUTED);
    }

    // A switch's caption: its words for on, or for off.
    protected void TrdMnSwitch(string button, bool on, string keyOn, string keyOff)
    {
        string text = TrdTr(keyOff);
        if (on)
            text = TrdTr(keyOn);
        int ink = OZT_Metal.MUTED;
        if (on)
            ink = OZT_Metal.TEXT;
        TrdInk(button + "Text", text, ink);
    }

    protected void TrdMnPaintSwitches()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        TrdMnLight("BtnTrdMnItems", st.Provider == OZT_Const.PROVIDER_ITEMS);
        TrdMnLight("BtnTrdMnBoth", st.Provider == "items+traderplus");
        TrdMnLight("BtnTrdMnBank", st.Provider == "traderplus");
        TrdMnLight("BtnTrdMnStart0", st.StartStock == 0);
        TrdMnLight("BtnTrdMnStart1", st.StartStock == 1);
        TrdMnLight("BtnTrdMnStart2", st.StartStock == 2);
        TrdMnSwitch("BtnTrdMnDestock", st.DestockAtStart == 1, "#STR_OZT_VPP_MN_DESTOCK_YES", "#STR_OZT_VPP_MN_DESTOCK_NO");
        TrdMnSwitch("BtnTrdMnPristine", st.StockOnlyPristine == 1, "#STR_OZT_VPP_MN_PRISTINE_YES", "#STR_OZT_VPP_MN_PRISTINE_NO");
        TrdMnSwitch("BtnTrdMnWear", st.WearBought == 1, "#STR_OZT_VPP_MN_WEAR_YES", "#STR_OZT_VPP_MN_WEAR_NO");
        TrdMnSwitch("BtnTrdMnGroup", st.GroupVariants == 1, "#STR_OZT_VPP_MN_GROUP_YES", "#STR_OZT_VPP_MN_GROUP_NO");
    }

    // ------------------------------------------------------------------ edits

    protected bool TrdMnIsField(string nm)
    {
        array<string> fields = {"TrdMnProv", "TrdMnHolders", "TrdMnCond0", "TrdMnCond1", "TrdMnCond2", "TrdMnCond3", "TrdMnFloor", "TrdMnCooldown"};
        return fields.Find(nm) != -1;
    }

    // A field typed into: the draft's settings follow it. A value equal to the
    // draft's is no edit (an OnChange the script's own fill may raise).
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
            TrdSplit(v, parts);
            if (TrdJoin(parts, ",") == TrdJoin(st.Holders, ","))
                changed = false;
            TrdSplit(v, st.Holders);
        }
        else if (nm == "TrdMnCond0")
        {
            if (st.Condition.Pristine == f)
                changed = false;
            st.Condition.Pristine = f;
        }
        else if (nm == "TrdMnCond1")
        {
            if (st.Condition.Worn == f)
                changed = false;
            st.Condition.Worn = f;
        }
        else if (nm == "TrdMnCond2")
        {
            if (st.Condition.Damaged == f)
                changed = false;
            st.Condition.Damaged = f;
        }
        else if (nm == "TrdMnCond3")
        {
            if (st.Condition.BadlyDamaged == f)
                changed = false;
            st.Condition.BadlyDamaged = f;
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
            TrdTouch(OZT_Const.CFG_SETTINGS);
    }

    protected void TrdMnProvider(string provider)
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        st.Provider = provider;
        TrdSet("TrdMnProv", provider);
        TrdMnPaintSwitches();
        TrdTouch(OZT_Const.CFG_SETTINGS);
    }

    // A note of the class in the fields: its value changed, or a new note.
    protected void TrdMnNoteSet()
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        string cls = GetEdit("TrdMnNoteCls").Trim();
        int worth = GetEdit("TrdMnNoteVal").Trim().ToInt();
        if (cls == "" || worth <= 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_MN_NOTE_NEED"));
            return;
        }
        for (int i = 0; i < st.Notes.Count(); i++)
        {
            if (st.Notes[i].ClassName == cls)
            {
                st.Notes[i].Value = worth;
                TrdTouch(OZT_Const.CFG_SETTINGS);
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
        m_TrdMnNote = at;
        TrdTouch(OZT_Const.CFG_SETTINGS);
        TrdMnPaintNotes();
    }

    protected void TrdMnNoteDel()
    {
        OZT_Settings st = TrdMnSet();
        if (!st || m_TrdMnNote < 0 || m_TrdMnNote >= st.Notes.Count())
        {
            Hint(TrdTr("#STR_OZT_VPP_MN_NOTE_PICK"));
            return;
        }
        st.Notes.RemoveOrdered(m_TrdMnNote);
        m_TrdMnNote = -1;
        TrdTouch(OZT_Const.CFG_SETTINGS);
        TrdMnPaintNotes();
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
        else if (what == "group")
        {
            if (st.GroupVariants == 1)
                st.GroupVariants = 0;
            else
                st.GroupVariants = 1;
        }
        TrdTouch(OZT_Const.CFG_SETTINGS);
        TrdMnPaintSwitches();
    }

    protected void TrdMnStart(int start)
    {
        OZT_Settings st = TrdMnSet();
        if (!st)
            return;
        st.StartStock = start;
        TrdTouch(OZT_Const.CFG_SETTINGS);
        TrdMnPaintSwitches();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && !m_Repaint && w.GetName() == "TrdMnNotes")
        {
            OZT_Settings st = TrdMnSet();
            if (st && row >= 0 && row < st.Notes.Count())
            {
                m_TrdMnNote = row;
                TrdSet("TrdMnNoteCls", st.Notes[row].ClassName);
                TrdSet("TrdMnNoteVal", st.Notes[row].Value.ToString());
            }
            return true;
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill && TrdMnIsField(w.GetName()))
        {
            TrdMnEdit(w.GetName());
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();
            if (nm.IndexOf("BtnTrdMn") == 0 && !TrdMnSet())
            {
                Hint(TrdTr("#STR_OZT_VPP_READING"));
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
            if (nm == "BtnTrdMnNoteDel")
            {
                TrdMnNoteDel();
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
            if (nm == "BtnTrdMnDestock")
            {
                TrdMnFlip("destock");
                return true;
            }
            if (nm == "BtnTrdMnPristine")
            {
                TrdMnFlip("pristine");
                return true;
            }
            if (nm == "BtnTrdMnWear")
            {
                TrdMnFlip("wear");
                return true;
            }
            if (nm == "BtnTrdMnGroup")
            {
                TrdMnFlip("group");
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
