// The server's frame for the trader: the opens going out by pages
// (OZT_OpenStreams), the way the storage mod moves its streams. No `extends` on
// a modded class.
modded class MissionServer
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (GetGame() && GetGame().IsDedicatedServer())
            OZT_OpenStreams.TickAll(timeslice);
    }
}
