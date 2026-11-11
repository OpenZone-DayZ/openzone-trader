// What keeps a trader's character quiet (design §9.2): no damage, no target for
// the infected and animals, no action of another player on it but trading, no
// inventory in anyone's vicinity.
//
// Most of a player never runs for it: the server makes it and never selects it
// for a player, so its modifiers do not tick, it has no action manager and the
// engine gets no callbacks to aim the infected's senses at it (PlayerBase.
// OnSelectPlayer never runs). What is left is closed here, for the trader's
// characters only. The server knows them as the bodies it made at the points;
// a client knows the points from the sync packet, and that a trader has no
// identity (a player who stands where a trader stands still has one).

modded class PlayerBase
{
    bool OZT_IsTrader()
    {
        if (GetGame().IsServer())
            return OZT_Points.IdOf(this) != "";
        if (GetIdentity())
            return false;
        return OZT_ClientPoints.At(this) != null;
    }

    // The damage switch is set when it is made (SetAllowDamage); this is the
    // second layer the engine asks right before damage is applied.
    override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        if (OZT_IsTrader())
            return false;
        return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
    }

    override bool CanBeTargetedByAI(EntityAI ai)
    {
        if (OZT_IsTrader())
            return false;
        return super.CanBeTargetedByAI(ai);
    }

    override bool CanBeRestrained()
    {
        if (OZT_IsTrader())
            return false;
        return super.CanBeRestrained();
    }

    override bool CanEatAndDrink()
    {
        if (OZT_IsTrader())
            return false;
        return super.CanEatAndDrink();
    }

    override bool CanConsumeFood(ConsumeConditionData data = null)
    {
        if (OZT_IsTrader())
            return false;
        return super.CanConsumeFood(data);
    }

    // Not in the vicinity panel; and nothing comes off it, whatever asks.
    override bool IsInventoryVisible()
    {
        if (OZT_IsTrader())
            return false;
        return super.IsInventoryVisible();
    }

    override bool CanReleaseAttachment(EntityAI attachment)
    {
        if (OZT_IsTrader())
            return false;
        return super.CanReleaseAttachment(attachment);
    }

    override bool CanReleaseCargo(EntityAI cargo)
    {
        if (OZT_IsTrader())
            return false;
        return super.CanReleaseCargo(cargo);
    }
}

// Every action a player does to another character goes through this one
// condition (restrain, feed, bandage, blood, pulse...): a trader is never such
// a target. The trade action looks at its target another way (CCTCursor).
modded class CCTMan
{
    override bool Can(PlayerBase player, ActionTarget target)
    {
        if (target)
        {
            PlayerBase man = PlayerBase.Cast(target.GetObject());
            if (man && man.OZT_IsTrader())
                return false;
        }
        return super.Can(player, target);
    }
}
