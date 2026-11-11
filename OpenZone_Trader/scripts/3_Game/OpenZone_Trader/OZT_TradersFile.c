// OZ_Trader_Traders.json: the trader points.
//
// A point is where a trader stands and what it deals in: `Kind` "object" (any
// class: a counter, a stall, a terminal) or "npc" (a character, phase 4), the
// class, position and facing, the radius a player must stand within, the
// categories of the price book it deals in, a factor on each side of its
// prices, and its own money provider ("" means the server's).

class OZT_Point
{
    string Id = "";
    string Name = "";
    string Kind = "object";
    string ClassName = "";
    ref array<float> Pos;
    float Yaw = 0;
    float Radius = 3;
    ref array<string> Categories;
    float BuyFactor = 1;
    float SellFactor = 1;
    string Provider = "";

    OZT_Point Copy()
    {
        OZT_Point c = new OZT_Point();
        c.Id = Id;
        c.Name = Name;
        c.Kind = Kind;
        c.ClassName = ClassName;
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
                else if (p.ClassName == "")
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

                if (p.Kind != OZT_Const.KIND_OBJECT && p.Kind != OZT_Const.KIND_NPC)
                {
                    OZT_Log.Warn("Traders: '" + p.Id + "' has the kind '" + p.Kind + "'; it is \"object\" or \"npc\". Using \"object\"");
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
                kept.Insert(p);
            }
        }
        Traders = kept;
    }
}
