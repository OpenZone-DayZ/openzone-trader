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

    // For the admin window's picker: every node of the tree, read as
    // "name - branch (its owners) - T<tier>".
    override void Choices(string kind, array<string> values, array<string> labels)
    {
        OZL_TreeConfig tree = OZL_Config.Get().Tree();
        if (!tree || !tree.Branches)
            return;
        OZ_IdentityService ids = OZ_Identity.Get();
        for (int b = 0; b < tree.Branches.Count(); b++)
        {
            OZL_TreeBranch br = tree.Branches[b];
            if (!br)
                continue;
            string owners = "";
            for (int o = 0; o < br.Owners.Count(); o++)
            {
                if (o > 0)
                    owners = owners + ", ";
                owners = owners + ids.FactionName(br.Owners[o]);
            }
            string branch = br.Name;
            if (branch == "")
                branch = br.Id;
            if (owners != "")
                branch = branch + " (" + owners + ")";
            for (int n = 0; n < br.Nodes.Count(); n++)
            {
                OZL_TreeNode node = br.Nodes[n];
                if (!node || node.Id == "")
                    continue;
                string name = node.Name;
                if (name == "")
                    name = node.Id;
                string label = name + " - " + branch + " - T" + node.Tier.ToString();
                values.Insert(node.Id);
                labels.Insert(label);
            }
        }
    }

    // The owners of research (factions) that have completed the node, or any of
    // the alternatives. Research knows every owner it has read (OZL_State.Owners:
    // all state files are read at its start); one it has not met has completed
    // nothing.
    override void Holders(string kind, string arg, array<string> who)
    {
        array<string> alts = new array<string>();
        arg.Split("|", alts);
        array<string> owners;
        OZL_State.Owners(owners);
        if (!owners)
            return;
        OZ_IdentityService ids = OZ_Identity.Get();
        for (int i = 0; i < owners.Count(); i++)
        {
            string owner = owners[i];
            OZL_FactionState state = OZL_State.Get(owner);
            if (!state)
                continue;
            bool has = false;
            for (int a = 0; a < alts.Count(); a++)
            {
                string node = alts[a].Trim();
                if (node != "" && state.IsCompleted(node))
                    has = true;
            }
            if (!has)
                continue;
            string shown = ids.FactionName(owner);
            if (shown != owner)
                shown = shown + " (" + owner + ")";
            who.Insert(shown);
        }
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
