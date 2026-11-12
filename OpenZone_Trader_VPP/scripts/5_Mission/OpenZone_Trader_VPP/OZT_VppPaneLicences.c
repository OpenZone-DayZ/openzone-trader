// Trader > Licences (design §5.6 and §11.2; mockup TraderLicences): the server's
// licences, who holds them (`licence_list`), and a licence given to a player or
// taken back with the section's `licence` op, by the name of a player online or by
// a Steam ID. Taking one back asks on the window's confirm strip first (design
// §3.4).
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The section's last licence_list, as the script's own copy; the licence
    // picked on the list; the rows of the held list (licence ids).
    protected ref OZT_AdminLicenceList m_TrdLc;
    protected int m_TrdLcPicked = -1;
    protected ref array<string> m_TrdLcHeldIds;

    // What the confirm strip is asked about: the player and the licence.
    protected string m_TrdLcRevWho = "";
    protected string m_TrdLcRevId = "";

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key == "licences")
            Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
    }

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
        TrdSize("TrdLcDefs", leftW, 190 * s);
        TrdSize("TrdLcDefInfo", leftW, 58 * s);
        TrdPlace("TrdLcRight", leftW + 40 * s, 76 * s, 440 * s, colH);
    }

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        super.TrdOnAnswer(op, ok, json, error);
        if (op == "licence_list" && ok)
        {
            OZT_AdminLicenceList parsed = new OZT_AdminLicenceList();
            string err;
            if (JsonFileLoader<OZT_AdminLicenceList>.LoadData(json, parsed, err))
            {
                m_TrdLc = parsed.Copy();
                TrdLcPaint();
            }
            return;
        }
        // A licence given or taken: who holds what has changed.
        if (op == "licence" && ok)
            Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
    }

    protected void TrdLcPaint()
    {
        if (!m_TrdLc)
            return;
        string holdersWord = TrdTr("#STR_OZT_VPP_LC_HOLDERS");
        array<string> rows = new array<string>();
        for (int i = 0; i < m_TrdLc.Defs.Count(); i++)
        {
            OZT_AdminLicenceDef d = m_TrdLc.Defs[i];
            string row = d.Name + "  (" + d.Id + ")  -  " + holdersWord;
            row = row + " " + d.Holders.ToString();
            rows.Insert(row);
        }
        if (m_TrdLcPicked >= rows.Count())
            m_TrdLcPicked = -1;
        TrdFill("TrdLcDefs", rows, null, m_TrdLcPicked);
        TrdLcPaintInfo();

        array<string> online = new array<string>();
        for (int k = 0; k < m_TrdLc.Online.Count(); k++)
            online.Insert(m_TrdLc.Online[k]);
        TrdFill("TrdLcOnline", online, null, -1);
        TrdLcPaintHeld();
    }

    // Where the picked licence is sold and what it opens.
    protected void TrdLcPaintInfo()
    {
        if (!m_TrdLc || m_TrdLcPicked < 0 || m_TrdLcPicked >= m_TrdLc.Defs.Count())
        {
            SayTrd("TrdLcDefInfo", TrdTr("#STR_OZT_VPP_LC_PICK"));
            return;
        }
        OZT_AdminLicenceDef d = m_TrdLc.Defs[m_TrdLcPicked];
        string soldWord = TrdTr("#STR_OZT_VPP_LC_SOLD");
        string opensWord = TrdTr("#STR_OZT_VPP_LC_OPENS");
        string lookWord = TrdTr("#STR_OZT_VPP_LC_LOOK");
        string sold = "";
        for (int i = 0; i < d.Sold.Count(); i++)
        {
            array<string> parts = new array<string>();
            d.Sold[i].Split("|", parts);
            if (parts.Count() < 2)
                continue;
            if (sold != "")
                sold = sold + ", ";
            sold = sold + parts[0] + " (" + parts[1] + ")";
        }
        if (sold == "")
            sold = "-";
        string opens = "";
        for (int k = 0; k < d.Opens.Count(); k++)
        {
            if (opens != "")
                opens = opens + "; ";
            opens = opens + TrdUseWords(d.Opens[k]);
        }
        if (opens == "")
            opens = "-";
        string info = soldWord + " " + sold + ".  " + lookWord + " " + d.Look;
        info = info + ".\n" + opensWord + " " + opens;
        SayTrd("TrdLcDefInfo", info);
    }

    // "line|AKM|weapons" -> "line AKM in weapons", in the admin's words.
    protected string TrdUseWords(string use)
    {
        array<string> p = new array<string>();
        use.Split("|", p);
        if (p.Count() < 2)
            return use;
        string inWord = TrdTr("#STR_OZT_VPP_IN");
        string what = "";
        if (p[0] == "point")
            what = TrdTr("#STR_OZT_VPP_USE_POINT");
        else if (p[0] == "category")
            what = TrdTr("#STR_OZT_VPP_USE_CATEGORY");
        else if (p[0] == "line")
            what = TrdTr("#STR_OZT_VPP_USE_LINE");
        else
            what = TrdTr("#STR_OZT_VPP_USE_DISCOUNT");
        string words = what + " " + p[1];
        if (p.Count() > 2)
            words = words + " " + inWord + " " + p[2];
        return words;
    }

    // What the player in the field holds: a holder online by name, any other by
    // the Steam ID the field holds.
    protected void TrdLcPaintHeld()
    {
        if (!m_TrdLcHeldIds)
            m_TrdLcHeldIds = new array<string>();
        m_TrdLcHeldIds.Clear();
        string who = GetEdit("TrdLcWho").Trim();
        string head = TrdTr("#STR_OZT_VPP_LC_HELD");
        SayTrd("TrdLcHeldHead", head + " " + who);
        array<string> rows = new array<string>();
        if (m_TrdLc && who != "")
        {
            for (int i = 0; i < m_TrdLc.Holders.Count(); i++)
            {
                OZT_AdminLicenceHolder h = m_TrdLc.Holders[i];
                if (h.Name != who && h.Uid != who)
                    continue;
                for (int k = 0; k < h.Ids.Count(); k++)
                {
                    m_TrdLcHeldIds.Insert(h.Ids[k]);
                    rows.Insert(TrdLcName(h.Ids[k]) + "  (" + h.Ids[k] + ")");
                }
            }
        }
        if (rows.Count() == 0 && who != "")
            rows.Insert(TrdTr("#STR_OZT_VPP_LC_NONE_HELD"));
        TrdFill("TrdLcHeld", rows, null, -1);
    }

    // A licence's name by its id, the id when the settings have no such licence.
    protected string TrdLcName(string id)
    {
        if (m_TrdLc)
        {
            for (int i = 0; i < m_TrdLc.Defs.Count(); i++)
            {
                if (m_TrdLc.Defs[i].Id == id)
                    return m_TrdLc.Defs[i].Name;
            }
        }
        return id;
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

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && M_SUB_WIDGET && !m_Repaint && m_TrdLc)
        {
            string nm = w.GetName();
            if (nm == "TrdLcDefs")
            {
                if (row >= 0 && row < m_TrdLc.Defs.Count())
                {
                    m_TrdLcPicked = row;
                    SetEdit("TrdLcId", m_TrdLc.Defs[row].Id);
                    TrdLcPaintInfo();
                }
                return true;
            }
            if (nm == "TrdLcOnline")
            {
                if (row >= 0 && row < m_TrdLc.Online.Count())
                {
                    SetEdit("TrdLcWho", m_TrdLc.Online[row]);
                    TrdLcPaintHeld();
                }
                return true;
            }
            if (nm == "TrdLcHeld")
                return true;
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && w.GetName() == "TrdLcWho")
        {
            TrdLcPaintHeld();
            return true;
        }
        return super.OnChange(w, x, y, finished);
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
            if (nm == "BtnTrdLcTake")
            {
                TrdLcTakePicked();
                return true;
            }
            if (nm == "BtnTrdLcRefresh")
            {
                Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }

    // The licence picked on the held list, taken back from the player in the field.
    protected void TrdLcTakePicked()
    {
        TextListboxWidget lb = TextListboxWidget.Cast(TrdW("TrdLcHeld"));
        int row = -1;
        if (lb)
            row = lb.GetSelectedRow();
        if (!m_TrdLcHeldIds || row < 0 || row >= m_TrdLcHeldIds.Count())
        {
            Hint(TrdTr("#STR_OZT_VPP_LC_PICK_HELD"));
            return;
        }
        TrdLcAskRevoke(GetEdit("TrdLcWho").Trim(), m_TrdLcHeldIds[row]);
    }
}

#endif
#endif
#endif
