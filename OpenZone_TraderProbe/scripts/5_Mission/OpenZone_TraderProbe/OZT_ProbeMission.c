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
