// The money service: where a trader's money comes from.
//
// A provider answers for one kind of money. The item provider lives in this
// mod; a bank mod's provider lives in a glue pbo that extends this class and
// calls OZT_Money.Provide in its own start. A provider that is named in a file
// and is not there never stops trade: the item provider is used and the boot
// says so.
class OZT_MoneyProvider
{
    // "items", or the id a bank glue registers.
    string Id()
    {
        return "";
    }

    // Money is items in the inventory: the deal plans and moves the notes
    // itself, and Take and Give are not called.
    bool Physical()
    {
        return false;
    }

    // The smallest amount that can change hands.
    int Unit()
    {
        return 1;
    }

    int Balance(PlayerBase player)
    {
        return 0;
    }

    // For a provider that is not Physical(). `why` is a short word for the
    // provider's own log.
    bool Take(PlayerBase player, int amount, string why)
    {
        return false;
    }

    bool Give(PlayerBase player, int amount, string why)
    {
        return false;
    }

    // A string-table key or the admin's own text for "what this money is called".
    string Label()
    {
        return "";
    }

    // The bank behind cash ("cash first, then the bank"): what pays the rest
    // when the notes do not cover a price, and takes what the bags have no
    // room for. Null for every plain provider.
    OZT_MoneyProvider Then()
    {
        return null;
    }
}

// "Cash first, then the bank" (the owner, 2026-11-12), named as "items+<bank>"
// where a provider is named: the notes pay first and the bank the rest; what
// the trader pays out comes in notes, and what of it finds no room in the
// bags goes to the bank. The deal plans the notes as for item money
// (Physical) and asks Then() for the bank's share.
class OZT_CashThenBank : OZT_MoneyProvider
{
    protected ref OZT_MoneyProvider m_Cash;
    protected ref OZT_MoneyProvider m_Bank;

    void OZT_CashThenBank(OZT_MoneyProvider cash, OZT_MoneyProvider bank)
    {
        m_Cash = cash;
        m_Bank = bank;
    }

    override string Id()
    {
        return m_Cash.Id() + "+" + m_Bank.Id();
    }

    override bool Physical()
    {
        return true;
    }

    override int Unit()
    {
        return m_Cash.Unit();
    }

    // The notes the player carries and the account together.
    override int Balance(PlayerBase player)
    {
        return m_Cash.Balance(player) + m_Bank.Balance(player);
    }

    override bool Take(PlayerBase player, int amount, string why)
    {
        return m_Bank.Take(player, amount, why);
    }

    override bool Give(PlayerBase player, int amount, string why)
    {
        return m_Bank.Give(player, amount, why);
    }

    override string Label()
    {
        return m_Cash.Label();
    }

    override OZT_MoneyProvider Then()
    {
        return m_Bank;
    }

    OZT_MoneyProvider Cash()
    {
        return m_Cash;
    }
}

class OZT_Money
{
    private static ref array<ref OZT_MoneyProvider> s_Providers = new array<ref OZT_MoneyProvider>();
    private static ref map<string, ref OZT_MoneyProvider> s_Chains = new map<string, ref OZT_MoneyProvider>();

    // The first to claim an id wins, and the second is named in the log (the
    // rule of the core's registries).
    static void Provide(OZT_MoneyProvider provider)
    {
        if (!provider || provider.Id() == "")
        {
            OZT_Log.Error("a money provider with no id was offered and is ignored");
            return;
        }
        if (ById(provider.Id()))
        {
            OZT_Log.Warn("the money provider '" + provider.Id() + "' is registered twice, the second is ignored");
            return;
        }
        s_Providers.Insert(provider);
        OZT_Log.Dbg("money provider registered: " + provider.Id());
    }

    static OZT_MoneyProvider ById(string id)
    {
        int plus = id.IndexOf("+");
        if (plus > 0)
            return ChainOf(id, plus);
        for (int i = 0; i < s_Providers.Count(); i++)
        {
            if (s_Providers[i].Id() == id)
                return s_Providers[i];
        }
        return null;
    }

    // "items+<bank>": cash first, then that bank. Null when either half is
    // not there or the first is not money in items, the second not a bank.
    protected static OZT_MoneyProvider ChainOf(string id, int plus)
    {
        OZT_MoneyProvider made;
        if (s_Chains.Find(id, made))
            return made;
        string first = id.Substring(0, plus);
        string second = id.Substring(plus + 1, id.Length() - plus - 1);
        OZT_MoneyProvider cash = ById(first);
        OZT_MoneyProvider bank = ById(second);
        if (!cash || !bank || !cash.Physical() || bank.Physical())
            return null;
        made = new OZT_CashThenBank(cash, bank);
        s_Chains.Set(id, made);
        return made;
    }

    // A trader's provider: its own when it names one that is there, else the
    // server's, else item money. "items+<bank>" whose bank is not there is
    // cash alone: the first half, never no trade.
    static OZT_MoneyProvider Of(OZT_Point trader)
    {
        OZT_MoneyProvider p;
        if (trader && trader.Provider != "")
            p = OrCash(trader.Provider);
        if (!p)
            p = OrCash(OZT_Config.Get().Settings().Provider);
        if (!p)
            p = ById(OZT_Const.PROVIDER_ITEMS);
        return p;
    }

    protected static OZT_MoneyProvider OrCash(string id)
    {
        OZT_MoneyProvider p = ById(id);
        if (p)
            return p;
        int plus = id.IndexOf("+");
        if (plus > 0)
            return ById(id.Substring(0, plus));
        return null;
    }

    // At the ready tick, when every glue has had its start: name the providers
    // the files ask for and nobody brought. Returns how many.
    static int CheckNamed()
    {
        int missing = 0;
        string server = OZT_Config.Get().Settings().Provider;
        if (!ById(server))
        {
            OZT_Log.Warn("Settings: the money provider '" + server + "' is not loaded (or is no \"items+<bank>\"), " + Fallback(server) + " is used");
            missing++;
        }
        array<ref OZT_Point> traders = OZT_Config.Get().Traders().Traders;
        for (int i = 0; i < traders.Count(); i++)
        {
            string own = traders[i].Provider;
            if (own == "" || ById(own))
                continue;
            OZT_Log.Warn("Traders: '" + traders[i].Id + "' names the money provider '" + own + "', which is not loaded (or is no \"items+<bank>\"); " + Fallback(own) + " is used");
            missing++;
        }
        return missing;
    }

    // What stands in for a provider that is not there, in words.
    protected static string Fallback(string id)
    {
        int plus = id.IndexOf("+");
        if (plus > 0 && ById(id.Substring(0, plus)))
            return "'" + id.Substring(0, plus) + "' alone";
        return "the server's or item money";
    }

    // 4_World classes held by a static: let go at mission end.
    static void Reset()
    {
        s_Providers.Clear();
        s_Chains.Clear();
    }
}
