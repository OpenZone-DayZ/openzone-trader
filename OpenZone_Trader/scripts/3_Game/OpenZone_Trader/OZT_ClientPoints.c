// The trader points a client knows: what the trade action needs to offer itself.
//
// They come in the core's sync packet (OZT_Points.Fill on the server) and live
// in 3_Game, because the action's condition is 4_World and the packet's reader
// (OZ_ClientState) is 5_Mission. The client knows where a point stands, what
// stands there and how far it reaches; who may trade there is the server's to
// judge when the action runs.

class OZT_ClientPoint
{
    string Id;
    string ClassName;
    vector Pos;
    float Radius;
}

class OZT_ClientPoints
{
    // The points in pieces, each value of the packet shorter than the 1023
    // bytes the engine cuts a JSON string at: "id|class|x|y|z|radius", points
    // joined by ";".
    static const string SYNC_POINTS   = "trader.points.";
    static const string SYNC_POINTS_N = "trader.points";
    static const int    CHUNK_CHARS   = 900;

    // How far, flat, the thing at a point may stand from the point's position:
    // a character settles on the ground it was put on.
    static const float NEAR = 1.0;

    // The key of a prop the map has no class for: its model, after this.
    static const string MODEL_KEY = "p3d:";

    private static ref array<ref OZT_ClientPoint> s_Points = new array<ref OZT_ClientPoint>();

    static void Clear()
    {
        s_Points.Clear();
    }

    // One piece of the packet, added to what was read before it.
    static void AddChunk(string chunk)
    {
        if (chunk == "")
            return;
        array<string> lines = new array<string>();
        chunk.Split(";", lines);
        for (int i = 0; i < lines.Count(); i++)
        {
            array<string> f = new array<string>();
            lines[i].Split("|", f);
            if (f.Count() != 6)
                continue;
            OZT_ClientPoint p = new OZT_ClientPoint();
            p.Id = f[0];
            p.ClassName = f[1];
            p.Pos = Vector(f[2].ToFloat(), f[3].ToFloat(), f[4].ToFloat());
            p.Radius = f[5].ToFloat();
            s_Points.Insert(p);
        }
    }

    static int Count()
    {
        return s_Points.Count();
    }

    // The point a thing in the world stands at, by its class and its place;
    // null when it stands at none.
    static OZT_ClientPoint At(Object obj)
    {
        if (!obj)
            return null;
        string type = KeyOf(obj);
        vector at = obj.GetPosition();
        for (int i = 0; i < s_Points.Count(); i++)
        {
            OZT_ClientPoint p = s_Points[i];
            if (p.ClassName != type)
                continue;
            float dx = at[0] - p.Pos[0];
            float dz = at[2] - p.Pos[2];
            if (dx * dx + dz * dz <= NEAR * NEAR)
                return p;
        }
        return null;
    }

    // What names a thing on both sides: its class, or -- for a prop of the map
    // that has none (a map point) -- its model behind MODEL_KEY.
    static string KeyOf(Object obj)
    {
        string type = obj.GetType();
        if (type == "")
            type = MODEL_KEY + obj.GetShapeName();
        return type;
    }

    // One point as a line of the packet. The server writes with it, so both
    // sides read the same shape.
    static string Line(string id, string type, vector pos, float radius)
    {
        string s = id + "|" + type + "|" + pos[0].ToString() + "|" + pos[1].ToString();
        s = s + "|" + pos[2].ToString() + "|" + radius.ToString();
        return s;
    }
}
