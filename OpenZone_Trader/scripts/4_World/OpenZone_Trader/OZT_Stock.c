// The stock of the shelves (design §11.1, the owner 2026-11-12: "as in
// TraderPlus"): how many things of a class, in a condition, a stock holds.
//
// A stock is named by its points (OZT_Point.StockId: a point's own id, or one
// several points share). Only goods with a Stock block are counted; everything
// else is unlimited, as before. One small JSON per stock in
// $profile:OpenZone\TraderStock, written right after a committed deal that
// changed it: the synchronous write of one big JSON is what froze a live
// server before.

class OZT_StockLine
{
    string ClassName = "";
    int Level = 0;
    int Count = 0;
}

class OZT_StockFile
{
    ref array<ref OZT_StockLine> Lines;

    void OZT_StockFile()
    {
        Lines = new array<ref OZT_StockLine>();
    }
}

// One stock in memory: parallel lists, small enough to search through.
class OZT_StockShelf
{
    ref array<string> Classes;
    ref array<int> Levels;
    ref array<int> Counts;

    void OZT_StockShelf()
    {
        Classes = new array<string>();
        Levels = new array<int>();
        Counts = new array<int>();
    }

    int IndexOf(string type, int level)
    {
        for (int i = 0; i < Classes.Count(); i++)
        {
            if (Levels[i] == level && Classes[i] == type)
                return i;
        }
        return -1;
    }

    int Count(string type, int level)
    {
        int at = IndexOf(type, level);
        if (at < 0)
            return 0;
        return Counts[at];
    }

    void Add(string type, int level, int delta)
    {
        int at = IndexOf(type, level);
        if (at < 0)
        {
            if (delta <= 0)
                return;
            Classes.Insert(type);
            Levels.Insert(level);
            Counts.Insert(delta);
            return;
        }
        int n = Counts[at] + delta;
        if (n > 0)
        {
            Counts.Set(at, n);
            return;
        }
        Classes.RemoveOrdered(at);
        Levels.RemoveOrdered(at);
        Counts.RemoveOrdered(at);
    }

    void Clear()
    {
        Classes.Clear();
        Levels.Clear();
        Counts.Clear();
    }
}

class OZT_Stock
{
    protected static ref map<string, ref OZT_StockShelf> s_Shelves = new map<string, ref OZT_StockShelf>();

    protected static string PathOf(string stock)
    {
        return OZT_Const.STOCK_DIR + "\\" + stock + ".json";
    }

    // The shelf of a stock, read from its file the first time it is asked.
    static OZT_StockShelf Shelf(string stock)
    {
        OZT_StockShelf shelf;
        if (s_Shelves.Find(stock, shelf))
            return shelf;
        shelf = new OZT_StockShelf();
        s_Shelves.Set(stock, shelf);
        string path = PathOf(stock);
        if (!FileExist(path))
            return shelf;
        OZT_StockFile file = new OZT_StockFile();
        string err;
        if (!JsonFileLoader<OZT_StockFile>.LoadFile(path, file, err))
        {
            OZT_Log.Error("stock '" + stock + "': the file could not be read, the shelf starts empty: " + err);
            return shelf;
        }
        if (!file.Lines)
            return shelf;
        for (int i = 0; i < file.Lines.Count(); i++)
        {
            OZT_StockLine line = file.Lines[i];
            if (!line || line.ClassName == "" || line.Count <= 0)
                continue;
            if (line.Level < 0 || line.Level >= OZT_Const.LEVEL_RUINED)
                continue;
            shelf.Add(line.ClassName, line.Level, line.Count);
        }
        return shelf;
    }

    static int Count(string stock, string type, int level)
    {
        return Shelf(stock).Count(type, level);
    }

    static void Add(string stock, string type, int level, int delta)
    {
        Shelf(stock).Add(type, level, delta);
    }

    static void Save(string stock)
    {
        OZT_StockShelf shelf = Shelf(stock);
        OZT_StockFile file = new OZT_StockFile();
        for (int i = 0; i < shelf.Classes.Count(); i++)
        {
            OZT_StockLine line = new OZT_StockLine();
            line.ClassName = shelf.Classes[i];
            line.Level = shelf.Levels[i];
            line.Count = shelf.Counts[i];
            file.Lines.Insert(line);
        }
        if (!FileExist(OZT_Const.STOCK_DIR))
            MakeDirectory(OZT_Const.STOCK_DIR);
        string err;
        if (!JsonFileLoader<OZT_StockFile>.SaveFile(PathOf(stock), file, err))
            OZT_Log.Error("stock '" + stock + "': the file could not be written: " + err);
    }

    // The stocks the points sell from, each once.
    static void StockIds(array<string> into)
    {
        array<ref OZT_Point> points = OZT_Config.Get().Traders().Traders;
        for (int i = 0; i < points.Count(); i++)
        {
            string id = points[i].StockId();
            if (into.Find(id) < 0)
                into.Insert(id);
        }
    }

    // The stocked goods a stock sells: the stocked lines of every category
    // of every point that sells from it, each class once.
    static void GoodsOf(string stock, array<OZT_BookEntry> into)
    {
        array<ref OZT_Point> points = OZT_Config.Get().Traders().Traders;
        array<string> seen = new array<string>();
        for (int i = 0; i < points.Count(); i++)
        {
            OZT_Point p = points[i];
            if (p.StockId() != stock)
                continue;
            for (int c = 0; c < p.Categories.Count(); c++)
            {
                OZT_Category cat = OZT_Config.Get().FindCategory(p.Categories[c]);
                if (!cat)
                    continue;
                for (int g = 0; g < cat.Goods.Count(); g++)
                {
                    OZT_BookEntry e = OZT_Book.EntryOf(cat.Goods[g].ClassName);
                    if (!e || !e.Stocked || seen.Find(e.ClassName) > -1)
                        continue;
                    seen.Insert(e.ClassName);
                    into.Insert(e);
                }
            }
        }
    }

    // A server start: every stock restocked and destocked by the settings
    // (TraderPlus's EnableDefaultTraderStock and EnableAutoDestockAtRestart),
    // and written back.
    static void Start()
    {
        OZT_Settings cfg = OZT_Config.Get().Settings();
        array<string> stocks = new array<string>();
        StockIds(stocks);
        for (int s = 0; s < stocks.Count(); s++)
        {
            string stock = stocks[s];
            OZT_StockShelf shelf = Shelf(stock);
            array<OZT_BookEntry> goods = new array<OZT_BookEntry>();
            GoodsOf(stock, goods);
            if (cfg.StartStock == 1 || cfg.StartStock == 2)
            {
                shelf.Clear();
                for (int g = 0; g < goods.Count(); g++)
                {
                    OZT_BookEntry e = goods[g];
                    if (e.MaxStock <= 0)
                        continue;
                    int n = e.MaxStock;
                    if (cfg.StartStock == 2)
                        n = Math.RandomIntInclusive(0, e.MaxStock);
                    shelf.Add(e.ClassName, 0, n);
                }
            }
            if (cfg.DestockAtStart == 1)
                Destock(shelf);
            Save(stock);
            OZT_Log.Info("stock '" + stock + "': " + shelf.Classes.Count().ToString() + " line(s) at the start");
        }
    }

    // A line at or above its Max loses its Destock share.
    protected static void Destock(OZT_StockShelf shelf)
    {
        for (int i = shelf.Classes.Count() - 1; i >= 0; i--)
        {
            OZT_BookEntry e = OZT_Book.EntryOf(shelf.Classes[i]);
            if (!e || !e.Stocked || e.Destock <= 0)
                continue;
            int n = shelf.Counts[i];
            if (n < e.MaxStock)
                continue;
            float cut = n * e.Destock;
            int gone = Math.Round(cut);
            if (gone > 0)
                shelf.Add(shelf.Classes[i], shelf.Levels[i], -gone);
        }
    }

    // A 4_World class held by a static: let go at mission end.
    static void Reset()
    {
        s_Shelves.Clear();
    }
}

// What one deal does to a stock, before it is committed: the units it takes and
// brings, line by line, so that one deal never prices two units at one stock and
// a quote changes nothing.
class OZT_StockCursor
{
    protected string m_Stock;
    protected ref OZT_StockShelf m_Delta;

    void OZT_StockCursor(string stock)
    {
        m_Stock = stock;
        m_Delta = new OZT_StockShelf();
    }

    string Stock()
    {
        return m_Stock;
    }

    // The line as this deal has left it so far.
    int Count(string type, int level)
    {
        int delta = 0;
        int at = m_Delta.IndexOf(type, level);
        if (at > -1)
            delta = m_Delta.Counts[at];
        return OZT_Stock.Count(m_Stock, type, level) + delta;
    }

    // `delta` units into the line (below zero: out of it). The delta list keeps
    // negative counts too, so it is not a shelf's Add.
    void Move(string type, int level, int delta)
    {
        int at = m_Delta.IndexOf(type, level);
        if (at < 0)
        {
            m_Delta.Classes.Insert(type);
            m_Delta.Levels.Insert(level);
            m_Delta.Counts.Insert(delta);
            return;
        }
        m_Delta.Counts.Set(at, m_Delta.Counts[at] + delta);
    }

    // After a committed deal: the moves onto the shelf, the shelf to its file.
    // True: the stock changed.
    bool Commit()
    {
        bool changed = false;
        for (int i = 0; i < m_Delta.Classes.Count(); i++)
        {
            int delta = m_Delta.Counts[i];
            if (delta == 0)
                continue;
            OZT_Stock.Add(m_Stock, m_Delta.Classes[i], m_Delta.Levels[i], delta);
            changed = true;
        }
        if (changed)
            OZT_Stock.Save(m_Stock);
        return changed;
    }
}

// After a deal that changed a stock, everybody else who stands at a point that
// sells from it is told (the owner, 2026-11-12: two players at one trader see
// one shelf), so an open screen asks for its shelf again. Who stands there is
// all the server knows of an open screen; a client with no screen open lets
// the notice go.
class OZT_StockNews
{
    static void Tell(string stock, PlayerBase dealer)
    {
        array<OZT_Point> points = new array<OZT_Point>();
        array<ref OZT_Point> all = OZT_Config.Get().Traders().Traders;
        for (int i = 0; i < all.Count(); i++)
        {
            if (all[i].StockId() == stock)
                points.Insert(all[i]);
        }
        if (points.Count() == 0)
            return;
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int p = 0; p < players.Count(); p++)
        {
            PlayerBase pb = PlayerBase.Cast(players[p]);
            if (!pb || pb == dealer || !pb.GetIdentity())
                continue;
            for (int k = 0; k < points.Count(); k++)
            {
                float away = OZT_Points.Away(points[k].Id, pb);
                if (away < 0 || away > points[k].Radius + NEAR_SLACK)
                    continue;
                OZ_Rpc.Notice(pb.GetIdentity(), OZT_Const.NOTICE_STOCK, true, stock);
                break;
            }
        }
    }

    // A screen stays open a step or two past the radius.
    static const float NEAR_SLACK = 2;
}
