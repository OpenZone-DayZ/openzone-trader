// The trade action: on whatever stands at a trader point, an object or a
// trader's character, whatever the hands hold.
//
// The client offers it on a thing the sync packet names as a point, the player
// within the point's radius (OZT_ClientPoints). The server checks again against
// the things it made itself (OZT_Points) and opens the screen the way the
// series opens its screens: OZ_Rpc.Show, the client's half of an action never
// opens a menu (the PDA's measurement). A refusal is a notice with the key.

class OZT_ActionTrade : ActionInteractBase
{
    void OZT_ActionTrade()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_Text = "#STR_OZT_ACTION_TRADE";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone();
        // Where the player looks, not the thing's middle: the middle of a
        // stall may stand metres from the counter (the storage mod's choice
        // for its boxes, for the same reason).
        m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
    }

    // Several players trade at one point at once.
    override bool IsLockTargetOnUse()
    {
        return false;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!player || !target)
            return false;
        Object obj = target.GetObject();
        if (!obj)
            return false;
        if (GetGame().IsServer())
            return OZT_Points.Reaches(OZT_Points.IdOf(obj), player);
        OZT_ClientPoint p = OZT_ClientPoints.At(obj);
        if (!p)
            return false;
        return vector.Distance(player.GetPosition(), p.Pos) <= p.Radius;
    }

    override void OnExecuteServer(ActionData action_data)
    {
        if (!action_data || !action_data.m_Player)
            return;
        PlayerBase player = action_data.m_Player;
        PlayerIdentity who = player.GetIdentity();
        if (!who)
            return;

        // The target may be gone by now: the action runs from an animation
        // event, not the frame its condition held in. Then the nearest point
        // that reaches the player is the one.
        Object obj = null;
        if (action_data.m_Target)
            obj = action_data.m_Target.GetObject();
        string id = OZT_Points.IdOf(obj);
        if (id == "")
            id = OZT_Points.Nearest(player);
        if (id == "")
            return;

        string why = OZT_Points.MayTrade(id, player);
        if (why != "")
        {
            OZ_Rpc.Notice(who, OZT_Const.NOTICE_OPEN, false, why);
            return;
        }
        OZT_Log.Dbg("trade action: show '" + id + "' to " + who.GetPlainId());
        OZ_Rpc.Show(who, OZT_Const.SHOW_PREFIX + id);
    }
}

// An action nobody registers is dropped without a word.
modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(OZT_ActionTrade);
    }
}

// The player's own actions are the ones offered on any target: the condition
// says which targets are traders.
modded class PlayerBase
{
    override void SetActions(out TInputActionMap InputActionMap)
    {
        super.SetActions(InputActionMap);
        AddAction(OZT_ActionTrade, InputActionMap);
    }
}
