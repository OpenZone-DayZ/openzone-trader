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

            Object body;
            if (p.Kind == OZT_Const.KIND_NPC)
            {
                body = MakeNpc(p, at);
            }
            else
            {
                // The object spawner's own flags, without its persistence.
                int flags = ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD;
                body = GetGame().CreateObjectEx(p.ClassName, at, flags, RF_IGNORE);
            }
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

        // The clients' list of points rides in the core's sync packet: after a
        // reload everyone in the game gets it again. At the mission's start
        // nobody is there yet, and the first packet of each client has it.
        OZ_SyncSender.SendAll("trader points");
    }

    // The points in the core's sync packet, in pieces under the engine's
    // limit for one value (OZT_ClientPoints). The class is the one the thing
    // standing there really has: the file may spell it in another case.
    static void Fill(OZ_SyncPayload p)
    {
        string chunk = "";
        int chunks = 0;
        for (int i = 0; i < s_Live.Count(); i++)
        {
            OZT_PointLive live = s_Live[i];
            OZT_Point point = OZT_Config.Get().FindTrader(live.Id);
            if (!point || !live.Body)
                continue;
            string line = OZT_ClientPoints.Line(live.Id, live.Body.GetType(), live.Stands, point.Radius);
            if (chunk != "" && chunk.Length() + line.Length() + 1 > OZT_ClientPoints.CHUNK_CHARS)
            {
                OZ_SyncExtras.Put(p, OZT_ClientPoints.SYNC_POINTS + chunks.ToString(), chunk);
                chunks++;
                chunk = "";
            }
            if (chunk != "")
                chunk = chunk + ";";
            chunk = chunk + line;
        }
        if (chunk != "")
        {
            OZ_SyncExtras.Put(p, OZT_ClientPoints.SYNC_POINTS + chunks.ToString(), chunk);
            chunks++;
        }
        OZ_SyncExtras.Put(p, OZT_ClientPoints.SYNC_POINTS_N, chunks.ToString());
    }

    // The thing the server made at a point; null when the point does not stand.
    static Object BodyOf(string id)
    {
        for (int i = 0; i < s_Live.Count(); i++)
        {
            if (s_Live[i].Id == id)
                return s_Live[i].Body;
        }
        return null;
    }

    // The point a thing in the world is: the thing the server made there.
    static string IdOf(Object obj)
    {
        if (!obj)
            return "";
        for (int i = 0; i < s_Live.Count(); i++)
        {
            if (s_Live[i].Body == obj)
                return s_Live[i].Id;
        }
        return "";
    }

    // The standing point nearest to the player whose radius reaches them; ""
    // when none does. The action's last resort when its target is gone by the
    // time it runs (it runs from an animation event, not the frame of its
    // condition).
    static string Nearest(PlayerBase player)
    {
        string best = "";
        float bestAway = -1;
        for (int i = 0; i < s_Live.Count(); i++)
        {
            OZT_Point point = OZT_Config.Get().FindTrader(s_Live[i].Id);
            if (!point)
                continue;
            float away = vector.Distance(player.GetPosition(), s_Live[i].Stands);
            if (away > point.Radius)
                continue;
            if (bestAway < 0 || away < bestAway)
            {
                best = s_Live[i].Id;
                bestAway = away;
            }
        }
        return best;
    }

    // Does the point stand and does its radius reach the player?
    static bool Reaches(string id, PlayerBase player)
    {
        if (id == "" || !player)
            return false;
        OZT_Point point = OZT_Config.Get().FindTrader(id);
        if (!point)
            return false;
        float away = Away(id, player);
        if (away < 0)
            return false;
        return away <= point.Radius;
    }

    // May this player trade at this point now? "" or the key of the refusal.
    // Who may trade at all (the point's Access) is phase 5's.
    static string MayTrade(string id, PlayerBase player)
    {
        if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained())
            return OZT_Const.NO_BUSY;
        OZT_Point point = OZT_Config.Get().FindTrader(id);
        float away = Away(id, player);
        if (!point || away < 0)
            return OZT_Const.NO_CLOSED;
        if (away > point.Radius)
            return OZT_Const.NO_TOO_FAR;
        if (!OZT_Gates.Admits(player, point))
            return OZT_Const.NO_ACCESS;
        return "";
    }

    // A trader's character: an ordinary survivor that the server makes and
    // never selects for a player, so nothing of a player's runs for it -- no
    // modifiers, no action manager, no senses of the infected aimed at it
    // (design §9.2; what else keeps it quiet is OZT_Npc.c). The way the
    // studied trader mods make theirs: created where it stands, turned, made
    // to take no damage, dressed.
    protected static PlayerBase MakeNpc(OZT_Point p, vector at)
    {
        Object made = GetGame().CreateObject(p.ClassName, at, false, false, true);
        PlayerBase npc = PlayerBase.Cast(made);
        if (!npc)
        {
            if (made)
                GetGame().ObjectDelete(made);
            return null;
        }
        npc.SetPosition(at);
        npc.SetOrientation(Vector(p.Yaw, 0, 0));
        npc.SetAllowDamage(false);
        Dress(npc, p.Id, p.Outfit);
        return npc;
    }

    // The point's outfit through the core's dresser (OZ_LoadoutApply), which
    // strips first and says every class it cannot make.
    protected static void Dress(PlayerBase npc, string id, OZT_Outfit outfit)
    {
        if (!outfit || !outfit.Items || outfit.Items.Count() == 0)
            return;
        OZ_LoadoutPreset preset = new OZ_LoadoutPreset();
        preset.Id = "trader " + id;
        for (int i = 0; i < outfit.Items.Count(); i++)
        {
            if (outfit.Items[i])
                preset.Items.Insert(LoadoutOf(outfit.Items[i]));
        }
        OZ_LoadoutApply.To(npc, preset);
    }

    protected static OZ_LoadoutItem LoadoutOf(OZT_OutfitItem src)
    {
        OZ_LoadoutItem it = new OZ_LoadoutItem();
        it.ClassName = src.ClassName;
        it.SlotName = src.SlotName;
        it.Quantity = src.Quantity;
        // Zero is what a missing key reads: left alone, never ruined.
        it.Health01 = src.Health01;
        if (it.Health01 <= 0)
            it.Health01 = -1;
        it.QuickBar = -1;
        if (src.Inside)
        {
            for (int k = 0; k < src.Inside.Count(); k++)
            {
                if (src.Inside[k])
                    it.Inside.Insert(LoadoutOf(src.Inside[k]));
            }
        }
        return it;
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
