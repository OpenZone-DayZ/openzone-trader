// A gate provider of a made-up family, the way a foreign mod brings its own
// kinds (design §8.2): a pbo that requires the trader, holds one class and
// registers it. The stand sets its words per player with the verb:
//
//   {"op": "flag", "flag": "vip", "value": "1"}    on (0 clears it)
//   {"op": "flag", "flag": "rep", "value": "25"}   a percent for stand_pct
//
// Kinds:
//   stand      Arg is a flag the verb set for the player
//   stand_pct  Arg is a flag; the provider's percent is the value set for it
//
// A server without this stand pbo has no provider for these kinds, and every
// condition of them passes: the stand's files stay usable on a real server.

class OZT_StandGate : OZT_GateProvider
{
    protected static ref map<string, float> s_Flags = new map<string, float>();

    override string Family()
    {
        return "stand";
    }

    override void Kinds(array<string> kinds)
    {
        kinds.Insert("stand");
        kinds.Insert("stand_pct");
    }

    override bool Holds(PlayerBase player, string kind, string arg)
    {
        return s_Flags.Contains(Key(player, arg));
    }

    override float Percent(PlayerBase player, string kind, string arg)
    {
        float v;
        if (s_Flags.Find(Key(player, arg), v))
            return v;
        return 0;
    }

    static void Set(PlayerBase player, string flag, float value)
    {
        string k = Key(player, flag);
        if (value == 0)
            s_Flags.Remove(k);
        else
            s_Flags.Set(k, value);
    }

    static string Key(PlayerBase player, string flag)
    {
        string uid = "";
        if (player && player.GetIdentity())
            uid = player.GetIdentity().GetPlainId();
        return uid + ":" + flag;
    }

    static bool OpFlag(PlayerBase player, map<string, string> args, out string detail)
    {
        string flag = OZT_VerbOps.Arg(args, "flag", "");
        float value = OZT_VerbOps.Arg(args, "value", "1").ToFloat();
        if (flag == "")
        {
            detail = "flag: name a flag";
            return false;
        }
        Set(player, flag, value);
        detail = "flag: '" + flag + "' = " + value.ToString() + " for the player";
        return true;
    }
}

// The stand's provider stands beside the trader's own from the mission's start.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        OZT_Gates.Provide(new OZT_StandGate());
    }
}
