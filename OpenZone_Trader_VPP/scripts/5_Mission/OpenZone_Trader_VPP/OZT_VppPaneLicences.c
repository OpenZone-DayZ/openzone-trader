// Trader > Licences (design §5.6 and §11.2; mockup TraderLicences): the server's
// licences and who holds them (`licence_list`); a licence given to a player or
// taken back with the section's `licence` op, by the name of a player online or by
// a Steam ID -- taking one back asks on the window's confirm strip first (design
// §3.4); and the licences themselves (the settings file's Licences: id, name,
// look), edited in the draft and applied by the draft's bar.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The section's last licence_list, as the script's own copy; the licence ids
    // of the list's rows and the row picked; the rows of the held list.
    protected ref OZT_AdminLicenceList m_TrdLc;
    protected ref array<string> m_TrdLcIds;
    protected int m_TrdLcPicked = -1;
    protected ref array<string> m_TrdLcHeldIds;

    // What the confirm strip is asked about: the player and the licence.
    protected string m_TrdLcRevWho = "";
    protected string m_TrdLcRevId = "";

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "licences")
            return;
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
        TrdLcPaint();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "licences")
            TrdLcPaint();
    }

    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        float s = UnitScale();
        TrdLayHead("TrdLc", w);
        float leftW = w - (24 + 16 + 440 + 24) * s;
        if (leftW < 300 * s)
            leftW = 300 * s;
        float colH = h - (76 + 16 + 56) * s;
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

    // The server's licence of that id in the last licence_list, or null.
    protected OZT_AdminLicenceDef TrdLcServerDef(string id)
    {
        if (!m_TrdLc)
            return null;
        for (int i = 0; i < m_TrdLc.Defs.Count(); i++)
        {
            if (m_TrdLc.Defs[i].Id == id)
                return m_TrdLc.Defs[i];
        }
        return null;
    }

    // The draft's licence of that id, or null.
    protected OZT_LicenceDef TrdLcDraftDef(string id)
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Settings.Licences)
            return null;
        array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
        for (int i = 0; i < defs.Count(); i++)
        {
            if (defs[i].Id == id)
                return defs[i];
        }
        return null;
    }

    // The licences as the draft holds them (the server's until the settings file
    // has come), with how many hold each.
    protected void TrdLcPaint()
    {
        if (!m_TrdLcIds)
            m_TrdLcIds = new array<string>();
        m_TrdLcIds.Clear();
        string holdersWord = TrdTr("#STR_OZT_VPP_LC_HOLDERS");
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        bool fromDraft = m_TrdDraft && m_TrdDraft.Settings && m_TrdDraft.Settings.Licences;
        if (fromDraft)
        {
            array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
            for (int i = 0; i < defs.Count(); i++)
            {
                m_TrdLcIds.Insert(defs[i].Id);
                int held = 0;
                OZT_AdminLicenceDef sd = TrdLcServerDef(defs[i].Id);
                if (sd)
                    held = sd.Holders;
                string row = defs[i].Name + "  (" + defs[i].Id + ")  -  " + holdersWord;
                rows.Insert(row + " " + held.ToString());
                int ink = 0;
                if (!sd)
                    ink = OZT_Metal.ALERT;
                inks.Insert(ink);
            }
        }
        else if (m_TrdLc)
        {
            for (int k = 0; k < m_TrdLc.Defs.Count(); k++)
            {
                OZT_AdminLicenceDef d = m_TrdLc.Defs[k];
                m_TrdLcIds.Insert(d.Id);
                string srow = d.Name + "  (" + d.Id + ")  -  " + holdersWord;
                rows.Insert(srow + " " + d.Holders.ToString());
                inks.Insert(0);
            }
        }
        if (m_TrdLcPicked >= rows.Count())
            m_TrdLcPicked = -1;
        TrdFill("TrdLcDefs", rows, inks, m_TrdLcPicked);
        TrdLcPaintInfo();

        array<string> online = new array<string>();
        if (m_TrdLc)
        {
            for (int n = 0; n < m_TrdLc.Online.Count(); n++)
                online.Insert(m_TrdLc.Online[n]);
        }
        TrdFill("TrdLcOnline", online, null, -1);
        TrdLcPaintHeld();
    }

    protected string TrdLcPickedId()
    {
        if (!m_TrdLcIds || m_TrdLcPicked < 0 || m_TrdLcPicked >= m_TrdLcIds.Count())
            return "";
        return m_TrdLcIds[m_TrdLcPicked];
    }

    // Where the picked licence is sold and what it opens, as the server has it.
    protected void TrdLcPaintInfo()
    {
        string id = TrdLcPickedId();
        if (id == "")
        {
            SayTrd("TrdLcDefInfo", TrdTr("#STR_OZT_VPP_LC_PICK"));
            return;
        }
        OZT_AdminLicenceDef d = TrdLcServerDef(id);
        if (!d)
        {
            SayTrd("TrdLcDefInfo", TrdTr("#STR_OZT_VPP_LC_NOT_APPLIED"));
            return;
        }
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

    // A licence's name by its id, the id when the server has no such licence.
    protected string TrdLcName(string id)
    {
        OZT_AdminLicenceDef d = TrdLcServerDef(id);
        if (d)
            return d.Name;
        return id;
    }

    // ------------------------------------------------------------------ grant

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

    // The licence picked on the held list, taken back from the player in the field.
    protected void TrdLcTakePicked()
    {
        int row = TrdPickedRow("TrdLcHeld");
        if (!m_TrdLcHeldIds || row < 0 || row >= m_TrdLcHeldIds.Count())
        {
            Hint(TrdTr("#STR_OZT_VPP_LC_PICK_HELD"));
            return;
        }
        TrdLcAskRevoke(GetEdit("TrdLcWho").Trim(), m_TrdLcHeldIds[row]);
    }

    // ------------------------------------------------------------------ the definitions

    // The licence of the id in the fields: its name and look changed, or a new one.
    protected void TrdLcDefSet()
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings)
        {
            Hint(TrdTr("#STR_OZT_VPP_READING"));
            return;
        }
        string id = GetEdit("TrdLcDefId").Trim();
        if (!OZT_Ids.IsSafe(id))
        {
            Hint(TrdTr("#STR_OZT_VPP_GD_BAD_ID"));
            return;
        }
        string name = GetEdit("TrdLcDefName").Trim();
        string look = GetEdit("TrdLcDefLook").Trim();
        OZT_LicenceDef d = TrdLcDraftDef(id);
        if (!d)
        {
            d = new OZT_LicenceDef();
            d.Id = id;
            m_TrdDraft.Settings.Licences.Insert(d);
        }
        d.Name = name;
        if (d.Name == "")
            d.Name = id;
        d.Look = look;
        if (d.Look == "")
            d.Look = "Paper";
        TrdTouch(OZT_Const.CFG_SETTINGS);
        // The list again with the licence in it, then picked there.
        TrdLcPaint();
        m_TrdLcPicked = m_TrdLcIds.Find(id);
        TrdLcPaint();
    }

    protected void TrdLcDefDel()
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings)
            return;
        string id = GetEdit("TrdLcDefId").Trim();
        array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
        for (int i = 0; i < defs.Count(); i++)
        {
            if (defs[i].Id != id)
                continue;
            defs.RemoveOrdered(i);
            m_TrdLcPicked = -1;
            TrdTouch(OZT_Const.CFG_SETTINGS);
            TrdLcPaint();
            return;
        }
        Hint(TrdTr("#STR_OZT_VPP_LC_PICK"));
    }

    // ------------------------------------------------------------------ gestures

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
        if (w && M_SUB_WIDGET && !m_Repaint)
        {
            string nm = w.GetName();
            if (nm == "TrdLcDefs")
            {
                if (m_TrdLcIds && row >= 0 && row < m_TrdLcIds.Count())
                {
                    m_TrdLcPicked = row;
                    string id = m_TrdLcIds[row];
                    SetEdit("TrdLcId", id);
                    TrdSet("TrdLcDefId", id);
                    OZT_LicenceDef d = TrdLcDraftDef(id);
                    if (d)
                    {
                        TrdSet("TrdLcDefName", d.Name);
                        TrdSet("TrdLcDefLook", d.Look);
                    }
                    TrdLcPaintInfo();
                }
                return true;
            }
            if (nm == "TrdLcOnline")
            {
                if (m_TrdLc && row >= 0 && row < m_TrdLc.Online.Count())
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
                TrdReread(OZT_Const.CFG_SETTINGS);
                return true;
            }
            if (nm == "BtnTrdLcDefAdd")
            {
                TrdLcDefSet();
                return true;
            }
            if (nm == "BtnTrdLcDefDel")
            {
                TrdLcDefDel();
                return true;
            }
        }
        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
