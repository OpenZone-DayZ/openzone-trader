// Trader > TraderPlus import (design §5.6 and §11.3; mockup TraderImport): the
// section's `import_traderplus` op, first as a dry run, then applied. The apply
// replaces the trader's three files, so it asks on the window's confirm strip
// first (design §3.4).
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // 0: nothing run in this window yet; 1: a dry run answered; 2: applied. And
    // whether the request on its way is the apply.
    protected int  m_TrdImStep = 0;
    protected bool m_TrdImApplying = false;

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key == "import")
            TrdImPaintSteps();
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdIm", w);
        float colH = h - (76 + 24) * s;
        float reportW = w - (24 + 400 + 16 + 24) * s;
        if (reportW < 240 * s)
            reportW = 240 * s;
        TrdPlace("TrdImRight", 440 * s, 76 * s, reportW, colH);
        TrdSize("TrdImReport", reportW, colH - 30 * s);
    }

    protected void TrdImPaintSteps()
    {
        string steps = TrdTr("#STR_OZT_VPP_IM_STEPS_NONE");
        if (m_TrdImStep == 1)
            steps = TrdTr("#STR_OZT_VPP_IM_STEPS_DRY");
        if (m_TrdImStep == 2)
            steps = TrdTr("#STR_OZT_VPP_IM_STEPS_DONE");
        SayTrd("TrdImSteps", steps);
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
        array<string> rows = new array<string>();
        rows.Insert(TrdTr("#STR_OZT_VPP_IM_WAIT"));
        TrdFill("TrdImReport", rows, null, -1);
    }

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        if (op == "import_traderplus")
            TrdImAnswer(ok, json, error);
        super.TrdOnAnswer(op, ok, json, error);
    }

    // The section answers the import with the points list; its report waits in the
    // server's profile.
    protected void TrdImAnswer(bool ok, string json, string error)
    {
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        if (!ok)
        {
            rows.Insert(Words(error));
            inks.Insert(OZT_Metal.BAD);
            TrdFill("TrdImReport", rows, inks, -1);
            return;
        }
        rows.Insert(TrdTr("#STR_OZT_VPP_IM_NO_REPORT"));
        inks.Insert(0);
        TrdFill("TrdImReport", rows, inks, -1);
        m_TrdImStep = 1;
        if (m_TrdImApplying)
            m_TrdImStep = 2;
        TrdImPaintSteps();
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
            if (nm == "BtnTrdImApply")
            {
                Confirm(TrdTr("#STR_OZT_VPP_IM_APPLY_ASK"), "#STR_OZT_VPP_YES_APPLY", "trd:import");
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
