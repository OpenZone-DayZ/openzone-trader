// The probe's own screen on the client: a scripted menu with a region for each
// question of docs/2026-11-11-phase-2a-plan.md. The stand drives it with lines
// "ui <words> #<k>" in control.txt (OZT_ProbeClient): every answer goes to
// client_ui.ack, the full report to client_results.log.

// One question: its widgets, its commands, its work each frame, its cleanup.
class OZT_ProbeQuestion
{
    protected OZT_ProbeMenu m_Menu;
    protected Widget m_Root;

    void Attach(OZT_ProbeMenu menu, Widget root)
    {
        m_Menu = menu;
        m_Root = root;
        OnAttach();
    }

    void OnAttach()
    {
    }

    // The answer to a command this question knows; "" when the word is not its
    // own. `words` is the whole line: "ui", the command, its arguments, "#<k>".
    string Command(array<string> words)
    {
        return "";
    }

    void Frame(float dt)
    {
    }

    // Everything the question made is deleted here.
    void Stop()
    {
    }

    // The word at a place of the line, unless it is the "#<k>" at its end.
    protected string Word(array<string> words, int at, string fallback)
    {
        if (at >= words.Count())
            return fallback;
        string w = words[at];
        if (w.IndexOf("#") == 0)
            return fallback;
        return w;
    }

    protected static string NameOf(Widget w)
    {
        if (!w)
            return "null";
        return w.GetName();
    }

    protected static string V(vector v)
    {
        return OZT_ProbeLog.R1(v[0]) + " " + OZT_ProbeLog.R1(v[1]) + " " + OZT_ProbeLog.R1(v[2]);
    }
}

class OZT_ProbeMenu : UIScriptedMenu
{
    // Beside the series' own ids (131-135 are the PDA's and the research
    // tree's; 136 is kept for the trade screen). A large id makes
    // EnterScriptedMenu answer nothing.
    static const int ID = 137;
    static const string LAYOUT = "OpenZone_TraderProbe/gui/layouts/ozt_probe_menu.layout";

    protected static OZT_ProbeMenu s_Current;
    protected static string s_OpenLine;

    protected TextWidget m_Status;
    protected bool m_Claimed;
    protected ref array<ref OZT_ProbeQuestion> m_Questions;
    protected ref OZT_ProbeQDrag m_Drag;
    protected ref OZT_ProbeQPreviews m_Previews;

    // Frame times since the last `ui frames reset`, in milliseconds.
    protected int m_Frames;
    protected float m_FrameSum;
    protected float m_FrameMax;
    protected int m_FramesSlow;

    static OZT_ProbeMenu Current()
    {
        return s_Current;
    }

    static void OpenedBy(string line)
    {
        s_OpenLine = line;
    }

    static string OpenLine()
    {
        return s_OpenLine;
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
        if (!layoutRoot)
            return layoutRoot;
        m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));

        m_Questions = new array<ref OZT_ProbeQuestion>();
        m_Drag = new OZT_ProbeQDrag();
        m_Previews = new OZT_ProbeQPreviews();
        m_Questions.Insert(m_Drag);
        m_Questions.Insert(m_Previews);
        m_Questions.Insert(new OZT_ProbeQItems());
        m_Questions.Insert(new OZT_ProbeQSlots());
        m_Questions.Insert(new OZT_ProbeQTryOn());
        for (int i = 0; i < m_Questions.Count(); i++)
            m_Questions[i].Attach(this, layoutRoot);
        return layoutRoot;
    }

    override void OnShow()
    {
        // The base locks the controls and shows the cursor; it does not stop
        // the character from walking, the excludes do.
        super.OnShow();
        s_Current = this;
        Mission mission = GetGame().GetMission();
        if (mission)
        {
            mission.AddActiveInputExcludes({"menu"});
            m_Claimed = true;
        }
        string said = "open: the screen is up";
        if (!layoutRoot)
            said = "open: the layout did not load";
        OZT_ProbeClient.Ack(s_OpenLine, said);
    }

    override void OnHide()
    {
        StopQuestions();
        if (m_Claimed)
        {
            Mission mission = GetGame().GetMission();
            if (mission)
                mission.RemoveActiveInputExcludes({"menu"}, true);
            m_Claimed = false;
        }
        if (s_Current == this)
            s_Current = null;
        super.OnHide();
    }

    protected void StopQuestions()
    {
        if (!m_Questions)
            return;
        for (int i = 0; i < m_Questions.Count(); i++)
            m_Questions[i].Stop();
    }

    void Say(string text)
    {
        if (m_Status)
            m_Status.SetText(text);
    }

    void ResetFrames()
    {
        m_Frames = 0;
        m_FrameSum = 0;
        m_FrameMax = 0;
        m_FramesSlow = 0;
    }

    // ---- commands ------------------------------------------------------------------

    void Command(array<string> words, string line)
    {
        string cmd = words[1];
        if (cmd == "close")
        {
            OZT_ProbeClient.Ack(line, "close: closing");
            Close();
            return;
        }
        string answer = "";
        if (cmd == "frames")
            answer = FramesCommand(words);
        for (int i = 0; i < m_Questions.Count(); i++)
        {
            if (answer != "")
                break;
            answer = m_Questions[i].Command(words);
        }
        if (answer == "")
            answer = "unknown command '" + cmd + "'";
        Say(answer);
        OZT_ProbeClient.Ack(line, answer);
    }

    // frames reset | frames: how long the frames took since the reset.
    protected string FramesCommand(array<string> words)
    {
        if (words.Count() > 2 && words[2] == "reset")
        {
            ResetFrames();
            return "frames: counting from now";
        }
        float avg = 0;
        if (m_Frames > 0)
            avg = m_FrameSum / m_Frames;
        string s = "frames=" + m_Frames.ToString() + " avg_ms=" + OZT_ProbeLog.R1(avg);
        s = s + " max_ms=" + OZT_ProbeLog.R1(m_FrameMax) + " over_33ms=" + m_FramesSlow.ToString();
        return s;
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);
        float ms = timeslice * 1000;
        m_Frames++;
        m_FrameSum = m_FrameSum + ms;
        if (ms > m_FrameMax)
            m_FrameMax = ms;
        if (ms > 33.4)
            m_FramesSlow++;
        if (m_Questions)
        {
            for (int i = 0; i < m_Questions.Count(); i++)
                m_Questions[i].Frame(timeslice);
        }
        if (GetUApi().GetInputByID(UAUIBack).LocalPress())
            Close();
    }

    // ---- what the engine tells the menu --------------------------------------------

    override bool OnDrag(Widget w, int x, int y)
    {
        if (m_Drag && m_Drag.Seen("OnDrag", w, x, y, null))
            return true;
        return super.OnDrag(w, x, y);
    }

    override bool OnDragging(Widget w, int x, int y, Widget reciever)
    {
        if (m_Drag && m_Drag.Seen("OnDragging", w, x, y, reciever))
            return true;
        return super.OnDragging(w, x, y, reciever);
    }

    override bool OnDraggingOver(Widget w, int x, int y, Widget reciever)
    {
        if (m_Drag && m_Drag.Seen("OnDraggingOver", w, x, y, reciever))
            return true;
        return super.OnDraggingOver(w, x, y, reciever);
    }

    override bool OnDrop(Widget w, int x, int y, Widget reciever)
    {
        if (m_Drag && m_Drag.Seen("OnDrop", w, x, y, reciever))
            return true;
        return super.OnDrop(w, x, y, reciever);
    }

    override bool OnDropReceived(Widget w, int x, int y, Widget reciever)
    {
        if (m_Drag && m_Drag.Seen("OnDropReceived", w, x, y, reciever))
            return true;
        return super.OnDropReceived(w, x, y, reciever);
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (m_Drag && m_Drag.MouseDown(w, x, y, button))
            return true;
        if (m_Previews && m_Previews.MouseDown(w, x, y, button))
            return true;
        return super.OnMouseButtonDown(w, x, y, button);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (m_Drag)
            m_Drag.Seen("OnMouseButtonUp", w, x, y, null);
        return super.OnMouseButtonUp(w, x, y, button);
    }
}
