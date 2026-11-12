// The draft of the trader's three files in the admin window (VPP rework 2026-11-12,
// design §5.6): the structured pages -- Points, Goods, Conditions and discounts,
// Money, Licences -- read the files with the core's cfg_get, edit these objects,
// and write them back whole with cfg_set, one file at a time. One draft for all the
// pages, so an edit made on one page is what the next page shows.
//
// A file's text is parsed the way the server's loader reads it: a root made by
// the script, then the file class's own Validate, which moves every nested object
// into one the script made (the core's OZ_ConfigBase header: what the serializer
// allocates has no field initialisers and lives only as long as its parse). The
// text as read is kept, so a discard needs no second request.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

class OZT_VppDraft
{
    ref OZT_Settings    Settings;
    ref OZT_GoodsFile   Goods;
    ref OZT_TradersFile Traders;

    // Per file, in the order an apply sends them: its cfg name, the text as read,
    // whether it is loaded, changed since, asked and not yet answered.
    protected ref array<string> m_Names;
    protected ref array<string> m_Text;
    protected ref array<int>    m_Loaded;
    protected ref array<int>    m_Dirty;
    protected ref array<int>    m_Asked;

    // The file an apply has sent and waits for ("" when none).
    protected string m_Sending = "";

    void OZT_VppDraft()
    {
        m_Names = new array<string>();
        m_Names.Insert(OZT_Const.CFG_SETTINGS);
        m_Names.Insert(OZT_Const.CFG_GOODS);
        m_Names.Insert(OZT_Const.CFG_TRADERS);
        m_Text = new array<string>();
        m_Loaded = new array<int>();
        m_Dirty = new array<int>();
        m_Asked = new array<int>();
        for (int i = 0; i < m_Names.Count(); i++)
        {
            m_Text.Insert("");
            m_Loaded.Insert(0);
            m_Dirty.Insert(0);
            m_Asked.Insert(0);
        }
    }

    array<string> Names()
    {
        return m_Names;
    }

    bool Owns(string name)
    {
        return m_Names.Find(name) != -1;
    }

    bool Loaded(string name)
    {
        int i = m_Names.Find(name);
        return i != -1 && m_Loaded[i] == 1;
    }

    bool Dirty(string name)
    {
        int i = m_Names.Find(name);
        return i != -1 && m_Dirty[i] == 1;
    }

    bool Asked(string name)
    {
        int i = m_Names.Find(name);
        return i != -1 && m_Asked[i] == 1;
    }

    void MarkAsked(string name, bool asked)
    {
        int i = m_Names.Find(name);
        if (i == -1)
            return;
        if (asked)
            m_Asked[i] = 1;
        else
            m_Asked[i] = 0;
    }

    // An edit was made to the file's object.
    void Touch(string name)
    {
        int i = m_Names.Find(name);
        if (i != -1 && m_Loaded[i] == 1)
            m_Dirty[i] = 1;
    }

    // How many files the draft has changed.
    int Changed()
    {
        int n = 0;
        for (int i = 0; i < m_Dirty.Count(); i++)
            n = n + m_Dirty[i];
        return n;
    }

    // The text of a file arrived (cfg_get). A changed file keeps the admin's
    // edits: false, and `err` empty. A text that does not parse: false, and why.
    bool Take(string name, string body, out string err)
    {
        err = "";
        int i = m_Names.Find(name);
        if (i == -1)
            return false;
        m_Asked[i] = 0;
        if (m_Dirty[i] == 1)
            return false;
        if (!Parse(name, body, err))
            return false;
        m_Text[i] = body;
        m_Loaded[i] = 1;
        return true;
    }

    protected bool Parse(string name, string body, out string err)
    {
        int warnings;
        if (name == OZT_Const.CFG_SETTINGS)
        {
            OZT_Settings s = new OZT_Settings();
            if (!JsonFileLoader<OZT_Settings>.LoadData(body, s, err))
                return false;
            s.Validate(warnings);
            Settings = s;
            return true;
        }
        if (name == OZT_Const.CFG_GOODS)
        {
            OZT_GoodsFile g = new OZT_GoodsFile();
            if (!JsonFileLoader<OZT_GoodsFile>.LoadData(body, g, err))
                return false;
            g.Validate(warnings);
            Goods = g;
            return true;
        }
        if (name == OZT_Const.CFG_TRADERS)
        {
            OZT_TradersFile t = new OZT_TradersFile();
            if (!JsonFileLoader<OZT_TradersFile>.LoadData(body, t, err))
                return false;
            t.Validate(warnings);
            Traders = t;
            return true;
        }
        return false;
    }

    // Every changed file back to its text as read.
    void Discard()
    {
        for (int i = 0; i < m_Names.Count(); i++)
        {
            if (m_Dirty[i] != 1)
                continue;
            string err;
            Parse(m_Names[i], m_Text[i], err);
            m_Dirty[i] = 0;
        }
    }

    // A fresh parse of a file's text as read: what is in force on the server,
    // whatever the draft holds (the research trade page reads it so).
    OZT_GoodsFile GoodsAsRead()
    {
        int i = m_Names.Find(OZT_Const.CFG_GOODS);
        if (m_Loaded[i] != 1)
            return null;
        if (m_Dirty[i] != 1)
            return Goods;
        OZT_GoodsFile g = new OZT_GoodsFile();
        string err;
        if (!JsonFileLoader<OZT_GoodsFile>.LoadData(m_Text[i], g, err))
            return null;
        int warnings;
        g.Validate(warnings);
        return g;
    }

    OZT_TradersFile TradersAsRead()
    {
        int i = m_Names.Find(OZT_Const.CFG_TRADERS);
        if (m_Loaded[i] != 1)
            return null;
        if (m_Dirty[i] != 1)
            return Traders;
        OZT_TradersFile t = new OZT_TradersFile();
        string err;
        if (!JsonFileLoader<OZT_TradersFile>.LoadData(m_Text[i], t, err))
            return null;
        int warnings;
        t.Validate(warnings);
        return t;
    }

    // The text an apply sends for a file.
    string Body(string name)
    {
        string body = "";
        string err;
        if (name == OZT_Const.CFG_SETTINGS && Settings)
            JsonFileLoader<OZT_Settings>.MakeData(Settings, body, err, false);
        else if (name == OZT_Const.CFG_GOODS && Goods)
            JsonFileLoader<OZT_GoodsFile>.MakeData(Goods, body, err, false);
        else if (name == OZT_Const.CFG_TRADERS && Traders)
            JsonFileLoader<OZT_TradersFile>.MakeData(Traders, body, err, false);
        return body;
    }

    // ---- an apply: the changed files one by one, in the order of m_Names

    string Sending()
    {
        return m_Sending;
    }

    // The next changed file to send, now marked as sent; "" when none is left.
    string Next()
    {
        m_Sending = "";
        for (int i = 0; i < m_Names.Count(); i++)
        {
            if (m_Dirty[i] == 1)
            {
                m_Sending = m_Names[i];
                return m_Sending;
            }
        }
        return "";
    }

    // The server took the file: it is the server's now, and its text is read
    // again (the server's Validate may have changed it).
    void Applied(string name)
    {
        int i = m_Names.Find(name);
        if (i != -1)
        {
            m_Dirty[i] = 0;
            m_Text[i] = Body(name);
        }
        m_Sending = "";
    }

    // The server refused the file: the apply stops and the draft keeps it.
    void Refused()
    {
        m_Sending = "";
    }
}

#endif
#endif
#endif
