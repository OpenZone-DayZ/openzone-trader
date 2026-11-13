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

        // How long the start takes with a big book: the files, the book, the
        // shelves and the points, said in the debug log.
        int t0 = TickCount(0);
        OZT_Config.ServerLoad();
        int tFiles = TickCount(t0);

        OZT_Book.Rebuild();
        int tBook = TickCount(t0);
        // The stock of every shelf, restocked and destocked by the settings
        // (design §11.1).
        OZT_Stock.Start();
        int tStock = TickCount(t0);
        OZT_Money.Provide(new OZT_ItemMoney());
        // The identity kinds are this mod's own: they need the core alone.
        OZT_Gates.Provide(new OZT_FactionGate());
        // Licences are this mod's own too (design §11.2).
        OZT_Gates.Provide(new OZT_LicenceGate());

        // The points ride in the core's sync packet: the handler first, then
        // the points, whose respawn sends the packet again.
        OZ_SyncExtras.OnFill().Insert(OZT_PointsFill);
        OZT_Points.Respawn();
        int tPoints = TickCount(t0);
        if (OZ_Log.IsDebug())
        {
            string said = "start: files " + OZT_Open.Ms(tFiles) + " ms, book " + OZT_Open.Ms(tBook - tFiles);
            said = said + " ms, stock " + OZT_Open.Ms(tStock - tBook) + " ms, points " + OZT_Open.Ms(tPoints - tStock) + " ms";
            OZT_Log.Dbg(said);
        }
        OZ_ServiceRegistry.Register(OZT_Const.SERVICE, new OZT_Service());

        // The admin's side: the pane's section and the three files in the
        // core's editor. The core empties its registries at mission end.
        OZ_AdminRegistry.Register(OZT_Const.SECTION_ADMIN, new OZT_Admin());
        // The trader's card on the core's overview page (VPP rework 2026-11-12).
        OZ_AdminFacts.Register(new OZT_Facts());
        OZ_AdminCfg.Register(OZT_Const.CFG_SETTINGS, OZT_Const.SETTINGS, new OZT_SettingsApplier(), OZT_Const.CFG_OWNER);
        OZ_AdminCfg.Register(OZT_Const.CFG_GOODS, OZT_Const.GOODS, new OZT_GoodsApplier(), OZT_Const.CFG_OWNER);
        OZ_AdminCfg.Register(OZT_Const.CFG_TRADERS, OZT_Const.TRADERS, new OZT_TradersApplier(), OZT_Const.CFG_OWNER);

        // -- later tasks start their parts above this line --

        // The ready line one tick later: a line printed here may speak before
        // a neighbour module has registered what it describes.
        m_ReadyTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_ReadyTimer.Run(READY_DELAY, this, "Ready", NULL, false);
    }

    // The core's sync packet asks for our part: the trader points. Not
    // private: the invoker calls it.
    void OZT_PointsFill(OZ_SyncPayload p)
    {
        OZT_Points.Fill(p);
    }

    // Called by the timer by name: it must not be private.
    void Ready()
    {
        // Every glue has had its start by now: say which named providers
        // nobody brought, then the line the stand waits for.
        OZT_Money.CheckNamed();
        OZT_Gates.CheckNamed();
        OZT_Log.Info(ReadyLine());
    }

    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        super.OnMissionFinish(sender, args);

        if (m_ReadyTimer)
            m_ReadyTimer.Stop();

        OZT_Money.Reset();
        OZT_Gates.Reset();
        OZT_Book.Reset();
        OZT_Stock.Reset();
        OZT_Licences.Reset();
        OZT_Fit.Reset();
        OZT_OpenStreams.Reset();
        OZT_ShelfCaches.Reset();

        // The core empties its own registries at mission end; the objects at
        // the points are ours to take away, and so is our handler of its packet.
        OZ_SyncExtras.OnFill().Remove(OZT_PointsFill);
        OZT_Points.Remove();

        OZT_Deal.Reset();

        // -- later tasks stop their parts above this line --
    }

    static string ReadyLine()
    {
        return "trader loaded: " + OZT_Config.Get().Counters();
    }
}
