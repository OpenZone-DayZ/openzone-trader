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

        // -- routes of the later tasks go above this line --
        detail = "unknown op '" + op + "'; known: " + KnownOps();
        return false;
    }

    protected string KnownOps()
    {
        return "status";
    }

    protected bool OpStatus(out string detail)
    {
        array<Man> men = new array<Man>();
        GetGame().GetPlayers(men);
        detail = "trader probe: players=" + men.Count().ToString() + " frame=" + m_Frame.ToString();
        detail = detail + " handles=" + m_Handles.Count().ToString() + " watches=" + m_Watches.Count().ToString();
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
