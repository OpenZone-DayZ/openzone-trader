// Constants of the trader: everything the mod names on disk or on the wire, so
// that a word typed in two files cannot drift apart.
class OZT_Const
{
    static const string LOG_PREFIX = "[OpenZone/Trader] ";

    // Config files: in the core's folder, named like the rest of the series.
    static const string SETTINGS = "$profile:OpenZone\\OZ_Trader_Settings.json";
    static const string GOODS    = "$profile:OpenZone\\OZ_Trader_Goods.json";
    static const string TRADERS  = "$profile:OpenZone\\OZ_Trader_Traders.json";

    // A licence as goods, and the folder of who holds which (design §11.2).
    static const string LICENCE_PREFIX = "@licence:";
    static const string LICENCE_DIR = "$profile:OpenZone\\TraderLicences";

    // The stock of each shelf (design §11.1): one small file per stock.
    static const string STOCK_DIR = "$profile:OpenZone\\TraderStock";

    // The stand verb writes the last full answer of the service here.
    static const string LAST_ANSWER = "$profile:OpenZone\\trader_last.json";

    // The service in the core's registry (OZ_ServiceRegistry) and its operations.
    static const string SERVICE = "trader";
    static const string OP_OPEN = "open";
    static const string OP_DEAL = "deal";
    static const string OP_QUOTE = "quote";
    // Server -> client, unasked: a part of the open (OZT_OpenRes.Part) after a
    // stock moved, for a screen open at a point that sells from that stock.
    static const string OP_STOCK = "stock";
    // Server -> client: a page of the goods of an open (OZT_OpenPage). A whole
    // open is a head (OP_OPEN, OZT_OpenRes with Ask) and pages after it; the
    // last page says so (More 0).
    static const string OP_PAGE = "page";

    // THE OPEN BY PAGES (load test, 2026-11-13). A big book's open was built in
    // one server frame (0.12 s that the whole server stood) and sent as one
    // message the screen waited three seconds for, empty. Now the goods of a
    // trader are built once per set of gate answers, a slice a frame
    // (OZT_ShelfCache), and sent in pages at a pace (OZT_OpenStream).
    //
    // Rows a page carries: about 3.7 KB, five parts of the core's transport.
    static const int PAGE_ROWS = 100;
    // Server work a frame for building shelves, in TickCount ticks (10 MHz):
    // 3 ms. A frame of the server is 16-33 ms; the storage mod keeps its jobs
    // to 5 ms by the same reasoning.
    static const int BUILD_TICKS = 30000;
    // The pace of a stream, bytes a second, and what may leave at once.
    // Measured on the stand with 126 KB in 35 pages (2026-11-13), the last page
    // on the client after: 2.88 s at 40 KB/s, 2.69 s at 60, 2.15 s at 150,
    // 1.94 s at 400; the whole answer as one message took 3.0 s. Past ~60 KB/s
    // the rest waits in the engine's queue, in front of the player's next
    // answer (a quote): 200 KB/s keeps that wait near a second and a half.
    static const float STREAM_RATE = 200000;
    static const float STREAM_BURST = 24000;
    // A built shelf nobody opened for this long is let go; at most this many
    // are kept (the oldest goes first).
    static const float SHELF_IDLE = 300;
    static const int SHELF_MAX = 16;

    // What the server tells a client to show (OZ_Rpc.Show): this prefix, then
    // the trader's id.
    static const string SHOW_PREFIX = "trader:";

    // The section of the core's admin registry the pane TRADERS talks to, and
    // the names of the three files in the core's file editor (cfg_get/cfg_set).
    static const string SECTION_ADMIN = "traders";
    static const string CFG_OWNER     = "trader";
    static const string CFG_SETTINGS  = "trader_settings";
    static const string CFG_GOODS     = "trader_goods";
    static const string CFG_TRADERS   = "trader_traders";

    // The op of a notice the trade action sends when it refuses (OZ_Rpc.Notice);
    // the client shows every notice whose op starts with "trader.".
    static const string NOTICE_OPEN = "trader.open";
    // The notice that a stock changed under an open screen (design §11.1):
    // `why` carries the stock's id; the screen asks `open` again.
    static const string NOTICE_STOCK = "trader.stock";

    // The trade screen's menu id, beside the series' own (131-135 are the PDA's
    // and the research tree's). A large id makes EnterScriptedMenu answer nothing.
    static const int MENU_TRADE = 136;

    // Kinds of a trader point. An object and an npc are made by the server; a
    // map point is a thing already standing in the world (a building, a prop of
    // the map), which the server finds and never makes, moves or deletes.
    static const string KIND_OBJECT = "object";
    static const string KIND_NPC    = "npc";
    static const string KIND_MAP    = "map";

    // Quantity rules of a goods entry: one price for the piece whatever is in
    // it; a price per unit of a stack; a price for a full one, scaled by what is
    // left.
    static const string RULE_EACH     = "";
    static const string RULE_UNIT     = "unit";
    static const string RULE_FRACTION = "fraction";

    // The money provider that counts items in the inventory.
    static const string PROVIDER_ITEMS = "items";

    // The category of the screen's change tab: the server's banknotes, given
    // and taken at face value (the owner, 2026-11-11: a 100 into five 20s).
    // `@` is not a letter of an admin's id, so no category of the goods file
    // can be it.
    static const string CATEGORY_CHANGE = "@change";

    // Why a deal was refused: KEYS of the string table, never ready text. The
    // client knows the player's language, the server does not.
    static const string NO_TOO_FAR   = "STR_OZT_NO_TOO_FAR";
    static const string NO_CLOSED    = "STR_OZT_NO_CLOSED";
    static const string NO_BUSY      = "STR_OZT_NO_BUSY";
    static const string NO_CHANGED   = "STR_OZT_NO_CHANGED";
    static const string NO_MONEY     = "STR_OZT_NO_MONEY";
    static const string NO_SPACE     = "STR_OZT_NO_SPACE";
    static const string NO_NOT_YOURS = "STR_OZT_NO_NOT_YOURS";
    static const string NO_NOT_TAKEN = "STR_OZT_NO_NOT_TAKEN";
    static const string NO_NOT_EMPTY = "STR_OZT_NO_NOT_EMPTY";
    static const string NO_LOCKED    = "STR_OZT_NO_LOCKED";
    // Parts that do not go on together (a light with a wooden handguard).
    static const string NO_PARTS     = "STR_OZT_NO_PARTS";
    static const string NO_ACCESS    = "STR_OZT_NO_ACCESS";
    static const string NO_STOCK     = "STR_OZT_NO_STOCK";
    // A licence the player holds already.
    static const string NO_HAS_LICENCE = "STR_OZT_NO_HAS_LICENCE";
    static const string NO_INTERNAL  = "STR_OZT_NO_INTERNAL";

    // Health levels the condition factors are named after
    // (GameConstants.STATE_PRISTINE .. STATE_RUINED are 0 .. 4).
    static const int LEVEL_RUINED = 4;
}

// Ids the admin types (a category, a trader) become keys, log words and, later,
// file names: letters, digits, `_` and `-`, at most 64.
class OZT_Ids
{
    static bool IsSafe(string id)
    {
        int n = id.Length();
        if (n == 0 || n > 64)
            return false;

        for (int i = 0; i < n; i++)
        {
            int c = id.Get(i).ToAscii();
            bool digit = c >= 48 && c <= 57;
            bool upper = c >= 65 && c <= 90;
            bool lower = c >= 97 && c <= 122;
            bool punct = c == 95 || c == 45;
            if (!digit && !upper && !lower && !punct)
                return false;
        }
        return true;
    }
}
