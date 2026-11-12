// The TraderPlus bank as a trader's money (design §7.3). Its id in the files is
// "traderplus": the server's Settings.Provider, or one trader's Provider.
//
// TraderPlus keeps every account in a JSON file of its own and reads it again
// on every call (TraderPlusBankingData.GetAccount). This does the same on each
// call of the trader's and keeps no copy, so an ATM and this never disagree;
// a change is written with TraderPlus's own UpdateAccount, which also sends
// the new sum to the player's client. The deal asks Balance before it plans,
// Take before anything leaves the player, and Give after the goods are gone.
//
// MaxAmount is TraderPlus's cap on deposits at its ATMs. A sale here pays in
// full even above it: refusing after the goods are gone, or cutting the sum,
// would cost the player money. The log says when it happens.
class OZT_TraderPlusBank : OZT_MoneyProvider
{
    static const string ID = "traderplus";

    override string Id()
    {
        return ID;
    }

    override bool Physical()
    {
        return false;
    }

    override int Unit()
    {
        return 1;
    }

    override int Balance(PlayerBase player)
    {
        TraderPlusBankingData acc = Account(player);
        if (!acc)
            return 0;
        return acc.MoneyAmount;
    }

    override bool Take(PlayerBase player, int amount, string why)
    {
        TraderPlusBankingData acc = Account(player);
        if (!acc || amount < 0 || acc.MoneyAmount < amount)
            return false;
        acc.MoneyAmount = acc.MoneyAmount - amount;
        acc.UpdateAccount(player);
        string line = "traderplus bank: " + Who(player) + " paid " + amount.ToString();
        line = line + " (" + why + "), " + acc.MoneyAmount.ToString() + " left";
        OZT_Log.Info(line);
        return true;
    }

    override bool Give(PlayerBase player, int amount, string why)
    {
        TraderPlusBankingData acc = Account(player);
        if (!acc || amount < 0)
            return false;
        acc.MoneyAmount = acc.MoneyAmount + amount;
        acc.UpdateAccount(player);
        string line = "traderplus bank: " + Who(player) + " got " + amount.ToString();
        line = line + " (" + why + "), " + acc.MoneyAmount.ToString() + " now";
        OZT_Log.Info(line);
        if (acc.MaxAmount > 0 && acc.MoneyAmount > acc.MaxAmount)
        {
            string over = "traderplus bank: " + Who(player) + " is above the account's MaxAmount (" + acc.MaxAmount.ToString();
            OZT_Log.Warn(over + ") after a sale; paid in full all the same");
        }
        return true;
    }

    protected TraderPlusBankingData Account(PlayerBase player)
    {
        if (!player || !player.GetIdentity())
            return null;
        return TraderPlusBankingData.GetAccount(player);
    }

    // The player by the in-game name, as TraderPlus's own log names them.
    protected string Who(PlayerBase player)
    {
        if (!player || !player.GetIdentity())
            return "?";
        return "'" + player.GetIdentity().GetName() + "'";
    }
}

// Registered at the mission's start, beside the trader's own item money.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        OZT_Money.Provide(new OZT_TraderPlusBank());
    }
}
