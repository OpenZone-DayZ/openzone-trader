// OZ_Trader_Settings.json: what holds for every trader of the server.
//
// Money first. `Provider` names the money provider of the server ("items", or
// the id a bank glue registers); a trader may name another. With item money
// `Notes` lists the banknote classes and what one unit of each is worth, and
// `Holders` the wallet-like classes that are filled before the bags.

class OZT_Note
{
    string ClassName = "";
    int Value = 0;

    OZT_Note Copy()
    {
        OZT_Note c = new OZT_Note();
        c.ClassName = ClassName;
        c.Value = Value;
        return c;
    }
}

// What the trader pays for an item that is not pristine, as a share of the
// pristine price. A ruined item is not taken at all.
class OZT_ConditionFactors
{
    float Pristine = 1;
    float Worn = 0.85;
    float Damaged = 0.6;
    float BadlyDamaged = 0.35;

    OZT_ConditionFactors Copy()
    {
        OZT_ConditionFactors c = new OZT_ConditionFactors();
        c.Pristine = Pristine;
        c.Worn = Worn;
        c.Damaged = Damaged;
        c.BadlyDamaged = BadlyDamaged;
        return c;
    }

    // By health level: GameConstants.STATE_PRISTINE (0) .. STATE_BADLY_DAMAGED (3).
    float Of(int level)
    {
        if (level <= 0)
            return Pristine;
        if (level == 1)
            return Worn;
        if (level == 2)
            return Damaged;
        if (level == 3)
            return BadlyDamaged;
        return 0;
    }
}

// A licence the server sells or gives (design §11.2): `Id` is what goods
// (`@licence:<Id>`) and gates (`licence`) name, `Name` what the player reads,
// `Look` the class the shelf draws for it.
class OZT_LicenceDef
{
    string Id = "";
    string Name = "";
    string Look = "";

    OZT_LicenceDef Copy()
    {
        OZT_LicenceDef c = new OZT_LicenceDef();
        c.Id = Id;
        c.Name = Name;
        c.Look = Look;
        return c;
    }
}

class OZT_Settings : OZ_ConfigBase
{
    string Provider = "items";
    ref array<ref OZT_Note> Notes;
    ref array<string> Holders;

    // The gate families switched off for the whole server (design §8.2): a
    // condition of such a family passes and its discounts are not given. A
    // trader turns one on again with its own GatesOn.
    ref array<string> GatesOff;
    ref OZT_ConditionFactors Condition;

    // The least time between two deals of one player, in milliseconds.
    int CooldownMs = 1000;

    // 1: a bought item that fits a free slot of the character is created on it
    // (decision 4 of the design); 0: everything goes to the bags.
    int WearBought = 1;

    // Stock at a server start (design §11.1, TraderPlus's switches): 0 keeps
    // the stock files as they are, 1 fills every line with a Max above 0 to Max
    // and drops the rest, 2 the same with a random count up to Max.
    int StartStock = 0;
    // 1: a line at or above its Max loses its Destock share at a start.
    int DestockAtStart = 0;
    // 1: what players sell goes into the pristine line whatever its condition.
    int StockOnlyPristine = 0;

    // The licences of the server (design §11.2).
    ref array<ref OZT_LicenceDef> Licences;

    override int LatestVersion()
    {
        return 1;
    }

    override void LoadDefaults()
    {
        super.LoadDefaults();
        Provider = OZT_Const.PROVIDER_ITEMS;
        Notes = new array<ref OZT_Note>();
        Holders = new array<string>();
        GatesOff = new array<string>();
        Condition = new OZT_ConditionFactors();
        CooldownMs = 1000;
        WearBought = 1;
        Licences = new array<ref OZT_LicenceDef>();
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (Provider == "")
            Provider = OZT_Const.PROVIDER_ITEMS;

        // A list the file does not carry: no family is off.
        array<string> off = new array<string>();
        if (GatesOff)
        {
            for (int g = 0; g < GatesOff.Count(); g++)
            {
                if (GatesOff[g] != "" && off.Find(GatesOff[g]) < 0)
                    off.Insert(GatesOff[g]);
            }
        }
        GatesOff = off;

        // Everything nested goes into objects a script made (OZ_ConfigBase.c):
        // what the loader allocated ran no field initialiser.
        array<ref OZT_Note> notes = new array<ref OZT_Note>();
        if (Notes)
        {
            for (int i = 0; i < Notes.Count(); i++)
            {
                OZT_Note n = Notes[i];
                if (n)
                    notes.Insert(n.Copy());
            }
        }
        Notes = notes;

        array<string> holders = new array<string>();
        if (Holders)
        {
            for (int h = 0; h < Holders.Count(); h++)
            {
                if (Holders[h] != "")
                    holders.Insert(Holders[h]);
            }
        }
        Holders = holders;

        if (!Condition)
            Condition = new OZT_ConditionFactors();
        else
            Condition = Condition.Copy();

        // An absent section comes back allocated and zeroed, and the copy
        // carries those zeros: four zeros mean "the file did not say".
        float sum = Condition.Pristine + Condition.Worn + Condition.Damaged + Condition.BadlyDamaged;
        if (sum <= 0)
            Condition = new OZT_ConditionFactors();

        bool negative = Condition.Pristine < 0 || Condition.Worn < 0;
        if (Condition.Damaged < 0 || Condition.BadlyDamaged < 0)
            negative = true;
        if (negative)
        {
            OZT_Log.Warn("Settings: a condition factor is below zero, using the defaults");
            Condition = new OZT_ConditionFactors();
            warnings++;
        }

        array<ref OZT_LicenceDef> defs = new array<ref OZT_LicenceDef>();
        array<string> ids = new array<string>();
        if (Licences)
        {
            for (int l = 0; l < Licences.Count(); l++)
            {
                OZT_LicenceDef raw = Licences[l];
                if (!raw)
                    continue;
                OZT_LicenceDef d = raw.Copy();
                if (!OZT_Ids.IsSafe(d.Id) || ids.Find(d.Id) > -1)
                {
                    OZT_Log.Warn("Settings: a licence with the Id '" + d.Id + "' is dropped: letters, digits, _ and - only, once each");
                    warnings++;
                    continue;
                }
                if (d.Name == "")
                    d.Name = d.Id;
                if (d.Look == "")
                    d.Look = "Paper";
                ids.Insert(d.Id);
                defs.Insert(d);
            }
        }
        Licences = defs;
        if (StartStock < 0 || StartStock > 2)
        {
            OZT_Log.Warn("Settings: StartStock " + StartStock.ToString() + " is 0, 1 or 2; using 0");
            StartStock = 0;
            warnings++;
        }
        if (CooldownMs < 0 || CooldownMs > 600000)
        {
            OZT_Log.Warn("Settings: CooldownMs " + CooldownMs.ToString() + " is outside 0..600000, using 1000");
            CooldownMs = 1000;
            warnings++;
        }
    }
}
