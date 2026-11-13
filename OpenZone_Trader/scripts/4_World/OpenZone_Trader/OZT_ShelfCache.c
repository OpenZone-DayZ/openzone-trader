// The open by pages (load test, 2026-11-13).
//
// A big book's open was built in one server frame -- 0.12 s that the whole
// server stood, for every player, at every open -- and went as one message the
// screen waited three seconds for with nothing on it. Now, after the storage
// mod's way with a box:
//
// - the goods of a trader are built ONCE for every set of gate answers
//   (OZT_GatePrint), a slice a frame (OZT_ShelfCache.BuildSlice), and kept;
// - an open sends its head at once and then the kept rows in pages, at a pace
//   (OZT_OpenStream), so nothing waits in the engine's queue in front of the
//   player's next answer;
// - a deal does not throw a kept shelf away: the classes it moved are read
//   again before they are sent next (MarkMoved, MendMoved).

// What a player's gates answer at one trader, as one string.
//
// A goods record differs from one player to another only where a gate speaks:
// a lock (Requires) hides it, a discount lowers its price, a part that comes
// along is sold or not by its own gates. So the answers to every condition and
// discount the trader's lines name, each asked once, decide the whole list: two
// players whose answers agree see the same goods at the same prices, and share
// one built shelf.
class OZT_GatePrint
{
    int Revision = -1;
    // The blocks a shelf of this trader has: one per line of the book it deals
    // in, one per note of the change tab. The progress of an open counts them.
    int Lines;
    protected ref array<string> m_Kinds;
    protected ref array<string> m_Args;
    // 0: does the condition hold; 1: the percent a provider gives.
    protected ref array<int> m_Ways;
    protected ref map<string, bool> m_Seen;

    void OZT_GatePrint()
    {
        m_Kinds = new array<string>();
        m_Args = new array<string>();
        m_Ways = new array<int>();
        m_Seen = new map<string, bool>();
    }

    // Every condition and discount the trader's lines name, each once.
    void ReadTrader(OZT_Point trader)
    {
        m_Kinds.Clear();
        m_Args.Clear();
        m_Ways.Clear();
        m_Seen.Clear();
        Lines = 0;
        for (int c = 0; c < trader.Categories.Count(); c++)
        {
            OZT_Category cat = OZT_Config.Get().FindCategory(trader.Categories[c]);
            if (!cat)
                continue;
            for (int g = 0; g < cat.Goods.Count(); g++)
            {
                OZT_BookEntry e = OZT_Book.EntryFor(trader, cat.Goods[g].ClassName);
                if (!e || e.Category != cat.Id)
                    continue;
                Lines++;
                AddConditions(e.Requires);
                if (!e.Discounts)
                    continue;
                for (int d = 0; d < e.Discounts.Count(); d++)
                {
                    OZT_Discount off = e.Discounts[d];
                    if (!off)
                        continue;
                    if (off.By && off.By.Kind != "")
                        AddAtom(1, off.By.Kind, off.By.Arg.Trim());
                    else
                        AddConditions(off.When);
                }
            }
        }
        if (OZT_Book.Changes(trader))
            Lines = Lines + OZT_Config.Get().Settings().Notes.Count();
        Revision = OZT_Config.Get().Revision();
    }

    // A condition's alternatives are asked one by one, as OZT_Gates.Holds asks
    // them.
    protected void AddConditions(array<ref OZT_Condition> conds)
    {
        if (!conds)
            return;
        for (int i = 0; i < conds.Count(); i++)
        {
            OZT_Condition c = conds[i];
            if (!c || c.Kind == "")
                continue;
            array<string> alts = new array<string>();
            c.Arg.Split("|", alts);
            if (alts.Count() == 0)
                alts.Insert("");
            for (int a = 0; a < alts.Count(); a++)
                AddAtom(0, c.Kind, alts[a].Trim());
        }
    }

    protected void AddAtom(int way, string kind, string arg)
    {
        string key = way.ToString() + "|" + kind + "|" + arg;
        if (m_Seen.Contains(key))
            return;
        m_Seen.Set(key, true);
        m_Ways.Insert(way);
        m_Kinds.Insert(kind);
        m_Args.Insert(arg);
    }

    // The player's answers, in the order the trader's lines named them; "-" for
    // a kind nobody answers here.
    string Answers(PlayerBase player, OZT_Point trader)
    {
        string print = "";
        for (int i = 0; i < m_Kinds.Count(); i++)
        {
            OZT_GateProvider p = OZT_Gates.Answering(trader, m_Kinds[i]);
            string said = "-";
            if (p && m_Ways[i] == 0)
            {
                said = "0";
                if (p.Holds(player, m_Kinds[i], m_Args[i]))
                    said = "1";
            }
            else if (p)
            {
                said = p.Percent(player, m_Kinds[i], m_Args[i]).ToString();
            }
            print = print + said + ",";
        }
        return print;
    }
}

// The records of one line of the book (one class) at a trader, and their rows.
// The change tab's notes stand in blocks of their own.
class OZT_ShelfBlock
{
    string Key;
    ref array<ref OZT_WireGoods> Goods;
    ref array<string> Rows;

    void OZT_ShelfBlock(string key)
    {
        Key = key;
        Goods = new array<ref OZT_WireGoods>();
        Rows = new array<string>();
    }

    // The colour group a record of this condition had.
    string GroupAt(int level)
    {
        for (int i = 0; i < Goods.Count(); i++)
        {
            if (Goods[i].Level == level)
                return Goods[i].Group;
        }
        return "";
    }
}

// The goods of one trader for one set of gate answers, built a slice at a time,
// kept between opens.
class OZT_ShelfCache
{
    string Id;
    string TraderId;
    string Stock;
    int Revision;
    int Lines;
    // The categories in the trader's order: a row names its category by its
    // place here (OZT_WireRows).
    ref array<string> Cats;
    ref array<ref OZT_ShelfBlock> Blocks;
    // The blocks whose rows are written: whole categories, with their colour
    // groups. A stream sends nothing past this.
    int Closed;
    bool Ready;
    float UsedAt;
    // Rows written, server time the build took, frames it took.
    int Records;
    int WorkTicks;
    int Frames;
    protected ref map<string, int> m_At;
    protected ref map<string, int> m_CatAt;
    protected ref map<string, bool> m_Moved;
    protected int m_Cat;
    protected int m_Good;
    protected int m_Open;
    protected int m_Groups;

    void OZT_ShelfCache(string id, OZT_Point trader, int lines)
    {
        Id = id;
        TraderId = trader.Id;
        Stock = trader.StockId();
        Revision = OZT_Config.Get().Revision();
        Lines = lines;
        Cats = new array<string>();
        Blocks = new array<ref OZT_ShelfBlock>();
        m_At = new map<string, int>();
        m_CatAt = new map<string, int>();
        m_Moved = new map<string, bool>();
        array<ref OZT_WireCategory> cats = new array<ref OZT_WireCategory>();
        OZT_Open.CategoriesOf(trader, cats);
        for (int c = 0; c < cats.Count(); c++)
        {
            m_CatAt.Set(cats[c].Id, Cats.Count());
            Cats.Insert(cats[c].Id);
        }
        UsedAt = GetGame().GetTickTime();
    }

    // Reads lines of the book until `budget` ticks are spent, a category's
    // colour groups and rows when it is whole; true when the shelf is built.
    // `player` is any player whose gates answer as this shelf's print says.
    bool BuildSlice(PlayerBase player, OZT_Point trader, int budget)
    {
        if (Ready)
            return true;
        int t0 = TickCount(0);
        int unit = OZT_Config.Get().Unit();
        bool grouped = OZT_Config.Get().Settings().GroupVariants == 1;
        while (!Ready)
        {
            if (m_Cat >= trader.Categories.Count())
            {
                // The change tab last, the notes at face value.
                array<ref OZT_WireGoods> notes = new array<ref OZT_WireGoods>();
                OZT_Open.NotesOf(trader, notes);
                for (int n = 0; n < notes.Count(); n++)
                {
                    OZT_ShelfBlock nb = new OZT_ShelfBlock(OZT_Const.CATEGORY_CHANGE + "|" + notes[n].ClassName);
                    nb.Goods.Insert(notes[n]);
                    Blocks.Insert(nb);
                }
                CloseCategory(false);
                Ready = true;
                break;
            }
            OZT_Category cat = OZT_Config.Get().FindCategory(trader.Categories[m_Cat]);
            if (!cat)
            {
                m_Cat++;
                m_Good = 0;
                continue;
            }
            if (m_Good < cat.Goods.Count())
            {
                string type = cat.Goods[m_Good].ClassName;
                m_Good++;
                // A class in two of the trader's categories stands in the first
                // only, as in a deal.
                OZT_BookEntry e = OZT_Book.EntryFor(trader, type);
                if (!e || e.Category != cat.Id)
                    continue;
                OZT_ShelfBlock b = new OZT_ShelfBlock(type);
                OZT_Open.AddRecords(player, trader, e, unit, b.Goods);
                m_At.Set(type, Blocks.Count());
                Blocks.Insert(b);
            }
            else
            {
                CloseCategory(grouped);
                m_Cat++;
                m_Good = 0;
            }
            if (TickCount(t0) >= budget)
                break;
        }
        WorkTicks = WorkTicks + TickCount(t0);
        Frames++;
        return Ready;
    }

    // The blocks read since the last close are a whole category: its colour
    // groups (they never cross a category), then its rows.
    protected void CloseCategory(bool grouped)
    {
        if (grouped)
        {
            // One by one: InsertAll between two arrays of `ref` does not compile
            // ("array<@T> and array<T> are unrelated").
            array<ref OZT_WireGoods> goods = new array<ref OZT_WireGoods>();
            for (int i = m_Open; i < Blocks.Count(); i++)
            {
                array<ref OZT_WireGoods> held = Blocks[i].Goods;
                for (int j = 0; j < held.Count(); j++)
                    goods.Insert(held[j]);
            }
            m_Groups = OZT_Open.GroupVariants(goods, m_Groups);
        }
        for (int k = m_Open; k < Blocks.Count(); k++)
            WriteRows(Blocks[k]);
        m_Open = Blocks.Count();
        Closed = Blocks.Count();
    }

    protected void WriteRows(OZT_ShelfBlock b)
    {
        Records = Records - b.Rows.Count();
        b.Rows.Clear();
        for (int i = 0; i < b.Goods.Count(); i++)
        {
            int cat = 0;
            m_CatAt.Find(b.Goods[i].Category, cat);
            b.Rows.Insert(OZT_WireRows.Row(b.Goods[i], cat));
        }
        Records = Records + b.Rows.Count();
    }

    // Classes a deal moved: their blocks are read again before they are sent.
    void MarkMoved(array<string> classes)
    {
        for (int i = 0; i < classes.Count(); i++)
            m_Moved.Set(classes[i], true);
    }

    bool HasMoved()
    {
        return m_Moved.Count() > 0;
    }

    // The moved classes' blocks read again for `player`, whose gates answer as
    // this shelf's print says. A class not read yet is read as it is now by the
    // build; one in the category being read waits for its close. The colour
    // groups stay as they were: the screen keeps them the same way (MergeOpen).
    void MendMoved(PlayerBase player, OZT_Point trader)
    {
        if (m_Moved.Count() == 0)
            return;
        int unit = OZT_Config.Get().Unit();
        array<string> waiting = new array<string>();
        for (int i = 0; i < m_Moved.Count(); i++)
        {
            string type = m_Moved.GetKey(i);
            int at;
            if (!m_At.Find(type, at))
                continue;
            if (at >= Closed)
            {
                waiting.Insert(type);
                continue;
            }
            OZT_ShelfBlock old = Blocks[at];
            OZT_ShelfBlock b = new OZT_ShelfBlock(type);
            OZT_BookEntry e = OZT_Book.EntryFor(trader, type);
            if (e)
                OZT_Open.AddRecords(player, trader, e, unit, b.Goods);
            for (int g = 0; g < b.Goods.Count(); g++)
                b.Goods[g].Group = old.GroupAt(b.Goods[g].Level);
            Records = Records - old.Rows.Count();
            Blocks.Set(at, b);
            WriteRows(b);
        }
        m_Moved.Clear();
        for (int w = 0; w < waiting.Count(); w++)
            m_Moved.Set(waiting[w], true);
    }
}

// The kept shelves, by trader and gate answers.
class OZT_ShelfCaches
{
    protected static ref map<string, ref OZT_ShelfCache> s_Shelves = new map<string, ref OZT_ShelfCache>();
    protected static ref map<string, ref OZT_GatePrint> s_Prints = new map<string, ref OZT_GatePrint>();

    static void Reset()
    {
        s_Shelves.Clear();
        s_Prints.Clear();
    }

    static int Count()
    {
        return s_Shelves.Count();
    }

    // What the trader's lines ask of the gates, read again after every load of
    // the files.
    static OZT_GatePrint PrintOf(OZT_Point trader)
    {
        OZT_GatePrint p;
        if (!s_Prints.Find(trader.Id, p))
        {
            p = new OZT_GatePrint();
            s_Prints.Set(trader.Id, p);
        }
        if (p.Revision != OZT_Config.Get().Revision())
            p.ReadTrader(trader);
        return p;
    }

    // The shelf of this trader for this player's gates: a kept one, or a new
    // one still to be built (`fresh`).
    static OZT_ShelfCache ShelfFor(PlayerBase player, OZT_Point trader, out bool fresh)
    {
        OZT_GatePrint print = PrintOf(trader);
        string id = trader.Id + "#" + print.Answers(player, trader);
        fresh = false;
        OZT_ShelfCache shelf;
        if (s_Shelves.Find(id, shelf) && shelf.Revision != OZT_Config.Get().Revision())
        {
            s_Shelves.Remove(id);
            shelf = null;
        }
        if (!shelf)
        {
            shelf = new OZT_ShelfCache(id, trader, print.Lines);
            s_Shelves.Set(id, shelf);
            fresh = true;
            LetGo();
        }
        shelf.UsedAt = GetGame().GetTickTime();
        return shelf;
    }

    // A stock moved: every kept shelf of a trader selling from it reads those
    // classes again before it sends them. Without the classes the shelves go.
    static void Moved(string stock, array<string> classes)
    {
        array<string> gone = new array<string>();
        for (int i = 0; i < s_Shelves.Count(); i++)
        {
            OZT_ShelfCache s = s_Shelves.GetElement(i);
            if (!s || s.Stock != stock)
                continue;
            if (classes && classes.Count() > 0)
                s.MarkMoved(classes);
            else
                gone.Insert(s_Shelves.GetKey(i));
        }
        for (int g = 0; g < gone.Count(); g++)
            s_Shelves.Remove(gone[g]);
    }

    // Shelves nobody opened for SHELF_IDLE seconds go, and the oldest while
    // there are more than SHELF_MAX. A stream holds its own shelf: one being
    // sent lives until its last page.
    static void LetGo()
    {
        float now = GetGame().GetTickTime();
        array<string> gone = new array<string>();
        for (int i = 0; i < s_Shelves.Count(); i++)
        {
            if (now - s_Shelves.GetElement(i).UsedAt > OZT_Const.SHELF_IDLE)
                gone.Insert(s_Shelves.GetKey(i));
        }
        for (int g = 0; g < gone.Count(); g++)
            s_Shelves.Remove(gone[g]);
        while (s_Shelves.Count() > OZT_Const.SHELF_MAX)
        {
            int oldest = 0;
            for (int k = 1; k < s_Shelves.Count(); k++)
            {
                if (s_Shelves.GetElement(k).UsedAt < s_Shelves.GetElement(oldest).UsedAt)
                    oldest = k;
            }
            s_Shelves.Remove(s_Shelves.GetKey(oldest));
        }
    }
}

// One open going to one player: its head went at once, its pages go as the
// pace and the shelf allow.
class OZT_OpenStream
{
    // The engine's entity: no ref, gone when the player is.
    PlayerBase Who;
    string Uid;
    int Ask;
    string TraderId;
    ref OZT_ShelfCache Shelf;
    bool Warm;
    int Next;
    int Seq;
    int Rows;
    int Bytes;
    float Tokens;
    int T0;
    int Work;
    bool Done;

    // Pages while the pace allows and the shelf has rows ready; four at most a
    // frame. The last page says More 0.
    void SendPages(float dt, OZT_Point trader)
    {
        if (Done || !Who || !Who.GetIdentity())
            return;
        Tokens = Tokens + OZT_OpenStreams.s_Rate * dt;
        if (Tokens > OZT_OpenStreams.s_Burst)
            Tokens = OZT_OpenStreams.s_Burst;
        int t0 = TickCount(0);
        if (Shelf.HasMoved())
            Shelf.MendMoved(Who, trader);
        int sent = 0;
        while (Tokens > 0 && sent < 4)
        {
            if (Next >= Shelf.Closed && !Shelf.Ready)
                break;
            OZT_OpenPage page = new OZT_OpenPage();
            page.Trader = TraderId;
            page.Ask = Ask;
            page.Seq = Seq;
            while (Next < Shelf.Closed && page.Rows.Count() < OZT_Const.PAGE_ROWS)
            {
                page.Rows.InsertAll(Shelf.Blocks[Next].Rows);
                Next++;
            }
            page.Done = Next;
            page.More = 1;
            if (Shelf.Ready && Next >= Shelf.Blocks.Count())
                page.More = 0;
            if (page.Rows.Count() == 0 && page.More == 1)
                break;
            string body;
            string err;
            if (!JsonFileLoader<OZT_OpenPage>.MakeData(page, body, err, false))
            {
                OZT_Log.Error("open: a page could not be written: " + err);
                Done = true;
                break;
            }
            OZ_Rpc.ServiceRespond(Who.GetIdentity(), OZT_Const.SERVICE, OZT_Const.OP_PAGE, true, body, "");
            Tokens = Tokens - body.Length();
            Bytes = Bytes + body.Length();
            Rows = Rows + page.Rows.Count();
            Seq++;
            sent++;
            if (page.More == 0)
                Done = true;
            if (Done)
                break;
        }
        Work = Work + TickCount(t0);
        if (Done && OZ_Log.IsDebug())
            SayDone();
    }

    protected void SayDone()
    {
        string said = "open " + TraderId + " sent: " + Rows.ToString() + " record(s) in " + Seq.ToString() + " page(s), ";
        said = said + Bytes.ToString() + " B, server " + OZT_Open.Ms(Work) + " ms over " + OZT_Open.Ms(TickCount(T0)) + " ms; shelf ";
        if (Warm)
            said = said + "kept";
        else
            said = said + "built in " + OZT_Open.Ms(Shelf.WorkTicks) + " ms over " + Shelf.Frames.ToString() + " frame(s)";
        OZT_Log.Dbg(said);
    }
}

// The streams of the server, moved on every frame (OZT_ServerMission).
class OZT_OpenStreams
{
    protected static ref array<ref OZT_OpenStream> s_Streams = new array<ref OZT_OpenStream>();
    protected static float s_Idle;
    // The pace, bytes a second and at once: OZT_Const by default, the stand's
    // verb may change them for a measurement.
    static float s_Rate = OZT_Const.STREAM_RATE;
    static float s_Burst = OZT_Const.STREAM_BURST;

    static void Reset()
    {
        s_Streams.Clear();
        s_Rate = OZT_Const.STREAM_RATE;
        s_Burst = OZT_Const.STREAM_BURST;
    }

    static int Count()
    {
        return s_Streams.Count();
    }

    // A screen's whole open: the head now (the money, the player's things, the
    // categories, how many lines will come), the goods after it by pages. False
    // with the refusal's key.
    static bool Start(PlayerBase player, OZT_OpenReq req, out string error)
    {
        error = OZT_Const.NO_INTERNAL;
        if (!player || !player.GetIdentity())
            return false;
        OZT_Point trader = OZT_Config.Get().FindTrader(req.Trader);
        string refused = OZT_Open.Refusal(player, trader);
        if (refused != "")
        {
            error = refused;
            return false;
        }
        OZT_MoneyProvider money = OZT_Money.Of(trader);
        if (!money)
            return false;

        int t0 = TickCount(0);
        string uid = OZT_Licences.UidOf(player);
        // One open at a time for a player: a new one ends the one before.
        for (int i = s_Streams.Count() - 1; i >= 0; i--)
        {
            if (s_Streams[i].Uid == uid)
                s_Streams.RemoveOrdered(i);
        }

        bool fresh;
        OZT_ShelfCache shelf = OZT_ShelfCaches.ShelfFor(player, trader, fresh);
        OZT_OpenRes head = OZT_Open.HeadOf(player, trader, money);
        head.Ask = req.Ask;
        head.Lines = shelf.Lines;
        OZT_Open.CategoriesOf(trader, head.Categories);
        head.RowCats.InsertAll(shelf.Cats);
        OZT_Open.YoursOf(player, trader, head.Yours);
        string body;
        string err;
        if (!JsonFileLoader<OZT_OpenRes>.MakeData(head, body, err, false))
        {
            OZT_Log.Error("open: the head could not be written: " + err);
            return false;
        }
        OZ_Rpc.ServiceRespond(player.GetIdentity(), OZT_Const.SERVICE, OZT_Const.OP_OPEN, true, body, "");

        OZT_OpenStream s = new OZT_OpenStream();
        s.Who = player;
        s.Uid = uid;
        s.Ask = req.Ask;
        s.TraderId = trader.Id;
        s.Shelf = shelf;
        s.Warm = shelf.Ready;
        s.T0 = t0;
        s.Bytes = body.Length();
        s.Tokens = s_Burst - body.Length();
        // A kept shelf: the first pages leave with the head, in this frame.
        if (shelf.Ready)
            s.SendPages(0, trader);
        s.Work = s.Work + TickCount(t0);
        if (!s.Done)
            s_Streams.Insert(s);
        return true;
    }

    // One server frame: the shelves being built get BUILD_TICKS between them,
    // the oldest open first; every stream sends what its pace allows.
    static void TickAll(float dt)
    {
        s_Idle = s_Idle + dt;
        if (s_Idle >= 10)
        {
            s_Idle = 0;
            OZT_ShelfCaches.LetGo();
        }
        if (s_Streams.Count() == 0)
            return;
        int budget = OZT_Const.BUILD_TICKS;
        int i = 0;
        while (i < s_Streams.Count())
        {
            OZT_OpenStream s = s_Streams[i];
            OZT_Point trader = OZT_Config.Get().FindTrader(s.TraderId);
            if (!trader || !s.Who || !s.Who.GetIdentity() || !s.Who.IsAlive())
            {
                s_Streams.RemoveOrdered(i);
                continue;
            }
            if (!s.Shelf.Ready && budget > 0)
            {
                int t0 = TickCount(0);
                s.Shelf.BuildSlice(s.Who, trader, budget);
                int spent = TickCount(t0);
                budget = budget - spent;
                s.Work = s.Work + spent;
            }
            s.SendPages(dt, trader);
            if (s.Done)
            {
                s_Streams.RemoveOrdered(i);
                continue;
            }
            i++;
        }
    }
}
