// The module TRADER of the OpenZone admin window (VPP rework 2026-11-12, design
// §5.6): its pages on the window's rail, in the window's metal. This file is the
// first of the trader's layers of OZ_VppAdminMenu: it registers the pages, routes
// their answers and their events, and holds what the pages share -- the draft of
// the trader's three files (OZT_VppDraft) with its bar (state, problems, discard,
// apply), the server's problems after a load, and the gate providers' choices.
//
// Every page has a layout of its own (ui/OpenZone_Trader_VPP/ozt_vpp_<key>.json,
// built by layout_build) and a layer of its own in the file OZT_VppPane<Page>.c,
// whose name sorts after this one: the files of a module folder compile in
// alphabetical order, and a layer that overrides a hook must come after the layer
// that declares it. A page's layer overrides the hooks below (TrdShown, TrdLayout,
// TrdOnAnswer, TrdRepaint) for its own key and calls super for the rest.
//
// Every name this module adds to the window's class starts with Trd / m_Trd: every
// glue mods the same OZ_VppAdminMenu, and a second declaration of one name in
// another glue is a compile error of the whole module. Confirm-strip actions it
// owns start with "trd:".
//
// The pages talk to the section `traders` of the trader mod (OZT_Admin) and to the
// core's `config` section for the three files (cfg_get / cfg_set). The admin's
// rights are the core dispatcher's to check; the pages only ask.
//
// Guards: NO_GUI -- the server compiles Mission without the UI; AVPPAdminTools
// and OpenZone_VPP -- names of CfgMods classes, defined by the engine.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    static const string TRD_MOD     = "trader";
    static const string TRD_LAYOUTS = "OpenZone_Trader_VPP/gui/layouts/ozt_vpp_";

    // The pages' roots by key ("points", ...) and the key of each rail id; the
    // content area the shell gave the panes, in pixels (OnWindowResized).
    protected ref map<string, Widget> m_TrdPanes;
    protected ref map<string, string> m_TrdKeys;
    protected float m_TrdW = 0;
    protected float m_TrdH = 0;

    // The draft of the three files; the script writing fields itself (an
    // OnChange that follows is not an edit).
    protected ref OZT_VppDraft m_TrdDraft;
    protected bool m_TrdFill = false;

    // The pages with the draft's bar, by their widget prefix; the server's last
    // `problems`, whether its panel is open, and the last apply's line.
    protected ref array<string> m_TrdBars;
    protected ref OZT_AdminProblems m_TrdProbs;
    protected bool m_TrdProbsOpen = false;
    protected string m_TrdNote = "";

    // The server's last `gates`: the providers, their kinds and choices.
    protected ref OZT_AdminGates m_TrdGates;
    protected bool m_TrdGatesAsked = false;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);
        if (!M_SUB_WIDGET)
            return;

        m_TrdPanes = new map<string, Widget>();
        m_TrdKeys = new map<string, string>();
        m_TrdDraft = new OZT_VppDraft();
        m_TrdBars = new array<string>();
        m_TrdBars.Insert("TrdPt");
        m_TrdBars.Insert("TrdGd");
        m_TrdBars.Insert("TrdGt");
        m_TrdBars.Insert("TrdLc");
        m_TrdBars.Insert("TrdMn");

        array<string> keys = new array<string>();
        array<string> labels = new array<string>();
        TrdPageList(keys, labels);
        for (int i = 0; i < keys.Count(); i++)
            TrdAddPage(TRD_MOD, "#STR_OZT_VPP_MOD", TRD_MOD + "." + keys[i], keys[i], labels[i]);

        OZ_ClientState.AdminWatch().Insert(this.OnTraderAnswer);
        TrdPaintBars();
    }

    void ~OZ_VppAdminMenu()
    {
        OZ_ClientState.AdminWatch().Remove(this.OnTraderAnswer);
    }

    // The module's pages in the rail's order, with their captions.
    protected void TrdPageList(array<string> keys, array<string> labels)
    {
        keys.Insert("points");
        labels.Insert("#STR_OZT_VPP_PAGE_POINTS");
        keys.Insert("goods");
        labels.Insert("#STR_OZT_VPP_PAGE_GOODS");
        keys.Insert("gates");
        labels.Insert("#STR_OZT_VPP_PAGE_GATES");
        keys.Insert("stock");
        labels.Insert("#STR_OZT_VPP_PAGE_STOCK");
        keys.Insert("licences");
        labels.Insert("#STR_OZT_VPP_PAGE_LICENCES");
        keys.Insert("money");
        labels.Insert("#STR_OZT_VPP_PAGE_MONEY");
        keys.Insert("import");
        labels.Insert("#STR_OZT_VPP_PAGE_IMPORT");
    }

    // One page: its own layout under the window, registered on the rail.
    protected Widget TrdAddPage(string module, string moduleLabel, string id, string key, string label)
    {
        string path = TRD_LAYOUTS + key + ".layout";
        Widget pane = GetGame().GetWorkspace().CreateWidgets(path, M_SUB_WIDGET);
        if (!pane)
        {
            OZT_Log.Error("trader vpp: a page's layout did not load: " + path);
            return null;
        }
        m_TrdPanes.Set(key, pane);
        m_TrdKeys.Set(id, key);
        RegisterPage(module, moduleLabel, id, label, pane, "");
        return pane;
    }

    // Is a page of ours on screen, and which: "" when none.
    protected string TrdShownKey()
    {
        string key = "";
        if (m_TrdKeys)
            m_TrdKeys.Find(CurrentPane(), key);
        return key;
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);
        string key;
        if (m_TrdKeys && m_TrdKeys.Find(id, key))
            TrdShown(key);
    }

    // A page of ours came on screen. Each page's layer takes its own key; a page
    // with the draft's bar asks for the server's problems.
    protected void TrdShown(string key)
    {
        TrdPaintBars();
        if (!m_TrdProbs && TrdHasBar(key))
            TrdAskProblems();
    }

    protected bool TrdHasBar(string key)
    {
        array<string> bars = {"points", "goods", "gates", "licences", "money"};
        return bars.Find(key) != -1;
    }

    // The window was resized; every page lays itself out from w x h (pixels) and
    // reads no size back (a widget measured before the engine's first layout pass
    // reads 0).
    override void OnWindowResized(float w, float h)
    {
        super.OnWindowResized(w, h);
        m_TrdW = w;
        m_TrdH = h;
        if (M_SUB_WIDGET && m_TrdPanes)
            TrdLayout(w, h);
    }

    protected void TrdLayout(float w, float h)
    {
        for (int i = 0; i < m_TrdBars.Count(); i++)
            TrdLayBar(m_TrdBars[i], w);
    }

    // The draft's bar and its problems panel follow the window's width.
    protected void TrdLayBar(string prefix, float w)
    {
        float s = UnitScale();
        TrdSize(prefix + "Bar", w, 56 * s);
        TrdSize(prefix + "BarLine", w, s);
        TrdSize(prefix + "Probs", w, 150 * s);
        TrdSize(prefix + "ProbsLine", w, s);
        TrdSize(prefix + "ProbsList", w - 48 * s, 110 * s);
    }

    // A page's content changed under it (a file arrived, a discard, an apply):
    // the shown page paints itself again.
    protected void TrdRepaint(string key)
    {
    }

    // ------------------------------------------------------------------ helpers

    protected Widget TrdW(string name)
    {
        if (!M_SUB_WIDGET)
            return null;
        return M_SUB_WIDGET.FindAnyWidget(name);
    }

    // Pixels, as exact-size widgets take them.
    protected void TrdSize(string name, float w, float h)
    {
        Widget x = TrdW(name);
        if (x)
            x.SetSize(w, h);
    }

    protected void TrdPlace(string name, float x, float y, float w, float h)
    {
        Widget it = TrdW(name);
        if (!it)
            return;
        it.SetPos(x, y);
        it.SetSize(w, h);
    }

    protected void TrdMove(string name, float x, float y)
    {
        Widget it = TrdW(name);
        if (it)
            it.SetPos(x, y);
    }

    // A page head's rule follows the window's width.
    protected void TrdLayHead(string prefix, float w)
    {
        TrdSize(prefix + "HeadRule", w, UnitScale());
    }

    // A string of the trader's stringtable in the client's language. Take it into
    // a local before it meets another: two translations in one expression lose
    // one of them (dayz-modding, mod-structure.md).
    protected string TrdTr(string key)
    {
        return Widget.TranslateString(key);
    }

    protected void SayTrd(string name, string text)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (t)
            t.SetText(text);
    }

    protected void TrdInk(string name, string text, int color)
    {
        TextWidget t = TextWidget.Cast(TrdW(name));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(color);
    }

    // A field written by the script: the OnChange it may raise is not an edit.
    protected void TrdSet(string name, string text)
    {
        m_TrdFill = true;
        SetEdit(name, text);
        m_TrdFill = false;
    }

    // A listbox from rows: `inks` colours rows (0: the layout's own), `picked` is
    // selected and kept in view. The core's m_Repaint swallows the selection
    // events SelectRow may raise.
    protected void TrdFill(string name, array<string> rows, array<int> inks, int picked)
    {
        TextListboxWidget lb = TextListboxWidget.Cast(TrdW(name));
        if (!lb)
            return;
        m_Repaint = true;
        lb.ClearItems();
        for (int i = 0; i < rows.Count(); i++)
        {
            lb.AddItem(rows[i], null, 0);
            if (inks && i < inks.Count() && inks[i] != 0)
                lb.SetItemColor(i, 0, inks[i]);
        }
        if (picked >= 0 && picked < rows.Count())
        {
            lb.SelectRow(picked);
            lb.EnsureVisible(picked);
        }
        m_Repaint = false;
    }

    protected int TrdPickedRow(string name)
    {
        TextListboxWidget lb = TextListboxWidget.Cast(TrdW(name));
        if (!lb)
            return -1;
        return lb.GetSelectedRow();
    }

    // The verb of an op: "move:npc" -> "move".
    protected string TrdVerb(string op)
    {
        int colon = op.IndexOf(":");
        if (colon < 0)
            return op;
        return op.Substring(0, colon);
    }

    // A comma list typed by the admin, trimmed, the empty ones left out.
    protected void TrdSplit(string text, array<string> into)
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

    protected string TrdJoin(array<string> items, string sep)
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

    // A float as the admin types it: at most three decimals, no trailing zeros.
    protected string TrdNum(float f)
    {
        int whole = Math.Round(f * 1000);
        float back = whole / 1000.0;
        return back.ToString();
    }

    // Lower-case for a filter (ASCII; Cyrillic is matched as typed).
    protected string TrdLow(string s)
    {
        string low = s;
        low.ToLower();
        return low;
    }

    // The kind of a point as the admin reads it.
    protected string TrdKindWord(string kind)
    {
        if (kind == OZT_Const.KIND_NPC)
            return TrdTr("#STR_OZT_VPP_KIND_NPC");
        if (kind == OZT_Const.KIND_MAP)
            return TrdTr("#STR_OZT_VPP_KIND_MAP");
        return TrdTr("#STR_OZT_VPP_KIND_OBJECT");
    }

    // A condition kind as the admin reads it; a kind of another mod as it is.
    protected string TrdCondKindWord(string kind)
    {
        if (kind == "faction")
            return TrdTr("#STR_OZT_VPP_CK_FACTION");
        if (kind == "post")
            return TrdTr("#STR_OZT_VPP_CK_POST");
        if (kind == "leader")
            return TrdTr("#STR_OZT_VPP_CK_LEADER");
        if (kind == "licence")
            return TrdTr("#STR_OZT_VPP_CK_LICENCE");
        if (kind == "research")
            return TrdTr("#STR_OZT_VPP_CK_RESEARCH");
        return kind;
    }

    // "research: Name (id) | Name (id)", "not licence: ...": a condition in words,
    // each alternative by the label its provider gives it when it has one.
    protected string TrdCondWords(OZT_Condition c)
    {
        if (!c)
            return "";
        string kind = TrdCondKindWord(c.Kind);
        string words = kind + ": " + TrdArgWords(c.Kind, c.Arg);
        if (c.Not == 1)
        {
            string notWord = TrdTr("#STR_OZT_VPP_NOT");
            words = notWord + " " + words;
        }
        return words;
    }

    protected string TrdArgWords(string kind, string arg)
    {
        array<string> alts = new array<string>();
        arg.Split("|", alts);
        string words = "";
        for (int i = 0; i < alts.Count(); i++)
        {
            string one = alts[i].Trim();
            if (one == "")
                continue;
            if (words != "")
                words = words + " | ";
            string label = TrdChoiceLabel(kind, one);
            if (label != "" && label != one)
                words = words + label + " (" + one + ")";
            else
                words = words + one;
        }
        if (words == "")
            words = "-";
        return words;
    }

    // ------------------------------------------------------------------ gates

    protected void TrdAskGates()
    {
        m_TrdGatesAsked = true;
        Ask(OZT_Const.SECTION_ADMIN, "gates", "{}");
    }

    protected void TrdNeedGates()
    {
        if (!m_TrdGates && !m_TrdGatesAsked)
            TrdAskGates();
    }

    // The server's gates have arrived (m_TrdGates). A layer that waits for them to
    // decide something -- Research > Trade appears only beside the research gate --
    // hears it here.
    protected void TrdGatesKnown()
    {
    }

    // The kind's entry of the last `gates`, or null.
    protected OZT_AdminGateKind TrdGateKind(string kind)
    {
        if (!m_TrdGates)
            return null;
        for (int f = 0; f < m_TrdGates.Families.Count(); f++)
        {
            OZT_AdminGateFamily fam = m_TrdGates.Families[f];
            for (int k = 0; k < fam.Kinds.Count(); k++)
            {
                if (fam.Kinds[k].Kind == kind)
                    return fam.Kinds[k];
            }
        }
        return null;
    }

    // The family that answers a kind ("" when no provider has it).
    protected string TrdFamilyOf(string kind)
    {
        if (!m_TrdGates)
            return "";
        for (int f = 0; f < m_TrdGates.Families.Count(); f++)
        {
            OZT_AdminGateFamily fam = m_TrdGates.Families[f];
            for (int k = 0; k < fam.Kinds.Count(); k++)
            {
                if (fam.Kinds[k].Kind == kind)
                    return fam.Family;
            }
        }
        return "";
    }

    protected string TrdChoiceLabel(string kind, string value)
    {
        OZT_AdminGateKind gk = TrdGateKind(kind);
        if (!gk)
            return "";
        int at = gk.Values.Find(value);
        if (at < 0 || at >= gk.Labels.Count())
            return "";
        return gk.Labels[at];
    }

    // Every kind the providers answer, the known ones first when `gates` has not
    // come yet.
    protected void TrdKinds(array<string> kinds)
    {
        kinds.Clear();
        if (m_TrdGates)
        {
            for (int f = 0; f < m_TrdGates.Families.Count(); f++)
            {
                OZT_AdminGateFamily fam = m_TrdGates.Families[f];
                for (int k = 0; k < fam.Kinds.Count(); k++)
                    kinds.Insert(fam.Kinds[k].Kind);
            }
        }
        array<string> known = {"faction", "post", "leader", "licence", "research"};
        for (int i = 0; i < known.Count(); i++)
        {
            if (kinds.Find(known[i]) < 0)
                kinds.Insert(known[i]);
        }
    }

    // Every family the providers name, the known ones when `gates` has not come.
    protected void TrdFamilies(array<string> families)
    {
        families.Clear();
        if (m_TrdGates)
        {
            for (int f = 0; f < m_TrdGates.Families.Count(); f++)
                families.Insert(m_TrdGates.Families[f].Family);
        }
        array<string> known = {"factions", "licences", "research"};
        for (int i = 0; i < known.Count(); i++)
        {
            if (families.Find(known[i]) < 0)
                families.Insert(known[i]);
        }
    }

    // ------------------------------------------------------------------ the draft

    // A file the shown page needs: asked once, the answer goes to the draft.
    protected void TrdNeedDoc(string name)
    {
        if (!m_TrdDraft || m_TrdDraft.Loaded(name) || m_TrdDraft.Asked(name))
            return;
        m_TrdDraft.MarkAsked(name, true);
        AskCfg(name);
    }

    // The server's file changed (an op, an apply, an import): read it again
    // unless the draft has changed it.
    protected void TrdReread(string name)
    {
        if (!m_TrdDraft || m_TrdDraft.Dirty(name))
            return;
        m_TrdDraft.MarkAsked(name, true);
        AskCfg(name);
    }

    protected void TrdTouch(string name)
    {
        if (!m_TrdDraft)
            return;
        m_TrdDraft.Touch(name);
        TrdPaintBars();
    }

    // Ops that rewrite a file on the server wait while the draft has changed it:
    // an apply would write the draft's older copy over the op's work.
    protected bool TrdDraftHolds(string name)
    {
        if (!m_TrdDraft || !m_TrdDraft.Dirty(name))
            return false;
        Hint(TrdTr("#STR_OZT_VPP_DRAFT_FIRST"));
        return true;
    }

    override void OnCfgText(string name, string body)
    {
        super.OnCfgText(name, body);
        if (!m_TrdDraft || !m_TrdDraft.Owns(name))
            return;
        string err;
        if (!m_TrdDraft.Take(name, body, err) && err != "")
        {
            string bad = TrdTr("#STR_OZT_VPP_BAD_FILE");
            Hint(bad + " " + name + ": " + err);
        }
        TrdPaintBars();
        TrdRepaint(TrdShownKey());
    }

    // The files a changed draft holds, in the admin's words.
    protected string TrdChangedWords()
    {
        array<string> names = m_TrdDraft.Names();
        string words = "";
        for (int i = 0; i < names.Count(); i++)
        {
            if (!m_TrdDraft.Dirty(names[i]))
                continue;
            if (words != "")
                words = words + ", ";
            words = words + TrdFileWord(names[i]);
        }
        return words;
    }

    protected string TrdFileWord(string name)
    {
        if (name == OZT_Const.CFG_SETTINGS)
            return TrdTr("#STR_OZT_VPP_FILE_SETTINGS");
        if (name == OZT_Const.CFG_GOODS)
            return TrdTr("#STR_OZT_VPP_FILE_GOODS");
        return TrdTr("#STR_OZT_VPP_FILE_TRADERS");
    }

    protected void TrdApplyAsk()
    {
        if (!m_TrdDraft || m_TrdDraft.Changed() == 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_NOTHING_CHANGED"));
            return;
        }
        if (m_TrdDraft.Sending() != "")
        {
            Hint(TrdTr("#STR_OZT_VPP_APPLYING"));
            return;
        }
        string ask = TrdTr("#STR_OZT_VPP_APPLY_ASK");
        string files = TrdChangedWords();
        Confirm(ask + " " + files + "?", "#STR_OZT_VPP_YES_APPLY", "trd:apply");
    }

    protected void TrdDiscardAsk()
    {
        if (!m_TrdDraft || m_TrdDraft.Changed() == 0)
        {
            Hint(TrdTr("#STR_OZT_VPP_NOTHING_CHANGED"));
            return;
        }
        string ask = TrdTr("#STR_OZT_VPP_DISCARD_ASK");
        string files = TrdChangedWords();
        Confirm(ask + " " + files + "?", "#STR_OZT_VPP_YES_DISCARD", "trd:discard");
    }

    // The next changed file to the server; after the last, the problems its load
    // reported.
    protected void TrdApplyNext()
    {
        string name = m_TrdDraft.Next();
        if (name == "")
        {
            string done = TrdTr("#STR_OZT_VPP_APPLIED");
            m_TrdNote = done + " " + OZ_VppJournal.Clock();
            m_TrdProbsOpen = true;
            TrdAskProblems();
            TrdPaintBars();
            return;
        }
        SendCfg(name, m_TrdDraft.Body(name));
        TrdPaintBars();
    }

    // The core's config answers: an applied file, or a file that could not be read.
    protected void TrdOnConfig(string op, bool ok, string json, string error)
    {
        if (op.IndexOf("cfg_set:") == 0)
        {
            string sent = op.Substring(8, op.Length() - 8);
            if (sent != m_TrdDraft.Sending())
            {
                // Applied elsewhere (the core's file page): read it again.
                if (ok && m_TrdDraft.Owns(sent))
                    TrdReread(sent);
                return;
            }
            if (!ok)
            {
                m_TrdDraft.Refused();
                m_TrdNote = Words(error);
                TrdAskProblems();
                TrdPaintBars();
                return;
            }
            m_TrdDraft.Applied(sent);
            TrdReread(sent);
            TrdApplyNext();
            TrdRepaint(TrdShownKey());
            return;
        }
        if (op.IndexOf("cfg_get:") == 0 && !ok)
        {
            string asked = op.Substring(8, op.Length() - 8);
            if (!m_TrdDraft.Owns(asked))
                return;
            m_TrdDraft.MarkAsked(asked, false);
            string unread = TrdTr("#STR_OZT_VPP_BAD_FILE");
            m_TrdNote = unread + " " + asked + ": " + Words(error);
            TrdPaintBars();
        }
    }

    // ------------------------------------------------------------------ the bar

    protected void TrdAskProblems()
    {
        Ask(OZT_Const.SECTION_ADMIN, "problems", "{}");
    }

    protected void TrdPaintBars()
    {
        if (!m_TrdBars)
            return;
        for (int i = 0; i < m_TrdBars.Count(); i++)
            TrdPaintBar(m_TrdBars[i]);
    }

    protected void TrdPaintBar(string prefix)
    {
        if (!m_TrdDraft || !TrdW(prefix + "Bar"))
            return;
        string state;
        int ink = OZT_Metal.MUTED;
        if (m_TrdDraft.Sending() != "")
        {
            state = TrdTr("#STR_OZT_VPP_APPLYING");
            ink = OZT_Metal.ALERT;
        }
        else if (m_TrdDraft.Changed() == 0)
        {
            state = TrdTr("#STR_OZT_VPP_NO_CHANGES");
        }
        else
        {
            string changed = TrdTr("#STR_OZT_VPP_CHANGED");
            string files = TrdChangedWords();
            state = changed + " " + files;
            ink = OZT_Metal.ALERT;
        }
        TrdInk(prefix + "BarState", state, ink);

        string probs;
        int probInk = OZT_Metal.MUTED;
        if (!m_TrdProbs)
            probs = TrdTr("#STR_OZT_VPP_PROBS_UNKNOWN");
        else if (m_TrdProbs.Count == 0)
        {
            probs = TrdTr("#STR_OZT_VPP_PROBS_NONE");
            probInk = OZT_Metal.OK;
        }
        else
        {
            string many = TrdTr("#STR_OZT_VPP_PROBS_N");
            probs = many + " " + m_TrdProbs.Count.ToString();
            probInk = OZT_Metal.ALERT;
        }
        TrdInk("Btn" + prefix + "ProbsText", probs, probInk);
        SayTrd(prefix + "BarNote", m_TrdNote);

        Widget panel = TrdW(prefix + "Probs");
        if (panel)
            panel.Show(m_TrdProbsOpen);
        if (m_TrdProbsOpen)
            TrdFillProbs(prefix);
    }

    protected void TrdFillProbs(string prefix)
    {
        array<string> rows = new array<string>();
        array<int> inks = new array<int>();
        if (!m_TrdProbs)
        {
            rows.Insert(TrdTr("#STR_OZT_VPP_PROBS_UNKNOWN"));
            inks.Insert(0);
        }
        else
        {
            for (int i = 0; i < m_TrdProbs.Lines.Count(); i++)
            {
                rows.Insert(m_TrdProbs.Lines[i]);
                inks.Insert(OZT_Metal.ALERT);
            }
            if (m_TrdProbs.Cut == 1)
            {
                string more = TrdTr("#STR_OZT_VPP_PROBS_MORE");
                int left = m_TrdProbs.Count - m_TrdProbs.Lines.Count();
                rows.Insert(more + " " + left.ToString());
                inks.Insert(0);
            }
            if (rows.Count() == 0)
            {
                rows.Insert(TrdTr("#STR_OZT_VPP_PROBS_NONE"));
                inks.Insert(OZT_Metal.OK);
            }
            rows.Insert(m_TrdProbs.Counters);
            inks.Insert(OZT_Metal.MUTED);
        }
        TrdFill(prefix + "ProbsList", rows, inks, -1);
    }

    // A click on a bar of any page: the bars' buttons are named after the page's
    // prefix.
    protected bool TrdBarClick(string nm)
    {
        for (int i = 0; i < m_TrdBars.Count(); i++)
        {
            string b = "Btn" + m_TrdBars[i];
            if (nm == b + "Apply")
            {
                TrdApplyAsk();
                return true;
            }
            if (nm == b + "Discard")
            {
                TrdDiscardAsk();
                return true;
            }
            if (nm == b + "Probs")
            {
                if (m_TrdProbsOpen)
                    m_TrdProbsOpen = false;
                else
                    m_TrdProbsOpen = true;
                if (m_TrdProbsOpen)
                    TrdAskProblems();
                TrdPaintBars();
                return true;
            }
            if (nm == b + "ProbsClose")
            {
                m_TrdProbsOpen = false;
                TrdPaintBars();
                return true;
            }
        }
        return false;
    }

    // ------------------------------------------------------------------ answers

    void OnTraderAnswer(string section, string op, bool ok, string json, string error)
    {
        if (!M_SUB_WIDGET)
            return;
        if (section == OZ_AdminSect.CONFIG)
        {
            TrdOnConfig(op, ok, json, error);
            return;
        }
        if (section != OZT_Const.SECTION_ADMIN)
            return;
        TrdOnAnswer(op, ok, json, error);
    }

    // An answer of the section `traders`. A failure is in the window's journal
    // already, with the server's words; the pages only follow what changed. The
    // later layers take their ops and call super.
    protected void TrdOnAnswer(string op, bool ok, string json, string error)
    {
        string err;
        if (op == "problems" && ok)
        {
            OZT_AdminProblems probs = new OZT_AdminProblems();
            if (JsonFileLoader<OZT_AdminProblems>.LoadData(json, probs, err))
            {
                m_TrdProbs = probs.Copy();
                TrdPaintBars();
            }
            return;
        }
        if (op == "gates")
        {
            m_TrdGatesAsked = false;
            if (!ok)
                return;
            OZT_AdminGates gates = new OZT_AdminGates();
            if (JsonFileLoader<OZT_AdminGates>.LoadData(json, gates, err))
            {
                m_TrdGates = gates.Copy();
                TrdGatesKnown();
                TrdRepaint(TrdShownKey());
            }
            return;
        }
    }

    // The section's reads are asked all the time and are not news for the
    // window's journal (its failures still reach it).
    override bool IsReadOp(string op)
    {
        array<string> reads = {"stock_list", "licence_list", "problems", "gates", "gate_holders"};
        if (reads.Find(op) != -1)
            return true;
        return super.IsReadOp(op);
    }

    // ------------------------------------------------------------------ gestures

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET && m_TrdBars && TrdBarClick(w.GetName()))
            return true;
        return super.OnClick(w, x, y, button);
    }

    // The confirm strip's "yes" for the draft; the rest go on.
    override void OnConfirmed(string action)
    {
        if (action == "trd:apply")
        {
            TrdApplyNext();
            return;
        }
        if (action == "trd:discard")
        {
            m_TrdDraft.Discard();
            m_TrdNote = TrdTr("#STR_OZT_VPP_DISCARDED");
            TrdPaintBars();
            TrdRepaint(TrdShownKey());
            return;
        }
        super.OnConfirmed(action);
    }
}

#endif
#endif
#endif
