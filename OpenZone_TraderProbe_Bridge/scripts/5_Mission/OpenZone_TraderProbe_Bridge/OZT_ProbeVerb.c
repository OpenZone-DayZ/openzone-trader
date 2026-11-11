// The `ozt_probe` verb, following the bridge's own contract for project verbs
// (DZMCP_BridgeCore.c, "HOW A PROJECT ADDS ITS OWN VERB"): the name is added to
// KnownVerbs and IsKnownVerb and routed in Dispatch. Every op answers within
// its tick.
//
//   world_exec verb=ozt_probe args={"op":"status"}
//
// Every value of args is a string.
modded class DZMCP_BridgeCore
{
    override protected string KnownVerbs()
    {
        return super.KnownVerbs() + ", ozt_probe";
    }

    override protected bool IsKnownVerb(string verb)
    {
        if (verb == "ozt_probe")
            return true;
        return super.IsKnownVerb(verb);
    }

    override protected void Dispatch(string verb, string raw)
    {
        if (verb != "ozt_probe")
        {
            super.Dispatch(verb, raw);
            return;
        }

        DZMCP_CommandFull full = new DZMCP_CommandFull();
        string parseError;
        if (!m_Json.ReadFromString(full, raw, parseError))
        {
            FinishCommand(DZMCP_STATUS_FAILED, "ozt_probe: the args block could not be parsed -- every value must be a string: " + Excerpt(parseError));
            return;
        }

        map<string, string> args = full.args;
        string op = "status";
        string given;
        if (args && args.Find("op", given))
            op = given;

        string detail;
        if (OZT_Probe.Get().Command(op, args, detail))
            FinishCommand(DZMCP_STATUS_DONE, detail);
        else
            FinishCommand(DZMCP_STATUS_FAILED, detail);
    }
}
