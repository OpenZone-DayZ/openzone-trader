// Mission hooks of the trader probe. No `extends` on a modded class: it would
// silently leave the chain.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        OZT_ProbeLog.EnsureDir();
        Print("[OpenZone] trader probe loaded; reports in $profile:OpenZone_TraderProbe/results.log");
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        OZT_Probe.Get().OnFrame();
    }
}

modded class MissionGameplay
{
    protected ref OZT_ProbeClient m_OZT_ProbeClient;

    override void OnInit()
    {
        super.OnInit();
        m_OZT_ProbeClient = new OZT_ProbeClient();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (m_OZT_ProbeClient)
            m_OZT_ProbeClient.OnFrame(timeslice);
    }

    // The probe's own screen, made for its id; everybody else's menus first.
    override UIScriptedMenu CreateScriptedMenu(int id)
    {
        UIScriptedMenu menu = super.CreateScriptedMenu(id);
        if (!menu && id == OZT_ProbeMenu.ID)
        {
            menu = new OZT_ProbeMenu();
            menu.SetID(id);
        }
        return menu;
    }
}
