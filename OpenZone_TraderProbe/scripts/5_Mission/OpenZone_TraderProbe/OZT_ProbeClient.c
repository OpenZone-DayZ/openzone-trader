// Stand only, client side. The stand writes
// $profile:OpenZone_TraderProbe/control.txt in the CLIENT's profile; the client
// reads its first line once a second, and a line it has not seen is a command:
//
//   dump <k>   the local player's inventory into client_dump.txt, in the format
//              of the server's server_dump.txt, and the command itself into
//              client_dump.ack so the reader knows which dump it holds.
class OZT_ProbeClient
{
    protected float  m_Timer;
    protected string m_Seen;

    void OZT_ProbeClient()
    {
        OZT_ProbeLog.EnsureDir();
    }

    void OnFrame(float timeslice)
    {
        m_Timer = m_Timer + timeslice;
        if (m_Timer < 1.0)
            return;
        m_Timer = 0;
        string path = OZT_ProbeLog.DIR + "/control.txt";
        if (!FileExist(path))
            return;
        FileHandle fh = OpenFile(path, FileMode.READ);
        if (fh == 0)
            return;
        string line;
        FGets(fh, line);
        CloseFile(fh);
        line = line.Trim();
        if (line == "" || line == m_Seen)
            return;
        m_Seen = line;
        if (line.IndexOf("dump") == 0)
            DumpInventory(line);
    }

    protected void DumpInventory(string command)
    {
        array<string> lines = new array<string>();
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
            OZT_ProbeInv.Tree(player, lines);
        OZT_ProbeLog.WriteAll("client_dump.txt", lines);
        array<string> ack = new array<string>();
        ack.Insert(command + " items=" + lines.Count().ToString());
        OZT_ProbeLog.WriteAll("client_dump.ack", ack);
    }
}
