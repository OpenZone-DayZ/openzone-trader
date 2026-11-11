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

    // Kinds of a trader point.
    static const string KIND_OBJECT = "object";
    static const string KIND_NPC    = "npc";

    // Quantity rules of a goods entry: one price for the piece whatever is in
    // it; a price per unit of a stack; a price for a full one, scaled by what is
    // left.
    static const string RULE_EACH     = "";
    static const string RULE_UNIT     = "unit";
    static const string RULE_FRACTION = "fraction";

    // The money provider that counts items in the inventory.
    static const string PROVIDER_ITEMS = "items";

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
    static const string NO_STOCK     = "STR_OZT_NO_STOCK";
    static const string NO_INTERNAL  = "STR_OZT_NO_INTERNAL";

    // Health levels the condition factors are named after
    // (GameConstants.STATE_PRISTINE .. STATE_RUINED are 0 .. 4).
    static const int LEVEL_RUINED = 4;
}
