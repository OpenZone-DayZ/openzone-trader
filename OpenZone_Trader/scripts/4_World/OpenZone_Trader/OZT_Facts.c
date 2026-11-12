// The trader's card on Ядро › Огляд (the core's OZ_AdminFacts, VPP rework
// 2026-11-12, design §4.3): points, goods lines and the money it takes; the
// problems of the last load (its three files and the cross-checks: OZT_Log counts
// the warnings since the load began, the same lines the window's `problems`
// shows). "Reload every module" is the live reload the admin's own respawn runs:
// the files read again, the price book rebuilt, the points made again.
class OZT_Facts : OZ_AdminFactsProvider
{
    override void Fill(OZ_AdminModuleCard card)
    {
        card.Id = "trader";
        card.Name = "#STR_OZT_VPP_MOD";

        // Pages in the window come with the VPP glue of this repo.
        card.Pages = 0;
#ifdef OpenZone_Trader_VPP
        card.Pages = 1;
#endif

        card.Problems = OZT_Log.Warned();
        OZT_Config cfg = OZT_Config.Get();
        card.Add("#STR_OZT_FACT_POINTS", cfg.TraderCount().ToString());
        card.Add("#STR_OZT_FACT_GOODS", cfg.GoodsCount().ToString());
        card.Add("#STR_OZT_FACT_MONEY", cfg.Settings().Provider);
    }

    override bool Reload(array<string> said)
    {
        OZT_Config.ReloadLive();
        said.Insert("#STR_OZT_FACT_RESPAWNED");
        return true;
    }
}
