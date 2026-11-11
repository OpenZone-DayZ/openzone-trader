// Question 8a: what the engine's drag and drop tells a menu of ours, and what a
// drag made from the mouse state alone needs.
//
// Three tiles stand between two drop panels:
//   TileEngine   a panel with WidgetFlags.DRAGGABLE (a picture inside it)
//   TilePreview  an ItemPreviewWidget with the same flag, dragged itself
//   TilePoll     a plain panel: pressed, then followed by the mouse state
// Every event the menu is told is counted, with the first and the last call
// kept: which widget, where, and which receiver.
//
//   drag reset                 forget the counts, put the tiles back
//   drag report                the counts in the answer, every sample in the log
//   drag returns 0|1           what the menu's drag events return
//   drag registered            hand the tiles and the right panel to the game's
//                              WidgetEventHandler, as the vanilla inventory does
//                              (until the screen is closed)
class OZT_ProbeQDrag : OZT_ProbeQuestion
{
    protected Widget m_TileEngine;
    protected ItemPreviewWidget m_TilePreview;
    protected Widget m_TilePoll;
    protected Widget m_DropRight;
    protected Widget m_Ghost;

    protected ref array<string> m_Names;
    protected ref map<string, int> m_Count;
    protected ref map<string, string> m_First;
    protected ref map<string, string> m_Last;
    protected bool m_Handled;
    protected bool m_Registered;

    protected float m_EngineX;
    protected float m_EngineY;
    protected float m_PreviewX;
    protected float m_PreviewY;

    // The drag made by hand.
    protected bool m_Polling;
    protected int m_PollMoves;
    protected string m_PollEnd;

    override void OnAttach()
    {
        m_TileEngine = m_Root.FindAnyWidget("TileEngine");
        m_TilePreview = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("TilePreview"));
        m_TilePoll = m_Root.FindAnyWidget("TilePoll");
        m_DropRight = m_Root.FindAnyWidget("DropRight");
        m_Ghost = m_Root.FindAnyWidget("Ghost");

        m_Names = new array<string>();
        m_Count = new map<string, int>();
        m_First = new map<string, string>();
        m_Last = new map<string, string>();
        m_PollEnd = "";

        if (m_TileEngine)
        {
            m_TileEngine.SetFlags(WidgetFlags.DRAGGABLE);
            m_TileEngine.GetPos(m_EngineX, m_EngineY);
        }
        if (m_TilePreview)
        {
            m_TilePreview.SetFlags(WidgetFlags.DRAGGABLE);
            m_TilePreview.GetPos(m_PreviewX, m_PreviewY);
        }
        ShowPictures();
    }

    // Real things of the player on the tiles: the jacket and the rifle of the kit.
    protected void ShowPictures()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;
        EntityAI jacket = player.FindAttachmentBySlotName("Body");
        ItemPreviewWidget pic = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("TileEnginePic"));
        if (pic && jacket)
        {
            pic.SetItem(jacket);
            pic.SetView(jacket.GetViewIndex());
        }
        EntityAI rifle = player.FindAttachmentBySlotName("Shoulder");
        if (m_TilePreview && rifle)
        {
            m_TilePreview.SetItem(rifle);
            m_TilePreview.SetView(rifle.GetViewIndex());
        }
    }

    // One event, counted. Returns what the menu is to answer the engine.
    bool Seen(string what, Widget w, int x, int y, Widget reciever)
    {
        int n = 0;
        m_Count.Find(what, n);
        m_Count.Set(what, n + 1);
        string s = "w=" + NameOf(w) + " x=" + x.ToString() + " y=" + y.ToString() + " reciever=" + NameOf(reciever);
        if (n == 0)
        {
            m_Names.Insert(what);
            m_First.Set(what, s);
        }
        m_Last.Set(what, s);
        return m_Handled;
    }

    // The press that starts a drag made by hand.
    bool MouseDown(Widget w, int x, int y, int button)
    {
        Seen("OnMouseButtonDown", w, x, y, null);
        if (w != m_TilePoll || button != 0)
            return false;
        m_Polling = true;
        m_PollMoves = 0;
        m_PollEnd = "";
        return true;
    }

    override void Frame(float dt)
    {
        if (!m_Polling)
            return;
        int mx;
        int my;
        GetMousePos(mx, my);
        if (GetMouseState(MouseState.LEFT) & 0x80000000)
        {
            m_PollMoves++;
            if (m_Ghost)
            {
                float gw;
                float gh;
                m_Ghost.GetScreenSize(gw, gh);
                m_Ghost.SetPos(mx - gw / 2, my - gh / 2);
                m_Ghost.Show(true);
            }
            return;
        }
        // Released: the ghost ignores the pointer, so what is under the cursor
        // is the widget the thing was dropped on.
        m_Polling = false;
        if (m_Ghost)
            m_Ghost.Show(false);
        Widget under = GetWidgetUnderCursor();
        m_PollEnd = "released at " + mx.ToString() + " " + my.ToString() + " over " + NameOf(under);
    }

    // The game's own registry, the way the vanilla inventory wires its icons.
    protected void Register()
    {
        WidgetEventHandler h = WidgetEventHandler.GetInstance();
        if (!h || m_Registered)
            return;
        h.RegisterOnDrag(m_TileEngine, this, "RegDrag");
        h.RegisterOnDrop(m_TileEngine, this, "RegDrop");
        h.RegisterOnDraggingOver(m_DropRight, this, "RegDraggingOver");
        h.RegisterOnDropReceived(m_DropRight, this, "RegDropReceived");
        m_Registered = true;
    }

    void RegDrag(Widget w, int x, int y)
    {
        Seen("registered OnDrag", w, x, y, null);
    }

    void RegDrop(Widget w, int x, int y)
    {
        Seen("registered OnDrop", w, x, y, null);
    }

    void RegDraggingOver(Widget w, int x, int y, Widget reciever)
    {
        Seen("registered OnDraggingOver", w, x, y, reciever);
    }

    void RegDropReceived(Widget w, int x, int y, Widget reciever)
    {
        Seen("registered OnDropReceived", w, x, y, reciever);
    }

    override string Command(array<string> words)
    {
        if (words[1] != "drag")
            return "";
        string sub = Word(words, 2, "report");
        if (sub == "reset")
        {
            m_Names.Clear();
            m_Count.Clear();
            m_First.Clear();
            m_Last.Clear();
            m_PollEnd = "";
            m_PollMoves = 0;
            if (m_TileEngine)
                m_TileEngine.SetPos(m_EngineX, m_EngineY);
            if (m_TilePreview)
                m_TilePreview.SetPos(m_PreviewX, m_PreviewY);
            return "drag: counts cleared, tiles back";
        }
        if (sub == "returns")
        {
            bool yes = Word(words, 3, "0") == "1";
            m_Handled = yes;
            return "drag: the menu's events now return " + OZT_ProbeLog.B(yes);
        }
        if (sub == "registered")
        {
            Register();
            return "drag: TileEngine and DropRight handed to WidgetEventHandler=" + OZT_ProbeLog.B(m_Registered);
        }
        return Report();
    }

    protected string Report()
    {
        string s = "drag:";
        for (int i = 0; i < m_Names.Count(); i++)
        {
            string what = m_Names[i];
            int n = m_Count.Get(what);
            s = s + " " + what + "=" + n.ToString();
            OZT_ProbeClient.Note(what + " x" + n.ToString() + " | first: " + m_First.Get(what) + " | last: " + m_Last.Get(what));
        }
        if (m_Names.Count() == 0)
            s = s + " no event yet";
        s = s + " | tiles now: " + Place(m_TileEngine) + " " + Place(m_TilePreview);
        if (m_PollEnd != "")
            s = s + " | by hand: moves=" + m_PollMoves.ToString() + " " + m_PollEnd;
        return s;
    }

    protected string Place(Widget w)
    {
        if (!w)
            return "(none)";
        float x;
        float y;
        w.GetScreenPos(x, y);
        int px = Math.Round(x);
        int py = Math.Round(y);
        return w.GetName() + "@" + px.ToString() + "," + py.ToString();
    }
}
