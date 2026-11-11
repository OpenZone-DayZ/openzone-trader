// The client's entry of the trader. The server says "show" for a trader
// (OZ_Rpc.Show with OZT_Const.SHOW_PREFIX and the trader's id); the trade screen
// opens a tick later and asks the service for that trader; the answers of the
// service go to the open screen, and so do the keys.
//
// A dedicated server makes no MissionGameplay, so none of this runs there.
modded class MissionGameplay
{
    protected string m_OZT_Trader;

    override void OnInit()
    {
        super.OnInit();
        OZ_Show.OnShow.Insert(OZT_Show);
        OZ_ClientState.ServiceWatch().Insert(OZT_SvcRes);
    }

    void OZT_Show(string what)
    {
        if (what.IndexOf(OZT_Const.SHOW_PREFIX) != 0)
            return;
        string prefix = OZT_Const.SHOW_PREFIX;
        int skip = prefix.Length();
        m_OZT_Trader = what.Substring(skip, what.Length() - skip);
        // Not in this frame: at the moment of an action the menu stack is
        // still busy (the way the PDA and the research tree open).
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(OZT_OpenTrade, 100, false);
    }

    void OZT_OpenTrade()
    {
        UIManager ui = GetGame().GetUIManager();
        if (ui.FindMenu(OZT_Const.MENU_TRADE))
            return;
        // Never over a player who is not there to trade (a menu open at the
        // moment of a death takes the client down).
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.IsAlive() || player.IsUnconscious())
            return;
        OZT_TradeMenu.For(m_OZT_Trader);
        ui.EnterScriptedMenu(OZT_Const.MENU_TRADE, null);
    }

    void OZT_SvcRes(string serviceId, string op, bool ok, string json, string error)
    {
        OZT_TradeMenu.OnService(serviceId, op, ok, json, error);
    }

    // A scripted menu's own key event goes only to the widget that holds the
    // keyboard; the mission hears every press (measured in the series' menus).
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);
        OZT_TradeMenu menu = OZT_TradeMenu.Current();
        if (menu)
            menu.KeyPressed(key);
    }

    override UIScriptedMenu CreateScriptedMenu(int id)
    {
        // super first, and out at once when it made something: that is how
        // other mods that touch this class live beside us.
        UIScriptedMenu menu = super.CreateScriptedMenu(id);
        if (menu)
            return menu;
        if (id == OZT_Const.MENU_TRADE)
        {
            menu = new OZT_TradeMenu();
            menu.SetID(id);
            return menu;
        }
        return null;
    }

    override void OnMissionFinish()
    {
        // The mirror of OnInit: the core's invokers are static and outlive
        // the mission.
        OZ_Show.OnShow.Remove(OZT_Show);
        OZ_ClientState.ServiceWatch().Remove(OZT_SvcRes);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(OZT_OpenTrade);
        super.OnMissionFinish();
    }
}
