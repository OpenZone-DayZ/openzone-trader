// What the admin section `traders` and the pane TRADERS say to each other
// (design §9.5). The section lives in this mod; the pane in the glue pbo
// OpenZone_Trader_VPP. 3_Game, because both sides read it.
//
// Ops: `list`; `place` with an OZT_AdminPlace; `move:<id>`, `turn:<id>`,
// `remove:<id>`, `outfit:<id>`, `respawn`; `set:<id>` with an OZT_AdminSet.
// Every op that changes something answers with the list as it is after.

// One point as the pane shows it: the file's values, and whether something
// stands there now.
class OZT_AdminPoint
{
    string Id = "";
    string Name = "";
    string Kind = "";
    string ClassName = "";
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

// The values of a point the pane edits; the place, the facing and the outfit
// have ops of their own.
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
