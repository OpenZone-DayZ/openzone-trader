// Constants of the trader: everything the mod names on disk or on the wire, so
// that a word typed in two files cannot drift apart.
class OZT_Const
{
    static const string LOG_PREFIX = "[OpenZone/Trader] ";

    // Config files: in the core's folder, named like the rest of the series.
    static const string SETTINGS = "$profile:OpenZone\\OZ_Trader_Settings.json";
    static const string GOODS    = "$profile:OpenZone\\OZ_Trader_Goods.json";
    static const string TRADERS  = "$profile:OpenZone\\OZ_Trader_Traders.json";

    // The stand verb writes the last full answer of the service here.
    static const string LAST_ANSWER = "$profile:OpenZone\\trader_last.json";

    // The service in the core's registry (OZ_ServiceRegistry) and its operations.
    static const string SERVICE = "trader";
    static const string OP_OPEN = "open";
    static const string OP_DEAL = "deal";
    static const string OP_QUOTE = "quote";

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
    static const string NO_ACCESS    = "STR_OZT_NO_ACCESS";
    static const string NO_STOCK     = "STR_OZT_NO_STOCK";
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
