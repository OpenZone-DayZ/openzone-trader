// The trader's log lines.
//
// A prefix of its own: in a server log with thirty mods a line must say whose
// it is, and the stand's verdict looks for this prefix. WARNING and ERROR are
// written in full: the verdict counts the first as a whole word and matches
// `[OpenZone/Trader] ERROR` (error_regex in dayz-mcp.toml).
//
// The debug switch is the core's: the core is always there (requiredAddons),
// and a second switch would be a second place for the two to disagree.
class OZT_Log
{
    // The warnings written since ResetWarned(). A load of the files counts its
    // findings with it, whoever writes them: the Validate of a file's own class
    // or a cross-check.
    private static int s_Warned;

    // The first PROBLEMS_KEPT of those warnings, as lines: the admin window's
    // pages show them after an apply (OZT_Admin, op `problems`).
    static const int PROBLEMS_KEPT = 200;
    private static ref array<string> s_Problems = new array<string>();

    static void Info(string msg)
    {
        Print(OZT_Const.LOG_PREFIX + msg);
    }

    static void Warn(string msg)
    {
        s_Warned++;
        if (s_Problems.Count() < PROBLEMS_KEPT)
            s_Problems.Insert(msg);
        Print(OZT_Const.LOG_PREFIX + "WARNING: " + msg);
    }

    static void ResetWarned()
    {
        s_Warned = 0;
        s_Problems.Clear();
    }

    static int Warned()
    {
        return s_Warned;
    }

    static void Problems(array<string> into)
    {
        for (int i = 0; i < s_Problems.Count(); i++)
            into.Insert(s_Problems[i]);
    }

    static void Error(string msg)
    {
        Print(OZT_Const.LOG_PREFIX + "ERROR: " + msg);
    }

    static void Dbg(string msg)
    {
        if (!OZ_Log.IsDebug())
            return;
        Print(OZT_Const.LOG_PREFIX + "dbg: " + msg);
    }
}
