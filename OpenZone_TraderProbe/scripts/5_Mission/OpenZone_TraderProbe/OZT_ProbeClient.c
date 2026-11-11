// Stand only, client side. The stand writes
// $profile:OpenZone_TraderProbe/control.txt in the CLIENT's profile; the client
// reads its first line once a second, and a line it has not seen is a command:
//
//   dump <k>            the local player's inventory into client_dump.txt, in the
//                       format of the server's server_dump.txt, and the command
//                       itself into client_dump.ack so the reader knows which
//                       dump it holds.
//   ui <words> #<k>     a command for the probe's own screen (OZT_ProbeMenu):
//                       `ui open` puts it up, every other one is the menu's.
//                       The answer goes to client_ui.ack, one line that begins
//                       with the command, and into client_results.log.
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
        else if (line.IndexOf("ui ") == 0)
            TakeUi(line);
    }

    protected void DumpInventory(string command)
    {
        array<string> lines = new array<string>();
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
            OZT_ProbeInv.Tree(player, lines);
        OZT_ProbeLog.WriteAll("client_dump.txt", lines);
        array<string> ack = new array<string>();
        // The hands as the client's own state machine has them (1 is empty, 2
        // is holding): the server's side of this is `ozt_probe hands`.
        string held = "none";
        string state = "";
        if (player)
        {
            EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
            if (inHands)
                held = inHands.GetType();
            state = player.GetDayZPlayerInventory().GetCurrentStateID().ToString();
        }
        ack.Insert(command + " items=" + lines.Count().ToString() + " hands=" + held + " state=" + state);
        OZT_ProbeLog.WriteAll("client_dump.ack", ack);
    }

    // ---- the probe's screen --------------------------------------------------------

    protected void TakeUi(string line)
    {
        array<string> words = new array<string>();
        line.Split(" ", words);
        if (words.Count() < 2)
        {
            Ack(line, "no command after `ui`");
            return;
        }
        if (words[1] == "open")
        {
            // A menu cannot be entered from anywhere: a tick later, on the GUI
            // queue, the way the series opens its screens.
            OZT_ProbeMenu.OpenedBy(line);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(OpenMenu, 100, false);
            return;
        }
        OZT_ProbeMenu menu = OZT_ProbeMenu.Current();
        if (!menu)
        {
            Ack(line, "the probe's screen is not open: send `ui open` first");
            return;
        }
        menu.Command(words, line);
    }

    protected void OpenMenu()
    {
        UIManager ui = GetGame().GetUIManager();
        if (ui.FindMenu(OZT_ProbeMenu.ID))
        {
            Ack(OZT_ProbeMenu.OpenLine(), "open: it is up already");
            return;
        }
        UIScriptedMenu made = ui.EnterScriptedMenu(OZT_ProbeMenu.ID, null);
        if (!made)
            Ack(OZT_ProbeMenu.OpenLine(), "open: EnterScriptedMenu gave nothing (another menu holds the screen?)");
    }

    // One answer: the whole of client_ui.ack, and a line of client_results.log.
    static void Ack(string line, string answer)
    {
        array<string> one = new array<string>();
        one.Insert(line + " -> " + answer);
        OZT_ProbeLog.WriteAll("client_ui.ack", one);
        OZT_ProbeLog.Append("client_results.log", line + " -> " + answer);
    }

    // A line of the report that is too long for an answer.
    static void Note(string text)
    {
        OZT_ProbeLog.Append("client_results.log", "    " + text);
    }
}
