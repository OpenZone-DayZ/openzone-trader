// The stand's ops for phase 4: what a trader's character costs and whether it
// can be hurt (design §12 item 10).
//
//   {"op": "fps", "seconds": "5"}   frames of the server over that time; the
//                                    answer is a log line when it is done
//   {"op": "hurt", "trader": "npc"} a rifle bullet to the trader's torso and to
//                                    the nearest other survivor, health before
//                                    and after each

class OZT_FpsMeter
{
    protected int m_Frames;
    protected float m_Seconds;
    protected float m_Left;
    protected bool m_On;

    void Start(float seconds)
    {
        m_Frames = 0;
        m_Seconds = seconds;
        m_Left = seconds;
        if (m_On)
            return;
        m_On = true;
        GetGame().GetUpdateQueue(CALL_CATEGORY_SYSTEM).Insert(this.OnUpdate);
    }

    void OnUpdate(float timeslice)
    {
        m_Frames++;
        m_Left = m_Left - timeslice;
        if (m_Left > 0)
            return;
        GetGame().GetUpdateQueue(CALL_CATEGORY_SYSTEM).Remove(this.OnUpdate);
        m_On = false;
        float fps = m_Frames / m_Seconds;
        float ms = 1000.0 * m_Seconds / m_Frames;
        string line = "fps: " + m_Frames.ToString() + " frames in " + m_Seconds.ToString() + " s = " + fps.ToString();
        line = line + " fps, " + ms.ToString() + " ms a frame, " + OZT_Points.Standing().ToString() + " point(s) standing";
        OZT_Log.Info(line);
    }
}

class OZT_VerbNpc
{
    protected static ref OZT_FpsMeter s_Meter;

    static bool OpFps(map<string, string> args, out string detail)
    {
        float seconds = OZT_VerbOps.Arg(args, "seconds", "5").ToFloat();
        if (seconds < 1)
            seconds = 1;
        if (!s_Meter)
            s_Meter = new OZT_FpsMeter();
        s_Meter.Start(seconds);
        detail = "fps: counting frames for " + seconds.ToString() + " s; the answer is a log line";
        return true;
    }

    static bool OpHurt(PlayerBase player, map<string, string> args, out string detail)
    {
        string id = OZT_VerbOps.Arg(args, "trader", "npc");
        EntityAI trader = EntityAI.Cast(OZT_Points.BodyOf(id));
        if (!trader)
        {
            detail = "hurt: the point '" + id + "' does not stand";
            return false;
        }
        detail = "hurt: trader " + Shot(trader);

        // The control: the nearest other survivor near the trader that is no
        // player and no trader.
        array<Object> near = new array<Object>();
        GetGame().GetObjectsAtPosition(trader.GetPosition(), 10, near, null);
        PlayerBase control = null;
        float best = -1;
        for (int i = 0; i < near.Count(); i++)
        {
            PlayerBase man = PlayerBase.Cast(near[i]);
            if (!man || man == trader || man.GetIdentity() || man.OZT_IsTrader())
                continue;
            float away = vector.Distance(man.GetPosition(), trader.GetPosition());
            if (best < 0 || away < best)
            {
                control = man;
                best = away;
            }
        }
        if (control)
            detail = detail + "; control " + Shot(control);
        else
            detail = detail + "; no control survivor within 10 m";
        return true;
    }

    // The admin section as the pane calls it, the stand's player as the admin
    // (the core's dispatcher, which checks the rights, is not in the way
    // here): {"op": "admin", "aop": "move:npc", "body": "{...}"}. The full
    // answer goes to OZT_Const.LAST_ANSWER.
    static bool OpAdmin(PlayerBase player, map<string, string> args, out string detail)
    {
        string aop = OZT_VerbOps.Arg(args, "aop", "list");
        string body = OZT_VerbOps.Arg(args, "body", "{}");
        // Another mod's section by its id (section=research, say), as the
        // core's dispatcher would hand it the request.
        string id = OZT_VerbOps.Arg(args, "section", OZT_Const.SECTION_ADMIN);
        OZ_AdminSection section = OZ_AdminRegistry.Get(id);
        if (!section)
        {
            detail = "admin: no section '" + id + "'";
            return false;
        }
        bool ok;
        string error;
        string answer = section.Handle(aop, body, player.GetIdentity(), ok, error);
        FileHandle f = OpenFile(OZT_Const.LAST_ANSWER, FileMode.WRITE);
        if (f)
        {
            FPrintln(f, answer);
            CloseFile(f);
        }
        detail = "admin " + aop + ": ok=" + ok.ToString() + " error=" + error;
        detail = detail + " " + answer.Length().ToString() + " bytes";
        return true;
    }

    // One bullet to the torso; the health before and after.
    protected static string Shot(EntityAI e)
    {
        float before = e.GetHealth("", "");
        e.ProcessDirectDamage(DamageType.FIRE_ARM, null, "Torso", "Bullet_762x39", "0 0 0", 1.0);
        float after = e.GetHealth("", "");
        return e.GetType() + " " + before.ToString() + " -> " + after.ToString();
    }
}
