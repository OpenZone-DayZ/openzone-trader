// Licences (design §11.2, the owner 2026-11-12: "as in TraderPlus"): a record on
// the character, not an item; bought as goods named `@licence:<Id>` or given by
// the admin; no expiry. What a licence opens is any gate of the kind `licence`.
//
// One small JSON per player in $profile:OpenZone\TraderLicences, read the first
// time the player is asked about and written on every change.

class OZT_LicenceFile
{
    ref array<string> Ids;

    void OZT_LicenceFile()
    {
        Ids = new array<string>();
    }
}

class OZT_Licences
{
    protected static ref map<string, ref array<string>> s_Held = new map<string, ref array<string>>();

    // "@licence:weapons" -> "weapons"; "" for anything that is no licence.
    static string IdOf(string type)
    {
        string prefix = OZT_Const.LICENCE_PREFIX;
        if (type.IndexOf(prefix) != 0)
            return "";
        return type.Substring(prefix.Length(), type.Length() - prefix.Length());
    }

    static bool IsLicence(string type)
    {
        return IdOf(type) != "";
    }

    // The settings' licence of that id; null: the server has none.
    static OZT_LicenceDef Def(string id)
    {
        array<ref OZT_LicenceDef> defs = OZT_Config.Get().Settings().Licences;
        for (int i = 0; i < defs.Count(); i++)
        {
            if (defs[i].Id == id)
                return defs[i];
        }
        return null;
    }

    protected static string PathOf(string uid)
    {
        return OZT_Const.LICENCE_DIR + "\\" + uid + ".json";
    }

    // What the player holds, read from their file the first time.
    static array<string> Of(string uid)
    {
        array<string> held;
        if (s_Held.Find(uid, held))
            return held;
        held = new array<string>();
        s_Held.Set(uid, held);
        string path = PathOf(uid);
        if (!FileExist(path))
            return held;
        OZT_LicenceFile file = new OZT_LicenceFile();
        string err;
        if (!JsonFileLoader<OZT_LicenceFile>.LoadFile(path, file, err))
        {
            OZT_Log.Error("licences of a player: the file could not be read: " + err);
            return held;
        }
        if (!file.Ids)
            return held;
        for (int i = 0; i < file.Ids.Count(); i++)
        {
            string id = file.Ids[i];
            if (id != "" && held.Find(id) < 0)
                held.Insert(id);
        }
        return held;
    }

    static bool Has(string uid, string id)
    {
        if (uid == "" || id == "")
            return false;
        return Of(uid).Find(id) > -1;
    }

    // False: held already.
    static bool Grant(string uid, string id)
    {
        if (uid == "" || id == "")
            return false;
        array<string> held = Of(uid);
        if (held.Find(id) > -1)
            return false;
        held.Insert(id);
        Save(uid);
        return true;
    }

    // False: not held.
    static bool Revoke(string uid, string id)
    {
        array<string> held = Of(uid);
        int at = held.Find(id);
        if (at < 0)
            return false;
        held.RemoveOrdered(at);
        Save(uid);
        return true;
    }

    protected static void Save(string uid)
    {
        OZT_LicenceFile file = new OZT_LicenceFile();
        array<string> held = Of(uid);
        for (int i = 0; i < held.Count(); i++)
            file.Ids.Insert(held[i]);
        if (!FileExist(OZT_Const.LICENCE_DIR))
            MakeDirectory(OZT_Const.LICENCE_DIR);
        string err;
        if (!JsonFileLoader<OZT_LicenceFile>.SaveFile(PathOf(uid), file, err))
            OZT_Log.Error("licences of a player: the file could not be written: " + err);
    }

    static string UidOf(PlayerBase player)
    {
        if (!player || !player.GetIdentity())
            return "";
        return player.GetIdentity().GetPlainId();
    }

    // A 4_World class held by a static: let go at mission end.
    static void Reset()
    {
        s_Held.Clear();
    }
}

// The gate kind `licence`: answered by this mod itself, always present.
class OZT_LicenceGate : OZT_GateProvider
{
    override string Family()
    {
        return "licences";
    }

    override void Kinds(array<string> kinds)
    {
        kinds.Insert("licence");
    }

    override bool Holds(PlayerBase player, string kind, string arg)
    {
        return OZT_Licences.Has(OZT_Licences.UidOf(player), arg);
    }

    override bool Knows(string kind, string arg, out string why)
    {
        why = "";
        if (OZT_Licences.Def(arg))
            return true;
        why = "the settings list no licence '" + arg + "'";
        return false;
    }

    override string Label(string kind, string arg)
    {
        OZT_LicenceDef def = OZT_Licences.Def(arg);
        if (def && def.Name != "")
            return def.Name;
        return arg;
    }
}
