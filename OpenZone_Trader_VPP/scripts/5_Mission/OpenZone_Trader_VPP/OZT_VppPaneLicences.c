// Trader > Licences (design §5.6 and §11.2; mockup TraderLicences): the server's
// licences in a table -- where each is sold, what it opens, how many hold it
// (`licence_list`); a licence given to a player or taken back with the section's
// `licence` op, by the name of a player online or by a Steam ID -- taking one back
// asks on the window's confirm strip first (design §3.4); and the licences
// themselves (the settings file's Licences: id, name, look), edited in the draft
// of the trader's files in the editor under the table and saved by the draft's bar,
// which the module's frame lays over the page's foot.
//
// The page is converted from the mockup (ui/css2layout.json, page `licences`):
// the table, the players online and what a player holds are lists of row
// templates; the note and the editor stand under the table's last row and move
// with it (TrdLcPlace). The server keeps no history of how a licence was got, so
// a held licence shows its id where the mockup drew one.
//
// A layer of OZ_VppAdminMenu after OZT_VppPaneGates.c (it takes OZT_VppKit from it).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_LC_ROW    = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_licence_row.layout";
    static const string TRD_LC_ONLINE = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_licence_online.layout";
    static const string TRD_LC_HELD   = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_licence_held.layout";

    // The section's last licence_list, as the script's own copy; the three lists;
    // the licence picked in the table and the one the grant names (ids, "" none);
    // what the confirm strip is asked about; the script writing a field itself.
    protected ref OZT_AdminLicenceList m_TrdLc;
    protected ref OZ_VppFlexRows m_TrdLcRows;
    protected ref OZ_VppFlexRows m_TrdLcOnRows;
    protected ref OZ_VppFlexRows m_TrdLcHeldRows;
    protected string m_TrdLcPick;
    protected string m_TrdLcGive;
    protected string m_TrdLcRevWho;
    protected string m_TrdLcRevId;
    protected bool   m_TrdLcPut;

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "licences")
            return;
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
        TrdLcPaint();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key == "licences")
            TrdLcPaint();
    }

    // The window changed its size: the lists follow it, and the note and the editor
    // stand under the table again (a resize puts them back where the layout has them).
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (!M_SUB_WIDGET || TrdShownKey() != "licences")
            return;
        if (m_TrdLcRows)
        {
            m_TrdLcRows.Fit();
            TrdLcPlace();
        }
        if (m_TrdLcOnRows)
            m_TrdLcOnRows.Fit();
        if (m_TrdLcHeldRows)
            m_TrdLcHeldRows.Fit();
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
                if (TrdShownKey() == "licences")
                    TrdLcPaint();
            }
            return;
        }
        // A licence given or taken: who holds what has changed.
        if (op == "licence" && ok)
            Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
    }

    // ------------------------------------------------------------------ what the page knows

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

    // A licence's name by its id, the id when the server has no such licence.
    protected string TrdLcName(string id)
    {
        OZT_AdminLicenceDef d = TrdLcServerDef(id);
        if (d)
            return d.Name;
        return id;
    }

    // A category's name by its id, as the draft's price book has it; the id otherwise.
    protected string TrdLcCatName(string id)
    {
        if (!m_TrdDraft || !m_TrdDraft.Goods || !m_TrdDraft.Goods.Categories)
            return id;
        array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
        for (int i = 0; i < cats.Count(); i++)
        {
            if (cats[i].Id == id && cats[i].Name != "")
                return cats[i].Name;
        }
        return id;
    }

    // "weapons|5000" -> "Зброя · 5000": where a licence is sold and for how much.
    protected string TrdLcSoldWords(OZT_AdminLicenceDef d)
    {
        string sold = "";
        array<string> parts = new array<string>();
        for (int i = 0; i < d.Sold.Count(); i++)
        {
            parts.Clear();
            d.Sold[i].Split("|", parts);
            if (parts.Count() < 2)
                continue;
            if (sold != "")
                sold = sold + ", ";
            sold = sold + TrdLcCatName(parts[0]) + " · " + parts[1];
        }
        if (sold == "")
            sold = "—";
        return sold;
    }

    protected string TrdLcOpensWords(OZT_AdminLicenceDef d)
    {
        string opens = "";
        for (int i = 0; i < d.Opens.Count(); i++)
        {
            if (opens != "")
                opens = opens + "; ";
            opens = opens + TrdUseWords(d.Opens[i]);
        }
        if (opens == "")
            opens = "—";
        return opens;
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
            words = words + " " + inWord + " " + TrdLcCatName(p[2]);
        return words;
    }

    // The player the grant names: what the field holds.
    protected string TrdLcWho()
    {
        return GetEdit("TrdLcWho").Trim();
    }

    // A field written by the script: the OnChange it may raise is not the admin's.
    protected void TrdLcSetField(string name, string text)
    {
        m_TrdLcPut = true;
        SetEdit(name, text);
        m_TrdLcPut = false;
    }

    // ------------------------------------------------------------------ painting

    protected void TrdLcPaint()
    {
        if (!M_SUB_WIDGET)
            return;
        TrdLcPaintDefs();
        TrdLcPaintOnline();
        TrdLcPaintGive();
        TrdLcPaintHeld();
        TrdLcPaintInfo();
    }

    // The table: the licences as the draft holds them (the server's until the
    // settings file has come), each with where the server sells it, what it opens
    // there and how many hold it.
    protected void TrdLcPaintDefs()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdLcDefs");
        if (!host)
            return;
        if (!m_TrdLcRows)
            m_TrdLcRows = new OZ_VppFlexRows(m_Flex, host, TRD_LC_ROW, "OZT_LcRow", 852, 52);
        m_TrdLcRows.Clear();

        int i;
        Widget row;
        bool fromDraft = false;
        if (m_TrdDraft && m_TrdDraft.Settings && m_TrdDraft.Settings.Licences)
            fromDraft = true;
        if (fromDraft)
        {
            array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
            for (i = 0; i < defs.Count(); i++)
            {
                row = m_TrdLcRows.Add(defs[i].Id);
                if (row)
                    TrdLcFillRow(row, defs[i].Id, defs[i].Name, defs[i].Look);
            }
        }
        else if (m_TrdLc)
        {
            for (i = 0; i < m_TrdLc.Defs.Count(); i++)
            {
                OZT_AdminLicenceDef d = m_TrdLc.Defs[i];
                row = m_TrdLcRows.Add(d.Id);
                if (row)
                    TrdLcFillRow(row, d.Id, d.Name, d.Look);
            }
        }
        // No licence yet: one row says so (its key is empty: a click picks nothing).
        if (m_TrdLcRows.Count() == 0)
        {
            row = m_TrdLcRows.Add("");
            if (row)
            {
                OZT_VppKit.PartInk(row, "LcName", "#STR_OZT_VPP_LC_NO_DEFS", OZT_Metal.MUTED);
                OZT_VppKit.PartSay(row, "LcSub", "");
                OZT_VppKit.PartSay(row, "LcSold", "");
                OZT_VppKit.PartSay(row, "LcOpens", "");
                OZT_VppKit.PartSay(row, "LcHeld", "");
                OZT_VppKit.PartShow(row, "OZT_LcRowBg", false);
            }
        }
        if (m_TrdLcPick != "" && m_TrdLcRows.Find(m_TrdLcPick) == -1)
            m_TrdLcPick = "";
        m_TrdLcRows.Fit();
        TrdLcPlace();
    }

    protected void TrdLcFillRow(Widget row, string id, string name, string look)
    {
        string idWord = TrdTr("#STR_OZT_VPP_LC_ID_WORD");
        string lookWord = TrdTr("#STR_OZT_VPP_LC_LOOK_WORD");
        string sub = idWord + " " + id + " · " + lookWord + " " + look;
        OZT_VppKit.PartInk(row, "LcName", name, OZT_Metal.TEXT);
        OZT_VppKit.PartInk(row, "LcSub", sub, OZT_Metal.MUTED);

        // A licence the draft holds and the server does not yet: nothing sells or
        // asks for it there, and nobody can hold it.
        OZT_AdminLicenceDef sd = TrdLcServerDef(id);
        if (!sd)
        {
            OZT_VppKit.PartInk(row, "LcSold", "#STR_OZT_VPP_LC_NOT_YET", OZT_Metal.ALERT);
            OZT_VppKit.PartSay(row, "LcOpens", "");
            OZT_VppKit.PartInk(row, "LcHeld", "—", OZT_Metal.MUTED);
        }
        else
        {
            OZT_VppKit.PartInk(row, "LcSold", TrdLcSoldWords(sd), OZT_Metal.TEXT);
            OZT_VppKit.PartInk(row, "LcOpens", TrdLcOpensWords(sd), OZT_Metal.MUTED);
            OZT_VppKit.PartInk(row, "LcHeld", sd.Holders.ToString(), OZT_Metal.TEXT);
        }
        OZT_VppKit.PartShow(row, "OZT_LcRowBg", id == m_TrdLcPick);
    }

    // The picked row lit, the others plain.
    protected void TrdLcMarkRows()
    {
        if (!m_TrdLcRows)
            return;
        for (int i = 0; i < m_TrdLcRows.Count(); i++)
            OZT_VppKit.PartShow(m_TrdLcRows.Row(i), "OZT_LcRowBg", m_TrdLcRows.Key(i) == m_TrdLcPick && m_TrdLcPick != "");
    }

    // The note and the editor stand 12 units under the table's last row, and the page's
    // scrolled part is as tall as what it holds (the draft's bar lies under the scroll,
    // on the page's own foot).
    protected void TrdLcPlace()
    {
        Widget defs = M_SUB_WIDGET.FindAnyWidget("TrdLcDefs");
        Widget note = M_SUB_WIDGET.FindAnyWidget("TrdLcNote");
        Widget ed = M_SUB_WIDGET.FindAnyWidget("TrdLcEd");
        Widget body = M_SUB_WIDGET.FindAnyWidget("TrdLcLeftBody");
        Widget scroll = M_SUB_WIDGET.FindAnyWidget("TrdLcLeftScroll");
        if (!defs || !note || !ed || !body || !scroll || !m_TrdLcRows)
            return;

        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        defs.GetPos(x, y);
        float at = y + m_TrdLcRows.Count() * m_TrdLcRows.RowHeight() + 12 * s;
        note.GetPos(x, y);
        note.SetPos(x, at);
        note.GetSize(w, h);
        at = at + h + 12 * s;
        ed.GetPos(x, y);
        ed.SetPos(x, at);
        ed.GetSize(w, h);
        at = at + h + 16 * s;

        scroll.GetSize(w, h);
        if (at < h)
            at = h;
        if (w > 0)
            body.SetSize(w, at);
        scroll.Update();
    }

    // The players online: every name, or those that hold what the field has while it
    // names nobody online; the one the field names is lit.
    protected void TrdLcPaintOnline()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdLcOnlineRows");
        if (!host)
            return;
        if (!m_TrdLcOnRows)
            m_TrdLcOnRows = new OZ_VppFlexRows(m_Flex, host, TRD_LC_ONLINE, "OZT_LcOnRow", 417, 34);
        m_TrdLcOnRows.Clear();
        if (!m_TrdLc)
        {
            m_TrdLcOnRows.Fit();
            return;
        }

        string who = TrdLcWho();
        string low = OZT_VppKit.Low(who);
        int i;
        string name;
        bool narrow = false;
        if (who != "" && m_TrdLc.Online.Find(who) == -1)
        {
            for (i = 0; i < m_TrdLc.Online.Count(); i++)
            {
                if (OZT_VppKit.Low(m_TrdLc.Online[i]).IndexOf(low) != -1)
                    narrow = true;
            }
        }
        Widget row;
        for (i = 0; i < m_TrdLc.Online.Count(); i++)
        {
            name = m_TrdLc.Online[i];
            if (narrow && OZT_VppKit.Low(name).IndexOf(low) == -1)
                continue;
            if (m_TrdLcOnRows.Count() >= 200)
                continue;
            row = m_TrdLcOnRows.Add(name);
            if (!row)
                continue;
            OZT_VppKit.PartInk(row, "OnName", name, OZT_Metal.TEXT);
            OZT_VppKit.PartShow(row, "OZT_LcOnRowBg", name == who);
        }
        m_TrdLcOnRows.Fit();
    }

    // The licence the grant names: the picked one, else the server's first.
    protected void TrdLcPaintGive()
    {
        if (m_TrdLcGive != "" && !TrdLcServerDef(m_TrdLcGive))
            m_TrdLcGive = "";
        if (m_TrdLcGive == "" && m_TrdLc && m_TrdLc.Defs.Count() > 0)
            m_TrdLcGive = m_TrdLc.Defs[0].Id;
        string caption = "—";
        if (m_TrdLcGive != "")
            caption = TrdLcName(m_TrdLcGive) + " (" + m_TrdLcGive + ")";
        OZT_VppKit.PartSay(M_SUB_WIDGET, "TrdLcLicText", caption);
    }

    // The select steps through the server's licences.
    protected void TrdLcGiveNext()
    {
        if (!m_TrdLc || m_TrdLc.Defs.Count() == 0)
        {
            Hint("#STR_OZT_VPP_LC_NO_DEFS");
            return;
        }
        int at = -1;
        for (int i = 0; i < m_TrdLc.Defs.Count(); i++)
        {
            if (m_TrdLc.Defs[i].Id == m_TrdLcGive)
                at = i;
        }
        at++;
        if (at >= m_TrdLc.Defs.Count())
            at = 0;
        m_TrdLcGive = m_TrdLc.Defs[at].Id;
        TrdLcPaintGive();
    }

    // What the player in the field holds: a holder online by name, any other by the
    // Steam ID the field holds. Each row takes its licence back.
    protected void TrdLcPaintHeld()
    {
        Widget host = M_SUB_WIDGET.FindAnyWidget("TrdLcHeldRows");
        if (!host)
            return;
        if (!m_TrdLcHeldRows)
            m_TrdLcHeldRows = new OZ_VppFlexRows(m_Flex, host, TRD_LC_HELD, "OZT_LcHeldRow", 427, 47);
        m_TrdLcHeldRows.Clear();

        string who = TrdLcWho();
        string head = TrdTr("#STR_OZT_VPP_LC_HELD");
        OZT_VppKit.PartSay(M_SUB_WIDGET, "TrdLcHeldHead", head + " " + who);

        string idWord = TrdTr("#STR_OZT_VPP_LC_ID_WORD");
        Widget row;
        int i;
        int k;
        if (m_TrdLc && who != "")
        {
            for (i = 0; i < m_TrdLc.Holders.Count(); i++)
            {
                OZT_AdminLicenceHolder hd = m_TrdLc.Holders[i];
                if (hd.Name != who && hd.Uid != who)
                    continue;
                for (k = 0; k < hd.Ids.Count(); k++)
                {
                    row = m_TrdLcHeldRows.Add(hd.Ids[k]);
                    if (!row)
                        continue;
                    OZT_VppKit.PartInk(row, "HeldName", TrdLcName(hd.Ids[k]), OZT_Metal.TEXT);
                    OZT_VppKit.PartInk(row, "HeldSub", idWord + " " + hd.Ids[k], OZT_Metal.MUTED);
                }
            }
        }
        // Nobody named, or a player who holds nothing: one row says which.
        if (m_TrdLcHeldRows.Count() == 0)
        {
            row = m_TrdLcHeldRows.Add("");
            if (row)
            {
                if (who == "")
                    OZT_VppKit.PartInk(row, "HeldName", "#STR_OZT_VPP_LC_HELD_NOBODY", OZT_Metal.MUTED);
                else
                    OZT_VppKit.PartInk(row, "HeldName", "#STR_OZT_VPP_LC_NONE_HELD", OZT_Metal.MUTED);
                OZT_VppKit.PartSay(row, "HeldSub", "");
                OZT_VppKit.PartShow(row, "BtnHeldTake", false);
            }
        }
        m_TrdLcHeldRows.Fit();
    }

    // The line under the editor's fields: what the editor does; for a picked licence
    // where the server sells it and what it opens there, in full -- or that it waits
    // for the draft to be applied.
    protected void TrdLcPaintInfo()
    {
        if (m_TrdLcPick == "" || !m_TrdLc)
        {
            OZT_VppKit.PartInk(M_SUB_WIDGET, "TrdLcDefInfo", "#STR_OZT_VPP_LC_ED_HINT", OZT_Metal.MUTED);
            return;
        }
        OZT_AdminLicenceDef sd = TrdLcServerDef(m_TrdLcPick);
        if (!sd)
        {
            OZT_VppKit.PartInk(M_SUB_WIDGET, "TrdLcDefInfo", "#STR_OZT_VPP_LC_NOT_APPLIED", OZT_Metal.ALERT);
            return;
        }
        // The picked licence in full: a cell of the table cuts a long list.
        string soldWord = TrdTr("#STR_OZT_VPP_LC_SOLD");
        string opensWord = TrdTr("#STR_OZT_VPP_LC_OPENS");
        string info = soldWord + " " + TrdLcSoldWords(sd);
        info = info + " · " + opensWord + " " + TrdLcOpensWords(sd);
        OZT_VppKit.PartInk(M_SUB_WIDGET, "TrdLcDefInfo", info, OZT_Metal.MUTED);
    }

    // ------------------------------------------------------------------ grant

    // Grant (or take back) a licence to the player named in the field.
    protected void TrdLcSend(string who, string id, bool grant)
    {
        if (who == "" || id == "")
        {
            Hint("#STR_OZT_VPP_LC_NEED");
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
            Hint("#STR_OZT_VPP_LC_NEED");
            return;
        }
        m_TrdLcRevWho = who;
        m_TrdLcRevId = id;
        string ask = TrdTr("#STR_OZT_VPP_LC_REVOKE_ASK");
        string from = TrdTr("#STR_OZT_VPP_LC_REVOKE_FROM");
        Confirm(ask + " " + id + " " + from + " " + who + "?", "#STR_OZT_VPP_YES_REVOKE", "trd:revoke");
    }

    // ------------------------------------------------------------------ the definitions

    // A row of the table picked: the editor and the grant take its licence.
    protected void TrdLcPickDef(string id)
    {
        m_TrdLcPick = id;
        if (TrdLcServerDef(id))
            m_TrdLcGive = id;
        TrdLcSetField("TrdLcDefId", id);
        OZT_LicenceDef d = TrdLcDraftDef(id);
        if (d)
        {
            TrdLcSetField("TrdLcDefName", d.Name);
            TrdLcSetField("TrdLcDefLook", d.Look);
        }
        else
        {
            OZT_AdminLicenceDef sd = TrdLcServerDef(id);
            if (sd)
            {
                TrdLcSetField("TrdLcDefName", sd.Name);
                TrdLcSetField("TrdLcDefLook", sd.Look);
            }
        }
        TrdLcMarkRows();
        TrdLcPaintGive();
        TrdLcPaintInfo();
    }

    // "+ Licence": the editor emptied for a new one, its id first.
    protected void TrdLcNew()
    {
        m_TrdLcPick = "";
        TrdLcSetField("TrdLcDefId", "");
        TrdLcSetField("TrdLcDefName", "");
        TrdLcSetField("TrdLcDefLook", "");
        TrdLcMarkRows();
        OZT_VppKit.PartInk(M_SUB_WIDGET, "TrdLcDefInfo", "#STR_OZT_VPP_LC_NEW_HINT", OZT_Metal.MUTED);
        Widget idBox = M_SUB_WIDGET.FindAnyWidget("TrdLcDefId");
        if (idBox)
            SetFocus(idBox);
    }

    // The licence of the id in the fields: its name and look changed, or a new one.
    protected void TrdLcDefSet()
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Loaded(OZT_Const.CFG_SETTINGS))
        {
            Hint("#STR_OZT_VPP_READING");
            return;
        }
        string id = GetEdit("TrdLcDefId").Trim();
        if (!OZT_Ids.IsSafe(id))
        {
            Hint("#STR_OZT_VPP_GD_BAD_ID");
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
        m_TrdLcPick = id;
        TrdLcPaint();
    }

    protected void TrdLcDefDel()
    {
        if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Loaded(OZT_Const.CFG_SETTINGS))
        {
            Hint("#STR_OZT_VPP_READING");
            return;
        }
        string id = GetEdit("TrdLcDefId").Trim();
        array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
        for (int i = 0; i < defs.Count(); i++)
        {
            if (defs[i].Id != id)
                continue;
            defs.RemoveOrdered(i);
            m_TrdLcPick = "";
            TrdTouch(OZT_Const.CFG_SETTINGS);
            TrdLcPaint();
            return;
        }
        Hint("#STR_OZT_VPP_LC_PICK_LIC");
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

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdLcPut && w.GetName() == "TrdLcWho")
        {
            TrdLcPaintOnline();
            TrdLcPaintHeld();
            return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET && TrdLcClick(w))
            return true;
        return super.OnClick(w, x, y, button);
    }

    protected bool TrdLcClick(Widget w)
    {
        string nm = w.GetName();
        int i;
        if (nm == "BtnTrdLcGrant")
        {
            TrdLcSend(TrdLcWho(), m_TrdLcGive, true);
            return true;
        }
        if (nm == "BtnTrdLcRevoke")
        {
            TrdLcAskRevoke(TrdLcWho(), m_TrdLcGive);
            return true;
        }
        if (nm == "TrdLcLic")
        {
            TrdLcGiveNext();
            return true;
        }
        if (nm == "BtnTrdLcRefresh")
        {
            Ask(OZT_Const.SECTION_ADMIN, "licence_list", "{}");
            TrdReread(OZT_Const.CFG_SETTINGS);
            TrdReread(OZT_Const.CFG_GOODS);
            return true;
        }
        if (nm == "BtnTrdLcNew")
        {
            TrdLcNew();
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
        // A held licence's own button takes it back from the player in the field.
        if (nm == "BtnHeldTake" && m_TrdLcHeldRows)
        {
            i = m_TrdLcHeldRows.IndexOf(w);
            if (i != -1 && m_TrdLcHeldRows.Key(i) != "")
                TrdLcAskRevoke(TrdLcWho(), m_TrdLcHeldRows.Key(i));
            return i != -1;
        }
        // A row of the table.
        if (m_TrdLcRows)
        {
            i = m_TrdLcRows.IndexOf(w);
            if (i != -1)
            {
                if (m_TrdLcRows.Key(i) != "")
                    TrdLcPickDef(m_TrdLcRows.Key(i));
                return true;
            }
        }
        // A player online: the name goes into the field.
        if (m_TrdLcOnRows)
        {
            i = m_TrdLcOnRows.IndexOf(w);
            if (i != -1)
            {
                TrdLcSetField("TrdLcWho", m_TrdLcOnRows.Key(i));
                TrdLcPaintOnline();
                TrdLcPaintHeld();
                return true;
            }
        }
        return false;
    }
}

#endif
#endif
#endif
