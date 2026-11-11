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
}

class OZT_Money
{
    private static ref array<ref OZT_MoneyProvider> s_Providers = new array<ref OZT_MoneyProvider>();

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
        for (int i = 0; i < s_Providers.Count(); i++)
        {
            if (s_Providers[i].Id() == id)
                return s_Providers[i];
        }
        return null;
    }

    // A trader's provider: its own when it names one that is there, else the
    // server's, else item money.
    static OZT_MoneyProvider Of(OZT_Point trader)
    {
        OZT_MoneyProvider p;
        if (trader && trader.Provider != "")
            p = ById(trader.Provider);
        if (!p)
            p = ById(OZT_Config.Get().Settings().Provider);
        if (!p)
            p = ById(OZT_Const.PROVIDER_ITEMS);
        return p;
    }

    // At the ready tick, when every glue has had its start: name the providers
    // the files ask for and nobody brought. Returns how many.
    static int CheckNamed()
    {
        int missing = 0;
        string server = OZT_Config.Get().Settings().Provider;
        if (!ById(server))
        {
            OZT_Log.Warn("Settings: the money provider '" + server + "' is not loaded, item money is used");
            missing++;
        }
        array<ref OZT_Point> traders = OZT_Config.Get().Traders().Traders;
        for (int i = 0; i < traders.Count(); i++)
        {
            string own = traders[i].Provider;
            if (own == "" || ById(own))
                continue;
            OZT_Log.Warn("Traders: '" + traders[i].Id + "' names the money provider '" + own + "', which is not loaded; the server's is used");
            missing++;
        }
        return missing;
    }

    // 4_World classes held by a static: let go at mission end.
    static void Reset()
    {
        s_Providers.Clear();
    }
}
