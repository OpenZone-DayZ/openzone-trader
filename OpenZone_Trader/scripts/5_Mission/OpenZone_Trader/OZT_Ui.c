// Small things every part of the trade screen needs.

// The series' palette as the screen paints it from script (ui/tokens.json of
// the core and of this mod name the same colours for the layouts).
class OZT_Look
{
    static int Edge()    { return ARGB(255, 56, 77, 97); }
    static int EdgeDim() { return ARGB(255, 42, 58, 73); }
    static int Accent()  { return ARGB(255, 79, 181, 232); }
    static int Text()    { return ARGB(255, 230, 237, 242); }
    static int Muted()   { return ARGB(255, 148, 166, 181); }
    static int Good()    { return ARGB(255, 99, 194, 92); }
    static int Bad()     { return ARGB(255, 224, 82, 82); }
    static int Alert()   { return ARGB(255, 240, 138, 36); }
    static int Gold()    { return ARGB(255, 217, 191, 107); }
    static int Raised()  { return ARGB(255, 34, 46, 57); }
    static int Tile()    { return ARGB(255, 32, 43, 54); }
    static int TileLit() { return ARGB(255, 44, 60, 76); }
    static int Dark()    { return ARGB(255, 14, 19, 24); }
    static int GridLine(){ return ARGB(14, 255, 255, 255); }
    static int Zone()    { return ARGB(46, 79, 181, 232); }
    static int None()    { return ARGB(0, 0, 0, 0); }

    // The pip of a health level: nothing for pristine.
    static int Condition(int level)
    {
        if (level == GameConstants.STATE_WORN)
            return ARGB(255, 99, 194, 92);
        if (level == GameConstants.STATE_DAMAGED)
            return ARGB(255, 217, 191, 107);
        if (level == GameConstants.STATE_BADLY_DAMAGED)
            return ARGB(255, 240, 138, 36);
        return ARGB(255, 224, 82, 82);
    }
}

class OZT_Ui
{
    // Screen pixels in one unit of a layout. Sizes a layout declares scale with
    // the height of the screen; SetPos and SetSize take pixels (measured in the
    // series' menus), so everything the screen places itself is multiplied.
    static float Px()
    {
        int w;
        int h;
        GetScreenSize(w, h);
        float high = h;
        if (high < 1)
            return 1;
        return high / 1080;
    }

    // Widgets of a layout file under a parent, with the guard the series
    // learned the hard way: the engine can hand back the widgets of ANOTHER
    // layout this client made earlier, and the spelling with backslashes gets
    // the right one. `root` is the name the file's root widget has.
    static Widget Make(string path, Widget parent, string root)
    {
        Widget w = GetGame().GetWorkspace().CreateWidgets(path, parent);
        if (!w)
            return null;
        if (w.GetName() == root)
            return w;
        string got = w.GetName();
        w.Unlink();
        string other = path;
        other.Replace("/", "\\");
        w = GetGame().GetWorkspace().CreateWidgets(other, parent);
        string msg = "the layout " + path + " came back as '" + got + "'";
        if (w && w.GetName() == root)
        {
            OZT_Log.Warn(msg + "; the backslash spelling gave the right one");
            return w;
        }
        OZT_Log.Error(msg + " twice");
        if (w)
            w.Unlink();
        return null;
    }

    // A string-table key as words; anything else as it is (a trader's name is
    // the admin's own text).
    static string Words(string key)
    {
        if (key.IndexOf("STR_") == 0)
            return Widget.TranslateString("#" + key);
        return key;
    }

    // A key whose text holds %1.
    static string Words1(string key, string arg)
    {
        string text = Words(key);
        text.Replace("%1", arg);
        return text;
    }

    // 12500 -> "12 500".
    static string Sum(int amount)
    {
        string sign = "";
        if (amount < 0)
        {
            sign = "-";
            amount = -amount;
        }
        string digits = amount.ToString();
        string s = "";
        int n = digits.Length();
        for (int i = 0; i < n; i++)
        {
            int left = n - i;
            if (i > 0 && left % 3 == 0)
                s = s + " ";
            s = s + digits.Get(i);
        }
        return sign + s;
    }

    // A sum for a place four digits wide: 9 500, 12.5k, 125k, 1.2M.
    // A price as a tile shows it (the owner, 2026-11-11): exact below ten
    // thousand ("9 995"), then thousands with one decimal ("12.3k"), whole
    // thousands from a hundred thousand ("125k"), millions ("1.2M", "12M");
    // the letters are the string table's. The exact sum is on the card. `up`
    // rounds up, for a price the player pays; otherwise down, for a price the
    // player is paid: a tile never promises better than the deal.
    static string Short(int amount, bool up)
    {
        if (amount < 10000)
            return Sum(amount);
        string k = Words("STR_OZT_UI_K");
        string m = Words("STR_OZT_UI_M");
        int tenths = ShortSteps(amount, 100, up);
        if (tenths < 1000)
            return ShortTenths(tenths, k);
        int thousands = ShortSteps(amount, 1000, up);
        if (thousands < 1000)
            return thousands.ToString() + k;
        int millionTenths = ShortSteps(amount, 100000, up);
        if (millionTenths < 100)
            return ShortTenths(millionTenths, m);
        int millions = ShortSteps(amount, 1000000, up);
        return millions.ToString() + m;
    }

    protected static int ShortSteps(int amount, int step, bool up)
    {
        int n = amount / step;
        if (up && n * step < amount)
            n++;
        return n;
    }

    protected static string ShortTenths(int tenths, string unit)
    {
        int whole = tenths / 10;
        int dec = tenths - whole * 10;
        if (dec == 0)
            return whole.ToString() + unit;
        return whole.ToString() + "." + dec.ToString() + unit;
    }

    static void Say(Widget root, string name, string text)
    {
        TextWidget t = TextWidget.Cast(root.FindAnyWidget(name));
        if (t)
            t.SetText(text);
    }

    static void Tint(Widget root, string name, int color)
    {
        Widget w = root.FindAnyWidget(name);
        if (w)
            w.SetColor(color);
    }

    static void Show(Widget root, string name, bool on)
    {
        Widget w = root.FindAnyWidget(name);
        if (w)
            w.Show(on);
    }

    // A thing's name as every vanilla surface shows it: from the item, which
    // may override what its config says.
    static string NameOf(EntityAI e)
    {
        if (!e)
            return "";
        return e.GetDisplayName();
    }
}
