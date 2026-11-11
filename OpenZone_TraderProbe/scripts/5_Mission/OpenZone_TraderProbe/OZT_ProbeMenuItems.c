// Question 9: client-local entities made only to be looked at.
//
//   items <n> near|far [still] n local entities of n different classes, each on
//                              a tile; `near` at the player's feet, `far` at the
//                              corner of the map ("0 0 0", where the game's own
//                              script console keeps its preview); `still` turns
//                              their simulation off, as that console does
//   items where                how many still exist, and where the first stands
//   items draggable            make every tile a thing the engine drags
//   items keep                 leave the entities in the world when the screen
//                              closes (to look at the vicinity with them there)
//   items drop                 delete what `keep` left
//   items clear                delete them and their tiles
//   kit rifle [show|sync]      a local rifle put together from parts, in KitView:
//                              optics, suppressor, stock, guard and magazine;
//                              `show` calls ShowMagazine, `sync` ForceSyncSelectionState
//   kit vest                   a vest with pouches and a holstered pistol
//   kit clear
//
// After `items` the frame counters start anew, so `ui frames` a few seconds
// later says what loading the models cost.
class OZT_ProbeQItems : OZT_ProbeQuestion
{
    static const string TILE = "OpenZone_TraderProbe/gui/layouts/ozt_probe_tile.layout";

    protected Widget m_Grid;
    protected ItemPreviewWidget m_KitView;
    protected ref array<EntityAI> m_Made;
    protected ref array<Widget> m_Tiles;
    protected EntityAI m_Kit;
    protected bool m_Keep;

    // What `items keep` left behind a closed screen.
    protected static ref array<EntityAI> s_Kept = new array<EntityAI>();

    override void OnAttach()
    {
        m_Grid = m_Root.FindAnyWidget("ItemsGrid");
        m_KitView = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("KitView"));
        m_Made = new array<EntityAI>();
        m_Tiles = new array<Widget>();
    }

    override void Stop()
    {
        if (m_Keep)
        {
            for (int i = 0; i < m_Made.Count(); i++)
                s_Kept.Insert(m_Made[i]);
            m_Made.Clear();
        }
        Clear();
        ClearKit();
    }

    protected int DropKept()
    {
        int n = 0;
        for (int i = 0; i < s_Kept.Count(); i++)
        {
            if (!s_Kept[i])
                continue;
            GetGame().ObjectDelete(s_Kept[i]);
            n++;
        }
        s_Kept.Clear();
        return n;
    }

    protected void Clear()
    {
        for (int i = 0; i < m_Made.Count(); i++)
        {
            if (m_Made[i])
                GetGame().ObjectDelete(m_Made[i]);
        }
        m_Made.Clear();
        for (int t = 0; t < m_Tiles.Count(); t++)
        {
            if (m_Tiles[t])
                m_Tiles[t].Unlink();
        }
        m_Tiles.Clear();
    }

    protected void ClearKit()
    {
        if (m_KitView)
            m_KitView.SetItem(null);
        // Its parts are its children and go with it.
        if (m_Kit)
            GetGame().ObjectDelete(m_Kit);
        m_Kit = null;
    }

    override string Command(array<string> words)
    {
        if (words[1] == "items")
            return ItemsCommand(words);
        if (words[1] == "kit")
            return KitCommand(words);
        return "";
    }

    // ---- many of them ------------------------------------------------------------

    protected string ItemsCommand(array<string> words)
    {
        string sub = Word(words, 2, "where");
        if (sub == "clear")
        {
            int had = m_Made.Count();
            Clear();
            return "items: " + had.ToString() + " deleted";
        }
        if (sub == "where")
            return Where();
        if (sub == "keep")
        {
            m_Keep = true;
            return "items: " + m_Made.Count().ToString() + " will stay in the world when the screen closes";
        }
        if (sub == "drop")
            return "items: " + DropKept().ToString() + " kept ones deleted";
        if (sub == "draggable")
        {
            for (int d = 0; d < m_Tiles.Count(); d++)
            {
                m_Tiles[d].ClearFlags(WidgetFlags.IGNOREPOINTER);
                m_Tiles[d].SetFlags(WidgetFlags.DRAGGABLE);
            }
            return "items: " + m_Tiles.Count().ToString() + " tiles take the pointer and can be dragged";
        }

        int want = sub.ToInt();
        if (want < 1 || want > 400)
            return "items: a count from 1 to 400, then near or far";
        bool near = Word(words, 3, "far") == "near";
        bool still = Word(words, 4, "") == "still";
        Clear();

        float t0 = GetGame().GetTickTime();
        array<string> classes = new array<string>();
        Classes(want, classes);
        float t1 = GetGame().GetTickTime();

        vector at = "0 0 0";
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (near && player)
            at = player.GetPosition();
        int failed = 0;
        for (int i = 0; i < classes.Count(); i++)
        {
            EntityAI e = EntityAI.Cast(GetGame().CreateObjectEx(classes[i], at, ECE_LOCAL));
            if (!e)
            {
                failed++;
                OZT_ProbeClient.Note("not made: " + classes[i]);
                continue;
            }
            if (still)
                e.DisableSimulation(true);
            m_Made.Insert(e);
        }
        float t2 = GetGame().GetTickTime();

        for (int k = 0; k < m_Made.Count(); k++)
        {
            Widget tile = GetGame().GetWorkspace().CreateWidgets(TILE, m_Grid);
            if (!tile)
                continue;
            m_Tiles.Insert(tile);
            ItemPreviewWidget pic = ItemPreviewWidget.Cast(tile.FindAnyWidget("TilePic"));
            if (!pic)
                continue;
            pic.SetItem(m_Made[k]);
            pic.SetView(m_Made[k].GetViewIndex());
        }
        float t3 = GetGame().GetTickTime();
        m_Menu.ResetFrames();

        string place = "far";
        if (near)
            place = "near";
        if (still)
            place = place + " still";
        string s = "items " + want.ToString() + " " + place + ": classes=" + classes.Count().ToString();
        s = s + " made=" + m_Made.Count().ToString() + " not_made=" + failed.ToString() + " tiles=" + m_Tiles.Count().ToString();
        s = s + " | ms: find_classes=" + Ms(t0, t1) + " create=" + Ms(t1, t2) + " tiles=" + Ms(t2, t3);
        return s;
    }

    protected string Ms(float from, float to)
    {
        return OZT_ProbeLog.R1((to - from) * 1000);
    }

    protected string Where()
    {
        int alive = 0;
        EntityAI first;
        for (int i = 0; i < m_Made.Count(); i++)
        {
            if (!m_Made[i])
                continue;
            alive++;
            if (!first)
                first = m_Made[i];
        }
        string s = "items: listed=" + m_Made.Count().ToString() + " alive=" + alive.ToString();
        if (first)
        {
            s = s + " first=" + first.GetType() + " at " + V(first.GetPosition());
            s = s + " id=" + first.GetNetworkIDString();
        }
        return s;
    }

    // `want` classes a trader could sell, spread over the game's config: things
    // (CfgVehicles under Inventory_Base), weapons, magazines; public ones only.
    protected void Classes(int want, array<string> list)
    {
        array<string> all = new array<string>();
        Gather("CfgVehicles", "Inventory_Base", all);
        Gather("CfgWeapons", "Weapon_Base", all);
        Gather("CfgMagazines", "Magazine_Base", all);
        if (all.Count() <= want)
        {
            list.Copy(all);
            return;
        }
        // Every step-th, so that the kinds are mixed.
        float step = all.Count();
        step = step / want;
        for (int i = 0; i < want; i++)
        {
            int at = i * step;
            list.Insert(all[at]);
        }
    }

    protected void Gather(string root, string kind, array<string> into)
    {
        int n = GetGame().ConfigGetChildrenCount(root);
        for (int i = 0; i < n; i++)
        {
            string name;
            GetGame().ConfigGetChildName(root, i, name);
            if (GetGame().ConfigGetInt(root + " " + name + " scope") != 2)
                continue;
            if (!GetGame().IsKindOf(name, kind))
                continue;
            into.Insert(name);
        }
    }

    // ---- one, put together from parts ----------------------------------------------

    protected string KitCommand(array<string> words)
    {
        string sub = Word(words, 2, "clear");
        ClearKit();
        if (sub == "clear")
            return "kit: cleared";
        if (!m_KitView)
            return "kit: the layout has no KitView";

        float t0 = GetGame().GetTickTime();
        string parts = "";
        if (sub == "rifle")
        {
            m_Kit = Local("AKM");
            parts = parts + Part(m_Kit, "PSO1Optic");
            parts = parts + Part(m_Kit, "AK_Suppressor");
            parts = parts + Part(m_Kit, "AK_WoodBttstck");
            parts = parts + Part(m_Kit, "AK_WoodHndgrd");
            parts = parts + Part(m_Kit, "Mag_AKM_30Rnd");
            // A magazine on a local weapon is there and is not drawn: the
            // weapon's own selection has to be told.
            string how = Word(words, 3, "");
            Weapon_Base wpn = Weapon_Base.Cast(m_Kit);
            if (wpn && how == "show")
                wpn.ShowMagazine();
            if (wpn && how == "sync")
                wpn.ForceSyncSelectionState();
            parts = parts + " magazine drawn by=" + how;
        }
        else if (sub == "vest")
        {
            m_Kit = Local("PlateCarrierVest");
            parts = parts + Part(m_Kit, "PlateCarrierPouches");
            parts = parts + Part(m_Kit, "PlateCarrierHolster");
            EntityAI holster;
            if (m_Kit)
                holster = m_Kit.FindAttachmentBySlotName("VestHolster");
            parts = parts + " holster found=" + OZT_ProbeLog.B(holster != null);
            EntityAI pistol;
            if (holster)
                pistol = holster.GetInventory().CreateAttachment("MakarovIJ70");
            parts = parts + " pistol in the holster=" + OZT_ProbeLog.B(pistol != null);
            parts = parts + Part(pistol, "Mag_IJ70_8Rnd");
        }
        else
        {
            return "kit: rifle, vest or clear";
        }
        if (!m_Kit)
            return "kit " + sub + ": the base was not made";
        m_KitView.SetItem(m_Kit);
        m_KitView.SetView(m_Kit.GetViewIndex());
        float t1 = GetGame().GetTickTime();

        string s = "kit " + sub + ": on it=" + m_Kit.GetInventory().AttachmentCount().ToString();
        s = s + " ms=" + Ms(t0, t1) + " |" + parts;
        return s;
    }

    protected EntityAI Local(string type)
    {
        return EntityAI.Cast(GetGame().CreateObjectEx(type, "0 0 0", ECE_LOCAL));
    }

    // One part created on a parent; the report says whether it came.
    protected string Part(EntityAI parent, string type)
    {
        if (!parent)
            return " " + type + "=no parent";
        EntityAI made = parent.GetInventory().CreateAttachment(type);
        return " " + type + "=" + OZT_ProbeLog.B(made != null);
    }
}
