// Trader > Points (design §5.6 and §9.5; board TraderPoints, layout
// ozt_vpp_points.layout): the trader points of the server, placed and dressed where
// the admin stands. The list is the section's `list` (what stands where), searched and
// filtered by kind; the card edits the picked point in the draft of the traders file
// -- its name, kind, class, radius, factors, place, facing, money, stock, the
// categories it deals in and who may trade there -- and the module's draft bar
// applies it (cfg_set). What needs the admin's own place or gear stays an op of the
// section: a new point where the admin stands, the aimed thing made a trader, move,
// turn, outfit; and remove, which asks on the confirm strip first, as respawning every
// point does. Those ops rewrite the file on the server, so they wait while the draft
// holds a change to it.
//
// A layer of OZ_VppAdminMenu after OZT_VppPane.c (the file names sort so).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    // The list holds no more rows than this; the search narrows it.
    static const int TRD_PT_ROWS_MAX = 300;

    // What the section last said, and the point picked on the list (by id: a
    // refreshed list keeps the pick while the point is there).
    protected ref OZT_AdminList m_TrdList;
    protected string m_TrdPickedId;

    // The list's filter ("": every kind, else a kind); the kind of a new point; whether
    // the categories box offers the rest of the price book; the condition the card's
    // add row holds (its kind, which of the kind's choices, turned round or not).
    protected string m_TrdPtFilter;
    protected bool   m_TrdNewNpc;
    protected bool   m_TrdPtCatPick;
    protected string m_TrdPtCondKind;
    protected int    m_TrdPtCondArg;
    protected bool   m_TrdPtCondNot;

    // The card's head as last written (its words are placed by their widths).
    protected string m_TrdPtHeadName;
    protected string m_TrdPtHeadChip;
    protected string m_TrdPtHeadState;

    // The rows of the list and of the card's conditions; the chips of its categories
    // and the tiles of what the trader wears.
    protected ref OZ_VppFlexRows m_TrdPtRows;
    protected ref OZ_VppFlexRows m_TrdPtCondRows;
    protected ref OZT_VppChips   m_TrdPtCatChips;
    protected ref OZT_VppChips   m_TrdPtWearTiles;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        m_TrdPickedId = "";
        m_TrdPtFilter = "";
        m_TrdNewNpc = false;
        m_TrdPtCatPick = false;
        m_TrdPtCondKind = "licence";
        m_TrdPtCondArg = 0;
        m_TrdPtCondNot = false;
        m_TrdPtHeadName = "";
        m_TrdPtHeadChip = "";
        m_TrdPtHeadState = "";
    }

    override void TrdShown(string key)
    {
        super.TrdShown(key);
        if (key != "points")
            return;
        Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
        TrdNeedDoc(OZT_Const.CFG_TRADERS);
        TrdNeedDoc(OZT_Const.CFG_SETTINGS);
        TrdNeedDoc(OZT_Const.CFG_GOODS);
        TrdNeedGates();
        FillTrdList();
        TrdPaintCard();
    }

    override void TrdRepaint(string key)
    {
        super.TrdRepaint(key);
        if (key != "points")
            return;
        FillTrdList();
        TrdPaintCard();
    }

    // The window was resized: the rows follow their lists, and what the script placed
    // by a width (the head's words, the chips, the tiles) is placed again.
    override void TrdLayout(float w, float h)
    {
        super.TrdLayout(w, h);
        if (m_TrdPtRows)
        {
            m_TrdPtRows.Fit();
            TrdPtPlaceRows();
        }
        if (m_TrdPtCondRows)
            m_TrdPtCondRows.Fit();
        if (TrdShownKey() != "points")
            return;
        TrdPtFlowHead();
        TrdPtFlowCats();
        TrdPtFlowWear();
    }

    // ------------------------------------------------------------------ answers

    override void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        super.TrdOnAnswer(op, ok, json, error);
        string verb = TrdVerb(op);
        array<string> listOps = {"list", "respawn", "place", "adopt", "move", "turn", "remove", "set", "outfit", "licence"};
        if (!ok || listOps.Find(verb) == -1)
            return;
        OZT_AdminList list = new OZT_AdminList();
        string err;
        if (!JsonFileLoader<OZT_AdminList>.LoadData(json, list, err))
        {
            string unreadable = TrdTr("#STR_OZT_VPP_UNREADABLE");
            Hint(unreadable + " " + op);
            return;
        }
        TrdTakeList(list);
        // The ops that wrote the traders file: the draft reads it again.
        array<string> writes = {"place", "adopt", "move", "turn", "remove", "set", "outfit"};
        if (writes.Find(verb) != -1)
            TrdReread(OZT_Const.CFG_TRADERS);
    }

    // The list as the script's own copy: what the serializer made lives only as
    // long as its parse (the core's OZ_ConfigBase header).
    protected void TrdTakeList(OZT_AdminList parsed)
    {
        OZT_AdminList list = new OZT_AdminList();
        for (int i = 0; i < parsed.Points.Count(); i++)
        {
            OZT_AdminPoint src = parsed.Points[i];
            if (!src)
                continue;
            OZT_AdminPoint p = new OZT_AdminPoint();
            p.Id = src.Id;
            p.Name = src.Name;
            p.Kind = src.Kind;
            p.ClassName = src.ClassName;
            p.Model = src.Model;
            p.X = src.X;
            p.Y = src.Y;
            p.Z = src.Z;
            p.Yaw = src.Yaw;
            p.Radius = src.Radius;
            if (src.Categories)
            {
                for (int c = 0; c < src.Categories.Count(); c++)
                    p.Categories.Insert(src.Categories[c]);
            }
            p.BuyFactor = src.BuyFactor;
            p.SellFactor = src.SellFactor;
            p.Provider = src.Provider;
            p.NoExchange = src.NoExchange;
            p.Worn = src.Worn;
            p.Standing = src.Standing;
            list.Points.Insert(p);
        }
        for (int k = 0; k < parsed.Categories.Count(); k++)
        {
            OZT_WireCategory wsrc = parsed.Categories[k];
            if (!wsrc)
                continue;
            OZT_WireCategory wc = new OZT_WireCategory();
            wc.Id = wsrc.Id;
            wc.Name = wsrc.Name;
            list.Categories.Insert(wc);
        }
        m_TrdList = list;

        if (!TrdListPoint())
            m_TrdPickedId = "";
        FillTrdList();
        TrdPaintCard();
    }

    // ------------------------------------------------------------------ reading

    // The picked point as the section listed it; null when none is picked.
    protected OZT_AdminPoint TrdListPoint()
    {
        if (!m_TrdList || m_TrdPickedId == "")
            return null;
        for (int i = 0; i < m_TrdList.Points.Count(); i++)
        {
            if (m_TrdList.Points[i].Id == m_TrdPickedId)
                return m_TrdList.Points[i];
        }
        return null;
    }

    protected string PickedTrd()
    {
        OZT_AdminPoint lp = TrdListPoint();
        if (!lp)
            return "";
        return lp.Id;
    }

    // A point as the draft holds it; null before the file has come.
    protected OZT_Point TrdPtDraftOf(string id)
    {
        if (!m_TrdDraft || !m_TrdDraft.Traders || id == "")
            return null;
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            if (points[i].Id == id)
                return points[i];
        }
        return null;
    }

    protected OZT_Point TrdPtDraft()
    {
        return TrdPtDraftOf(m_TrdPickedId);
    }

    // What stands at a point: its class, or a classless prop's model.
    protected string TrdThing(OZT_AdminPoint p)
    {
        if (p.ClassName != "")
            return p.ClassName;
        return p.Model;
    }

    // A category's name as the players read it: the draft's price book first, then
    // what the section listed, then the id itself.
    protected string TrdPtCatName(string id)
    {
        int i;
        if (m_TrdDraft && m_TrdDraft.Goods && m_TrdDraft.Goods.Categories)
        {
            array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
            for (i = 0; i < cats.Count(); i++)
            {
                if (cats[i].Id == id && cats[i].Name != "")
                    return cats[i].Name;
            }
        }
        if (m_TrdList)
        {
            for (i = 0; i < m_TrdList.Categories.Count(); i++)
            {
                if (m_TrdList.Categories[i].Id == id && m_TrdList.Categories[i].Name != "")
                    return m_TrdList.Categories[i].Name;
            }
        }
        return id;
    }

    // Does the price book have the category? Unknown before either has come.
    protected bool TrdPtCatKnown(string id)
    {
        int i;
        if (m_TrdDraft && m_TrdDraft.Goods && m_TrdDraft.Goods.Categories)
        {
            array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
            for (i = 0; i < cats.Count(); i++)
            {
                if (cats[i].Id == id)
                    return true;
            }
            return false;
        }
        if (m_TrdList)
        {
            for (i = 0; i < m_TrdList.Categories.Count(); i++)
            {
                if (m_TrdList.Categories[i].Id == id)
                    return true;
            }
            return false;
        }
        return true;
    }

    // Every category of the price book, in its order: the draft's, else the section's.
    protected void TrdPtBookCats(array<string> ids)
    {
        ids.Clear();
        int i;
        if (m_TrdDraft && m_TrdDraft.Goods && m_TrdDraft.Goods.Categories)
        {
            array<ref OZT_Category> cats = m_TrdDraft.Goods.Categories;
            for (i = 0; i < cats.Count(); i++)
                ids.Insert(cats[i].Id);
            return;
        }
        if (m_TrdList)
        {
            for (i = 0; i < m_TrdList.Categories.Count(); i++)
                ids.Insert(m_TrdList.Categories[i].Id);
        }
    }

    // The point's money in words: the server's, cash, cash then the bank, the bank,
    // or the provider's own id.
    protected string TrdPtMoneyWord(string provider)
    {
        if (provider == "")
            return TrdTr("#STR_OZT_VPP_MONEY_SERVER");
        if (provider == OZT_Const.PROVIDER_ITEMS)
            return TrdTr("#STR_OZT_VPP_MONEY_CASH");
        if (provider == "items+traderplus")
            return TrdTr("#STR_OZT_VPP_MONEY_BOTH");
        if (provider == "traderplus")
            return TrdTr("#STR_OZT_VPP_MONEY_BANK");
        return provider;
    }

    // The kind's word on a chip, in capitals (the engine's ToUpper knows Latin only).
    protected string TrdPtChipWord(string kind)
    {
        if (kind == OZT_Const.KIND_NPC)
            return TrdTr("#STR_OZT_VPP_PT_CHIP_NPC");
        if (kind == OZT_Const.KIND_MAP)
            return TrdTr("#STR_OZT_VPP_PT_CHIP_MAP");
        return TrdTr("#STR_OZT_VPP_PT_CHIP_OBJ");
    }

    // How far the point is from the admin: "3 m", "2.1 km".
    protected string TrdPtAway(float x, float y, float z)
    {
        Man me = GetGame().GetPlayer();
        if (!me)
            return "";
        vector to = Vector(x, y, z);
        float d = vector.Distance(me.GetPosition(), to);
        if (d < 1000)
        {
            int m = Math.Round(d);
            string mWord = TrdTr("#STR_OZT_VPP_M");
            return m.ToString() + " " + mWord;
        }
        int tenths = Math.Round(d / 100);
        float km = tenths / 10.0;
        string kmWord = TrdTr("#STR_OZT_VPP_KM");
        return km.ToString() + " " + kmWord;
    }

    // A number as the admin typed it: digits, one point, a minus in front.
    protected bool TrdIsNumber(string v)
    {
        int n = v.Length();
        if (n == 0)
            return false;
        int digits = 0;
        int points = 0;
        for (int i = 0; i < n; i++)
        {
            int c = v.Get(i).ToAscii();
            if (c >= 48 && c <= 57)
            {
                digits++;
                continue;
            }
            if (c == 46)
            {
                points++;
                continue;
            }
            if (c == 45 && i == 0)
                continue;
            return false;
        }
        return digits > 0 && points < 2;
    }

    // ------------------------------------------------------------------ the list

    protected bool TrdPtMakeRows()
    {
        if (m_TrdPtRows)
            return true;
        Widget host = TrdW("TrdListRows");
        if (!host)
            return false;
        m_TrdPtRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "point_row.layout", "OZT_PtRow", 364, 76);
        return true;
    }

    // Does the search keep the point: its name, id or class holds the words.
    protected bool TrdPtMatches(OZT_AdminPoint lp, OZT_Point dp, string find)
    {
        string name = lp.Name;
        string thing = TrdThing(lp);
        if (dp)
        {
            name = dp.Name;
            thing = dp.ClassName;
        }
        if (TrdLow(name).IndexOf(find) != -1)
            return true;
        if (TrdLow(lp.Id).IndexOf(find) != -1)
            return true;
        return TrdLow(thing).IndexOf(find) != -1;
    }

    // The list: the points the search and the kind's pill keep, a row each; the pills
    // count every kind.
    protected void FillTrdList()
    {
        if (!m_TrdList || !TrdPtMakeRows())
            return;
        string find = TrdLow(GetEdit("TrdPtFind").Trim());
        int all = 0;
        int npcs = 0;
        int objects = 0;
        int maps = 0;
        int hidden = 0;
        m_TrdPtRows.Clear();
        for (int i = 0; i < m_TrdList.Points.Count(); i++)
        {
            OZT_AdminPoint lp = m_TrdList.Points[i];
            OZT_Point dp = TrdPtDraftOf(lp.Id);
            string kind = lp.Kind;
            if (dp)
                kind = dp.Kind;
            all++;
            if (kind == OZT_Const.KIND_NPC)
                npcs++;
            else if (kind == OZT_Const.KIND_MAP)
                maps++;
            else
                objects++;
            if (m_TrdPtFilter != "" && kind != m_TrdPtFilter)
                continue;
            if (find != "" && !TrdPtMatches(lp, dp, find))
                continue;
            if (m_TrdPtRows.Count() >= TRD_PT_ROWS_MAX)
            {
                hidden++;
                continue;
            }
            Widget row = m_TrdPtRows.Add(lp.Id);
            if (row)
                TrdPtPaintRow(row, lp, dp);
        }
        m_TrdPtRows.Fit();
        TrdPtPlaceRows();
        TrdPtMarkRows();

        string allWord = TrdTr("#STR_OZT_VPP_PT_F_ALL");
        string npcWord = TrdTr("#STR_OZT_VPP_KIND_NPC");
        string objWord = TrdTr("#STR_OZT_VPP_PT_F_OBJ");
        string mapWord = TrdTr("#STR_OZT_VPP_PT_F_MAP");
        SayTrd("BtnTrdPtAllText", allWord + " · " + all.ToString());
        SayTrd("BtnTrdPtNpcText", npcWord + " · " + npcs.ToString());
        SayTrd("BtnTrdPtObjText", objWord + " · " + objects.ToString());
        SayTrd("BtnTrdPtMapText", mapWord + " · " + maps.ToString());
        TrdPaintPill("BtnTrdPtAll", m_TrdPtFilter == "");
        TrdPaintPill("BtnTrdPtNpc", m_TrdPtFilter == OZT_Const.KIND_NPC);
        TrdPaintPill("BtnTrdPtObj", m_TrdPtFilter == OZT_Const.KIND_OBJECT);
        TrdPaintPill("BtnTrdPtMap", m_TrdPtFilter == OZT_Const.KIND_MAP);

        if (hidden > 0)
        {
            string cut = TrdTr("#STR_OZT_VPP_LIST_CUT");
            int rowsMax = TRD_PT_ROWS_MAX;
            Hint(cut + " " + rowsMax.ToString() + " / " + all.ToString());
        }
    }

    // One point's row: whether it stands, its name and kind, how far it is, its id,
    // stock and money, and who may trade there or what it deals in. The draft's values
    // where the draft has the point, so an edit shows at once.
    protected void TrdPtPaintRow(Widget row, OZT_AdminPoint lp, OZT_Point dp)
    {
        string name = lp.Name;
        string kind = lp.Kind;
        string provider = lp.Provider;
        int noChange = lp.NoExchange;
        float x = lp.X;
        float y = lp.Y;
        float z = lp.Z;
        if (dp)
        {
            name = dp.Name;
            kind = dp.Kind;
            provider = dp.Provider;
            noChange = dp.NoExchange;
            if (dp.Pos && dp.Pos.Count() == 3)
            {
                x = dp.Pos[0];
                y = dp.Pos[1];
                z = dp.Pos[2];
            }
        }
        if (name == "")
            name = lp.Id;
        RowText(row, "PtRowName", name, OZT_Metal.TEXT);
        RowText(row, "PtRowKindText", TrdPtChipWord(kind), OZT_Metal.MUTED);
        RowText(row, "PtRowAway", TrdPtAway(x, y, z), OZT_Metal.MUTED);

        string facts = "id " + lp.Id;
        if (dp)
        {
            string stockWord = TrdTr("#STR_OZT_VPP_SK_STOCK");
            facts = facts + " · " + stockWord + " " + dp.StockId();
        }
        facts = facts + " · " + TrdPtMoneyWord(provider);
        RowText(row, "PtRowFacts", facts, OZT_Metal.MUTED);

        string note = TrdPtRowNote(lp, dp, noChange);
        int noteInk = OZT_Metal.MUTED;
        int dotInk = OZT_Metal.OK;
        if (lp.Standing != 1)
        {
            string notStanding = TrdTr("#STR_OZT_VPP_PT_NOT_STANDING");
            if (note != "")
                note = notStanding + " · " + note;
            else
                note = notStanding;
            noteInk = OZT_Metal.ALERT;
            dotInk = OZT_Metal.ALERT;
        }
        RowText(row, "PtRowNote", note, noteInk);
        Widget dot = row.FindAnyWidget("PtRowDot");
        if (dot)
            dot.SetColor(dotInk);
    }

    // The row's last line: who may trade there when the point asks for something,
    // else the categories it deals in; and that it changes no notes.
    protected string TrdPtRowNote(OZT_AdminPoint lp, OZT_Point dp, int noChange)
    {
        string note = "";
        int i;
        if (dp && dp.Access && dp.Access.Count() > 0)
        {
            note = TrdTr("#STR_OZT_VPP_PT_ACCESS_W");
            for (i = 0; i < dp.Access.Count(); i++)
            {
                if (i > 0)
                    note = note + ",";
                note = note + " " + TrdCondWords(dp.Access[i]);
            }
        }
        else
        {
            array<string> cats = lp.Categories;
            if (dp && dp.Categories)
                cats = dp.Categories;
            for (i = 0; i < cats.Count(); i++)
            {
                if (i > 0)
                    note = note + ", ";
                note = note + TrdPtCatName(cats[i]);
            }
        }
        if (noChange == 1)
        {
            string noWord = TrdTr("#STR_OZT_VPP_CHANGE_NO");
            if (note != "")
                note = note + " · ";
            note = note + noWord;
        }
        return note;
    }

    // The kind's chip of every row stands after the row's name and hugs its word.
    protected void TrdPtPlaceRows()
    {
        if (!m_TrdPtRows || !m_TrdList)
            return;
        for (int i = 0; i < m_TrdPtRows.Count(); i++)
            TrdPtPlaceRow(m_TrdPtRows.Row(i), m_TrdPtRows.Key(i));
    }

    protected void TrdPtPlaceRow(Widget row, string id)
    {
        if (!row || !m_TrdList)
            return;
        OZT_AdminPoint lp = null;
        for (int i = 0; i < m_TrdList.Points.Count(); i++)
        {
            if (m_TrdList.Points[i].Id == id)
                lp = m_TrdList.Points[i];
        }
        if (!lp)
            return;
        OZT_Point dp = TrdPtDraftOf(id);
        string name = lp.Name;
        string kind = lp.Kind;
        if (dp)
        {
            name = dp.Name;
            kind = dp.Kind;
        }
        if (name == "")
            name = id;

        TextWidget nameW = TextWidget.Cast(row.FindAnyWidget("PtRowName"));
        Widget chip = row.FindAnyWidget("PtRowKind");
        Widget chipBg = row.FindAnyWidget("PtRowKindBg");
        TextWidget chipText = TextWidget.Cast(row.FindAnyWidget("PtRowKindText"));
        if (!nameW || !chip || !chipText)
            return;
        float s = UnitScale();
        float nx;
        float ny;
        float cx;
        float cy;
        float w;
        float h;
        float tw = TrdTextW(nameW, name, 15);
        // The name stops where the distance begins: 364 units of row, 70 of distance.
        float most = 200 * s;
        if (tw > most)
            tw = most;
        nameW.GetPos(nx, ny);
        nameW.GetSize(w, h);
        nameW.SetSize(tw + 2, h);
        TrdHug(chip, chipBg, chipText, TrdPtChipWord(kind), 11, 6 * s);
        chip.GetPos(cx, cy);
        chip.SetPos(nx + tw + 8 * s, cy);
    }

    // The picked point's row is lit: the accent's frame on its soft ground, as the
    // board draws it; the rest lie on a box's colour.
    protected void TrdPtMarkRows()
    {
        if (!m_TrdPtRows)
            return;
        Widget row;
        for (int r = 0; r < m_TrdPtRows.Count(); r++)
        {
            row = m_TrdPtRows.Row(r);
            if (!row)
                continue;
            if (m_TrdPtRows.Key(r) == m_TrdPickedId)
            {
                OZ_VppFlex.Paint(row, "OZT_PtRowEdge", OZT_Metal.ACCENT);
                OZ_VppFlex.Paint(row, "OZT_PtRowBg", OZT_Metal.PICK);
            }
            else
            {
                OZ_VppFlex.Paint(row, "OZT_PtRowEdge", OZT_Metal.HEAD);
                OZ_VppFlex.Paint(row, "OZT_PtRowBg", OZT_Metal.HEAD);
            }
        }
    }

    // The picked point's row again, after an edit of the card, without making the
    // list again.
    protected void TrdPtRepaintRow()
    {
        if (!m_TrdPtRows)
            return;
        OZT_AdminPoint lp = TrdListPoint();
        Widget row = m_TrdPtRows.Row(m_TrdPtRows.Find(m_TrdPickedId));
        if (!lp || !row)
            return;
        TrdPtPaintRow(row, lp, TrdPtDraft());
        TrdPtPlaceRow(row, lp.Id);
    }

    // ------------------------------------------------------------------ the card

    protected void TrdPaintCard()
    {
        OZT_AdminPoint lp = TrdListPoint();
        OZT_Point p = TrdPtDraft();
        bool picked = false;
        if (lp)
            picked = true;
        TrdShow("TrdPtForm", picked);
        TrdShow("TrdPtHeadChip", picked);
        TrdShow("TrdPtState", picked);
        TrdShow("TrdPtStateDot", picked);
        TrdShow("BtnTrdRemove", picked);
        TrdPaintSeg("BtnTrdNewNpc", m_TrdNewNpc);
        TrdPaintSeg("BtnTrdNewObj", !m_TrdNewNpc);
        if (!picked)
        {
            m_TrdPtHeadName = TrdTr("#STR_OZT_VPP_PT_PICK");
            m_TrdPtHeadChip = "";
            m_TrdPtHeadState = "";
            TrdInk("TrdPtHead", m_TrdPtHeadName, OZT_Metal.MUTED);
            TrdPtFlowHead();
            return;
        }

        // The file's values where the draft has the point; what the section listed
        // until the file has come.
        string name = lp.Name;
        string kind = lp.Kind;
        string thing = TrdThing(lp);
        float radius = lp.Radius;
        float buy = lp.BuyFactor;
        float sell = lp.SellFactor;
        string provider = lp.Provider;
        string stock = "";
        int noChange = lp.NoExchange;
        float x = lp.X;
        float y = lp.Y;
        float z = lp.Z;
        float yaw = lp.Yaw;
        if (p)
        {
            name = p.Name;
            kind = p.Kind;
            thing = p.ClassName;
            if (thing == "")
                thing = p.Model;
            radius = p.Radius;
            buy = p.BuyFactor;
            sell = p.SellFactor;
            provider = p.Provider;
            stock = p.Stock;
            noChange = p.NoExchange;
            yaw = p.Yaw;
            if (p.Pos && p.Pos.Count() == 3)
            {
                x = p.Pos[0];
                y = p.Pos[1];
                z = p.Pos[2];
            }
        }

        SayTrd("TrdPtIdText", lp.Id);
        TrdSet("TrdName", name);
        TrdSet("TrdClass", thing);
        TrdSet("TrdRadius", TrdNum(radius));
        TrdSet("TrdBuy", TrdNum(buy));
        TrdSet("TrdSell", TrdNum(sell));
        TrdSet("TrdPtStock", stock);
        TrdSet("TrdProvider", provider);
        TrdSet("TrdPtX", TrdNum(x));
        TrdSet("TrdPtY", TrdNum(y));
        TrdSet("TrdPtZ", TrdNum(z));
        TrdSet("TrdPtYaw", TrdNum(yaw));
        TrdPtPaintKind(kind);
        TrdPtPaintStock(lp.Id, stock);
        SayTrd("BtnTrdPtMoneyText", TrdPtMoneyWord(provider));
        TrdShow("BtnTrdChangeMark", noChange == 1);

        TrdPtPaintHead(name, kind, thing, lp.Standing);
        TrdPtPaintCats(p, lp);
        TrdPtPaintAccess(p);
        TrdPtPaintCondAdd();
        TrdPtPaintWear(p, lp);
    }

    // The head: the name, the kind and the thing on a chip, whether it stands.
    protected void TrdPtPaintHead(string name, string kind, string thing, int standing)
    {
        if (name == "")
            name = m_TrdPickedId;
        m_TrdPtHeadName = name;
        m_TrdPtHeadChip = TrdPtChipWord(kind) + " · " + thing;
        int ink = OZT_Metal.ALERT;
        m_TrdPtHeadState = TrdTr("#STR_OZT_VPP_PT_NOT_STANDING");
        if (standing == 1)
        {
            m_TrdPtHeadState = TrdTr("#STR_OZT_VPP_PT_STANDING");
            ink = OZT_Metal.OK;
        }
        TrdInk("TrdPtHead", m_TrdPtHeadName, OZT_Metal.TEXT);
        TrdInk("TrdPtHeadSub", m_TrdPtHeadChip, OZT_Metal.MUTED);
        TrdInk("TrdPtState", m_TrdPtHeadState, ink);
        Widget dot = TrdW("TrdPtStateDot");
        if (dot)
            dot.SetColor(ink);
        TrdPtFlowHead();
    }

    // The head's words stand one after another, whatever their length: the name, 12
    // units, the chip as wide as its words, 12 units, the state's sign and the state.
    protected void TrdPtFlowHead()
    {
        TextWidget nameW = TextWidget.Cast(TrdW("TrdPtHead"));
        Widget chip = TrdW("TrdPtHeadChip");
        Widget chipBg = TrdW("TrdPtHeadChipBg");
        TextWidget chipText = TextWidget.Cast(TrdW("TrdPtHeadSub"));
        Widget dot = TrdW("TrdPtStateDot");
        TextWidget state = TextWidget.Cast(TrdW("TrdPtState"));
        if (!nameW)
            return;
        float s = UnitScale();
        float x;
        float y;
        float w;
        float h;
        float tw = TrdTextW(nameW, m_TrdPtHeadName, 22);
        nameW.GetPos(x, y);
        nameW.GetSize(w, h);
        nameW.SetSize(tw + 2, h);
        float at = x + tw + 12 * s;
        if (!chip || !chipText || !dot || !state)
            return;
        float cw = TrdHug(chip, chipBg, chipText, m_TrdPtHeadChip, 12, 8 * s);
        chip.GetPos(x, y);
        chip.SetPos(at, y);
        at = at + cw + 12 * s;
        dot.GetPos(x, y);
        dot.GetSize(w, h);
        dot.SetPos(at, y);
        at = at + w + 5 * s;
        tw = TrdTextW(state, m_TrdPtHeadState, 13);
        state.GetPos(x, y);
        state.GetSize(w, h);
        state.SetPos(at, y);
        state.SetSize(tw + 2, h);
    }

    // The three segments of the kind: the point's own lit.
    protected void TrdPtPaintKind(string kind)
    {
        TrdPaintSeg("BtnTrdKindNpc", kind == OZT_Const.KIND_NPC);
        TrdPaintSeg("BtnTrdKindObj", kind == OZT_Const.KIND_OBJECT);
        TrdPaintSeg("BtnTrdKindMap", kind == OZT_Const.KIND_MAP);
    }

    // The stock's select: the point's own shelf, or the one it shares.
    protected void TrdPtPaintStock(string id, string stock)
    {
        string word;
        if (stock == "" || stock == id)
        {
            string own = TrdTr("#STR_OZT_VPP_PT_STOCK_OWN");
            word = own + " (" + id + ")";
        }
        else
        {
            string sharedWord = TrdTr("#STR_OZT_VPP_PT_STOCK_SHARED");
            word = sharedWord + " " + stock;
        }
        SayTrd("BtnTrdPtStockPickText", word);
    }

    // ---- what the point deals in: a chip a category

    protected bool TrdPtMakeCats()
    {
        if (m_TrdPtCatChips)
            return true;
        Widget host = TrdW("TrdPtCatsBody");
        if (!host)
            return false;
        m_TrdPtCatChips = new OZT_VppChips(m_Flex, host, TRD_LAYOUTS + "point_cat.layout", "OZT_PtCat");
        return true;
    }

    // The point's categories as chips with a cross; while the box offers more, the
    // rest of the price book as outlined chips with a plus.
    protected void TrdPtPaintCats(OZT_Point p, OZT_AdminPoint lp)
    {
        if (!TrdPtMakeCats())
            return;
        m_TrdPtCatChips.Clear();
        array<string> has = new array<string>();
        int i;
        if (p && p.Categories)
        {
            for (i = 0; i < p.Categories.Count(); i++)
                has.Insert(p.Categories[i]);
        }
        else if (lp)
        {
            for (i = 0; i < lp.Categories.Count(); i++)
                has.Insert(lp.Categories[i]);
        }
        for (i = 0; i < has.Count(); i++)
            TrdPtCatChip("has:" + has[i], has[i], true);
        if (m_TrdPtCatPick && p)
        {
            array<string> book = new array<string>();
            TrdPtBookCats(book);
            for (i = 0; i < book.Count(); i++)
            {
                if (has.Find(book[i]) == -1)
                    TrdPtCatChip("add:" + book[i], book[i], false);
            }
        }
        // The button that opens the rest of the book is lit while they are shown.
        Widget addText = TrdW("BtnTrdPtCatAddText");
        if (addText)
        {
            if (m_TrdPtCatPick)
                addText.SetColor(OZT_Metal.ACCENT);
            else
                addText.SetColor(OZT_Metal.MUTED);
        }
        TrdPtFlowCats();
    }

    // One chip: as wide as its name. A category the point has: solid, a cross; one it
    // may add: outlined, a plus; one the price book lacks: its id in the alert colour.
    protected void TrdPtCatChip(string key, string id, bool has)
    {
        Widget chip = m_TrdPtCatChips.Add(key);
        if (!chip)
            return;
        string name = TrdPtCatName(id);
        int ink = OZT_Metal.TEXT;
        if (has)
        {
            if (!TrdPtCatKnown(id))
                ink = OZT_Metal.ALERT;
            OZ_VppFlex.Paint(chip, "OZT_PtCatEdge", OZT_Metal.RAISED);
            OZ_VppFlex.Paint(chip, "OZT_PtCatBg", OZT_Metal.RAISED);
            RowText(chip, "PtCatDelText", "×", OZT_Metal.MUTED);
        }
        else
        {
            ink = OZT_Metal.MUTED;
            OZ_VppFlex.Paint(chip, "OZT_PtCatEdge", OZT_Metal.EDGE);
            OZ_VppFlex.Paint(chip, "OZT_PtCatBg", OZT_Metal.HEAD);
            RowText(chip, "PtCatDelText", "+", OZT_Metal.ACCENT);
        }
        RowText(chip, "PtCatName", name, ink);

        // The template is 125 x 28: 10 before the name, 42 around it, the cross 26
        // from the right edge.
        float s = UnitScale();
        TextWidget nameW = TextWidget.Cast(chip.FindAnyWidget("PtCatName"));
        float tw = TrdTextW(nameW, name, 13);
        float w = tw + 42 * s;
        chip.SetSize(w, 28 * s);
        Widget edge = chip.FindAnyWidget("OZT_PtCatEdge");
        if (edge)
            edge.SetSize(w, 28 * s);
        Widget bg = chip.FindAnyWidget("OZT_PtCatBg");
        if (bg)
            bg.SetSize(w - 2 * s, 26 * s);
        if (nameW)
            nameW.SetSize(tw + 2, 15 * s);
        Widget del = chip.FindAnyWidget("PtCatDel");
        if (del)
            del.SetPos(w - 26 * s, 4 * s);
        m_TrdPtCatChips.SetWidth(m_TrdPtCatChips.Count() - 1, w);
    }

    // The chips in lines inside their box, the button that adds one after the last;
    // the box's scrolled part as tall as the lines.
    protected void TrdPtFlowCats()
    {
        if (!m_TrdPtCatChips)
            return;
        Widget body = TrdW("TrdPtCatsBody");
        Widget scroll = TrdW("TrdPtCatsScroll");
        Widget add = TrdW("BtnTrdPtCatAdd");
        if (!body || !scroll)
            return;
        float s = UnitScale();
        float room;
        float view;
        scroll.GetSize(room, view);
        if (room <= 0)
            room = 265 * s;
        float addW = 0;
        float addH = 0;
        if (add)
            add.GetSize(addW, addH);
        float need = m_TrdPtCatChips.Flow(room - 10 * s, 6 * s, 28 * s, add, addW);
        if (need < view)
            need = view;
        body.SetSize(room, need);
        scroll.Update();
    }

    // ---- who may trade here: a row a condition

    protected bool TrdPtMakeConds()
    {
        if (m_TrdPtCondRows)
            return true;
        Widget host = TrdW("TrdPtAccessRows");
        if (!host)
            return false;
        m_TrdPtCondRows = new OZ_VppFlexRows(m_Flex, host, TRD_LAYOUTS + "point_cond.layout", "OZT_PtCond", 265, 34);
        return true;
    }

    // Who may trade here (the point's Access), a condition a row, its cross takes it
    // out of the draft.
    protected void TrdPtPaintAccess(OZT_Point p)
    {
        if (!TrdPtMakeConds())
            return;
        m_TrdPtCondRows.Clear();
        if (p && p.Access)
        {
            for (int i = 0; i < p.Access.Count(); i++)
            {
                Widget row = m_TrdPtCondRows.Add(i.ToString());
                if (row)
                    RowText(row, "PtCondText", TrdCondWords(p.Access[i]), OZT_Metal.TEXT);
            }
        }
        m_TrdPtCondRows.Fit();
    }

    // The values an admin may pick for a kind of condition, with what the admin
    // reads: a licence of the draft's settings (one not applied yet too), else what
    // the kind's provider offers. None: the kind takes a free word.
    protected void TrdPtCondChoices(string kind, array<string> values, array<string> labels)
    {
        values.Clear();
        labels.Clear();
        int i;
        if (kind == "licence")
        {
            if (!m_TrdDraft || !m_TrdDraft.Settings || !m_TrdDraft.Settings.Licences)
                return;
            array<ref OZT_LicenceDef> defs = m_TrdDraft.Settings.Licences;
            for (i = 0; i < defs.Count(); i++)
            {
                values.Insert(defs[i].Id);
                labels.Insert(defs[i].Name);
            }
            return;
        }
        OZT_AdminGateKind gk = TrdGateKind(kind);
        if (!gk)
            return;
        for (i = 0; i < gk.Values.Count(); i++)
        {
            values.Insert(gk.Values[i]);
            string label = "";
            if (i < gk.Labels.Count())
                label = gk.Labels[i];
            labels.Insert(label);
        }
    }

    // The add row: the kind, must hold or must not, the value.
    protected void TrdPtPaintCondAdd()
    {
        SayTrd("BtnTrdPtCondKindText", TrdCondKindWord(m_TrdPtCondKind));
        string must = TrdTr("#STR_OZT_VPP_PT_MUST");
        if (m_TrdPtCondNot)
            must = TrdTr("#STR_OZT_VPP_PT_MUST_NOT");
        SayTrd("BtnTrdPtCondNotText", must);

        array<string> values = new array<string>();
        array<string> labels = new array<string>();
        TrdPtCondChoices(m_TrdPtCondKind, values, labels);
        if (values.Count() == 0)
        {
            SayTrd("BtnTrdPtCondArgText", TrdTr("#STR_OZT_VPP_PT_COND_NONE"));
            return;
        }
        if (m_TrdPtCondArg < 0 || m_TrdPtCondArg >= values.Count())
            m_TrdPtCondArg = 0;
        string shown = values[m_TrdPtCondArg];
        string label = labels[m_TrdPtCondArg];
        if (label != "" && label != shown)
            shown = label + " (" + shown + ")";
        SayTrd("BtnTrdPtCondArgText", shown);
    }

    // The add row's kind, one step on through the kinds the providers answer.
    protected void TrdPtCondKindNext()
    {
        array<string> kinds = new array<string>();
        TrdKinds(kinds);
        int at = kinds.Find(m_TrdPtCondKind) + 1;
        if (at >= kinds.Count())
            at = 0;
        if (kinds.Count() > 0)
            m_TrdPtCondKind = kinds[at];
        m_TrdPtCondArg = 0;
        TrdPtPaintCondAdd();
    }

    protected void TrdPtCondAdd()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_PICK_FIRST"));
            return;
        }
        array<string> values = new array<string>();
        array<string> labels = new array<string>();
        TrdPtCondChoices(m_TrdPtCondKind, values, labels);
        if (values.Count() == 0)
        {
            if (m_TrdPtCondKind == "licence")
                Hint(TrdTr("#STR_OZT_VPP_PT_NO_LICENCES"));
            else
                Hint(TrdTr("#STR_OZT_VPP_PT_COND_NO_LIST"));
            return;
        }
        if (m_TrdPtCondArg < 0 || m_TrdPtCondArg >= values.Count())
            m_TrdPtCondArg = 0;
        string arg = values[m_TrdPtCondArg];
        int turned = 0;
        if (m_TrdPtCondNot)
            turned = 1;
        if (!p.Access)
            p.Access = new array<ref OZT_Condition>();
        for (int i = 0; i < p.Access.Count(); i++)
        {
            OZT_Condition had = p.Access[i];
            if (had.Kind == m_TrdPtCondKind && had.Arg == arg && had.Not == turned)
            {
                Hint(TrdTr("#STR_OZT_VPP_PT_COND_HAS"));
                return;
            }
        }
        OZT_Condition c = new OZT_Condition();
        c.Kind = m_TrdPtCondKind;
        c.Arg = arg;
        c.Not = turned;
        p.Access.Insert(c);
        TrdPtPaintAccess(p);
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtRepaintRow();
    }

    protected void TrdPtCondDel(int index)
    {
        OZT_Point p = TrdPtDraft();
        if (!p || !p.Access || index < 0 || index >= p.Access.Count())
            return;
        p.Access.RemoveOrdered(index);
        TrdPtPaintAccess(p);
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtRepaintRow();
    }

    // ---- what the trader wears: a tile a thing

    protected bool TrdPtMakeWear()
    {
        if (m_TrdPtWearTiles)
            return true;
        Widget host = TrdW("TrdPtWearBody");
        if (!host)
            return false;
        m_TrdPtWearTiles = new OZT_VppChips(m_Flex, host, TRD_LAYOUTS + "point_wear.layout", "OZT_PtWear");
        return true;
    }

    // The things the file dresses the trader in (the top of the outfit: each with
    // how many parts hang on it), a line each: the slot and the class; before the file
    // has come, how many the section counted.
    protected void TrdPtPaintWear(OZT_Point p, OZT_AdminPoint lp)
    {
        if (!TrdPtMakeWear())
            return;
        m_TrdPtWearTiles.Clear();
        int worn = 0;
        if (p && p.Outfit && p.Outfit.Items)
        {
            for (int i = 0; i < p.Outfit.Items.Count(); i++)
            {
                OZT_OutfitItem it = p.Outfit.Items[i];
                if (!it)
                    continue;
                Widget tile = m_TrdPtWearTiles.Add(i.ToString());
                if (!tile)
                    continue;
                string slot = it.SlotName;
                slot.ToUpper();
                string cls = it.ClassName;
                if (it.Inside && it.Inside.Count() > 0)
                    cls = cls + "  +" + it.Inside.Count().ToString();
                RowText(tile, "PtWearSlot", slot, OZT_Metal.MUTED);
                RowText(tile, "PtWearCls", cls, OZT_Metal.TEXT);
                worn++;
            }
        }
        string none = "";
        if (worn == 0)
        {
            none = TrdTr("#STR_OZT_VPP_PT_WEAR_NONE");
            if (!p && lp && lp.Worn > 0)
            {
                string wears = TrdTr("#STR_OZT_VPP_PT_WEARS");
                none = wears + " " + lp.Worn.ToString();
            }
        }
        TrdInk("TrdPtWearNone", none, OZT_Metal.MUTED);
        TrdShow("TrdPtWearNone", worn == 0);
        TrdPtFlowWear();
    }

    // The tiles one under another, as wide as their box (class names are long); the
    // box's scrolled part as tall as the tiles.
    protected void TrdPtFlowWear()
    {
        if (!m_TrdPtWearTiles)
            return;
        Widget body = TrdW("TrdPtWearBody");
        Widget scroll = TrdW("TrdPtWearScroll");
        Widget none = TrdW("TrdPtWearNone");
        if (!body || !scroll)
            return;
        float s = UnitScale();
        float room;
        float view;
        scroll.GetSize(room, view);
        if (room <= 0)
            room = 265 * s;
        // 10 units of the box are the scrollbar's; the template is 256 x 28.
        float tileW = room - 10 * s;
        for (int i = 0; i < m_TrdPtWearTiles.Count(); i++)
        {
            Widget tile = m_TrdPtWearTiles.Chip(i);
            if (tile && m_Flex)
                m_Flex.Resize(tile, tileW, 28 * s, 256 * s, 28 * s);
            m_TrdPtWearTiles.SetWidth(i, tileW);
        }
        float need = m_TrdPtWearTiles.Flow(tileW, 6 * s, 28 * s, null, 0);
        if (m_TrdPtWearTiles.Count() == 0)
            need = 0;
        if (none)
            none.SetPos(0, 0);
        if (need < view)
            need = view;
        body.SetSize(room, need);
        scroll.Update();
    }

    // ------------------------------------------------------------------ edits

    protected bool TrdPtIsField(string nm)
    {
        array<string> fields = {"TrdName", "TrdClass", "TrdRadius", "TrdBuy", "TrdSell", "TrdProvider", "TrdPtStock", "TrdPtX", "TrdPtY", "TrdPtZ", "TrdPtYaw"};
        return fields.Find(nm) != -1;
    }

    // A field of the card typed into: the draft's point follows it. A value equal
    // to the draft's is no edit (an OnChange the script's own fill may raise).
    protected void TrdPtEdit(string nm)
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        string v = GetEdit(nm).Trim();
        float f = v.ToFloat();
        bool isNum = TrdIsNumber(v);
        bool changed = false;
        bool row = false;
        if (nm == "TrdName")
        {
            if (p.Name != v)
            {
                p.Name = v;
                changed = true;
                row = true;
                TrdPtEditHead(p);
            }
        }
        else if (nm == "TrdClass")
        {
            if (p.Kind != OZT_Const.KIND_MAP && p.ClassName != v)
            {
                p.ClassName = v;
                changed = true;
                TrdPtEditHead(p);
            }
        }
        else if (nm == "TrdRadius")
        {
            if (isNum && f > 0 && f != p.Radius)
            {
                p.Radius = f;
                changed = true;
            }
        }
        else if (nm == "TrdBuy")
        {
            if (isNum && f > 0 && f != p.BuyFactor)
            {
                p.BuyFactor = f;
                changed = true;
            }
        }
        else if (nm == "TrdSell")
        {
            if (isNum && f > 0 && f != p.SellFactor)
            {
                p.SellFactor = f;
                changed = true;
            }
        }
        else if (nm == "TrdProvider")
        {
            if (p.Provider != v)
            {
                p.Provider = v;
                SayTrd("BtnTrdPtMoneyText", TrdPtMoneyWord(v));
                changed = true;
                row = true;
            }
        }
        else if (nm == "TrdPtStock")
        {
            if (p.Stock != v)
            {
                p.Stock = v;
                TrdPtPaintStock(p.Id, v);
                changed = true;
                row = true;
            }
        }
        else if (nm == "TrdPtYaw")
        {
            if (isNum && f != p.Yaw)
                changed = TrdPtEditPlace(p, 3, f);
        }
        else if (nm == "TrdPtX")
        {
            if (isNum)
                changed = TrdPtEditPlace(p, 0, f);
        }
        else if (nm == "TrdPtY")
        {
            if (isNum)
                changed = TrdPtEditPlace(p, 1, f);
        }
        else if (nm == "TrdPtZ")
        {
            if (isNum)
                changed = TrdPtEditPlace(p, 2, f);
        }
        if (!changed)
            return;
        TrdTouch(OZT_Const.CFG_TRADERS);
        if (row || nm == "TrdPtX" || nm == "TrdPtY" || nm == "TrdPtZ")
            TrdPtRepaintRow();
    }

    // The place or the facing typed into (which: 0 x, 1 y, 2 z, 3 the facing). A
    // thing of the map stays where the map has it: the file's place is how the server
    // finds it. True: the draft changed.
    protected bool TrdPtEditPlace(OZT_Point p, int which, float f)
    {
        if (p.Kind == OZT_Const.KIND_MAP)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_MAP_FIXED"));
            return false;
        }
        if (which == 3)
        {
            p.Yaw = f;
            return true;
        }
        if (!p.Pos || p.Pos.Count() != 3)
            return false;
        if (p.Pos[which] == f)
            return false;
        p.Pos.Set(which, f);
        return true;
    }

    // The head follows the name and the class as they are typed.
    protected void TrdPtEditHead(OZT_Point p)
    {
        OZT_AdminPoint lp = TrdListPoint();
        int standing = 0;
        if (lp)
            standing = lp.Standing;
        string thing = p.ClassName;
        if (thing == "")
            thing = p.Model;
        TrdPtPaintHead(p.Name, p.Kind, thing, standing);
    }

    // A segment of the kind pressed. A character or an object: the draft's point
    // takes it. A thing of the map is made from what the admin looks at and keeps its
    // kind: the segment only says so.
    protected void TrdPtSetKind(string kind)
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        if (kind == OZT_Const.KIND_MAP)
        {
            if (p.Kind != OZT_Const.KIND_MAP)
                Hint(TrdTr("#STR_OZT_VPP_PT_MAP_HOW"));
            return;
        }
        if (p.Kind == OZT_Const.KIND_MAP)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_MAP_FIXED"));
            return;
        }
        if (p.Kind == kind)
            return;
        p.Kind = kind;
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtPaintKind(kind);
        TrdPtEditHead(p);
        // The pills count the kinds.
        FillTrdList();
    }

    protected void TrdPtChange()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        if (p.NoExchange == 1)
            p.NoExchange = 0;
        else
            p.NoExchange = 1;
        TrdShow("BtnTrdChangeMark", p.NoExchange == 1);
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtRepaintRow();
    }

    // The money a point may name, one step on: the server's, cash, cash then the
    // TraderPlus bank, the bank.
    protected void TrdPtMoneyNext()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        array<string> ids = {"", OZT_Const.PROVIDER_ITEMS, "items+traderplus", "traderplus"};
        int at = ids.Find(p.Provider) + 1;
        if (at >= ids.Count())
            at = 0;
        p.Provider = ids[at];
        TrdSet("TrdProvider", p.Provider);
        SayTrd("BtnTrdPtMoneyText", TrdPtMoneyWord(p.Provider));
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtRepaintRow();
    }

    // The stock a point sells from, one step on: its own, then every other stock the
    // draft's points sell from. A name of a new shared stock is typed beside it.
    protected void TrdPtStockNext()
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
            return;
        array<string> opts = new array<string>();
        opts.Insert("");
        array<ref OZT_Point> points = m_TrdDraft.Traders.Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            string sid = points[i].StockId();
            if (sid == p.Id || opts.Find(sid) != -1)
                continue;
            opts.Insert(sid);
        }
        string cur = p.Stock;
        if (cur == p.Id)
            cur = "";
        int at = opts.Find(cur) + 1;
        if (at >= opts.Count())
            at = 0;
        p.Stock = opts[at];
        TrdSet("TrdPtStock", p.Stock);
        TrdPtPaintStock(p.Id, p.Stock);
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtRepaintRow();
    }

    // A chip of the categories box pressed: one the point has leaves it, one it may
    // add joins it.
    protected void TrdPtCatClick(string key)
    {
        OZT_Point p = TrdPtDraft();
        if (!p)
        {
            Hint(TrdTr("#STR_OZT_VPP_READING"));
            return;
        }
        if (!p.Categories)
            p.Categories = new array<string>();
        string id = key.Substring(4, key.Length() - 4);
        int at = p.Categories.Find(id);
        if (key.IndexOf("has:") == 0)
        {
            if (at == -1)
                return;
            p.Categories.RemoveOrdered(at);
        }
        else
        {
            if (at != -1)
                return;
            p.Categories.Insert(id);
        }
        TrdTouch(OZT_Const.CFG_TRADERS);
        TrdPtPaintCats(p, TrdListPoint());
        TrdPtRepaintRow();
    }

    // ------------------------------------------------------------------ gestures

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w && M_SUB_WIDGET && !m_TrdFill)
        {
            string nm = w.GetName();
            if (TrdPtIsField(nm))
            {
                TrdPtEdit(nm);
                return true;
            }
            if (nm == "TrdPtFind")
            {
                FillTrdList();
                return true;
            }
        }
        return super.OnChange(w, x, y, finished);
    }

    // A click inside one of the page's lists; false: not there.
    protected bool TrdPtListClick(Widget w)
    {
        int i;
        if (m_TrdPtRows)
        {
            i = m_TrdPtRows.IndexOf(w);
            if (i != -1)
            {
                m_TrdPickedId = m_TrdPtRows.Key(i);
                m_TrdPtCatPick = false;
                TrdPtMarkRows();
                TrdPaintCard();
                return true;
            }
        }
        if (m_TrdPtCondRows)
        {
            i = m_TrdPtCondRows.IndexOf(w);
            if (i != -1)
            {
                if (w.GetName() == "PtCondDel")
                    TrdPtCondDel(m_TrdPtCondRows.Key(i).ToInt());
                return true;
            }
        }
        if (m_TrdPtCatChips)
        {
            i = m_TrdPtCatChips.IndexOf(w);
            if (i != -1)
            {
                if (w.GetName() == "PtCatDel")
                    TrdPtCatClick(m_TrdPtCatChips.Key(i));
                return true;
            }
        }
        return false;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w || !M_SUB_WIDGET)
            return super.OnClick(w, x, y, button);
        string nm = w.GetName();

        if (TrdPtListClick(w))
            return true;

        if (nm == "BtnTrdPtAll" || nm == "BtnTrdPtNpc" || nm == "BtnTrdPtObj" || nm == "BtnTrdPtMap")
        {
            m_TrdPtFilter = "";
            if (nm == "BtnTrdPtNpc")
                m_TrdPtFilter = OZT_Const.KIND_NPC;
            if (nm == "BtnTrdPtObj")
                m_TrdPtFilter = OZT_Const.KIND_OBJECT;
            if (nm == "BtnTrdPtMap")
                m_TrdPtFilter = OZT_Const.KIND_MAP;
            FillTrdList();
            return true;
        }
        if (nm == "BtnTrdKindNpc")
        {
            TrdPtSetKind(OZT_Const.KIND_NPC);
            return true;
        }
        if (nm == "BtnTrdKindObj")
        {
            TrdPtSetKind(OZT_Const.KIND_OBJECT);
            return true;
        }
        if (nm == "BtnTrdKindMap")
        {
            TrdPtSetKind(OZT_Const.KIND_MAP);
            return true;
        }
        if (nm == "BtnTrdNewNpc" || nm == "BtnTrdNewObj")
        {
            bool newNpc = nm == "BtnTrdNewNpc";
            m_TrdNewNpc = newNpc;
            TrdPaintSeg("BtnTrdNewNpc", m_TrdNewNpc);
            TrdPaintSeg("BtnTrdNewObj", !m_TrdNewNpc);
            return true;
        }
        if (nm == "BtnTrdChange")
        {
            TrdPtChange();
            return true;
        }
        if (nm == "BtnTrdPtMoney")
        {
            TrdPtMoneyNext();
            return true;
        }
        if (nm == "BtnTrdPtStockPick")
        {
            TrdPtStockNext();
            return true;
        }
        if (nm == "BtnTrdPtCatAdd")
        {
            if (m_TrdPtCatPick)
                m_TrdPtCatPick = false;
            else
                m_TrdPtCatPick = true;
            TrdPtPaintCats(TrdPtDraft(), TrdListPoint());
            return true;
        }
        if (nm == "BtnTrdPtCondKind")
        {
            TrdPtCondKindNext();
            return true;
        }
        if (nm == "BtnTrdPtCondArg")
        {
            m_TrdPtCondArg++;
            TrdPtPaintCondAdd();
            return true;
        }
        if (nm == "BtnTrdPtCondNot")
        {
            if (m_TrdPtCondNot)
                m_TrdPtCondNot = false;
            else
                m_TrdPtCondNot = true;
            TrdPtPaintCondAdd();
            return true;
        }
        if (nm == "BtnTrdPtCondAdd")
        {
            TrdPtCondAdd();
            return true;
        }
        if (nm == "BtnTrdRefresh")
        {
            Ask(OZT_Const.SECTION_ADMIN, "list", "{}");
            TrdReread(OZT_Const.CFG_TRADERS);
            return true;
        }
        // Respawning every point goes through the confirm strip (design §3.4).
        if (nm == "BtnTrdRespawn")
        {
            Confirm(TrdTr("#STR_OZT_VPP_PT_RESPAWN_ASK"), "#STR_OZT_VPP_YES_RESPAWN", "trd:respawn");
            return true;
        }
        if (nm == "BtnTrdPlace")
        {
            if (!TrdDraftHolds(OZT_Const.CFG_TRADERS))
                PlaceTrd();
            return true;
        }
        if (nm == "BtnTrdAdopt")
        {
            if (!TrdDraftHolds(OZT_Const.CFG_TRADERS))
                AdoptTrd();
            return true;
        }

        bool onPoint = nm == "BtnTrdMove" || nm == "BtnTrdTurn" || nm == "BtnTrdOutfit";
        if (nm == "BtnTrdRemove")
            onPoint = true;
        if (!onPoint)
            return super.OnClick(w, x, y, button);

        string id = PickedTrd();
        if (id == "")
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_PICK_FIRST"));
            return true;
        }
        if (TrdDraftHolds(OZT_Const.CFG_TRADERS))
            return true;
        if (nm == "BtnTrdMove")
            Ask(OZT_Const.SECTION_ADMIN, "move:" + id, "{}");
        else if (nm == "BtnTrdTurn")
            Ask(OZT_Const.SECTION_ADMIN, "turn:" + id, "{}");
        else if (nm == "BtnTrdOutfit")
            Ask(OZT_Const.SECTION_ADMIN, "outfit:" + id, "{}");
        else
            RemoveTrd(id);
        return true;
    }

    // The id a new point takes, from the field under the list; "" and a word to the
    // admin when the field is empty.
    protected string TrdPtNewId()
    {
        string id = GetEdit("TrdId").Trim();
        if (id == "")
            Hint(TrdTr("#STR_OZT_VPP_PT_NEED_ID"));
        return id;
    }

    // A new point where the admin stands: the id, class and kind of the new
    // point's fields; its name is its id until the card says otherwise.
    protected void PlaceTrd()
    {
        string id = TrdPtNewId();
        if (id == "")
            return;
        OZT_AdminPlace req = new OZT_AdminPlace();
        req.Id = id;
        req.Name = req.Id;
        req.Kind = OZT_Const.KIND_OBJECT;
        if (m_TrdNewNpc)
            req.Kind = OZT_Const.KIND_NPC;
        req.ClassName = GetEdit("TrdNewClass").Trim();
        string body;
        string err;
        JsonFileLoader<OZT_AdminPlace>.MakeData(req, body, err, false);
        m_TrdPickedId = req.Id;
        Ask(OZT_Const.SECTION_ADMIN, "place", body);
    }

    // The thing where the admin looked before opening the window becomes a
    // point of kind map: the camera has not moved since, so its ray still
    // points at it. The server finds the thing again and makes nothing.
    protected void AdoptTrd()
    {
        string id = TrdPtNewId();
        if (id == "")
            return;
        Object thing = AimedTrd();
        if (!thing)
        {
            Hint(TrdTr("#STR_OZT_VPP_PT_NOTHING_AIMED"));
            return;
        }
        OZT_AdminAdopt req = new OZT_AdminAdopt();
        req.Id = id;
        req.Name = req.Id;
        req.ClassName = thing.GetType();
        if (req.ClassName == "")
            req.Model = thing.GetShapeName();
        vector at = thing.GetPosition();
        req.X = at[0];
        req.Y = at[1];
        req.Z = at[2];
        string body;
        string err;
        JsonFileLoader<OZT_AdminAdopt>.MakeData(req, body, err, false);
        m_TrdPickedId = req.Id;
        Ask(OZT_Const.SECTION_ADMIN, "adopt", body);
        string adopting = TrdTr("#STR_OZT_VPP_PT_ADOPTING");
        Hint(adopting + " " + OZT_ClientPoints.KeyOf(thing));
    }

    // The first thing on the camera's ray, the player left out.
    protected Object AimedTrd()
    {
        Object me = GetGame().GetPlayer();
        vector from = GetGame().GetCurrentCameraPosition();
        vector to = from + GetGame().GetCurrentCameraDirection() * 50;
        vector contact;
        vector normal;
        int comp;
        set<Object> hits = new set<Object>();
        DayZPhysics.RaycastRV(from, to, contact, normal, comp, hits, null, me, true, false, ObjIntersectView, 0.0);
        for (int i = 0; i < hits.Count(); i++)
        {
            Object o = hits.Get(i);
            if (o && o != me)
                return o;
        }
        return null;
    }

    // Removing a point asks on the window's confirm strip (design §3.4).
    protected void RemoveTrd(string id)
    {
        string ask = TrdTr("#STR_OZT_VPP_PT_REMOVE_ASK");
        string tail = TrdTr("#STR_OZT_VPP_PT_REMOVE_TAIL");
        Confirm(ask + " " + id + tail, "#STR_OZT_VPP_YES_REMOVE", "trd:remove:" + id);
    }

    override void OnConfirmed(string action)
    {
        if (action.IndexOf("trd:remove:") == 0)
        {
            string id = action.Substring(11, action.Length() - 11);
            Ask(OZT_Const.SECTION_ADMIN, "remove:" + id, "{}");
            m_TrdPickedId = "";
            return;
        }
        if (action == "trd:respawn")
        {
            Ask(OZT_Const.SECTION_ADMIN, "respawn", "{}");
            return;
        }
        super.OnConfirmed(action);
    }
}

#endif
#endif
#endif
