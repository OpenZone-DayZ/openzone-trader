// Trader > Licences (design §5.6 and §11.2; mockup TraderLicences): a licence given
// to a player or taken back with the section's `licence` op, by the name of a
// player online or by a Steam ID. Taking one back asks on the window's confirm
// strip first (design §3.4).
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // What the confirm strip is asked about: the player and the licence.
    protected string m_TrdLcRevWho = "";
    protected string m_TrdLcRevId = "";

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdLc", w);
        float leftW = w - (24 + 16 + 440 + 24) * s;
        if (leftW < 300 * s)
            leftW = 300 * s;
        float colH = h - (76 + 24) * s;
        TrdPlace("TrdLcLeft", 24 * s, 76 * s, leftW, colH);
        TrdSize("TrdLcDefs", leftW, 250 * s);
        TrdPlace("TrdLcRight", leftW + 40 * s, 76 * s, 440 * s, colH);
    }

    // Grant (or take back) the licence named in the field to the player named in
    // the other.
    protected void TrdLcSend(string who, string id, bool grant)
    {
        if (who == "" || id == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_LC_NEED"));
            return;
        }
        OZT_AdminLicence req = new OZT_AdminLicence();
        req.Player = who;
        req.Id = id;
        req.Grant = 0;
        if (grant)
            req.Grant = 1;
        string body;
        string err;
        JsonFileLoader<OZT_AdminLicence>.MakeData(req, body, err, false);
        Ask(OZT_Const.SECTION_ADMIN, "licence", body);
    }

    protected void TrdLcAskRevoke(string who, string id)
    {
        if (who == "" || id == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_LC_NEED"));
            return;
        }
        m_TrdLcRevWho = who;
        m_TrdLcRevId = id;
        string ask = TrdTr("#STR_OZT_VPP_LC_REVOKE_ASK");
        string from = TrdTr("#STR_OZT_VPP_LC_REVOKE_FROM");
        Confirm(ask + " " + id + " " + from + " " + who + "?", "#STR_OZT_VPP_YES_REVOKE", "trd:revoke");
    }

    override void OnConfirmed(string action)
    {
        if (action == "trd:revoke")
        {
            TrdLcSend(m_TrdLcRevWho, m_TrdLcRevId, false);
            return;
        }
        super.OnConfirmed(action);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();
            if (nm == "BtnTrdLcGrant")
            {
                TrdLcSend(GetEdit("TrdLcWho").Trim(), GetEdit("TrdLcId").Trim(), true);
                return true;
            }
            if (nm == "BtnTrdLcRevoke")
            {
                TrdLcAskRevoke(GetEdit("TrdLcWho").Trim(), GetEdit("TrdLcId").Trim());
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
