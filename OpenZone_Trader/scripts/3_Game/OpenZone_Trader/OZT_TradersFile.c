// OZ_Trader_Traders.json: the trader points.
//
// A point is where a trader stands and what it deals in: `Kind` "object" (any
// class: a counter, a stall, a terminal) or "npc" (a character, phase 4), the
// class, position and facing, the radius a player must stand within, the
// categories of the price book it deals in, a factor on each side of its
// prices, and its own money provider ("" means the server's).

// One thing a trader's character wears or carries, in the shape of the core's
// loadout item (OZ_LoadoutItem: the same keys, so an outfit reads like a
// preset of the factions mod). It lives in 3_Game with the point; the 4_World
// half turns it into the core's preset when the character is dressed.
//
// A key the file does not carry reads ZERO in a nested object (the loader
// leaves its default unset), so zero means "as the class has it" for both
// numbers: Quantity 0 is the class's own, Health01 0 or less is left alone (an
// outfit of ruined things is never meant).
class OZT_OutfitItem
{
    string ClassName = "";
    string SlotName = "";
    int Quantity = 0;
    float Health01 = -1;
    ref array<ref OZT_OutfitItem> Inside;

    OZT_OutfitItem Copy()
    {
        OZT_OutfitItem c = new OZT_OutfitItem();
        c.ClassName = ClassName;
        c.SlotName = SlotName;
        c.Quantity = Quantity;
        c.Health01 = Health01;
        c.Inside = new array<ref OZT_OutfitItem>();
        if (Inside)
        {
            for (int i = 0; i < Inside.Count(); i++)
            {
                if (Inside[i])
                    c.Inside.Insert(Inside[i].Copy());
            }
        }
        return c;
    }
}

class OZT_Outfit
{
    ref array<ref OZT_OutfitItem> Items;

    OZT_Outfit Copy()
    {
        OZT_Outfit c = new OZT_Outfit();
        c.Items = new array<ref OZT_OutfitItem>();
        if (Items)
        {
            for (int i = 0; i < Items.Count(); i++)
            {
                if (Items[i])
                    c.Items.Insert(Items[i].Copy());
            }
        }
        return c;
    }
}

class OZT_Point
{
    string Id = "";
    string Name = "";
    string Kind = "object";
    string ClassName = "";

    // A map point's model, for a prop the map has no class for (its class is
    // then ""): the thing is known by its model and its place.
    string Model = "";
    ref array<float> Pos;
    float Yaw = 0;
    float Radius = 3;
    ref array<string> Categories;
    float BuyFactor = 1;
    float SellFactor = 1;
    string Provider = "";

    // The stock this point sells from (design §11.1): its own id when empty;
    // points that name one stock share one shelf.
    string Stock = "";

    // 1: this trader does not change the server's banknotes; 0, and a file that
    // does not say: it does (the screen's change tab, at face value). Said this
    // way round because a key a nested object lacks reads zero. Only item money
    // has notes to change.
    int NoExchange = 0;

    // What a trader of kind npc wears (§9.3 of the design); nothing for an
    // object.
    ref OZT_Outfit Outfit;

    // Who may trade here at all (§8.2), and the gate families switched on or
    // off for this trader against the server's GatesOff.
    ref array<ref OZT_Condition> Access;
    ref array<string> GatesOn;
    ref array<string> GatesOff;

    string StockId()
    {
        if (Stock != "")
            return Stock;
        return Id;
    }

    OZT_Point Copy()
    {
        OZT_Point c = new OZT_Point();
        c.Id = Id;
        c.Name = Name;
        c.Kind = Kind;
        c.ClassName = ClassName;
        c.Model = Model;
        c.Pos = new array<float>();
        if (Pos)
        {
            for (int i = 0; i < Pos.Count(); i++)
                c.Pos.Insert(Pos[i]);
        }
        c.Yaw = Yaw;
        c.Radius = Radius;
        c.Categories = new array<string>();
        if (Categories)
        {
            for (int k = 0; k < Categories.Count(); k++)
                c.Categories.Insert(Categories[k]);
        }
        c.BuyFactor = BuyFactor;
        c.SellFactor = SellFactor;
        c.Provider = Provider;
        c.Stock = Stock;
        c.NoExchange = NoExchange;
        if (Outfit)
            c.Outfit = Outfit.Copy();
        else
            c.Outfit = new OZT_Outfit();
        c.Access = new array<ref OZT_Condition>();
        OZT_Condition.CopyAll(Access, c.Access);
        c.GatesOn = new array<string>();
        c.GatesOff = new array<string>();
        if (GatesOn)
        {
            for (int g = 0; g < GatesOn.Count(); g++)
                c.GatesOn.Insert(GatesOn[g]);
        }
        if (GatesOff)
        {
            for (int h = 0; h < GatesOff.Count(); h++)
                c.GatesOff.Insert(GatesOff[h]);
        }
        return c;
    }

    vector Position()
    {
        return Vector(Pos[0], Pos[1], Pos[2]);
    }
}

class OZT_TradersFile : OZ_ConfigBase
{
    ref array<ref OZT_Point> Traders;

    override int LatestVersion()
    {
        return 1;
    }

    override void LoadDefaults()
    {
        super.LoadDefaults();
        Traders = new array<ref OZT_Point>();
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        array<ref OZT_Point> kept = new array<ref OZT_Point>();
        array<string> ids = new array<string>();
        if (Traders)
        {
            for (int i = 0; i < Traders.Count(); i++)
            {
                OZT_Point raw = Traders[i];
                if (!raw)
                    continue;
                OZT_Point p = raw.Copy();
                string why = "";
                if (!OZT_Ids.IsSafe(p.Id))
                    why = "its Id must be letters, digits, _ and - only, at most 64";
                else if (ids.Find(p.Id) > -1)
                    why = "the Id is listed twice";
                else if (p.ClassName == "" && (p.Kind != OZT_Const.KIND_MAP || p.Model == ""))
                    why = "it has no ClassName";
                else if (p.Pos.Count() != 3)
                    why = "Pos must be three numbers";
                if (why != "")
                {
                    OZT_Log.Warn("Traders: the point '" + p.Id + "' is dropped: " + why);
                    warnings++;
                    continue;
                }
                ids.Insert(p.Id);

                bool known = p.Kind == OZT_Const.KIND_OBJECT || p.Kind == OZT_Const.KIND_NPC;
                if (!known && p.Kind != OZT_Const.KIND_MAP)
                {
                    OZT_Log.Warn("Traders: '" + p.Id + "' has the kind '" + p.Kind + "'; it is \"object\", \"npc\" or \"map\". Using \"object\"");
                    p.Kind = OZT_Const.KIND_OBJECT;
                    warnings++;
                }

                // A nested field the file did not carry reads zero after the
                // copy: zero cannot be meant for any of these three.
                if (p.Radius <= 0)
                    p.Radius = 3;
                if (p.BuyFactor <= 0)
                    p.BuyFactor = 1;
                if (p.SellFactor <= 0)
                    p.SellFactor = 1;
                if (p.Stock != "" && !OZT_Ids.IsSafe(p.Stock))
                {
                    OZT_Log.Warn("Traders: '" + p.Id + "' names the stock '" + p.Stock + "': letters, digits, _ and - only. Using its own");
                    p.Stock = "";
                    warnings++;
                }
                kept.Insert(p);
            }
        }
        Traders = kept;
    }
}
