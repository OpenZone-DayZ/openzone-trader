// Trader points in the world: what stands at each, and how far a player is.
//
// The server makes the object at every start and after every reload, and it is
// never saved to the world (ECE_NOPERSISTENCY_WORLD; a building class is not
// saved anyway). So no class has to be ours, and a point removed from the file
// leaves nothing behind.
//
// WHERE IT STANDS. `Pos` of a point is the object's own position, exactly: what
// the engine reports for a placed object, what an editor exports, what the
// game's own object spawner takes (objectspawner.c) -- for a building the middle
// of its model, not its foot. The object is created there the spawner's way: no
// tracing, no placing on a surface. Both other ways were measured on the stand
// and are not used. The engine's ECE_PLACE_ON_SURFACE put the same stall in two
// places: at mission start on the ground, two and a half metres aside of the
// asked point; after a reload at the asked point with its middle at ground
// level, half buried. And a model's bounding box does not say where its foot
// is: a building's box includes the foundation it carries below the ground.

class OZT_PointLive
{
    string Id;
    Object Body;

    // The point's position, as the file gives it. Distances are measured from
    // here; for a tall model that is metres above the ground.
    vector Stands;
}

class OZT_Points
{
    private static ref array<ref OZT_PointLive> s_Live = new array<ref OZT_PointLive>();

    static void Respawn()
    {
        Remove();
        array<ref OZT_Point> traders = OZT_Config.Get().Traders().Traders;
        for (int i = 0; i < traders.Count(); i++)
        {
            OZT_Point p = traders[i];
            vector at = p.Position();

            // The object spawner's own flags, without its persistence.
            int flags = ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD;
            Object body = GetGame().CreateObjectEx(p.ClassName, at, flags, RF_IGNORE);
            if (!body)
            {
                OZT_Log.Error("point '" + p.Id + "': the game made nothing of class '" + p.ClassName + "'");
                continue;
            }
            body.SetOrientation(Vector(p.Yaw, 0, 0));

            OZT_PointLive live = new OZT_PointLive();
            live.Id = p.Id;
            live.Body = body;
            live.Stands = at;
            s_Live.Insert(live);

            // The place it really took, read back from the engine: it must be
            // the place the file asked for, at the start and after every reload.
            vector took = body.GetPosition();
            string line = "point '" + p.Id + "' (" + p.ClassName + ") stands at " + took.ToString();
            OZT_Log.Info(line);
        }
    }

    static void Remove()
    {
        for (int i = 0; i < s_Live.Count(); i++)
        {
            if (s_Live[i].Body)
                GetGame().ObjectDelete(s_Live[i].Body);
        }
        s_Live.Clear();
    }

    // How far the player stands from the point, in metres. Below zero: the
    // point does not stand.
    static float Away(string id, PlayerBase player)
    {
        for (int i = 0; i < s_Live.Count(); i++)
        {
            if (s_Live[i].Id != id)
                continue;
            vector there = s_Live[i].Stands;
            vector here = player.GetPosition();
            return vector.Distance(here, there);
        }
        return -1;
    }

    static int Standing()
    {
        return s_Live.Count();
    }
}
