// The server half of the trader: the order of its start, and the ready line.
//
// The order matters: the files are read before anything that reads them, and
// the services stand before the first client arrives. The ready line is
// printed LAST and in the form `key=value`: the stand's verdict reads its
// counters from it.
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

        // The profile's folders first, before any read or write. The core
        // builds them in its own OnMissionStart, but the order of CF modules is
        // not promised. Idempotent.
        OZ_Json.EnsureTree();

        OZT_Config.ServerLoad();

        // -- later tasks start their parts above this line --

        // The ready line one tick later: a line printed here may speak before
        // a neighbour module has registered what it describes.
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

        // -- later tasks stop their parts above this line --
    }

    static string ReadyLine()
    {
        return "trader loaded: " + OZT_Config.Get().Counters();
    }
}
