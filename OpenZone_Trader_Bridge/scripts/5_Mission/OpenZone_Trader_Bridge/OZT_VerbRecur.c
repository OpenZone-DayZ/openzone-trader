// A stand measurement of an engine trap (2026-11-12). The array form is in
// dayz-modding (enforce-script.md): a recursive call handed an object's array
// field from inside a loop over the function's own array parameter comes back
// with that parameter pointing at the field. The trade screen's strip of parts
// showed the same shape with a STRING parameter handed an object's string
// field. This asks each kind once:
//
//   {"op": "recur"}   per kind, what the outer frame's parameter reads after
//                     each of its two inner calls; unchanged is "top top",
//                     "100 100" and "root root"
class OZT_RecurNode
{
    string Name;
    int Num;
    ref OZT_RecurNode Kid;
}

class OZT_VerbRecur
{
    protected static string s_Log;

    static bool Op(out string detail)
    {
        s_Log = "";
        ByString("top", 0);
        string a = s_Log;

        s_Log = "";
        ByInt(100, 0);
        string b = s_Log;

        s_Log = "";
        OZT_RecurNode root = new OZT_RecurNode();
        root.Name = "root";
        ByObject(root, 0);
        string c = s_Log;

        s_Log = "";
        ByStringLocal("top", 0);
        string d = s_Log;

        detail = "string field:" + a + " | int field:" + b;
        detail = detail + " | object field:" + c + " | string via a local:" + d;
        OZT_Log.Info("recur probe: " + detail);
        return true;
    }

    // A string parameter; the recursion is handed `k.Name`.
    protected static void ByString(string host, int depth)
    {
        array<ref OZT_RecurNode> kids = new array<ref OZT_RecurNode>();
        for (int n = 0; n < 2; n++)
        {
            OZT_RecurNode made = new OZT_RecurNode();
            made.Name = host + "." + n.ToString();
            kids.Insert(made);
        }
        for (int i = 0; i < kids.Count(); i++)
        {
            OZT_RecurNode k = kids[i];
            if (depth < 1)
                ByString(k.Name, depth + 1);
            if (depth == 0)
                s_Log = s_Log + " " + host;
        }
    }

    // An int parameter; the recursion is handed `k.Num`.
    protected static void ByInt(int num, int depth)
    {
        array<ref OZT_RecurNode> kids = new array<ref OZT_RecurNode>();
        for (int n = 0; n < 2; n++)
        {
            OZT_RecurNode made = new OZT_RecurNode();
            made.Num = num + n + 1;
            kids.Insert(made);
        }
        for (int i = 0; i < kids.Count(); i++)
        {
            OZT_RecurNode k = kids[i];
            if (depth < 1)
                ByInt(k.Num, depth + 1);
            if (depth == 0)
                s_Log = s_Log + " " + num.ToString();
        }
    }

    // An object parameter; the recursion is handed `k.Kid`, an object field.
    protected static void ByObject(OZT_RecurNode node, int depth)
    {
        array<ref OZT_RecurNode> kids = new array<ref OZT_RecurNode>();
        for (int n = 0; n < 2; n++)
        {
            OZT_RecurNode made = new OZT_RecurNode();
            made.Kid = new OZT_RecurNode();
            made.Kid.Name = node.Name + "." + n.ToString();
            kids.Insert(made);
        }
        for (int i = 0; i < kids.Count(); i++)
        {
            OZT_RecurNode k = kids[i];
            if (depth < 1)
                ByObject(k.Kid, depth + 1);
            if (depth == 0)
                s_Log = s_Log + " " + node.Name;
        }
    }

    // As ByString, the field copied into a local first.
    protected static void ByStringLocal(string host, int depth)
    {
        array<ref OZT_RecurNode> kids = new array<ref OZT_RecurNode>();
        for (int n = 0; n < 2; n++)
        {
            OZT_RecurNode made = new OZT_RecurNode();
            made.Name = host + "." + n.ToString();
            kids.Insert(made);
        }
        for (int i = 0; i < kids.Count(); i++)
        {
            OZT_RecurNode k = kids[i];
            string sub = k.Name;
            if (depth < 1)
                ByStringLocal(sub, depth + 1);
            if (depth == 0)
                s_Log = s_Log + " " + host;
        }
    }
}
