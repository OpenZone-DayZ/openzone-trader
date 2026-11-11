// The research gate (design §8.2): one provider among equals, in a glue pbo.
//
//   research  the player's faction has completed the node Arg
//
// The owner of research progress is the faction (OZL_Owner.OfPlayer), so a
// lock opens for the whole faction. Known() is asked before Get(): Get creates
// the owner's file, and research reads every owner's file at its start, so an
// owner it does not know has completed nothing. Node ids are the admin's and
// are checked against the tree at the trader's ready tick; a node the tree no
// longer has is said there.

class OZT_ResearchGate : OZT_GateProvider
{
    override string Family()
    {
        return "research";
    }

    override void Kinds(array<string> kinds)
    {
        kinds.Insert("research");
    }

    override bool Holds(PlayerBase player, string kind, string arg)
    {
        string owner = OZL_Owner.OfPlayer(player);
        if (owner == "" || !OZL_State.Known(owner))
            return false;
        OZL_FactionState state = OZL_State.Get(owner);
        if (!state)
            return false;
        return state.IsCompleted(arg);
    }

    override bool Knows(string kind, string arg, out string why)
    {
        why = "";
        array<string> alts = new array<string>();
        arg.Split("|", alts);
        for (int i = 0; i < alts.Count(); i++)
        {
            string id = alts[i].Trim();
            if (!OZL_Config.Get().Tree().FindNode(id))
            {
                why = "the research tree has no node '" + id + "'";
                return false;
            }
        }
        return true;
    }

    override string Label(string kind, string arg)
    {
        OZL_TreeNode node = OZL_Config.Get().Tree().FindNode(arg);
        if (node && node.Name != "")
            return node.Name;
        return arg;
    }
}

// Registered at the mission's start, beside the trader's own providers.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        OZT_Gates.Provide(new OZT_ResearchGate());
    }
}
