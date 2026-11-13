// Trader > TraderPlus import (design §5.6 and §11.3; board TraderImport, layout
// ozt_vpp_import.layout): the section's `import_traderplus` op, first as a dry run,
// then applied. The report comes as rows the window words in the admin's language
// (OZT_AdminImportItem; the server's English lines are its log's), with the files
// the import found. Right after a dry run the window's confirm strip asks to apply
// it: the apply replaces the trader's three files (design §3.4).
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // 0: nothing run in this window yet; 1: a dry run answered; 2: applied. And
    // whether the request on its way is the apply.
    protected int  m_TrdImStep;
    protected bool m_TrdImApplying;

    // The report's rows (a row's key is its kind, "ok", "warn", "skip" or "bad", and
    // its number) and the files found (key "1" found, "0" not).
    protected ref OZ_VppFlexRows m_TrdImRows;
    protected ref OZ_VppFlexRows m_TrdImFiles;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        m_TrdImStep = 0;
        m_TrdImApplying = false;
    }

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key == "import")
        {
            TrdImPaintSteps();
            TrdImFlowCounts();
            TrdImFlowTitle();
            // A heading drawn for the first time answers its width a frame later.
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TrdImFlowTitle, 80, false);
        }
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (m_TrdImRows)
            m_TrdImRows.Fit();
        if (m_TrdImFiles)
            m_TrdImFiles.Fit();
        TrdImPlaceSigns();
        TrdImFlowCounts();
        TrdImFlowTitle();
    }

    // The report's heading as wide as its word and the run's time 10 units after it: the
    // layout drew both for "Звіт", and "Report" is longer.
    protected void TrdImFlowTitle()
    {
        if (!M_SUB_WIDGET)
            return;
        CoreFlowAfter("TrdImReportTitle", "TrdImWhenBox", 10);
    }

    // The three steps of the head: a step done wears a check, the one to do next is
    // lit, the rest wait. The dry run's button says whether it runs again.
    protected void TrdImPaintSteps()
    {
        TrdImPaintStep(1, m_TrdImStep >= 1, m_TrdImStep == 0);
        TrdImPaintStep(2, m_TrdImStep >= 2, m_TrdImStep == 1);
        TrdImPaintStep(3, m_TrdImStep >= 2, false);
        string dry = "#STR_OZT_VPP_IM_DRY";
        if (m_TrdImStep >= 1)
            dry = "#STR_OZT_VPP_IM_DRY_AGAIN";
        SayTrd("BtnTrdImDryText", TrdTr(dry));
    }

    // As the board draws them: a step done green with a check after its words (the
    // first step's; the others only turn green), the next one lit, the rest muted.
    protected void TrdImPaintStep(int n, bool done, bool next)
    {
        string name = "TrdImStep" + n.ToString();
        float s = UnitScale();
        Widget text = TrdW(name + "Text");
        Widget icon = TrdW(name + "Icon");
        int ground = OZT_Metal.RAISED;
        int ink = OZT_Metal.MUTED;
        if (done)
        {
            ink = OZT_Metal.OK;
        }
        else if (next)
        {
            ground = OZT_Metal.PICK;
            ink = OZT_Metal.TEXT;
        }
        OZ_VppFlex.Paint(M_SUB_WIDGET, name + "Bg", ground);
        if (text)
            text.SetColor(ink);
        if (icon)
        {
            icon.Show(done);
            if (done)
                TrdSignSized(icon, "check", OZT_Metal.OK, 12 * s);
        }
    }

    protected bool TrdImMakeRows()
    {
        if (!m_TrdImRows)
        {
            Widget host = TrdW("TrdImReportRows");
            if (!host)
                return false;
            m_TrdImRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "import_row.layout", "OZT_ImRow", 920, 49);
        }
        if (!m_TrdImFiles)
        {
            Widget files = TrdW("TrdImFilesRows");
            if (files)
                m_TrdImFiles = new OZ_VppFlexRows(m_Flex, files, TRD_LAYOUTS + "import_file.layout", "OZT_ImFile", 359, 30);
        }
        return true;
    }

    protected void TrdImSend(bool apply)
    {
        OZT_AdminImport req = new OZT_AdminImport();
        req.Apply = 0;
        if (apply)
            req.Apply = 1;
        req.Root = GetEdit("TrdImRoot").Trim();
        string body;
        string err;
        JsonFileLoader<OZT_AdminImport>.MakeData(req, body, err, false);
        m_TrdImApplying = apply;
        Ask(OZT_Const.SECTION_ADMIN, "import_traderplus", body);

        // The old report goes: the next one is on its way.
        if (TrdImMakeRows())
        {
            m_TrdImRows.Clear();
            m_TrdImRows.Fit();
            if (m_TrdImFiles)
            {
                m_TrdImFiles.Clear();
                m_TrdImFiles.Fit();
            }
        }
        SayTrd("TrdImWhen", TrdTr("#STR_OZT_VPP_IM_WAIT"));
        SayTrd("TrdImOkN", "");
        SayTrd("TrdImWarnN", "");
        SayTrd("TrdImSkipN", "");
    }

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        if (op == "import_traderplus")
            TrdImAnswer(ok, json, error);
        super.TrdOnAnswer(op, ok, json, error);
    }

    // The section answers the import with its report whether it ran or not: what it
    // could not read is in it.
    protected void TrdImAnswer(bool ok, string json, string error)
    {
        if (!TrdImMakeRows())
            return;
        m_TrdImRows.Clear();
        if (m_TrdImFiles)
            m_TrdImFiles.Clear();
        OZT_AdminImportReport rep;
        OZT_AdminImportReport parsed = new OZT_AdminImportReport();
        string err;
        if (json != "" && JsonFileLoader<OZT_AdminImportReport>.LoadData(json, parsed, err))
            rep = parsed.Copy();

        int good = 0;
        int changed = 0;
        int skipped = 0;
        int i;
        if (rep && rep.Items.Count() > 0)
        {
            // the board's order: what failed, then what came over whole, changed, left
            array<string> kinds = {"bad", "ok", "warn", "skip"};
            for (int k = 0; k < kinds.Count(); k++)
            {
                for (i = 0; i < rep.Items.Count(); i++)
                {
                    OZT_AdminImportItem it = rep.Items[i];
                    if (!it || it.Kind != kinds[k] || it.Key == "dry")
                        continue;
                    if (it.Kind == "ok")
                        good++;
                    else if (it.Kind == "warn")
                        changed++;
                    else if (it.Kind == "skip")
                        skipped++;
                    string title;
                    string more;
                    string count;
                    TrdImWord(it, title, more, count);
                    TrdImRow(it.Kind, title, more, count);
                }
            }
            for (i = 0; i < rep.Files.Count(); i++)
                TrdImFile(rep.Files[i]);
        }
        else
        {
            // a server without the worded report: its lines as they are
            if (!ok)
                TrdImRow("bad", Words(error), "", "");
            if (rep)
            {
                for (i = 0; i < rep.Lines.Count(); i++)
                {
                    string kind = TrdImKind(rep.Lines[i]);
                    if (kind == "ok")
                        good++;
                    else if (kind == "warn")
                        changed++;
                    else
                        skipped++;
                    string t;
                    string m;
                    string c;
                    TrdImSplit(rep.Lines[i], t, m, c);
                    TrdImRow(kind, t, m, c);
                }
            }
        }
        m_TrdImRows.Fit();
        if (m_TrdImFiles)
            m_TrdImFiles.Fit();
        TrdImPlaceSigns();

        SayTrd("TrdImOkN", string.Format(TrdTr("#STR_OZT_VPP_IM_N_OK"), good));
        SayTrd("TrdImWarnN", string.Format(TrdTr("#STR_OZT_VPP_IM_N_WARN"), changed));
        SayTrd("TrdImSkipN", string.Format(TrdTr("#STR_OZT_VPP_IM_N_SKIP"), skipped));
        // a text set this frame has no width yet: the flow again once it is drawn
        TrdImFlowCounts();
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TrdImFlowCounts, 80, false);
        if (!ok)
        {
            SayTrd("TrdImWhen", OZ_VppJournal.Clock());
            return;
        }

        string when = TrdTr("#STR_OZT_VPP_IM_WHEN_DRY");
        m_TrdImStep = 1;
        if (m_TrdImApplying)
        {
            m_TrdImStep = 2;
            when = TrdTr("#STR_OZT_VPP_IM_WHEN_APPLY");
            // The three files and the points are new on the server.
            Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
            TrdReread(OZT_Const.CFG_SETTINGS);
            TrdReread(OZT_Const.CFG_GOODS);
            TrdReread(OZT_Const.CFG_TRADERS);
        }
        SayTrd("TrdImWhen", when + " · " + OZ_VppJournal.Clock());
        TrdImPaintSteps();
        if (!m_TrdImApplying && rep)
            TrdImAskApply(rep);
    }

    // Step 2 to 3: the window's strip asks to apply what the dry run showed. A draft
    // that changed one of the three files would write its older copy over the import
    // on its own apply: that one is saved or dropped first.
    protected void TrdImAskApply(OZT_AdminImportReport rep)
    {
        if (m_TrdDraft && m_TrdDraft.Changed() > 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_DRAFT_FIRST"));
            return;
        }
        string shelves = rep.Shelves;
        if (shelves == "")
            shelves = TrdTr("#STR_OZT_VPP_IM_NO_SHELF");
        string ask = string.Format(TrdTr("#STR_OZT_VPP_IM_APPLY_ASK"), shelves, rep.Give);
        ConfirmApply(ask, "#STR_OZT_VPP_YES_APPLY", "trd:import");
    }

    // A report row in the admin's words: its title, the line under it and its number.
    protected void TrdImWord(OZT_AdminImportItem it, out string title, out string more, out string count)
    {
        string up = it.Key;
        up.ToUpper();
        title = TrdTr("#STR_OZT_VPP_IM_T_" + up);
        if (title == "" || title.IndexOf("STR_") > -1)
            title = it.Key;
        more = it.Names;
        count = "";
        if (it.N > 0)
            count = it.N.ToString();

        string key = it.Key;
        if (key == "notes")
        {
            if (it.M > 0)
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_NOTES_SKIP"), it.N, it.M, it.Names);
            else
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_NOTES"), it.N);
        }
        else if (key == "categories")
        {
            if (it.M > 0)
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_CATEGORIES"), it.M, it.Names);
            else
                more = TrdTr("#STR_OZT_VPP_IM_M_CATEGORIES_0");
        }
        else if (key == "goods")
        {
            more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_GOODS"), it.M, it.K);
        }
        else if (key == "traders")
        {
            if (it.Names != "")
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_TRADERS"), it.M, it.Names, it.K);
            else
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_TRADERS_0"), it.M, it.K);
        }
        else if (key == "rounded")
        {
            more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_ROUNDED"), it.Names);
        }
        else if (key == "fitted")
        {
            title = string.Format(title, it.M);
            if (it.K > 0)
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_FITTED_LOST"), it.Names, it.K);
            else
                more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_FITTED"), it.Names);
        }
        else if (key == "frozen")
        {
            more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_FROZEN"), it.Names);
        }
        else if (key == "twice")
        {
            more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_TWICE"), it.Names);
        }
        else if (key == "own_money")
        {
            more = string.Format(TrdTr("#STR_OZT_VPP_IM_M_OWN_MONEY"), it.Names);
        }
        else if (key == "applied" || key == "given" || key == "tp_left")
        {
            more = TrdTr("#STR_OZT_VPP_IM_M_" + up);
        }
        else if (key == "loaded")
        {
            more = it.More;
        }
        else if (key == "unread")
        {
            title = string.Format(title, it.Names);
            more = it.More;
        }
    }

    protected void TrdImRow(string kind, string title, string more, string count)
    {
        Widget row = m_TrdImRows.Add(kind + ":" + m_TrdImRows.Count().ToString());
        if (!row)
            return;
        int ink = OZT_Metal.TEXT;
        if (kind == "bad")
            ink = OZT_Metal.BAD;
        RowText(row, "ImRowText", title, ink);
        RowText(row, "ImRowMore", more, OZT_Metal.MUTED);
        RowText(row, "ImRowCount", count, OZT_Metal.MUTED);
    }

    // One of TraderPlus's files: its name and what it holds, or that it is missing.
    protected void TrdImFile(OZT_AdminImportFile f)
    {
        if (!f || !m_TrdImFiles)
            return;
        string key = "0";
        if (f.Found == 1)
            key = "1";
        Widget row = m_TrdImFiles.Add(key);
        if (!row)
            return;
        string note = TrdTr("#STR_OZT_VPP_IM_F_NONE");
        if (f.Found == 1)
        {
            string which = "";
            if (f.File == "TraderPlusGeneralConfig.json")
                which = "GENERAL";
            else if (f.File == "TraderPlusPriceConfig.json")
                which = "PRICES";
            else if (f.File == "TraderPlusIDsConfig.json")
                which = "IDS";
            else if (f.File == "TraderPlusDatabase")
                which = "DATABASE";
            else if (f.File == "TraderPlusBankDatabase")
                which = "BANK";
            note = f.N.ToString();
            if (which != "")
                note = string.Format(TrdTr("#STR_OZT_VPP_IM_F_" + which), f.N);
        }
        int ink = OZT_Metal.TEXT;
        if (f.Found != 1)
            ink = OZT_Metal.MUTED;
        RowText(row, "ImFileName", f.File, ink);
        RowText(row, "ImFileNote", note, OZT_Metal.MUTED);
    }

    // The head's three counts from the right, 10 px apart: each text is right-aligned
    // in its box, so a box ends where the text of the one after it begins.
    protected void TrdImFlowCounts()
    {
        Widget head = TrdW("TrdImHead");
        if (!head)
            return;
        float s = UnitScale();
        float hx;
        float hy;
        float hw;
        float hh;
        head.GetScreenPos(hx, hy);
        head.GetScreenSize(hw, hh);
        if (hw <= 0)
            return;
        float right = hx + hw - 20 * s;
        array<string> names = {"TrdImSkipN", "TrdImWarnN", "TrdImOkN"};
        for (int i = 0; i < names.Count(); i++)
        {
            TextWidget t = TextWidget.Cast(TrdW(names[i]));
            if (!t)
                continue;
            float x;
            float y;
            float w;
            float h;
            t.GetScreenPos(x, y);
            t.GetScreenSize(w, h);
            Widget parent = t.GetParent();
            float px = 0;
            float py = 0;
            if (parent)
                parent.GetScreenPos(px, py);
            t.SetPos(right - w - px, y - py);
            int tw;
            int th;
            t.GetTextSize(tw, th);
            if (tw > 0)
                right = right - tw - 10 * s;
            else
                right = right - w - 10 * s;
        }
    }

    // A report line of a server without the worded report, in three: what it is about
    // (up to its first colon), the rest of it, and its number when the rest is a number
    // alone or a number before names in brackets.
    protected void TrdImSplit(string line, out string title, out string more, out string count)
    {
        title = line;
        more = "";
        count = "";
        int colon = line.IndexOf(": ");
        if (colon <= 0)
            return;
        title = line.Substring(0, colon);
        string rest = line.Substring(colon + 2, line.Length() - colon - 2);
        more = rest;
        int len = rest.Length();
        int digits = 0;
        bool digit = true;
        while (digit && digits < len)
        {
            int c = rest.Get(digits).ToAscii();
            if (c >= 48 && c <= 57)
                digits++;
            else
                digit = false;
        }
        if (digits == 0)
            return;
        if (digits == len)
        {
            count = rest;
            more = "";
            return;
        }
        if (len - digits >= 2 && rest.Substring(digits, 2) == " (")
        {
            count = rest.Substring(0, digits);
            more = rest.Substring(digits + 1, len - digits - 1);
        }
    }

    // What such a line says of what it names: "skip" -- left out, "warn" -- taken over
    // changed, "ok" -- the rest. By the words the import uses (OZT_TPImport).
    protected string TrdImKind(string line)
    {
        array<string> left = {"skipped", "not taken", "left to", "could not"};
        array<string> turned = {"dropped", "pays nothing", "rounded", "twice", "MaxStock 0", "of their own"};
        int i;
        for (i = 0; i < left.Count(); i++)
        {
            if (line.Contains(left[i]))
                return "skip";
        }
        for (i = 0; i < turned.Count(); i++)
        {
            if (line.Contains(turned[i]))
                return "warn";
        }
        return "ok";
    }

    // Every row's sign by its kind: a check for what came over, "!" for what was
    // changed, "-" for what was left out, a cross for what could not be read; a file
    // found wears a check, one missing a cross.
    protected void TrdImPlaceSigns()
    {
        float s = UnitScale();
        int i;
        if (m_TrdImRows)
        {
            for (i = 0; i < m_TrdImRows.Count(); i++)
            {
                Widget row = m_TrdImRows.Row(i);
                if (!row)
                    continue;
                Widget sign = row.FindAnyWidget("ImRowSign");
                string key = m_TrdImRows.Key(i);
                if (key.IndexOf("ok:") == 0)
                    TrdSignSized(sign, "check", OZT_Metal.OK, 13 * s);
                else if (key.IndexOf("warn:") == 0)
                    TrdSignSized(sign, "excl", OZT_Metal.ALERT, 13 * s);
                else if (key.IndexOf("skip:") == 0)
                    TrdSignSized(sign, "dash", OZT_Metal.MUTED, 12 * s);
                else
                    TrdSignSized(sign, "cross", OZT_Metal.BAD, 12 * s);
            }
        }
        if (m_TrdImFiles)
        {
            for (i = 0; i < m_TrdImFiles.Count(); i++)
            {
                Widget file = m_TrdImFiles.Row(i);
                if (!file)
                    continue;
                Widget mark = file.FindAnyWidget("ImFileSign");
                if (m_TrdImFiles.Key(i) == "1")
                    TrdSignSized(mark, "check", OZT_Metal.OK, 12 * s);
                else
                    TrdSignSized(mark, "cross", OZT_Metal.BAD, 11 * s);
            }
        }
    }

    override void OnConfirmed(string action)
    {
        if (action == "trd:import")
        {
            TrdImSend(true);
            return;
        }
        super.OnConfirmed(action);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();
            if (nm == "BtnTrdImDry")
            {
                TrdImSend(false);
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
