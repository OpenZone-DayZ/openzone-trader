// Questions 8b and 8c: the character and an item in preview widgets of our own
// menu.
//
//   player on                  the real player into PlayerView, as the vanilla
//                              inventory sets it up
//   player rot <deg>           turn it about the vertical axis
//   player refresh             Refresh(): take the gear again
//   (press and drag on PlayerView turns it by the mouse, as vanilla does)
//   item <slot>                the thing the player wears on a slot into ItemView
//   item flip 0|1|off          the forced flip the vanilla cargo icons use
//   item turn <x> <y> <z>      SetModelOrientation
//   item pos <x> <y> <z>       SetModelPosition
class OZT_ProbeQPreviews : OZT_ProbeQuestion
{
    protected PlayerPreviewWidget m_PlayerView;
    protected Widget m_PlayerTurn;
    protected ItemPreviewWidget m_ItemView;

    protected bool m_Turning;
    protected int m_TurnFromX;
    protected int m_TurnMoves;
    protected vector m_Orientation;

    override void OnAttach()
    {
        m_PlayerView = PlayerPreviewWidget.Cast(m_Root.FindAnyWidget("PlayerView"));
        m_PlayerTurn = m_Root.FindAnyWidget("PlayerTurn");
        m_ItemView = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("ItemView"));
    }

    bool MouseDown(Widget w, int x, int y, int button)
    {
        if (w != m_PlayerTurn || button != 0 || !m_PlayerView)
            return false;
        int my;
        GetMousePos(m_TurnFromX, my);
        m_Turning = true;
        m_TurnMoves = 0;
        return true;
    }

    // The vanilla inventory's way (playerpreview.c): the yaw follows the
    // mouse while the button is down.
    override void Frame(float dt)
    {
        if (!m_Turning)
            return;
        int mx;
        int my;
        GetMousePos(mx, my);
        vector o = m_Orientation;
        o[1] = o[1] - (m_TurnFromX - mx);
        m_PlayerView.SetModelOrientation(o);
        if (GetMouseState(MouseState.LEFT) & 0x80000000)
        {
            m_TurnMoves++;
            return;
        }
        m_Orientation = o;
        m_Turning = false;
    }

    override string Command(array<string> words)
    {
        if (words[1] == "player")
            return PlayerCommand(words);
        if (words[1] == "item")
            return ItemCommand(words);
        return "";
    }

    protected string PlayerCommand(array<string> words)
    {
        if (!m_PlayerView)
            return "player: the layout has no PlayerView";
        string sub = Word(words, 2, "state");
        DayZPlayer me = GetGame().GetPlayer();
        if (sub == "on")
        {
            m_PlayerView.SetPlayer(me);
            m_PlayerView.SetModelPosition("0 0 0.605");
            if (me)
                m_PlayerView.UpdateItemInHands(me.GetHumanInventory().GetEntityInHands());
            m_Orientation = m_PlayerView.GetModelOrientation();
        }
        else if (sub == "rot")
        {
            m_Orientation = Vector(0, Word(words, 3, "0").ToFloat(), 0);
            m_PlayerView.SetModelOrientation(m_Orientation);
        }
        else if (sub == "refresh")
        {
            m_PlayerView.Refresh();
        }
        DayZPlayer dummy = m_PlayerView.GetDummyPlayer();
        string s = "player " + sub + ": orientation=" + V(m_PlayerView.GetModelOrientation());
        s = s + " position=" + V(m_PlayerView.GetModelPosition());
        if (dummy)
        {
            s = s + " dummy=" + dummy.GetType() + " id=" + dummy.GetNetworkIDString();
            s = s + " worn=" + dummy.GetInventory().AttachmentCount().ToString();
            s = s + " same as the player=" + OZT_ProbeLog.B(dummy == me);
        }
        else
        {
            s = s + " dummy=null";
        }
        s = s + " | turned by mouse: moves=" + m_TurnMoves.ToString();
        return s;
    }

    protected string ItemCommand(array<string> words)
    {
        if (!m_ItemView)
            return "item: the layout has no ItemView";
        string sub = Word(words, 2, "state");
        if (sub == "flip")
        {
            string how = Word(words, 3, "off");
            if (how == "off")
            {
                m_ItemView.SetForceFlipEnable(false);
            }
            else
            {
                m_ItemView.SetForceFlipEnable(true);
                m_ItemView.SetForceFlip(how == "1");
            }
        }
        else if (sub == "turn")
        {
            m_ItemView.SetModelOrientation(Three(words));
        }
        else if (sub == "pos")
        {
            m_ItemView.SetModelPosition(Three(words));
        }
        else if (sub != "state")
        {
            PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
            EntityAI worn;
            if (player)
                worn = player.FindAttachmentBySlotName(sub);
            if (!worn)
                return "item: the player wears nothing on slot '" + sub + "'";
            m_ItemView.SetItem(worn);
            m_ItemView.SetView(worn.GetViewIndex());
        }
        EntityAI shown = m_ItemView.GetItem();
        string s = "item " + sub + ": shows=";
        if (shown)
            s = s + shown.GetType();
        else
            s = s + "nothing";
        s = s + " view=" + m_ItemView.GetView().ToString();
        s = s + " orientation=" + V(m_ItemView.GetModelOrientation()) + " position=" + V(m_ItemView.GetModelPosition());
        return s;
    }

    protected vector Three(array<string> words)
    {
        float x = Word(words, 3, "0").ToFloat();
        float y = Word(words, 4, "0").ToFloat();
        float z = Word(words, 5, "0").ToFloat();
        return Vector(x, y, z);
    }
}
