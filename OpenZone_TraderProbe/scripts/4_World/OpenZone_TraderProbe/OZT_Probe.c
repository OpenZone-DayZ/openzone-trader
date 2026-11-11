// The server-side op engine of the trader probe.
//
// One op per bridge command, answered within the tick that carries it. An op
// gathers its lines with AddLine; Command writes them under `=== #<n>` in
// results.log and hands the bridge a one-line summary (the bridge cuts a
// summary at 400 characters, which is why the report lives in a file).
//
// Three things outlive an op and are driven from MissionServer.OnUpdate:
//   handles  names for items, given by dress / find / create, used by later ops;
//   watches  an item's place, written to results.log every time it changes for
//            three seconds after the op -- this is how "the same call", "the
//            next frame" and "some frames later" are told apart;
//   later    one creation or deletion queued for a named server frame.

class OZT_ProbeWatch
{
    EntityAI m_Item;        // weak: reads null once the engine destroyed the entity
    string   m_Label;
    string   m_Last;
    float    m_StartedAt;
    float    m_CheckedAt;
    int      m_StartFrame;
    int      m_Seq;
}

class OZT_ProbeLater
{
    int      m_DueFrame;
    int      m_Seq;
    string   m_Kind;        // "create" or "delete"
    string   m_Class;
    string   m_Handle;
    EntityAI m_Target;      // create: the holder; delete: the item
    int      m_LocType;
    int      m_Row;
    int      m_Col;
    int      m_Slot;
    bool     m_Flip;
}

class OZT_Probe
{
    static const float  WATCH_SECONDS = 3.0;
    static const string RESULTS = "results.log";

    protected static ref OZT_Probe s_Instance;

    protected ref map<string, EntityAI>      m_Handles;
    protected ref array<ref OZT_ProbeWatch>  m_Watches;
    protected ref array<ref OZT_ProbeLater>  m_Later;
    protected ref array<string>              m_Lines;
    protected int m_Seq;
    protected int m_Frame;

    static OZT_Probe Get()
    {
        if (!s_Instance)
            s_Instance = new OZT_Probe();
        return s_Instance;
    }

    void OZT_Probe()
    {
        m_Handles = new map<string, EntityAI>();
        m_Watches = new array<ref OZT_ProbeWatch>();
        m_Later = new array<ref OZT_ProbeLater>();
        m_Lines = new array<string>();
    }

    int FrameNo()
    {
        return m_Frame;
    }

    int SeqNo()
    {
        return m_Seq;
    }

    // ---- the bridge's entry point -------------------------------------------

    bool Command(string op, map<string, string> args, out string detail)
    {
        m_Seq++;
        m_Lines.Clear();
        bool ok = RunOp(op, args, detail);

        string head = "=== #" + m_Seq.ToString() + " " + op + " ok=" + OZT_ProbeLog.B(ok);
        head = head + " frame=" + m_Frame.ToString() + " t=" + OZT_ProbeLog.R1(GetGame().GetTickTime());
        head = head + " args:" + ArgsText(args);
        OZT_ProbeLog.Append(RESULTS, head);
        OZT_ProbeLog.Append(RESULTS, "    " + detail);
        for (int i = 0; i < m_Lines.Count(); i++)
            OZT_ProbeLog.Append(RESULTS, "    " + m_Lines.Get(i));

        // Nothing of the report goes to the script log: a class name or an
        // engine answer could carry a word the log verdict fails a boot on.
        Print("[OpenZone] trader probe #" + m_Seq.ToString() + " " + op + " finished");
        detail = detail + " [#" + m_Seq.ToString() + "]";
        return ok;
    }

    protected bool RunOp(string op, map<string, string> args, out string detail)
    {
        if (op == "status")
            return OpStatus(detail);

        PlayerBase player = FirstPlayer();
        if (!player)
        {
            detail = "nobody is connected: start a client first";
            return false;
        }

        if (op == "strip")
            return OpStrip(player, detail);
        if (op == "dress")
            return OpDress(player, detail);
        if (op == "dump")
            return OpDump(player, args, detail);
        if (op == "find")
            return OpFind(player, args, detail);
        if (op == "where")
            return OpWhere(player, args, detail);
        if (op == "weapon")
            return OpWeapon(player, args, detail);
        // -- routes of the later tasks go above this line --
        detail = "unknown op '" + op + "'; known: " + KnownOps();
        return false;
    }

    protected string KnownOps()
    {
        return "status, strip, dress, dump, find, where, weapon";
    }

    protected bool OpStatus(out string detail)
    {
        array<Man> men = new array<Man>();
        GetGame().GetPlayers(men);
        detail = "trader probe: players=" + men.Count().ToString() + " frame=" + m_Frame.ToString();
        detail = detail + " handles=" + m_Handles.Count().ToString() + " watches=" + m_Watches.Count().ToString();
        return true;
    }

    // ---- the scene ----------------------------------------------------------------

    // Everything the character wears or holds is deleted; children go with
    // their parents. `dress` is a separate command on purpose: whether a
    // creation may share a call with a deletion is one of the questions.
    protected bool OpStrip(PlayerBase player, out string detail)
    {
        array<EntityAI> tops = new array<EntityAI>();
        int n = player.GetInventory().AttachmentCount();
        for (int i = 0; i < n; i++)
            tops.Insert(player.GetInventory().GetAttachmentFromIndex(i));
        EntityAI held = player.GetHumanInventory().GetEntityInHands();
        if (held)
            tops.Insert(held);
        for (int k = 0; k < tops.Count(); k++)
        {
            if (tops.Get(k))
                GetGame().ObjectDelete(tops.Get(k));
        }
        ForgetAll();
        detail = "deleted " + tops.Count().ToString() + " top-level item(s); send dress as the next command";
        return true;
    }

    protected EntityAI DressOn(EntityAI parent, string type, string handle)
    {
        EntityAI e = parent.GetInventory().CreateAttachment(type);
        if (!e)
            AddLine("dress: NOT created as an attachment: " + type + " on " + parent.GetType());
        RememberAs(handle, e);
        return e;
    }

    protected EntityAI DressInto(EntityAI parent, string type, string handle)
    {
        EntityAI e = parent.GetInventory().CreateEntityInCargo(type);
        if (!e)
            AddLine("dress: NOT created in cargo: " + type + " in " + parent.GetType());
        RememberAs(handle, e);
        return e;
    }

    // A magazine the way vanilla loads a spawned weapon: through the weapon, so
    // that its state machine knows about it.
    protected EntityAI DressLoad(EntityAI weapon, string magType, string handle)
    {
        EntityAI mag;
        Weapon_Base wpn = Weapon_Base.Cast(weapon);
        if (wpn)
            mag = wpn.SpawnAttachedMagazine(magType, WeaponWithAmmoFlags.NONE);
        if (!mag)
            AddLine("dress: NOT loaded: " + magType + " into " + weapon.GetType());
        RememberAs(handle, mag);
        return mag;
    }

    // The fixed kit every measurement starts from:
    //   worn: jacket, trousers, boots, a plate carrier with pouches and a holster
    //         (a pistol with its magazine in it), a backpack, a rifle on the
    //         shoulder with magazine, scope, suppressor, buttstock and handguard;
    //   in the backpack: a second pistol with magazine and suppressor, an empty
    //         protector case, a wallet, nails;
    //   in the jacket: a wallet, a bandage.
    protected bool OpDress(PlayerBase player, out string detail)
    {
        EntityAI jacket = DressOn(player, "M65Jacket_Black", "jacket");
        DressOn(player, "CargoPants_Black", "pants");
        DressOn(player, "MilitaryBoots_Black", "boots");
        EntityAI vest = DressOn(player, "PlateCarrierVest", "vest");
        EntityAI bag = DressOn(player, "TaloonBag_Blue", "bag");
        EntityAI rifle = DressOn(player, "AKM", "rifle");
        if (vest)
        {
            DressOn(vest, "PlateCarrierPouches", "pouches");
            EntityAI holster = DressOn(vest, "PlateCarrierHolster", "holster");
            if (holster)
            {
                EntityAI pistol = DressOn(holster, "MakarovIJ70", "pistol");
                if (pistol)
                    DressLoad(pistol, "Mag_IJ70_8Rnd", "pistol_mag");
            }
        }
        if (rifle)
        {
            DressLoad(rifle, "Mag_AKM_30Rnd", "rifle_mag");
            DressOn(rifle, "PSO1Optic", "rifle_optic");
            DressOn(rifle, "AK_Suppressor", "rifle_muzzle");
            DressOn(rifle, "AK_WoodBttstck", "rifle_stock");
            DressOn(rifle, "AK_WoodHndgrd", "rifle_guard");
        }
        if (bag)
        {
            EntityAI bagPistol = DressInto(bag, "FNX45", "bagpistol");
            if (bagPistol)
            {
                DressLoad(bagPistol, "Mag_FNX45_15Rnd", "bagpistol_mag");
                DressOn(bagPistol, "PistolSuppressor", "bagpistol_muzzle");
            }
            DressInto(bag, "SmallProtectorCase", "case");
            DressInto(bag, "OZT_ProbeWallet", "wallet_bag");
            DressInto(bag, "Nail", "nails_bag");
        }
        if (jacket)
        {
            DressInto(jacket, "OZT_ProbeWallet", "wallet_jacket");
            DressInto(jacket, "BandageDressing", "bandage");
        }
        detail = "dressed: " + m_Handles.Count().ToString() + " of 23 handles; a missing one has a 'dress: NOT' line in results.log";
        return true;
    }

    // ---- reading ops -----------------------------------------------------------------

    // The character's inventory into server_dump.txt, and for every cargo it
    // carries: the grid, whether its children can be reached, and whether the
    // map built from item locations agrees with the engine cell by cell. The
    // engine is asked with a local probe entity and the occupancy test alone.
    protected bool OpDump(PlayerBase player, map<string, string> args, out string detail)
    {
        array<string> lines = new array<string>();
        OZT_ProbeInv.Tree(player, lines);
        OZT_ProbeLog.WriteAll("server_dump.txt", lines);

        string probeType = ArgOf(args, "probe", "Paper");
        EntityAI probe = EntityAI.Cast(GetGame().CreateObjectEx(probeType, player.GetPosition(), ECE_LOCAL));
        int pw;
        int ph;
        OZT_ProbeInv.SizeOf(probe, pw, ph);
        AddLine("probe entity " + probeType + " " + pw.ToString() + "x" + ph.ToString() + " made=" + OZT_ProbeLog.B(probe != null));

        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        int cargos = 0;
        int cells = 0;
        int wrong = 0;
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI holder = all.Get(i);
            if (!holder || holder == player)
                continue;
            array<bool> taken;
            int gw;
            int gh;
            if (!OZT_ProbeInv.Map(holder, null, taken, gw, gh))
                continue;
            cargos++;
            int bad = 0;
            if (probe)
            {
                for (int r = 0; r < gh; r++)
                {
                    for (int c = 0; c < gw; c++)
                    {
                        bool modelFree = OZT_ProbeInv.RectFree(taken, gw, gh, r, c, pw, ph);
                        bool engineFree = holder.GetInventory().TestAddEntityInCargoEx(probe, 0, r, c, false, false, false, false, true, false, false);
                        cells++;
                        if (modelFree == engineFree)
                            continue;
                        bad++;
                        if (bad <= 5)
                            AddLine("  MISMATCH r" + r.ToString() + " c" + c.ToString() + " model free=" + OZT_ProbeLog.B(modelFree) + " engine free=" + OZT_ProbeLog.B(engineFree));
                    }
                }
            }
            wrong = wrong + bad;
            string row = "cargo " + OZT_ProbeInv.Name(holder) + " " + gw.ToString() + "x" + gh.ToString();
            row = row + " items=" + holder.GetInventory().GetCargo().GetItemCount().ToString();
            row = row + " children accessible=" + OZT_ProbeLog.B(holder.GetInventory().AreChildrenAccessible());
            row = row + " mismatches=" + bad.ToString() + " at " + OZT_ProbeInv.Where(holder);
            AddLine(row);
        }
        if (probe)
            GetGame().ObjectDelete(probe);

        detail = "dump: " + lines.Count().ToString() + " items -> server_dump.txt; cargos=" + cargos.ToString();
        detail = detail + " cells asked=" + cells.ToString() + " model-vs-engine mismatches=" + wrong.ToString();
        return true;
    }

    protected bool OpFind(PlayerBase player, map<string, string> args, out string detail)
    {
        string type = ArgOf(args, "class", "");
        int nth = ArgOf(args, "nth", "0").ToInt();
        string handle = ArgOf(args, "as", "");
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        int seen = 0;
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI e = all.Get(i);
            if (!e || e == player || e.GetType() != type)
                continue;
            if (seen == nth)
            {
                RememberAs(handle, e);
                detail = "found " + OZT_ProbeInv.Line(e) + " as '" + handle + "'";
                return true;
            }
            seen++;
        }
        detail = "no item number " + nth.ToString() + " of class " + type + " on the character";
        return false;
    }

    protected bool OpWhere(PlayerBase player, map<string, string> args, out string detail)
    {
        string name = ArgOf(args, "item", "");
        detail = name + ": " + OZT_ProbeInv.Line(ItemOf(player, name));
        return true;
    }

    // A weapon as its own state machine sees it: the magazine it believes it
    // has, whether it is idle, and what hangs on it.
    protected bool OpWeapon(PlayerBase player, map<string, string> args, out string detail)
    {
        Weapon_Base wpn = Weapon_Base.Cast(ItemOf(player, ArgOf(args, "item", "")));
        if (!wpn)
        {
            detail = "weapon needs item=<handle of a weapon>";
            return false;
        }
        Magazine mag = wpn.GetMagazine(wpn.GetCurrentMuzzle());
        int n = wpn.GetInventory().AttachmentCount();
        detail = wpn.GetType() + " at " + OZT_ProbeInv.Where(wpn) + " | GetMagazine=" + OZT_ProbeInv.Name(mag);
        detail = detail + " | IsIdle=" + OZT_ProbeLog.B(wpn.IsIdle()) + " | attachments=" + n.ToString();
        for (int i = 0; i < n; i++)
            AddLine("  " + OZT_ProbeInv.Line(wpn.GetInventory().GetAttachmentFromIndex(i)));
        return true;
    }

    // ---- what the ops share ---------------------------------------------------

    void AddLine(string line)
    {
        m_Lines.Insert(line);
    }

    string ArgOf(map<string, string> args, string key, string fallback)
    {
        if (!args)
            return fallback;
        string v;
        if (args.Find(key, v))
            return v;
        return fallback;
    }

    protected string ArgsText(map<string, string> args)
    {
        string s = "";
        if (!args)
            return s;
        for (int i = 0; i < args.Count(); i++)
            s = s + " " + args.GetKey(i) + "=" + args.GetElement(i);
        return s;
    }

    PlayerBase FirstPlayer()
    {
        array<Man> men = new array<Man>();
        GetGame().GetPlayers(men);
        if (men.Count() == 0)
            return null;
        return PlayerBase.Cast(men.Get(0));
    }

    // "player" is the character itself; any other name is a handle.
    EntityAI ItemOf(PlayerBase player, string name)
    {
        if (name == "player")
            return player;
        EntityAI e;
        if (m_Handles.Find(name, e))
            return e;
        return null;
    }

    void RememberAs(string handle, EntityAI e)
    {
        if (handle == "" || !e)
            return;
        m_Handles.Set(handle, e);
    }

    protected void ForgetAll()
    {
        m_Handles.Clear();
    }

    void WatchItem(EntityAI e, string label)
    {
        if (!e)
            return;
        OZT_ProbeWatch w = new OZT_ProbeWatch();
        w.m_Item = e;
        w.m_Label = label;
        w.m_Last = OZT_ProbeInv.Where(e);
        w.m_StartedAt = GetGame().GetTickTime();
        w.m_CheckedAt = w.m_StartedAt;
        w.m_StartFrame = m_Frame;
        w.m_Seq = m_Seq;
        m_Watches.Insert(w);
        AddLine("watching " + label + " from " + w.m_Last);
    }

    void QueueLater(OZT_ProbeLater job)
    {
        job.m_Seq = m_Seq;
        m_Later.Insert(job);
    }

    // The cell an op means:
    //   free       the first place the occupancy map calls free for w x h
    //   taken      the top-left cell of the first item lying in that cargo
    //   last       the bottom-right cell of the grid
    //   <row>,<col> as given
    bool PickCell(EntityAI holder, string cellArg, int w, int h, EntityAI skip, out int row, out int col)
    {
        row = -1;
        col = -1;
        array<bool> taken;
        int gw;
        int gh;
        if (!OZT_ProbeInv.Map(holder, skip, taken, gw, gh))
            return false;
        if (cellArg == "free")
        {
            for (int r = 0; r < gh; r++)
            {
                for (int c = 0; c < gw; c++)
                {
                    if (OZT_ProbeInv.RectFree(taken, gw, gh, r, c, w, h))
                    {
                        row = r;
                        col = c;
                        return true;
                    }
                }
            }
            return false;
        }
        if (cellArg == "taken")
        {
            CargoBase cargo = holder.GetInventory().GetCargo();
            if (cargo.GetItemCount() == 0)
                return false;
            InventoryLocation il = new InventoryLocation();
            cargo.GetItem(0).GetInventory().GetCurrentInventoryLocation(il);
            row = il.GetRow();
            col = il.GetCol();
            return true;
        }
        if (cellArg == "last")
        {
            row = gh - 1;
            col = gw - 1;
            return true;
        }
        array<string> parts = new array<string>();
        cellArg.Split(",", parts);
        if (parts.Count() != 2)
            return false;
        row = parts.Get(0).ToInt();
        col = parts.Get(1).ToInt();
        return true;
    }

    // One creation at a named place, the way a deal would make a bought item.
    EntityAI MakeAt(PlayerBase player, int locType, EntityAI holder, string type, int row, int col, bool flip, int slot)
    {
        InventoryLocation loc = new InventoryLocation();
        if (locType == InventoryLocationType.HANDS)
        {
            if (!player)
                return null;
            loc.SetHands(player, null);
        }
        else
        {
            if (!holder)
                return null;
            if (locType == InventoryLocationType.ATTACHMENT)
                loc.SetAttachment(holder, null, slot);
            else
                loc.SetCargo(holder, null, 0, row, col, flip);
        }
        return GameInventory.LocationCreateEntity(loc, type, ECE_IN_INVENTORY, RF_DEFAULT);
    }

    // ---- every server frame -----------------------------------------------------

    void OnFrame()
    {
        m_Frame++;
        RunLater();
        RunWatches();
    }

    protected void RunLater()
    {
        for (int i = m_Later.Count() - 1; i >= 0; i--)
        {
            OZT_ProbeLater job = m_Later.Get(i);
            if (m_Frame < job.m_DueFrame)
                continue;
            m_Later.Remove(i);
            string line = "    later #" + job.m_Seq.ToString() + " frame=" + m_Frame.ToString() + " " + job.m_Kind + ": ";
            if (job.m_Kind == "delete")
            {
                line = line + OZT_ProbeInv.Name(job.m_Target);
                if (job.m_Target)
                    GetGame().ObjectDelete(job.m_Target);
            }
            else
            {
                EntityAI made = MakeAt(FirstPlayer(), job.m_LocType, job.m_Target, job.m_Class, job.m_Row, job.m_Col, job.m_Flip, job.m_Slot);
                line = line + job.m_Class + " -> " + OZT_ProbeInv.Where(made);
                if (made)
                {
                    RememberAs(job.m_Handle, made);
                    OZT_ProbeWatch w = new OZT_ProbeWatch();
                    w.m_Item = made;
                    w.m_Label = "made later " + job.m_Class;
                    w.m_Last = OZT_ProbeInv.Where(made);
                    w.m_StartedAt = GetGame().GetTickTime();
                    w.m_CheckedAt = w.m_StartedAt;
                    w.m_StartFrame = m_Frame;
                    w.m_Seq = job.m_Seq;
                    m_Watches.Insert(w);
                }
            }
            OZT_ProbeLog.Append(RESULTS, line);
        }
    }

    // An idle dedicated server runs thousands of frames a second. The first
    // fifty frames after an op are all looked at (that is where "same call" and
    // "next frame" are decided); after that ten looks a second are enough.
    protected void RunWatches()
    {
        if (m_Watches.Count() == 0)
            return;
        float now = GetGame().GetTickTime();
        for (int i = m_Watches.Count() - 1; i >= 0; i--)
        {
            OZT_ProbeWatch w = m_Watches.Get(i);
            int frames = m_Frame - w.m_StartFrame;
            float age = now - w.m_StartedAt;
            bool closing = age >= WATCH_SECONDS;
            if (!closing && frames > 50 && now - w.m_CheckedAt < 0.1)
                continue;
            w.m_CheckedAt = now;
            string spot = OZT_ProbeInv.Where(w.m_Item);
            if (w.m_Item && w.m_Item.IsPendingDeletion())
                spot = spot + " (pending deletion)";
            if (spot != w.m_Last || closing)
            {
                string tag = "changed";
                if (closing)
                    tag = "ends";
                string line = "    watch #" + w.m_Seq.ToString() + " " + w.m_Label + " " + tag;
                line = line + " +" + OZT_ProbeLog.R1(age * 1000) + " ms, " + frames.ToString() + " frames: " + spot;
                OZT_ProbeLog.Append(RESULTS, line);
                w.m_Last = spot;
            }
            if (closing || spot == "GONE")
                m_Watches.Remove(i);
        }
    }
}
