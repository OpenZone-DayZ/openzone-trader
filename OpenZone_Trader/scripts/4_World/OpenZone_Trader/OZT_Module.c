// The server half of the trader: the order of its start, and the ready line.
//
// The ready line is printed LAST and in the form `key=value`: the stand's
// verdict reads its counters from it.
[CF_RegisterModule(OZT_Module)]
class OZT_Module : CF_ModuleWorld
{
    private ref Timer m_ReadyTimer;
    private static const float READY_DELAY = 1.0;

    override void OnInit()
    {
        super.OnInit();

        // super first, then the subscriptions: otherwise CF has not registered
        // the module yet and the event arrives nowhere.
        EnableMissionStart();
        EnableMissionFinish();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        if (!GetGame().IsServer())
            return;

        // The ready line one tick later: the order of CF modules is not
        // promised, and a line printed here may speak before a neighbour has
        // registered what it describes (the series' own lesson).
        m_ReadyTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_ReadyTimer.Run(READY_DELAY, this, "Ready", NULL, false);
    }

    // Called by the timer by name: it must not be private.
    void Ready()
    {
        OZT_Log.Info(ReadyLine());
    }

    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        super.OnMissionFinish(sender, args);

        if (m_ReadyTimer)
            m_ReadyTimer.Stop();
    }

    static string ReadyLine()
    {
        return "trader loaded: traders=0 goods=0 money=none gates=none problems=0";
    }
}
