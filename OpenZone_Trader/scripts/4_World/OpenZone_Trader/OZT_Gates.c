// The gate service (design §8.2): locks and discounts on the word of any mod.
//
// The base mod holds a registry of providers and knows no kind by name. A
// provider answers for one family (the word the switches use) and brings the
// kinds of condition it understands, in the form of the money service: a class
// to extend, registered in the provider's own start:
//
//     OZT_Gates.Provide(new MyMod_TraderGate());
//
// The identity kinds (faction, post, leader) are this mod's own provider,
// because they need the core alone (OZT_FactionGate below); research is a glue
// pbo; any other mod brings its own the same way.
//
// When the answering side is absent -- no provider for the kind, its mod not
// loaded, the identity contract with the core's stub only -- or its family is
// switched off, every lock of that kind is off and passes, a trader's Access
// included, and its discounts are not given (the owner, 2026-11-10). A missing
// mod never blocks trade and never gives money away.

class OZT_GateProvider
{
    // The family the switches name: "factions", "research", "reputation".
    string Family()
    {
        return "";
    }

    // The kinds of condition it answers.
    void Kinds(array<string> kinds)
    {
    }

    // Is the side that knows there today? False: every condition of its kinds
    // passes and no discount of them is given.
    bool Answers()
    {
        return true;
    }

    // Server only: does the condition hold for this player? `arg` is one of
    // the alternatives, trimmed.
    bool Holds(PlayerBase player, string kind, string arg)
    {
        return false;
    }

    // At load: can this condition ever be answered? `why` for the log.
    bool Knows(string kind, string arg, out string why)
    {
        why = "";
        return true;
    }

    // Optional: a discount the provider computes for this player, in percent.
    float Percent(PlayerBase player, string kind, string arg)
    {
        return 0;
    }

    // What the screen may show for the lock.
    string Label(string kind, string arg)
    {
        return kind + " " + arg;
    }

    // For the admin window: the values a condition of this kind may name, and
    // beside each what the admin reads (both lists the same length). None: the
    // value is a free word.
    void Choices(string kind, array<string> values, array<string> labels)
    {
    }

    // For the admin window: who holds the condition `kind arg` now (`arg` may hold
    // alternatives joined by "|"), as the admin reads it -- the factions that have
    // completed a research node, say. None: nobody, or the provider cannot tell.
    void Holders(string kind, string arg, array<string> who)
    {
    }
}

class OZT_Gates
{
    private static ref array<ref OZT_GateProvider> s_Providers = new array<ref OZT_GateProvider>();

    // The first to claim a family wins; the second is named in the log.
    static void Provide(OZT_GateProvider p)
    {
        if (!p || p.Family() == "")
        {
            OZT_Log.Error("a gate provider with no family was offered and is ignored");
            return;
        }
        for (int i = 0; i < s_Providers.Count(); i++)
        {
            if (s_Providers[i].Family() == p.Family())
            {
                OZT_Log.Warn("the gate family '" + p.Family() + "' is registered twice, the second is ignored");
                return;
            }
        }
        s_Providers.Insert(p);
        OZT_Log.Dbg("gate provider registered: " + p.Family());
    }

    // Every provider in the order they came (the admin window's `gates`).
    static int Count()
    {
        return s_Providers.Count();
    }

    static OZT_GateProvider At(int i)
    {
        if (i < 0 || i >= s_Providers.Count())
            return null;
        return s_Providers[i];
    }

    static OZT_GateProvider ByKind(string kind)
    {
        for (int i = 0; i < s_Providers.Count(); i++)
        {
            array<string> kinds = new array<string>();
            s_Providers[i].Kinds(kinds);
            if (kinds.Find(kind) > -1)
                return s_Providers[i];
        }
        return null;
    }

    // Is a family on at this trader: the trader's own GatesOn and GatesOff
    // first, then the server's GatesOff.
    static bool FamilyOn(OZT_Point trader, string family)
    {
        if (trader && trader.GatesOn && trader.GatesOn.Find(family) > -1)
            return true;
        if (trader && trader.GatesOff && trader.GatesOff.Find(family) > -1)
            return false;
        array<string> off = OZT_Config.Get().Settings().GatesOff;
        if (off && off.Find(family) > -1)
            return false;
        return true;
    }

    // The provider that answers a kind here and now, or null.
    static OZT_GateProvider Answering(OZT_Point trader, string kind)
    {
        OZT_GateProvider p = ByKind(kind);
        if (!p || !p.Answers() || !FamilyOn(trader, p.Family()))
            return null;
        return p;
    }

    // Does one condition hold? When nobody answers it here, `absent` is the
    // answer: true for a lock (it is off), false for a discount (not given).
    static bool Holds(PlayerBase player, OZT_Point trader, OZT_Condition c, bool absent)
    {
        if (!c || c.Kind == "")
            return true;
        OZT_GateProvider p = Answering(trader, c.Kind);
        if (!p)
            return absent;
        bool any = false;
        array<string> alts = new array<string>();
        c.Arg.Split("|", alts);
        if (alts.Count() == 0)
            alts.Insert("");
        for (int i = 0; i < alts.Count(); i++)
        {
            string alt = alts[i].Trim();
            if (p.Holds(player, c.Kind, alt))
                any = true;
        }
        if (c.Not == 1)
            return !any;
        return any;
    }

    // Do all conditions of a list hold? An empty list does.
    static bool All(PlayerBase player, OZT_Point trader, array<ref OZT_Condition> conds, bool absent)
    {
        if (!conds)
            return true;
        for (int i = 0; i < conds.Count(); i++)
        {
            if (!Holds(player, trader, conds[i], absent))
                return false;
        }
        return true;
    }

    // May this player trade at this trader at all (its Access)? A condition
    // nobody answers passes.
    static bool Admits(PlayerBase player, OZT_Point trader)
    {
        if (!trader)
            return false;
        return All(player, trader, trader.Access, true);
    }

    // The best single discount of a list for this player, in percent; 0 when
    // none is given. Never more than 100.
    static float Best(PlayerBase player, OZT_Point trader, array<ref OZT_Discount> discounts)
    {
        float best = 0;
        if (!discounts || !player)
            return 0;
        for (int i = 0; i < discounts.Count(); i++)
        {
            OZT_Discount d = discounts[i];
            float pct = 0;
            if (d.By)
            {
                OZT_GateProvider p = Answering(trader, d.By.Kind);
                if (!p || d.Max <= 0)
                    continue;
                pct = p.Percent(player, d.By.Kind, d.By.Arg.Trim());
                if (pct > d.Max)
                    pct = d.Max;
            }
            else if (d.Percent > 0 && All(player, trader, d.When, false))
            {
                pct = d.Percent;
            }
            if (pct > best)
                best = pct;
        }
        if (best > 100)
            best = 100;
        return best;
    }

    // The families that answer today, for the ready line: "none" when no one.
    static string Families()
    {
        string all = "";
        for (int i = 0; i < s_Providers.Count(); i++)
        {
            if (!s_Providers[i].Answers())
                continue;
            if (all != "")
                all = all + ",";
            all = all + s_Providers[i].Family();
        }
        if (all == "")
            return "none";
        return all;
    }

    // At the ready tick, when every glue has had its start: say which
    // conditions nobody answers (they pass) and which a provider does not know.
    // Returns how many were said.
    static int CheckNamed()
    {
        int said = 0;
        OZT_TradersFile traders = OZT_Config.Get().Traders();
        for (int t = 0; t < traders.Traders.Count(); t++)
        {
            OZT_Point p = traders.Traders[t];
            said = said + CheckList("trader '" + p.Id + "' Access", p.Access, false);
        }
        array<ref OZT_Category> cats = OZT_Config.Get().Goods().Categories;
        for (int c = 0; c < cats.Count(); c++)
        {
            OZT_Category cat = cats[c];
            said = said + CheckList("category '" + cat.Id + "' Requires", cat.Requires, false);
            said = said + CheckDiscounts("category '" + cat.Id + "'", cat.Discounts);
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_Goods line = cat.Goods[g];
                said = said + CheckList("'" + line.ClassName + "' in '" + cat.Id + "' Requires", line.Requires, false);
                said = said + CheckDiscounts("'" + line.ClassName + "' in '" + cat.Id + "'", line.Discounts);
            }
        }
        return said;
    }

    protected static int CheckDiscounts(string where, array<ref OZT_Discount> discounts)
    {
        int said = 0;
        if (!discounts)
            return 0;
        for (int i = 0; i < discounts.Count(); i++)
        {
            OZT_Discount d = discounts[i];
            said = said + CheckList(where + " discount When", d.When, true);
            if (d.By)
            {
                array<ref OZT_Condition> by = new array<ref OZT_Condition>();
                by.Insert(d.By);
                said = said + CheckList(where + " discount By", by, true);
            }
        }
        return said;
    }

    // A lock nobody answers passes; a discount nobody answers is not given:
    // the line says which.
    protected static int CheckList(string where, array<ref OZT_Condition> conds, bool discount)
    {
        string result = "it passes";
        if (discount)
            result = "the discount is not given";
        int said = 0;
        if (!conds)
            return 0;
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            OZT_GateProvider p = ByKind(c.Kind);
            if (!p)
            {
                OZT_Log.Info("gates: " + where + " names the kind '" + c.Kind + "', which nobody answers here: " + result);
                said++;
                continue;
            }
            if (!p.Answers())
            {
                string absent = "gates: " + where + " names '" + c.Kind + "' of the family '" + p.Family() + "'";
                OZT_Log.Info(absent + ", which is not there today: " + result);
                said++;
                continue;
            }
            string why;
            if (!p.Knows(c.Kind, c.Arg, why))
            {
                OZT_Log.Warn("gates: " + where + ": '" + c.Kind + " " + c.Arg + "' cannot be answered: " + why);
                said++;
            }
        }
        return said;
    }

    // 4_World classes held by a static: let go at mission end.
    static void Reset()
    {
        s_Providers.Clear();
    }
}

// The identity kinds, answered through the core's identity contract, which any
// faction mod provides (OZ_Identity.Provide). With the core's stub alone they
// are not there.
//
//   faction  the player's organisation or base faction is Arg; "org:<id>" or
//            "base:<id>" names the axis
//   post     the player holds the post Arg
//   leader   the player leads an organisation; with an Arg, that one
class OZT_FactionGate : OZT_GateProvider
{
    override string Family()
    {
        return "factions";
    }

    override void Kinds(array<string> kinds)
    {
        kinds.Insert("faction");
        kinds.Insert("post");
        kinds.Insert("leader");
    }

    override bool Answers()
    {
        return OZ_Identity.Present();
    }

    override bool Holds(PlayerBase player, string kind, string arg)
    {
        if (!player || !player.GetIdentity())
            return false;
        string uid = player.GetIdentity().GetPlainId();
        OZ_IdentityService id = OZ_Identity.Get();
        if (kind == "post")
            return id.HasPost(uid, arg);
        if (kind == "leader")
        {
            if (!id.IsLeader(uid))
                return false;
            return arg == "" || id.OrgOf(uid) == arg;
        }
        if (arg.IndexOf("org:") == 0)
            return id.OrgOf(uid) == arg.Substring(4, arg.Length() - 4);
        if (arg.IndexOf("base:") == 0)
            return id.BaseOf(uid) == arg.Substring(5, arg.Length() - 5);
        return id.OrgOf(uid) == arg || id.BaseOf(uid) == arg;
    }

    override string Label(string kind, string arg)
    {
        if (kind == "faction")
            return OZ_Identity.Get().FactionName(arg);
        return kind + " " + arg;
    }

    // The factions the source of factions knows, for `faction` and `leader`; a
    // post is a free word.
    override void Choices(string kind, array<string> values, array<string> labels)
    {
        if (kind != "faction" && kind != "leader")
            return;
        OZ_IdentityService id = OZ_Identity.Get();
        array<string> ids;
        id.FactionIds(ids);
        if (!ids)
            return;
        for (int i = 0; i < ids.Count(); i++)
        {
            values.Insert(ids[i]);
            labels.Insert(id.FactionName(ids[i]));
        }
    }
}
