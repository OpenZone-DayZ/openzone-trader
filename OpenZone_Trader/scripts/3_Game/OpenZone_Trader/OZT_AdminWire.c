// What the admin section `traders` and the admin window's TRADER pages say to
// each other (design §9.5). The section lives in this mod; the pages in the glue
// pbo OpenZone_Trader_VPP. 3_Game, because both sides read it.
//
// Ops: `list`; `place` with an OZT_AdminPlace; `adopt` with an OZT_AdminAdopt;
// `move:<id>`, `turn:<id>`, `remove:<id>`, `outfit:<id>`, `respawn`; `set:<id>`
// with an OZT_AdminSet. Every op that changes something answers with the list
// as it is after.

// One point as the pane shows it: the file's values, and whether something
// stands there now.
class OZT_AdminPoint
{
    string Id = "";
    string Name = "";
    string Kind = "";
    string ClassName = "";
    // A map point's model: the name of a prop the map has no class for.
    string Model = "";
    float X = 0;
    float Y = 0;
    float Z = 0;
    float Yaw = 0;
    float Radius = 0;
    ref array<string> Categories;
    float BuyFactor = 1;
    float SellFactor = 1;
    string Provider = "";
    int NoExchange = 0;
    // The things the character wears, counted at the top.
    int Worn = 0;
    // 1: the server made something there.
    int Standing = 0;

    void OZT_AdminPoint()
    {
        Categories = new array<string>();
    }
}

class OZT_AdminList
{
    ref array<ref OZT_AdminPoint> Points;
    // The categories of the price book, for the pane's choice.
    ref array<ref OZT_WireCategory> Categories;

    void OZT_AdminList()
    {
        Points = new array<ref OZT_AdminPoint>();
        Categories = new array<ref OZT_WireCategory>();
    }
}

// A new point where the admin stands, facing where the admin faces.
class OZT_AdminPlace
{
    string Id = "";
    string Name = "";
    string Kind = "object";
    string ClassName = "";
}

// A thing already in the world made a trader (kind map): what the admin's
// client saw where the admin looks -- its class (empty for a prop the map has
// no class for), its model and its position. The server finds it again there
// and takes nothing else from the client.
class OZT_AdminAdopt
{
    string Id = "";
    string Name = "";
    string ClassName = "";
    string Model = "";
    float X = 0;
    float Y = 0;
    float Z = 0;
}

// The values of a point the pane edits; the place, the facing and the outfit
// have ops of their own.
// Grant or take back a licence (design §11.2): `Player` is a Steam id or the
// name of a player online.
class OZT_AdminLicence
{
    string Player = "";
    string Id = "";
    int Grant = 1;
}

// The TraderPlus import (design §11.3): a dry run unless Apply is 1; Root is
// the folder of TraderPlus's files under the profile, "" for its own.
class OZT_AdminImport
{
    int Apply = 0;
    string Root = "";
}

class OZT_AdminSet
{
    string Name = "";
    string Kind = "object";
    string ClassName = "";
    float Radius = 3;
    ref array<string> Categories;
    float BuyFactor = 1;
    float SellFactor = 1;
    string Provider = "";
    int NoExchange = 0;

    void OZT_AdminSet()
    {
        Categories = new array<string>();
    }
}

// ---- the admin window's pages (VPP rework 2026-11-12, design §4.3 and §5.6) ----
//
// Read ops: `stock_list`, `licence_list`, `problems`, `gates`, `gate_holders` (with
// an OZT_AdminGateAsk). Writes: `stock_set` with an OZT_AdminStockSet (answers the
// stock list), `import_traderplus` (answers OZT_AdminImportReport, failed or not).
//
// A string travels whole only below 1023 bytes (dayz-modding: the serializer cuts
// a longer value), so every text that can grow goes as a list of lines, each cut
// by the server (OZT_AdminText.Cut). Each class has a Copy(): what the serializer
// made lives only as long as its parse (the core's OZ_ConfigBase header), so the
// client takes a copy at once and keeps that.

class OZT_AdminText
{
    static const int LINE_MAX = 900;

    // At most LINE_MAX bytes, cut between two characters (the core's cutter).
    static string Cut(string line)
    {
        if (line.Length() <= LINE_MAX)
            return line;
        int end = OZ_Rpc.CutSafe(line, 0, LINE_MAX);
        return line.Substring(0, end);
    }

    static void CopyAll(array<string> from, array<string> into)
    {
        if (!from || !into)
            return;
        for (int i = 0; i < from.Count(); i++)
            into.Insert(from[i]);
    }
}

// One stocked line at a point: its count in each condition, pristine first
// (levels 0..3, OZT_Const.LEVEL_RUINED is never stocked), and its Max.
class OZT_AdminStockLine
{
    string ClassName = "";
    int Max = 0;
    ref array<int> Counts;

    void OZT_AdminStockLine()
    {
        Counts = new array<int>();
    }

    OZT_AdminStockLine Copy()
    {
        OZT_AdminStockLine c = new OZT_AdminStockLine();
        c.ClassName = ClassName;
        c.Max = Max;
        if (Counts)
        {
            for (int i = 0; i < Counts.Count(); i++)
                c.Counts.Insert(Counts[i]);
        }
        return c;
    }
}

class OZT_AdminStockCat
{
    string Id = "";
    string Name = "";
    ref array<ref OZT_AdminStockLine> Lines;

    void OZT_AdminStockCat()
    {
        Lines = new array<ref OZT_AdminStockLine>();
    }

    OZT_AdminStockCat Copy()
    {
        OZT_AdminStockCat c = new OZT_AdminStockCat();
        c.Id = Id;
        c.Name = Name;
        if (Lines)
        {
            for (int i = 0; i < Lines.Count(); i++)
            {
                if (Lines[i])
                    c.Lines.Insert(Lines[i].Copy());
            }
        }
        return c;
    }
}

// A point, the stock it sells from (points that name one stock share one shelf),
// and its categories that hold stocked lines.
class OZT_AdminStockPoint
{
    string Id = "";
    string Name = "";
    string Stock = "";
    ref array<ref OZT_AdminStockCat> Cats;

    void OZT_AdminStockPoint()
    {
        Cats = new array<ref OZT_AdminStockCat>();
    }

    OZT_AdminStockPoint Copy()
    {
        OZT_AdminStockPoint c = new OZT_AdminStockPoint();
        c.Id = Id;
        c.Name = Name;
        c.Stock = Stock;
        if (Cats)
        {
            for (int i = 0; i < Cats.Count(); i++)
            {
                if (Cats[i])
                    c.Cats.Insert(Cats[i].Copy());
            }
        }
        return c;
    }
}

// `stock_list`: per point and category, the stocked lines; the settings' stock
// switches beside them. Cut 1: the list stopped at the server's bound.
class OZT_AdminStockList
{
    ref array<ref OZT_AdminStockPoint> Points;
    int StartStock = 0;
    int DestockAtStart = 0;
    int OnlyPristine = 0;
    float Floor = 0;
    int Cut = 0;

    void OZT_AdminStockList()
    {
        Points = new array<ref OZT_AdminStockPoint>();
    }

    OZT_AdminStockList Copy()
    {
        OZT_AdminStockList c = new OZT_AdminStockList();
        if (Points)
        {
            for (int i = 0; i < Points.Count(); i++)
            {
                if (Points[i])
                    c.Points.Insert(Points[i].Copy());
            }
        }
        c.StartStock = StartStock;
        c.DestockAtStart = DestockAtStart;
        c.OnlyPristine = OnlyPristine;
        c.Floor = Floor;
        c.Cut = Cut;
        return c;
    }
}

// `stock_set`: the count of one stocked line of a point's category, in one
// condition (0 pristine .. 3 badly damaged), the bridge verb `stock` as an admin op.
class OZT_AdminStockSet
{
    string Point = "";
    string Category = "";
    string ClassName = "";
    int Level = 0;
    int Count = 0;
}

// One licence of the settings: where it is sold ("<category>|<price>") and what
// it opens ("point|<id>", "category|<id>", "line|<class>|<category>",
// "discount|<category>", "discount|<class>|<category>"), and how many hold it.
class OZT_AdminLicenceDef
{
    string Id = "";
    string Name = "";
    string Look = "";
    int Holders = 0;
    ref array<string> Sold;
    ref array<string> Opens;

    void OZT_AdminLicenceDef()
    {
        Sold = new array<string>();
        Opens = new array<string>();
    }

    OZT_AdminLicenceDef Copy()
    {
        OZT_AdminLicenceDef c = new OZT_AdminLicenceDef();
        c.Id = Id;
        c.Name = Name;
        c.Look = Look;
        c.Holders = Holders;
        OZT_AdminText.CopyAll(Sold, c.Sold);
        OZT_AdminText.CopyAll(Opens, c.Opens);
        return c;
    }
}

// Who holds licences: a player online by name; anyone else by the Steam ID, which
// the server sends for the page only and writes in no log.
class OZT_AdminLicenceHolder
{
    string Name = "";
    string Uid = "";
    int Online = 0;
    ref array<string> Ids;

    void OZT_AdminLicenceHolder()
    {
        Ids = new array<string>();
    }

    OZT_AdminLicenceHolder Copy()
    {
        OZT_AdminLicenceHolder c = new OZT_AdminLicenceHolder();
        c.Name = Name;
        c.Uid = Uid;
        c.Online = Online;
        OZT_AdminText.CopyAll(Ids, c.Ids);
        return c;
    }
}

// `licence_list`: the licences, who holds them, and the names of the players online.
class OZT_AdminLicenceList
{
    ref array<ref OZT_AdminLicenceDef> Defs;
    ref array<ref OZT_AdminLicenceHolder> Holders;
    ref array<string> Online;
    int Cut = 0;

    void OZT_AdminLicenceList()
    {
        Defs = new array<ref OZT_AdminLicenceDef>();
        Holders = new array<ref OZT_AdminLicenceHolder>();
        Online = new array<string>();
    }

    OZT_AdminLicenceList Copy()
    {
        OZT_AdminLicenceList c = new OZT_AdminLicenceList();
        int i;
        if (Defs)
        {
            for (i = 0; i < Defs.Count(); i++)
            {
                if (Defs[i])
                    c.Defs.Insert(Defs[i].Copy());
            }
        }
        if (Holders)
        {
            for (i = 0; i < Holders.Count(); i++)
            {
                if (Holders[i])
                    c.Holders.Insert(Holders[i].Copy());
            }
        }
        OZT_AdminText.CopyAll(Online, c.Online);
        c.Cut = Cut;
        return c;
    }
}

// `import_traderplus`: the report a line a row; Done 0 when TraderPlus's files
// could not be read (the op fails then, and still carries the report).
class OZT_AdminImportReport
{
    int Apply = 0;
    int Done = 0;
    ref array<string> Lines;

    void OZT_AdminImportReport()
    {
        Lines = new array<string>();
    }

    OZT_AdminImportReport Copy()
    {
        OZT_AdminImportReport c = new OZT_AdminImportReport();
        c.Apply = Apply;
        c.Done = Done;
        OZT_AdminText.CopyAll(Lines, c.Lines);
        return c;
    }
}

// `problems`: the warnings of the files' last load, since it started (the first
// OZT_Log.PROBLEMS_KEPT of them); Count is all of them; the ready line's counters.
class OZT_AdminProblems
{
    int Count = 0;
    int Cut = 0;
    string Counters = "";
    ref array<string> Lines;

    void OZT_AdminProblems()
    {
        Lines = new array<string>();
    }

    OZT_AdminProblems Copy()
    {
        OZT_AdminProblems c = new OZT_AdminProblems();
        c.Count = Count;
        c.Cut = Cut;
        c.Counters = Counters;
        OZT_AdminText.CopyAll(Lines, c.Lines);
        return c;
    }
}

// One kind of condition and the values an admin may pick for it, each with what
// the admin reads (OZT_GateProvider.Choices); none: a free word.
class OZT_AdminGateKind
{
    string Kind = "";
    int Cut = 0;
    ref array<string> Values;
    ref array<string> Labels;

    void OZT_AdminGateKind()
    {
        Values = new array<string>();
        Labels = new array<string>();
    }

    OZT_AdminGateKind Copy()
    {
        OZT_AdminGateKind c = new OZT_AdminGateKind();
        c.Kind = Kind;
        c.Cut = Cut;
        OZT_AdminText.CopyAll(Values, c.Values);
        OZT_AdminText.CopyAll(Labels, c.Labels);
        return c;
    }
}

// A gate provider: its family, whether its side is there today (Answers 1), and
// its kinds.
class OZT_AdminGateFamily
{
    string Family = "";
    int Answers = 0;
    ref array<ref OZT_AdminGateKind> Kinds;

    void OZT_AdminGateFamily()
    {
        Kinds = new array<ref OZT_AdminGateKind>();
    }

    OZT_AdminGateFamily Copy()
    {
        OZT_AdminGateFamily c = new OZT_AdminGateFamily();
        c.Family = Family;
        c.Answers = Answers;
        if (Kinds)
        {
            for (int i = 0; i < Kinds.Count(); i++)
            {
                if (Kinds[i])
                    c.Kinds.Insert(Kinds[i].Copy());
            }
        }
        return c;
    }
}

// `gates`: every gate provider the server has.
class OZT_AdminGates
{
    ref array<ref OZT_AdminGateFamily> Families;

    void OZT_AdminGates()
    {
        Families = new array<ref OZT_AdminGateFamily>();
    }

    OZT_AdminGates Copy()
    {
        OZT_AdminGates c = new OZT_AdminGates();
        if (Families)
        {
            for (int i = 0; i < Families.Count(); i++)
            {
                if (Families[i])
                    c.Families.Insert(Families[i].Copy());
            }
        }
        return c;
    }
}

// `gate_holders`: who holds the condition `Kind Arg` now (OZT_GateProvider.Holders).
// Answers 0: no provider answers the kind here today.
class OZT_AdminGateAsk
{
    string Kind = "";
    string Arg = "";
}

class OZT_AdminGateHolders
{
    string Kind = "";
    string Arg = "";
    int Answers = 0;
    int Cut = 0;
    ref array<string> Who;

    void OZT_AdminGateHolders()
    {
        Who = new array<string>();
    }

    OZT_AdminGateHolders Copy()
    {
        OZT_AdminGateHolders c = new OZT_AdminGateHolders();
        c.Kind = Kind;
        c.Arg = Arg;
        c.Answers = Answers;
        c.Cut = Cut;
        OZT_AdminText.CopyAll(Who, c.Who);
        return c;
    }
}
