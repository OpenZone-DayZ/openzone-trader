// The trade screen (design §3).
//
// The server is the judge and the screen knows it: nothing here prices or
// plans. `open` gives the shelf, the prices and the player's own things with
// what each brings; every change of the cart asks the server for a `quote`,
// the request of a deal with nothing changed, and the deal panel shows that
// answer; Confirm sends the same request as a `deal` with the net the quote
// gave, and the server refuses it if its own net has moved.
//
// What the engine tells the menu only sets state. Everything that deletes a
// tile's widgets is done in Update, with the mouse button up and no drag in
// the air: the tile under a held button must not be unlinked under it.

class OZT_TradeMenu : UIScriptedMenu
{
    static const string LAYOUT     = "OpenZone_Trader/gui/layouts/ozt_trade.layout";
    static const string ROOT       = "OZT_Trade";
    static const string PICK_LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_pick.layout";
    static const string PICK_ROOT   = "OZT_TradePick";

    // The category dropdowns, in layout units (ozt_trade.json): where the
    // button stands and how tall it is, how tall a row of its list is, and
    // where the grid under it ends. They were tab strips, in as many rows as
    // the categories needed, and a big book's took the shelf (the owner,
    // 2026-11-13: "a dropdown, it is handier").
    static const float STRIP_TOP   = 48;
    static const float PICK_HIGH   = 36;
    static const float PICK_ROW    = 32;
    static const float GRID_BOTTOM = 492;
    // Which dropdown is open.
    static const int PICK_NONE  = 0;
    static const int PICK_SHELF = 1;
    static const int PICK_OWN   = 2;
    static const string SLOT_LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_slot.layout";
    static const string SLOT_ROOT   = "OZT_TradeSlot";
    static const string SLOT_LIST   = "CfgVehicles SurvivorBase InventoryEquipment playerSlots";
    static const int SLOT_ROWS = 7;

    // A quote is asked this long after the cart last changed, and asked again
    // this long after the server said "busy" (it answers one quote per 100 ms).
    static const float QUOTE_AFTER = 0.25;
    static const float QUOTE_AGAIN = 0.2;
    static const int QUOTE_TRIES = 6;

    // How often the screen looks whether the player's things have changed.
    static const float WATCH_EVERY = 0.5;

    // Two presses on one tile within this many milliseconds are a double click.
    static const int DOUBLE_MS = 400;
    static const int TOLD_TWICE_MS = 250;

    // A press becomes a drag after this many pixels.
    static const int DRAG_FROM = 6;

    // The card comes up when the cursor has rested on a tile this long.
    static const float CARD_AFTER = 0.35;

    // Where a tile was dropped.
    static const int ZONE_NONE  = 0;
    static const int ZONE_BUY   = 1;
    static const int ZONE_SELL  = 2;
    static const int ZONE_SHELF = 3;
    static const int ZONE_OWN   = 4;
    static const int ZONE_CHAR  = 5;

    protected static OZT_TradeMenu s_Open;
    protected static string s_Trader;

    protected string m_Trader;
    protected ref OZT_OpenRes m_Data;
    protected ref OZT_Shelf m_Shelf;
    protected ref OZT_Cart m_Cart;

    protected ref OZT_TileGrid m_ShelfGrid;
    protected ref OZT_TileGrid m_OwnGrid;
    protected ref OZT_TileGrid m_BuyGrid;
    protected ref OZT_TileGrid m_SellGrid;

    // Tabs: "" for ALL, then the trader's categories.
    // The categories of the two strips: the shelf has a tab for what the
    // player may buy, the player's things one for what the trader takes.
    protected ref array<string> m_TabIds;
    protected ref array<string> m_OwnIds;
    protected ref array<string> m_TabNames;
    protected ref array<string> m_OwnNames;
    protected int m_ShelfTab;
    protected int m_OwnTab;
    protected string m_TabSig;

    // The open dropdown (PICK_*), its rows, the row under the cursor; a row
    // pressed is taken, and the list shut, in Update with the button up (a
    // row is not unlinked under its own press).
    protected int m_PickOpen;
    protected ref array<Widget> m_PickRows;
    protected int m_PickHover = -1;
    protected bool m_PickShutSoon;
    protected int m_PickWant = -1;
    protected int m_PickWantSide;

    // The player's things: everything in the bags, or only what sells (the
    // owner, 2026-11-12). The player's choice for the whole session.
    protected static bool s_OwnSellable;

    protected Widget m_BuyPanel;
    protected Widget m_SellPanel;
    protected Widget m_ShelfPanel;
    protected Widget m_OwnPanel;
    protected Widget m_CharPanel;
    protected PlayerPreviewWidget m_View;
    protected Widget m_Ghost;
    protected ItemPreviewWidget m_GhostPic;

    protected bool m_Claimed;
    protected bool m_Closing;

    // When the open was asked (TickCount): the debug log says how long a big
    // book takes, and so it does for every lay and repaint of the shelf.
    protected int m_OpenT0;

    // The rows of the shelf that have widgets now (ShelfReveal), and whether some
    // near the view still wait for theirs.
    protected int m_ShelfFrom = -1;
    protected int m_ShelfTo = -1;
    protected bool m_ShelfMore;
    static const int SHELF_BUILDS = 40;

    // What must be laid anew at the next quiet moment.
    protected bool m_DirtyShelf;
    protected bool m_DirtyOwn;
    protected bool m_DirtyCarts;
    protected bool m_DirtySlots;

    // The character's slots and the tiles of what is worn in them.
    protected ref array<Widget> m_SlotWidgets;
    protected ref array<ref OZT_Tile> m_SlotTiles;

    // Parts of the player's things that come off with a part the deal takes
    // off, because they hold it on (PruneOffs): left off the twins the screen
    // asks the engine with.
    protected ref array<EntityAI> m_OffToo;

    // What the last change of the cart took out of it or took off with it, in
    // words: the quote's own line comes after it, never over it.
    protected string m_PruneSaid;

    // The selected thing: a tile's kind and key. A part taken off in the
    // strip leaves with its tile; the thing it was on is selected instead and
    // painted once the grids are laid again.
    protected int m_PickKind;
    protected string m_PickKey;
    protected bool m_PaintPickSoon;

    // The last press, for the double click.
    protected int m_PressAt;
    protected int m_PressKind;
    protected string m_PressKey;

    // The last gesture told, and when.
    protected string m_ToldKey;
    protected int m_ToldAt;

    // What a double click or a drop asked for; done in Update.
    protected int m_ActKind;
    protected string m_ActType;
    protected int m_ActLow;
    protected int m_ActHigh;
    protected int m_ActIndex;
    // A stock changed under the screen (StockMoved): the open it asks for
    // says so if the cart had to change; one more open once the one on its
    // way is in.
    protected bool m_ByNews;
    protected bool m_NewsWaits;
    // The condition of the stock line the quantity dialog buys from.
    protected int m_QtyLevel;
    protected int m_ActZone;

    // The tile under a held button, where the press was, and the drag in the
    // air once the mouse has travelled DRAG_FROM pixels from there.
    protected OZT_Tile m_Held;
    protected int m_HeldX;
    protected int m_HeldY;
    protected OZT_Tile m_Dragged;
    protected int m_DragZone;
    protected float m_GrabX;
    protected float m_GrabY;

    // Turning the character.
    protected bool m_Turning;
    protected int m_TurnFrom;
    protected vector m_Turned;

    // The quote: when the next is due (below zero: none), whether one is on
    // its way and for which state of the cart, and the last answer.
    protected float m_QuoteIn;
    protected bool m_QuoteFlying;
    protected int m_QuoteFor;
    protected int m_QuoteTries;
    protected ref OZT_DealRes m_Quote;
    protected bool m_QuoteOk;
    protected string m_QuoteWhy;

    protected bool m_OpenFlying;
    protected bool m_DealFlying;

    // THE OPEN BY PAGES (load test, 2026-11-13). The number of the last whole
    // open asked; while its pages come the shelf fills as they do and a panel
    // says how far it is; news of a stock that comes meanwhile waits for the
    // last page, as the storage mod's changes wait for its stream's end.
    protected int m_Ask;
    protected bool m_Loading;
    protected bool m_Streaming;
    protected int m_LoadLines;
    protected int m_LoadDone;
    protected int m_LoadPages;
    protected int m_LoadBytes;
    protected ref array<string> m_RowCats;
    protected ref array<string> m_Pending;
    protected Widget m_Load;
    protected Widget m_LoadTrack;
    protected Widget m_LoadBar;
    // The categories with anything to buy, and with anything to sell: one walk
    // over the goods (it was a walk per category and side at every answer).
    protected ref map<string, bool> m_CatBuy;
    protected ref map<string, bool> m_CatSell;

    // True while the info line holds what stays with the player: the next
    // quote that keeps nothing takes it away.
    protected bool m_SaidStays;

    // The parts strip: the selected thing's slots; what it shows; the empty
    // slot the shelf is filtered for; the part a double click or a drag
    // acts on, and the strip's cell a drop landed on.
    protected ref OZT_PartsStrip m_Parts;
    protected ref OZT_PartsAt m_PartsAt;
    protected bool m_DirtyParts;
    protected ref OZT_PartsEntry m_Filter;
    protected ref OZT_PartsEntry m_ActEntry;
    protected ref OZT_PartsEntry m_ActDropEntry;

    // Trying on: the character shown in goods that are not bought.
    protected ref OZT_TryOn m_Try;
    protected string m_PickType;

    // The item card and the tile the cursor rests on; the price list.
    protected ref OZT_Card m_Card;
    protected ref OZT_PriceList m_List;
    protected OZT_Tile m_Hover;
    protected float m_HoverFor;

    // The quantity dialog, and what it wants the server to price: a part of
    // one stack, asked as a quote of that one line when no other is on its way.
    protected ref OZT_QtyDialog m_Qty;
    protected bool m_QtyAsking;
    protected bool m_QtyFlying;
    protected int m_QtyLow;
    protected int m_QtyHigh;
    protected int m_QtyCount;
    protected float m_WatchIn;
    protected int m_Seen;

    void OZT_TradeMenu()
    {
        m_Shelf = new OZT_Shelf();
        m_Cart = new OZT_Cart();
        m_TabIds = new array<string>();
        m_OwnIds = new array<string>();
        m_TabNames = new array<string>();
        m_OwnNames = new array<string>();
        m_PickRows = new array<Widget>();
        m_SlotWidgets = new array<Widget>();
        m_SlotTiles = new array<ref OZT_Tile>();
        m_OffToo = new array<EntityAI>();
        m_QuoteIn = -1;
        m_RowCats = new array<string>();
        m_Pending = new array<string>();
        m_CatBuy = new map<string, bool>();
        m_CatSell = new map<string, bool>();
    }

    // The trader the next opening is for.
    static void For(string trader)
    {
        s_Trader = trader;
    }

    static OZT_TradeMenu Current()
    {
        return s_Open;
    }

    // An answer of the service, from OZT_TradeMission, for the open screen.
    static void OnService(string serviceId, string op, bool ok, string json, string error)
    {
        if (serviceId != OZT_Const.SERVICE || !s_Open)
            return;
        s_Open.TakeAnswer(op, ok, json, error);
    }

    // ---- life -------------------------------------------------------------------------

    override Widget Init()
    {
        layoutRoot = OZT_Ui.Make(LAYOUT, null, ROOT);
        if (!layoutRoot)
        {
            OZT_Log.Error("the trade screen's layout did not load");
            return null;
        }
        m_ShelfGrid = new OZT_TileGrid(layoutRoot, "Shelf", 9);
        m_OwnGrid = new OZT_TileGrid(layoutRoot, "Own", 9);
        m_BuyGrid = new OZT_TileGrid(layoutRoot, "Buy", 6);
        m_SellGrid = new OZT_TileGrid(layoutRoot, "Sell", 6);
        m_BuyPanel = layoutRoot.FindAnyWidget("BuyPanel");
        m_SellPanel = layoutRoot.FindAnyWidget("SellPanel");
        m_ShelfPanel = layoutRoot.FindAnyWidget("ShelfPanel");
        m_OwnPanel = layoutRoot.FindAnyWidget("OwnPanel");
        m_CharPanel = layoutRoot.FindAnyWidget("CharPanel");
        m_View = PlayerPreviewWidget.Cast(layoutRoot.FindAnyWidget("CharView"));
        m_Try = new OZT_TryOn(m_View);
        m_Parts = new OZT_PartsStrip(layoutRoot.FindAnyWidget("PartsStrip"));
        m_Qty = new OZT_QtyDialog(this);
        m_Card = new OZT_Card();
        m_List = new OZT_PriceList();
        m_Ghost = layoutRoot.FindAnyWidget("TradeGhost");
        m_GhostPic = ItemPreviewWidget.Cast(layoutRoot.FindAnyWidget("GhostPic"));
        m_Load = layoutRoot.FindAnyWidget("ShelfLoad");
        m_LoadTrack = layoutRoot.FindAnyWidget("ShelfLoadTrack");
        m_LoadBar = layoutRoot.FindAnyWidget("ShelfLoadBar");
        if (m_Load)
            m_Load.Show(false);
        return layoutRoot;
    }

    override void OnShow()
    {
        // The base locks the controls and shows the cursor; the excludes stop
        // the character from walking.
        super.OnShow();
        if (!layoutRoot)
        {
            GetGame().GetUIManager().CloseMenu(OZT_Const.MENU_TRADE);
            return;
        }
        s_Open = this;
        m_Trader = s_Trader;
        m_Claimed = true;
        // Every step through a local: at a death the mission or its HUD is
        // already gone when the engine hides the menu.
        Mission mission = GetGame().GetMission();
        if (mission)
        {
            mission.AddActiveInputExcludes({"menu"});
            Hud hud = mission.GetHud();
            if (hud)
            {
                hud.ShowHudUI(false);
                hud.ShowQuickbarUI(false);
            }
        }
        ShowPlayer();
        PaintDeal();
        AskOpen();
    }

    override void OnHide()
    {
        ClosePick();
        if (m_GhostPic)
            m_GhostPic.SetItem(null);
        ClearSlots();
        m_Held = null;
        m_Dragged = null;
        m_Hover = null;
        if (m_Try)
            m_Try.Drop();
        if (m_Parts)
            m_Parts.Clear();
        m_Filter = null;
        m_PartsAt = null;
        if (m_Qty)
            m_Qty.Destroy();
        if (m_Card)
            m_Card.Destroy();
        if (m_List)
            m_List.Destroy();
        if (m_ShelfGrid)
        {
            m_ShelfGrid.Clear();
            m_OwnGrid.Clear();
            m_BuyGrid.Clear();
            m_SellGrid.Clear();
        }
        // The shelf's stand-ins are entities of this client: they go with the
        // screen.
        m_Shelf.Clear();
        if (m_Claimed)
        {
            m_Claimed = false;
            Mission mission = GetGame().GetMission();
            if (mission)
            {
                mission.RemoveActiveInputExcludes({"menu"}, true);
                Hud hud = mission.GetHud();
                if (hud)
                {
                    hud.ShowHudUI(true);
                    hud.ShowQuickbarUI(true);
                }
            }
        }
        if (s_Open == this)
            s_Open = null;
        super.OnHide();
    }

    void CloseTrade()
    {
        if (m_Closing)
            return;
        m_Closing = true;
        Close();
    }

    protected bool LeftDown()
    {
        if (GetMouseState(MouseState.LEFT) & 0x80000000)
            return true;
        return false;
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);
        if (!layoutRoot || m_Closing)
            return;
        if (GetUApi().GetInputByID(UAUIBack).LocalPress())
        {
            if (m_Qty && m_Qty.IsOpen())
                m_Qty.Shut();
            else if (m_List && m_List.IsOpen())
                m_List.Shut();
            else if (m_PickOpen != PICK_NONE)
                ClosePick();
            else if (m_Filter)
                ClearFilter();
            else
            {
                CloseTrade();
                return;
            }
        }
        // No trade over a player who cannot trade: the server would refuse
        // anyway, and a menu left open over a death takes the client down.
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained())
        {
            CloseTrade();
            return;
        }

        TurnPlayer();
        DragFrame();
        if (m_PickOpen != PICK_NONE)
            PickHoverFrame();
        else
            HoverFrame(timeslice);

        if (!LeftDown() && !m_Dragged)
        {
            // A dropdown's row pressed, or a press beside an open list.
            if (m_PickShutSoon)
            {
                m_PickShutSoon = false;
                int wantSide = m_PickWantSide;
                int want = m_PickWant;
                m_PickWant = -1;
                m_PickWantSide = PICK_NONE;
                ClosePick();
                if (want > -1)
                    ChooseTab(wantSide, want);
            }
            if (m_ActKind != 0)
                RunAct();
            if (m_DirtyShelf)
            {
                FillShelf();
            }
            int sideT0 = TickCount(0);
            bool side = m_DirtyOwn || m_DirtySlots || m_DirtyCarts || m_DirtyParts;
            if (m_DirtyOwn)
                FillOwn();
            if (m_DirtySlots)
                FillSlots();
            if (m_DirtyCarts)
                FillCarts();
            if (m_DirtyParts)
                FillParts();
            if (side && OZ_Log.IsDebug())
                OZT_Log.Dbg("own side, carts and parts filled in " + OZT_Open.Ms(TickCount(sideT0)) + " ms");
            if (m_PaintPickSoon)
                RepaintPick();
            if (m_PickKind != 0 && !PickStands())
                Unpick();
        }
        // The shelf scrolled, or tiles near the view still wait: their widgets.
        if (!m_DirtyShelf)
            ShelfReveal(false);

        if (m_QuoteIn >= 0)
        {
            m_QuoteIn = m_QuoteIn - timeslice;
            if (m_QuoteIn < 0)
            {
                if (m_QuoteFlying || m_DealFlying)
                    m_QuoteIn = 0.05;
                else
                    AskQuote();
            }
        }

        if (m_Qty)
            m_Qty.Frame(timeslice);
        if (m_QtyAsking && !m_QuoteFlying && !m_DealFlying)
            AskPart();

        m_WatchIn = m_WatchIn - timeslice;
        if (m_WatchIn <= 0)
        {
            m_WatchIn = WATCH_EVERY;
            WatchThings();
        }
    }

    // ---- the service ----------------------------------------------------------------------

    // The player's money: the sum, or with a bank behind the notes the notes
    // and the account apart.
    protected string MoneyWords(OZT_OpenRes res)
    {
        if (res.Bank < 0)
            return OZT_Ui.Sum(res.Balance);
        string cash = OZT_Ui.Sum(res.Balance - res.Bank);
        return cash + " + " + OZT_Ui.Words1("STR_OZT_UI_IN_BANK", OZT_Ui.Sum(res.Bank));
    }

    // Somebody else's deal changed the stock this trader sells from (the
    // server's NOTICE_STOCK): the shelf, its counts and its prices are asked
    // again, and the cart is quoted again once they are in.
    void StockMoved()
    {
        if (!m_Data)
            return;
        m_ByNews = true;
        if (m_OpenFlying || m_DealFlying)
        {
            // The answer on its way may predate the news: ask once it is in.
            m_NewsWaits = true;
            return;
        }
        m_Quote = null;
        PaintDeal();
        AskOpen();
    }

    protected void AskOpen()
    {
        AskOpenOf(null);
    }

    // The open, or a part of it: with `only` (empty too), the records of those
    // classes and the player's side -- after the player's own deal, what it moved.
    protected void AskOpenOf(array<string> only)
    {
        OZT_OpenReq req = new OZT_OpenReq();
        req.Trader = m_Trader;
        if (only)
        {
            req.Part = 1;
            for (int i = 0; i < only.Count(); i++)
                req.Only.Insert(only[i]);
        }
        else
        {
            // A whole open comes by pages, numbered so that the pages of an
            // older one are let go.
            m_Ask++;
            req.Ask = m_Ask;
        }
        string json;
        string err;
        if (!JsonFileLoader<OZT_OpenReq>.MakeData(req, json, err, false))
            return;
        m_OpenFlying = true;
        m_OpenT0 = TickCount(0);
        if (!only)
        {
            m_Loading = true;
            m_Streaming = false;
            m_LoadLines = 0;
            m_LoadDone = 0;
            PaintLoading();
        }
        OZ_Rpc.ServiceRequest(OZT_Const.SERVICE, OZT_Const.OP_OPEN, json);
    }

    protected void QuoteSoon(float after)
    {
        m_QuoteIn = after;
    }

    protected void AskQuote()
    {
        m_QuoteIn = -1;
        if (m_Cart.IsEmpty() || !m_Data)
            return;
        string json = m_Cart.Request(m_Trader, 0);
        if (json == "")
            return;
        m_QuoteFlying = true;
        m_QuoteFor = m_Cart.Revision;
        OZ_Rpc.ServiceRequest(OZT_Const.SERVICE, OZT_Const.OP_QUOTE, json);
    }

    void TakeAnswer(string op, bool ok, string json, string error)
    {
        if (op == OZT_Const.OP_OPEN)
        {
            m_OpenFlying = false;
            if (!ok)
            {
                m_Streaming = false;
                m_Loading = false;
                PaintLoading();
                SayInfo(OZT_Ui.Words(error), OZT_Look.Bad());
                return;
            }
            // The head of an open by pages keeps the open flying (BeginStream).
            TakeOpen(json);
            // A stock changed while this answer flew: it may predate that. An
            // open by pages asks once its last page is in (EndStream).
            if (m_NewsWaits && !m_OpenFlying)
            {
                m_NewsWaits = false;
                m_Quote = null;
                AskOpen();
            }
            return;
        }
        if (op == OZT_Const.OP_PAGE)
        {
            TakePage(ok, json);
            return;
        }
        if (op == OZT_Const.OP_STOCK)
        {
            TakeStock(ok, json);
            return;
        }
        if (op == OZT_Const.OP_QUOTE)
        {
            m_QuoteFlying = false;
            TakeQuote(ok, json, error);
            return;
        }
        if (op == OZT_Const.OP_DEAL)
        {
            m_DealFlying = false;
            TakeDeal(ok, json, error);
        }
    }

    // Another player's deal moved the stock this trader sells from: the server
    // sent this screen its part of the open, put in place here. An open on its
    // way may predate the news: the whole open is asked once it is in.
    protected void TakeStock(bool ok, string json)
    {
        if (!ok || !m_Data)
            return;
        m_ByNews = true;
        // The pages of an open are coming: the news is put in place after the
        // last one (EndStream).
        if (m_Streaming)
        {
            m_Pending.Insert(json);
            return;
        }
        if (m_OpenFlying || m_DealFlying)
        {
            m_NewsWaits = true;
            return;
        }
        m_Quote = null;
        TakeOpen(json, true);
    }

    protected void TakeOpen(string json, bool pushed = false)
    {
        int flew = TickCount(m_OpenT0);
        int t0 = TickCount(0);
        OZT_OpenRes res = new OZT_OpenRes();
        string err;
        if (!JsonFileLoader<OZT_OpenRes>.LoadData(json, res, err))
        {
            OZT_Log.Error("the answer of open could not be read: " + err);
            SayInfo(OZT_Ui.Words(OZT_Const.NO_INTERNAL), OZT_Look.Bad());
            return;
        }
        OZT_WireRows.Unpack(res);
        // The head of an open by pages: the goods come after it (TakePage). One
        // that answers an older open is let go.
        if (res.Ask > 0)
        {
            if (res.Ask == m_Ask && !pushed)
                BeginStream(res, json.Length());
            return;
        }
        bool part = res.Part == 1;
        int records = res.Goods.Count();
        // A whole open in one message (a server that does not page): the
        // loading ends with it.
        if (!part && !pushed)
        {
            m_Loading = false;
            PaintLoading();
        }
        // The shelf keeps its shape: its tiles are repainted where they lie.
        bool keep = false;
        if (part)
        {
            // A part of another trader's open, or nothing to put it into: the
            // whole open instead.
            if (!m_Data || res.Trader != m_Trader)
            {
                if (!m_OpenFlying)
                    AskOpen();
                return;
            }
            keep = MergeOpen(res);
            res = m_Data;
        }
        if (OZ_Log.IsDebug())
        {
            string kind = "open answered: ";
            if (part && pushed)
                kind = "open part pushed: ";
            else if (part)
                kind = "open part answered: ";
            string said = kind + json.Length().ToString() + " B, " + records.ToString() + " record(s), ";
            if (!pushed)
                said = said + OZT_Open.Ms(flew) + " ms after the ask, ";
            said = said + "read in " + OZT_Open.Ms(TickCount(t0)) + " ms";
            if (part)
                said = said + " with the merge, shape kept: " + keep.ToString();
            OZT_Log.Dbg(said);
        }
        bool first = m_Data == null;
        m_Data = res;
        CountShelved();
        // A licence bought opens a category, a stock sold out may close one.
        if (first || TabsSig() != m_TabSig)
        {
            MakeTabs();
            keep = false;
        }
        if (first)
        {
            PaintOwnFilter();
            SayInfo(OZT_Ui.Words("STR_OZT_UI_HELLO"), OZT_Look.Text());
        }
        SayShelfMeta();
        OZT_Ui.Say(layoutRoot, "OwnMoney", MoneyWords(res));
        m_DirtyParts = true;

        if (m_Cart.Prune(res) > 0)
        {
            m_Quote = null;
            if (m_ByNews)
                SayInfo(OZT_Ui.Words("STR_OZT_UI_CART_CUT"), OZT_Look.Alert());
        }
        if (!m_NewsWaits)
            m_ByNews = false;
        if (keep && !m_DirtyShelf)
            RepaintShelf();
        else
            m_DirtyShelf = true;
        m_DirtyOwn = true;
        m_DirtySlots = true;
        m_DirtyCarts = true;
        m_Seen = ThingsSignature();
        if (!m_Cart.IsEmpty() && !m_Quote)
            QuoteSoon(QUOTE_AFTER);
        PaintDeal();
    }

    // The head of an open by pages: the money, the player's things and the
    // tabs now, the goods page by page after it (TakePage). The cart is held to
    // the shelf only once the last page is in.
    protected void BeginStream(OZT_OpenRes res, int bytes)
    {
        m_OpenFlying = true;
        m_Streaming = true;
        m_Loading = true;
        m_LoadLines = res.Lines;
        m_LoadDone = 0;
        m_LoadPages = 0;
        m_LoadBytes = bytes;
        m_RowCats.Clear();
        if (res.RowCats)
            m_RowCats.InsertAll(res.RowCats);
        m_Pending.Clear();
        bool first = m_Data == null;
        m_Data = res;
        CountShelved();
        if (first || TabsSig() != m_TabSig)
            MakeTabs();
        if (first)
        {
            PaintOwnFilter();
            SayInfo(OZT_Ui.Words("STR_OZT_UI_HELLO"), OZT_Look.Text());
        }
        SayShelfMeta();
        OZT_Ui.Say(layoutRoot, "OwnMoney", MoneyWords(res));
        m_DirtyParts = true;
        m_DirtyShelf = true;
        m_DirtyOwn = true;
        m_DirtySlots = true;
        m_DirtyCarts = true;
        m_Seen = ThingsSignature();
        PaintLoading();
        if (OZ_Log.IsDebug())
        {
            string said = "open head: " + bytes.ToString() + " B, " + res.Lines.ToString() + " line(s) to come, ";
            said = said + OZT_Open.Ms(TickCount(m_OpenT0)) + " ms after the ask";
            OZT_Log.Dbg(said);
        }
    }

    // A page of the open: its goods after the ones the screen has, their tiles
    // packed where the shelf's packing stopped; the last page ends the loading.
    protected void TakePage(bool ok, string json)
    {
        if (!ok || !m_Streaming || !m_Data)
            return;
        OZT_OpenPage page = new OZT_OpenPage();
        string err;
        if (!JsonFileLoader<OZT_OpenPage>.LoadData(json, page, err))
        {
            OZT_Log.Error("a page of the open could not be read: " + err);
            return;
        }
        if (page.Ask != m_Ask)
            return;
        int from = m_Data.Goods.Count();
        OZT_WireRows.UnpackRows(page.Rows, m_RowCats, m_Data.Goods);
        m_LoadDone = page.Done;
        m_LoadPages++;
        m_LoadBytes = m_LoadBytes + json.Length();
        if (m_LoadPages == 1 && OZ_Log.IsDebug())
            OZT_Log.Dbg("open first page: " + page.Rows.Count().ToString() + " record(s), " + OZT_Open.Ms(TickCount(m_OpenT0)) + " ms after the ask");
        // A category's first goods give it its tab.
        if (CountShelvedFrom(from) && TabsSig() != m_TabSig)
            MakeTabs();
        if (!m_DirtyShelf)
            ShelfAppend(from);
        if (page.More == 0)
            EndStream();
        else
            PaintLoading();
    }

    // The last page is in: the loading ends, and what waited for it is done.
    protected void EndStream()
    {
        m_Streaming = false;
        m_OpenFlying = false;
        m_Loading = false;
        PaintLoading();
        if (OZ_Log.IsDebug())
        {
            string said = "open streamed: " + m_Data.Goods.Count().ToString() + " record(s), " + m_LoadPages.ToString() + " page(s), ";
            said = said + m_LoadBytes.ToString() + " B, the last " + OZT_Open.Ms(TickCount(m_OpenT0)) + " ms after the ask";
            OZT_Log.Dbg(said);
        }
        if (m_Cart.Prune(m_Data) > 0)
        {
            m_Quote = null;
            if (m_ByNews)
                SayInfo(OZT_Ui.Words("STR_OZT_UI_CART_CUT"), OZT_Look.Alert());
        }
        if (!m_NewsWaits)
            m_ByNews = false;
        m_DirtyCarts = true;
        // News of a stock that came with the pages, in the order it came.
        array<string> waited = new array<string>();
        waited.InsertAll(m_Pending);
        m_Pending.Clear();
        for (int i = 0; i < waited.Count(); i++)
            TakeOpen(waited[i], true);
        if (m_NewsWaits)
        {
            m_NewsWaits = false;
            m_Quote = null;
            AskOpen();
            return;
        }
        if (!m_Cart.IsEmpty() && !m_Quote)
            QuoteSoon(QUOTE_AFTER);
        PaintDeal();
    }

    // The loading of a whole open: in the middle of the empty shelf until the
    // first goods are on it, then at the foot of the shelf; the share of the
    // book's lines that came.
    protected void PaintLoading()
    {
        if (!m_Load)
            return;
        m_Load.Show(m_Loading);
        if (!m_Loading)
            return;
        float sx;
        float sy;
        float sw;
        float sh;
        if (!m_ShelfGrid || !m_ShelfGrid.Frame(sx, sy, sw, sh))
            return;
        float bw;
        float bh;
        m_Load.GetSize(bw, bh);
        float x = sx + (sw - bw) / 2;
        float y = sy + (sh - bh) / 2;
        if (m_ShelfGrid.Count() > 0)
            y = sy + sh - bh - Math.Round(8 * OZT_Ui.Px());
        m_Load.SetPos(x, y);

        float share = 0;
        string pct = "";
        if (m_LoadLines > 0)
        {
            float done = m_LoadDone;
            share = done / m_LoadLines;
            if (share > 1)
                share = 1;
            int whole = Math.Floor(share * 100);
            pct = whole.ToString() + " %";
        }
        OZT_Ui.Say(layoutRoot, "ShelfLoadPct", pct);
        if (m_LoadTrack && m_LoadBar)
        {
            float tw;
            float th;
            m_LoadTrack.GetSize(tw, th);
            m_LoadBar.SetSize(tw * share, th);
        }
    }

    // Which categories have anything to buy and anything to sell, from the
    // goods the screen has.
    protected void CountShelved()
    {
        m_CatBuy.Clear();
        m_CatSell.Clear();
        CountShelvedFrom(0);
    }

    // The goods from `from` on into the counts; true when a category got its
    // first goods on either side.
    protected bool CountShelvedFrom(int from)
    {
        bool grew = false;
        if (!m_Data)
            return false;
        for (int i = from; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (g.Buy > 0 && !m_CatBuy.Contains(g.Category))
            {
                m_CatBuy.Set(g.Category, true);
                grew = true;
            }
            if (g.Sell > 0 && !m_CatSell.Contains(g.Category))
            {
                m_CatSell.Set(g.Category, true);
                grew = true;
            }
        }
        return grew;
    }

    // A part of the open into the screen's picture: the player's side whole, and
    // the records of the classes it names in the place of the ones the screen
    // had (a class that was not there goes to the end). Colour groups are the
    // whole open's: a record keeps the group its class had in that condition.
    //
    // True when the shelf keeps its shape: the same records in the same places,
    // shown and grouped as they were, and the same licences. Then only numbers
    // moved, and the tiles take them where they lie (RepaintShelf).
    protected bool MergeOpen(OZT_OpenRes part)
    {
        bool same = true;
        if (part.Licences && !SameWords(m_Data.Licences, part.Licences))
            same = false;
        array<ref OZT_WireGoods> before = m_Data.Goods;

        m_Data.Revision = part.Revision;
        m_Data.Balance = part.Balance;
        m_Data.Bank = part.Bank;
        m_Data.WearBought = part.WearBought;
        if (part.Yours)
            m_Data.Yours = part.Yours;
        if (part.Licences)
            m_Data.Licences = part.Licences;
        if (part.Notes)
            m_Data.Notes = part.Notes;
        if (part.Condition)
            m_Data.Condition = part.Condition;

        map<string, bool> named = new map<string, bool>();
        if (part.Only)
        {
            for (int n = 0; n < part.Only.Count(); n++)
                named.Set(part.Only[n], true);
        }
        map<string, ref array<ref OZT_WireGoods>> fresh = new map<string, ref array<ref OZT_WireGoods>>();
        for (int f = 0; f < part.Goods.Count(); f++)
        {
            OZT_WireGoods ng = part.Goods[f];
            array<ref OZT_WireGoods> list = fresh.Get(ng.ClassName);
            if (!list)
            {
                list = new array<ref OZT_WireGoods>();
                fresh.Set(ng.ClassName, list);
            }
            list.Insert(ng);
        }

        // The order kept: the untouched records as they stood, a named class's
        // new records where its first record stood.
        array<ref OZT_WireGoods> merged = new array<ref OZT_WireGoods>();
        map<string, bool> put = new map<string, bool>();
        for (int k = 0; k < m_Data.Goods.Count(); k++)
        {
            OZT_WireGoods cur = m_Data.Goods[k];
            if (cur.Category == OZT_Const.CATEGORY_CHANGE || !named.Contains(cur.ClassName))
            {
                merged.Insert(cur);
                continue;
            }
            if (put.Contains(cur.ClassName))
                continue;
            put.Set(cur.ClassName, true);
            PutFresh(fresh.Get(cur.ClassName), merged);
        }
        if (part.Only)
        {
            for (int q = 0; q < part.Only.Count(); q++)
            {
                string type = part.Only[q];
                if (put.Contains(type))
                    continue;
                put.Set(type, true);
                PutFresh(fresh.Get(type), merged);
            }
        }
        m_Data.Goods = merged;

        if (!same || merged.Count() != before.Count())
            return false;
        for (int s = 0; s < merged.Count(); s++)
        {
            if (!SameShape(before[s], merged[s]))
                return false;
        }
        return true;
    }

    // Would the shelf lay these two records alike: the same class in the same
    // condition, category and look, shown or not alike?
    protected static bool SameShape(OZT_WireGoods a, OZT_WireGoods b)
    {
        if (a.ClassName != b.ClassName || a.Level != b.Level || a.Category != b.Category)
            return false;
        if (a.Look != b.Look)
            return false;
        bool shownA = a.Buy > 0;
        bool shownB = b.Buy > 0;
        return shownA == shownB;
    }

    protected static bool SameWords(array<string> a, array<string> b)
    {
        if (!a && !b)
            return true;
        if (!a || !b || a.Count() != b.Count())
            return false;
        for (int i = 0; i < a.Count(); i++)
        {
            if (a[i] != b[i])
                return false;
        }
        return true;
    }

    protected void PutFresh(array<ref OZT_WireGoods> list, array<ref OZT_WireGoods> into)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
            into.Insert(list[i]);
    }

    // Does the cart buy a licence? A licence may open categories: after such a
    // deal the screen asks for the whole open, not a part.
    protected bool CartBuysLicence()
    {
        if (!m_Data)
            return false;
        for (int i = 0; i < m_Cart.Buys.Count(); i++)
        {
            string type = m_Cart.Buys[i].Type;
            for (int g = 0; g < m_Data.Goods.Count(); g++)
            {
                if (m_Data.Goods[g].ClassName == type && m_Data.Goods[g].Look != "")
                    return true;
            }
        }
        return false;
    }

    // The dialog asks what a part of one stack brings.
    void QtyAsk(int low, int high, int count)
    {
        m_QtyAsking = true;
        m_QtyLow = low;
        m_QtyHigh = high;
        m_QtyCount = count;
    }

    protected void AskPart()
    {
        m_QtyAsking = false;
        OZT_DealReq req = new OZT_DealReq();
        req.Trader = m_Trader;
        OZT_WireSell part = new OZT_WireSell();
        part.Low = m_QtyLow;
        part.High = m_QtyHigh;
        part.Count = m_QtyCount;
        req.Sells.Insert(part);
        string json;
        string err;
        if (!JsonFileLoader<OZT_DealReq>.MakeData(req, json, err, false))
            return;
        m_QuoteFlying = true;
        m_QtyFlying = true;
        OZ_Rpc.ServiceRequest(OZT_Const.SERVICE, OZT_Const.OP_QUOTE, json);
    }

    // The answer to the dialog's question. The line's own sum is in it
    // whether the deal of that one line would stand or not.
    protected void TakePart(bool ok, string json, string error)
    {
        if (!ok && error == OZT_Const.NO_BUSY)
        {
            if (m_Qty && m_Qty.IsOpen())
                m_QtyAsking = true;
            return;
        }
        OZT_DealRes res = new OZT_DealRes();
        string err;
        if (json == "" || !JsonFileLoader<OZT_DealRes>.LoadData(json, res, err))
            return;
        if (res.SellPrices.Count() > 0 && m_Qty)
            m_Qty.Priced(m_QtyLow, m_QtyHigh, m_QtyCount, res.SellPrices[0]);
    }

    protected void TakeQuote(bool ok, string json, string error)
    {
        if (m_QtyFlying)
        {
            m_QtyFlying = false;
            TakePart(ok, json, error);
            return;
        }
        // The cart has moved on: this answer is for a cart that is no more.
        if (m_QuoteFor != m_Cart.Revision)
        {
            if (!m_Cart.IsEmpty() && m_QuoteIn < 0)
                QuoteSoon(QUOTE_AGAIN);
            return;
        }
        if (!ok && error == OZT_Const.NO_BUSY && m_QuoteTries < QUOTE_TRIES)
        {
            m_QuoteTries++;
            QuoteSoon(QUOTE_AGAIN);
            return;
        }
        OZT_DealRes res = new OZT_DealRes();
        string err;
        if (json != "" && !JsonFileLoader<OZT_DealRes>.LoadData(json, res, err))
            OZT_Log.Error("the answer of quote could not be read: " + err);
        m_Quote = res;
        m_QuoteOk = ok;
        m_QuoteWhy = error;
        m_QuoteTries = 0;
        m_Cart.Take(res);
        PaintCarts();
        PaintDeal();
        if (!ok && error == OZT_Const.NO_TOO_FAR)
            SayInfo(OZT_Ui.Words(error), OZT_Look.Bad());

        // What comes off the sold things and stays: said in the line below as
        // well, where the verdict's two lines may be taken by the hands.
        string stays = "";
        if (ok)
            stays = StaysWords(res);
        if (m_PruneSaid != "")
        {
            if (stays != "")
                stays = m_PruneSaid + ". " + stays;
            else
                stays = m_PruneSaid;
        }
        if (stays != "")
        {
            SayInfo(stays, OZT_Look.Alert());
            m_SaidStays = true;
        }
        else if (m_SaidStays)
        {
            SayInfo("", OZT_Look.Text());
        }
    }

    protected void TakeDeal(bool ok, string json, string error)
    {
        if (ok)
        {
            // What the deal moved: a part of the open for those classes and the
            // player's side, not the whole open again (half a megabyte for a big
            // book). A licence bought may open categories: the whole open then.
            bool whole = CartBuysLicence();
            OZT_DealRes done = new OZT_DealRes();
            string doneErr;
            if (json == "" || !JsonFileLoader<OZT_DealRes>.LoadData(json, done, doneErr) || !done.Moved)
                whole = true;
            EndTry();
            m_Cart.Clear();
            m_PruneSaid = "";
            m_Quote = null;
            m_DirtyOwn = true;
            m_DirtySlots = true;
            m_DirtyCarts = true;
            SayInfo(OZT_Ui.Words("STR_OZT_UI_DONE"), OZT_Look.Good());
            PaintDeal();
            if (whole)
                AskOpen();
            else
                AskOpenOf(done.Moved);
            return;
        }
        OZT_DealRes res = new OZT_DealRes();
        string err;
        if (json != "")
            JsonFileLoader<OZT_DealRes>.LoadData(json, res, err);
        SayInfo(RefusalWords(error, res.Left), OZT_Look.Bad());
        // Whatever it was, the screen's picture is no longer the server's:
        // ask for both again.
        m_Quote = null;
        PaintDeal();
        AskOpen();
        QuoteSoon(QUOTE_AFTER);
    }

    // A refusal as words; the two that carry a number get it.
    protected string RefusalWords(string key, int left)
    {
        if (key == OZT_Const.NO_SPACE || key == OZT_Const.NO_NOT_EMPTY)
            return OZT_Ui.Words1(key, left.ToString());
        return OZT_Ui.Words(key);
    }

    // Has anything the player carries changed under the open screen? Then the
    // server is asked again: prices and "not empty" are its to say.
    protected void WatchThings()
    {
        if (!m_Data || m_OpenFlying || m_DealFlying)
            return;
        int now = ThingsSignature();
        if (now == m_Seen)
            return;
        m_Seen = now;
        // The player's side alone: a part of the open with no classes (it was
        // the whole open, a big book's every page, at every change of a bag).
        AskOpenOf(new array<string>());
    }

    protected int ThingsSignature()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return 0;
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        int sig = all.Count();
        // Where each thing is counts too: a thing taken off into a bag keeps
        // its id, and only its place says it moved.
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI e = all[i];
            if (!e)
                continue;
            int low;
            int high;
            e.GetNetworkID(low, high);
            sig = sig * 31 + low;
            sig = sig + OZT_Stack.Units(e) * 7 + e.GetHealthLevel();
            if (e.GetInventory().GetCurrentInventoryLocation(il))
            {
                sig = sig * 13 + il.GetType() * 5 + il.GetSlot();
                sig = sig + il.GetRow() * 3 + il.GetCol();
            }
        }
        return sig;
    }

    // ---- what is on the screen --------------------------------------------------------------

    void SayInfo(string text, int color)
    {
        // Whatever is said replaces what the line held.
        m_SaidStays = false;
        TextWidget t = TextWidget.Cast(layoutRoot.FindAnyWidget("TradeInfo"));
        if (!t)
            return;
        t.SetText(text);
        t.SetColor(color);
    }

    protected void ShowPlayer()
    {
        if (!m_View)
            return;
        DayZPlayer me = GetGame().GetPlayer();
        m_View.SetModelOrientation(m_Turned);
        if (m_Try && m_Try.IsOn())
            return;
        m_View.SetPlayer(me);
        m_View.SetModelPosition("0 0 0.605");
        if (me)
            m_View.UpdateItemInHands(me.GetHumanInventory().GetEntityInHands());
    }

    // ---- trying on ---------------------------------------------------------------------------------

    // The button under the selected thing: there for goods the character
    // could wear, and it says what a press would do.
    protected void PaintTry()
    {
        bool goods = m_PickKind == OZT_Tile.SHELF || m_PickKind == OZT_Tile.BUY;
        bool wearable = false;
        if (goods && m_PickType != "" && OZT_TryOn.SlotFor(m_PickType) != InventorySlots.INVALID)
            wearable = true;
        OZT_Ui.Show(layoutRoot, "TryBtn", wearable);
        bool on = false;
        if (m_Try && m_Try.IsOn())
            on = true;
        // The button speaks of the selected goods: off when they are tried on.
        bool tried = false;
        if (on && m_PickType != "" && m_Try.Has(m_PickType))
            tried = true;
        if (tried)
            OZT_Ui.Say(layoutRoot, "TryBtnText", OZT_Ui.Words("STR_OZT_UI_TAKE_OFF"));
        else
            OZT_Ui.Say(layoutRoot, "TryBtnText", OZT_Ui.Words("STR_OZT_UI_TRY_ON"));
        OZT_Ui.Show(layoutRoot, "TryOffBtn", on);
        if (on)
        {
            OZT_Ui.Say(layoutRoot, "CharMeta", OZT_Ui.Words("STR_OZT_UI_TRYING"));
            OZT_Ui.Tint(layoutRoot, "CharMeta", OZT_Look.Accent());
        }
        else
        {
            OZT_Ui.Say(layoutRoot, "CharMeta", "");
        }
    }

    // The button: the selected goods on, beside what is tried already, or off
    // again when they are on.
    protected void ToggleTry()
    {
        if (!m_Try || m_PickType == "")
            return;
        if (m_Try.Has(m_PickType))
            m_Try.TakeOff(m_PickType);
        else if (!m_Try.Put(m_PickType))
            return;
        m_DirtySlots = true;
        PaintTry();
    }

    // All the trying ends with TAKE ALL OFF, when the last thing is taken off,
    // when a deal is made and when the screen closes; choosing other goods
    // leaves it as it is.
    protected void EndTry()
    {
        if (!m_Try || !m_Try.IsOn())
            return;
        m_Try.Drop();
        m_DirtySlots = true;
        PaintTry();
    }

    // The vanilla inventory's way: the yaw follows the mouse while the button
    // is down.
    protected void TurnPlayer()
    {
        if (!m_Turning || !m_View)
            return;
        int mx;
        int my;
        GetMousePos(mx, my);
        vector o = m_Turned;
        o[1] = o[1] - (m_TurnFrom - mx);
        m_View.SetModelOrientation(o);
        if (LeftDown())
            return;
        m_Turned = o;
        m_Turning = false;
    }

    // Has the category anything for this player on one side: to buy on the
    // shelf, to sell among the player's things? A category all of whose goods
    // are locked for the player has no tab on the shelf, as TraderPlus hides
    // it; what the trader takes of it keeps its tab beside the player's things.
    protected bool Shelved(string category, bool buying)
    {
        if (buying)
            return m_CatBuy.Contains(category);
        return m_CatSell.Contains(category);
    }

    // The categories of one strip: the change tab, and those with anything
    // on that side.
    protected void TabsOf(bool buying, array<string> ids, array<string> names)
    {
        ids.Insert("");
        names.Insert(OZT_Ui.Words("STR_OZT_UI_ALL"));
        for (int c = 0; c < m_Data.Categories.Count(); c++)
        {
            string cid = m_Data.Categories[c].Id;
            // The change tab is the server's, not an admin's category: the
            // screen names it in the player's language.
            if (cid == OZT_Const.CATEGORY_CHANGE)
            {
                ids.Insert(cid);
                names.Insert(OZT_Ui.Words("STR_OZT_UI_CHANGE"));
                continue;
            }
            if (!Shelved(cid, buying))
                continue;
            ids.Insert(cid);
            names.Insert(m_Data.Categories[c].Name);
        }
    }

    // The tabs both strips have now, as one string: they are made again
    // when it changes.
    protected string TabsSig()
    {
        array<string> shelf = new array<string>();
        array<string> own = new array<string>();
        array<string> words = new array<string>();
        TabsOf(true, shelf, words);
        TabsOf(false, own, words);
        string sig = "";
        for (int s = 0; s < shelf.Count(); s++)
            sig = sig + shelf[s] + "|";
        sig = sig + "/";
        for (int o = 0; o < own.Count(); o++)
            sig = sig + own[o] + "|";
        return sig;
    }

    protected void MakeTabs()
    {
        // The open tabs stay open by their category when the strip is made
        // again.
        string shelfWas = "";
        string ownWas = "";
        if (m_ShelfTab < m_TabIds.Count())
            shelfWas = m_TabIds[m_ShelfTab];
        if (m_OwnTab < m_OwnIds.Count())
            ownWas = m_OwnIds[m_OwnTab];
        m_TabSig = TabsSig();
        m_TabIds.Clear();
        m_OwnIds.Clear();
        m_TabNames.Clear();
        m_OwnNames.Clear();
        TabsOf(true, m_TabIds, m_TabNames);
        TabsOf(false, m_OwnIds, m_OwnNames);
        // The grid starts below its dropdown and ends where the layout ends it.
        float px = OZT_Ui.Px();
        float top = Math.Round(STRIP_TOP * px);
        float strip = Math.Round(PICK_HIGH * px);
        float gap = Math.Round(4 * px);
        float bottom = Math.Round(GRID_BOTTOM * px);
        m_ShelfGrid.Reframe(top + strip + gap, bottom);
        m_OwnGrid.Reframe(top + strip + gap, bottom);
        m_ShelfTab = m_TabIds.Find(shelfWas);
        if (m_ShelfTab < 0)
            m_ShelfTab = 0;
        m_OwnTab = m_OwnIds.Find(ownWas);
        if (m_OwnTab < 0)
            m_OwnTab = 0;
        // An open list is laid again for the categories there are now (the
        // pages of an open bring them one by one).
        if (m_PickOpen != PICK_NONE)
        {
            int side = m_PickOpen;
            ClosePick();
            OpenPick(side);
        }
        PaintTabs();
    }

    protected void PaintTabs()
    {
        PaintPick("Shelf", m_TabNames, m_ShelfTab);
        PaintPick("Own", m_OwnNames, m_OwnTab);
        if (m_PickOpen != PICK_NONE)
            PaintPickRows();
    }

    // A dropdown's button: the name of the category in view, and the arrow.
    protected void PaintPick(string side, array<string> names, int current)
    {
        string name = "";
        if (current >= 0 && current < names.Count())
            name = names[current];
        OZT_Ui.Say(layoutRoot, side + "PickText", name);
        bool open = m_PickOpen == PICK_OWN;
        if (side == "Shelf")
            open = m_PickOpen == PICK_SHELF;
        DrawArrow(side, open);
    }

    // The arrow of a dropdown, drawn as two strokes: down while it is shut, up
    // while its list is open (the fonts of the series have no arrow glyph).
    protected void DrawArrow(string side, bool up)
    {
        CanvasWidget c = CanvasWidget.Cast(layoutRoot.FindAnyWidget(side + "PickArrow"));
        if (!c)
            return;
        c.Clear();
        float w;
        float h;
        c.GetSize(w, h);
        float px = OZT_Ui.Px();
        float cx = Math.Round(w / 2);
        float cy = Math.Round(h / 2);
        float dx = Math.Round(5 * px);
        float dy = Math.Round(3 * px);
        float stroke = Math.Round(2 * px);
        if (stroke < 1)
            stroke = 1;
        int color = OZT_Look.Text();
        float tipY = cy + dy;
        float endY = cy - dy;
        if (up)
        {
            tipY = cy - dy;
            endY = cy + dy;
        }
        c.DrawLine(cx - dx, endY, cx, tipY, stroke, color);
        c.DrawLine(cx, tipY, cx + dx, endY, stroke, color);
    }

    // The list of a dropdown opened under its button: a row per category, the
    // one in view marked and brought into sight; more rows than the shelf is
    // tall scroll.
    protected void OpenPick(int side)
    {
        ClosePick();
        string prefix = "Own";
        array<string> names = m_OwnNames;
        int current = m_OwnTab;
        if (side == PICK_SHELF)
        {
            prefix = "Shelf";
            names = m_TabNames;
            current = m_ShelfTab;
        }
        Widget list = layoutRoot.FindAnyWidget(prefix + "PickList");
        Widget content = layoutRoot.FindAnyWidget(prefix + "PickContent");
        Widget button = layoutRoot.FindAnyWidget(prefix + "Pick");
        ScrollWidget scroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget(prefix + "PickScroll"));
        if (!list || !content || !button || names.Count() == 0)
            return;
        DropHover();
        float px = OZT_Ui.Px();
        float rowH = Math.Round(PICK_ROW * px);
        float step = rowH + 1;
        float bx;
        float by;
        button.GetPos(bx, by);
        float bw;
        float bh;
        button.GetSize(bw, bh);
        float all = names.Count() * step - 1;
        float room = Math.Round(GRID_BOTTOM * px) - by - bh - Math.Round(2 * px);
        float tall = all;
        float rowW = bw;
        if (tall > room)
        {
            tall = room;
            // The scrollbar stands on the right; the rows end before it.
            rowW = bw - Math.Round(12 * px);
        }
        for (int i = 0; i < names.Count(); i++)
        {
            Widget row = OZT_Ui.Make(PICK_LAYOUT, content, PICK_ROOT);
            if (!row)
                continue;
            row.SetUserID(i);
            row.SetPos(0, i * step);
            row.SetSize(rowW, rowH);
            OZT_Ui.Say(row, "PickText", names[i]);
            m_PickRows.Insert(row);
        }
        list.SetPos(bx, by + bh + Math.Round(2 * px));
        list.SetSize(bw, tall);
        content.SetSize(rowW, all);
        list.Show(true);
        m_PickOpen = side;
        m_PickHover = -1;
        if (scroll)
        {
            scroll.Update();
            float pos01 = 0;
            if (all > tall)
                pos01 = (current * step - (tall - rowH) / 2) / (all - tall);
            if (pos01 < 0)
                pos01 = 0;
            if (pos01 > 1)
                pos01 = 1;
            scroll.VScrollToPos01(pos01);
        }
        PaintPickRows();
        DrawArrow(prefix, true);
    }

    protected void ClosePick()
    {
        if (m_PickOpen == PICK_NONE)
            return;
        for (int i = 0; i < m_PickRows.Count(); i++)
        {
            if (m_PickRows[i])
                m_PickRows[i].Unlink();
        }
        m_PickRows.Clear();
        OZT_Ui.Show(layoutRoot, "ShelfPickList", false);
        OZT_Ui.Show(layoutRoot, "OwnPickList", false);
        m_PickOpen = PICK_NONE;
        m_PickHover = -1;
        DrawArrow("Shelf", false);
        DrawArrow("Own", false);
    }

    // The rows of the open list: the category in view marked, the row under
    // the cursor lit.
    protected void PaintPickRows()
    {
        int current = m_OwnTab;
        if (m_PickOpen == PICK_SHELF)
            current = m_ShelfTab;
        for (int i = 0; i < m_PickRows.Count(); i++)
        {
            Widget row = m_PickRows[i];
            if (!row)
                continue;
            int id = row.GetUserID();
            bool on = id == current;
            OZT_Ui.Show(row, "PickMark", on);
            int text = OZT_Look.Muted();
            if (on || id == m_PickHover)
                text = OZT_Look.Text();
            OZT_Ui.Tint(row, "PickText", text);
            int bed = OZT_Look.Raised();
            if (id == m_PickHover)
                bed = OZT_Look.TileLit();
            OZT_Ui.Tint(row, "PickBg", bed);
        }
    }

    // A category taken from a dropdown, or stepped to by the wheel over its
    // button.
    protected void ChooseTab(int side, int tab)
    {
        if (side == PICK_SHELF)
        {
            if (tab < 0 || tab >= m_TabIds.Count() || tab == m_ShelfTab)
                return;
            m_ShelfTab = tab;
            m_DirtyShelf = true;
            FollowChange(tab, true);
        }
        else
        {
            if (tab < 0 || tab >= m_OwnIds.Count() || tab == m_OwnTab)
                return;
            m_OwnTab = tab;
            m_DirtyOwn = true;
            m_DirtySlots = true;
            FollowChange(tab, false);
        }
        PaintTabs();
    }

    protected bool IsChangeTab(int tab, bool shelf)
    {
        array<string> ids = m_OwnIds;
        if (shelf)
            ids = m_TabIds;
        if (tab < 0 || tab >= ids.Count())
            return false;
        return ids[tab] == OZT_Const.CATEGORY_CHANGE;
    }

    // The change tab is one for both sides: the trader's notes on the shelf
    // and the player's own beside them. Opened on one side it opens on the
    // other; left on one side, the other goes back to everything.
    protected void FollowChange(int tab, bool fromShelf)
    {
        bool change = IsChangeTab(tab, fromShelf);
        if (fromShelf)
        {
            int ownChange = m_OwnIds.Find(OZT_Const.CATEGORY_CHANGE);
            if (change && ownChange > -1 && m_OwnTab != ownChange)
            {
                m_OwnTab = ownChange;
                m_DirtyOwn = true;
                m_DirtySlots = true;
            }
            else if (!change && IsChangeTab(m_OwnTab, false))
            {
                m_OwnTab = 0;
                m_DirtyOwn = true;
                m_DirtySlots = true;
            }
            return;
        }
        int shelfChange = m_TabIds.Find(OZT_Const.CATEGORY_CHANGE);
        if (change && shelfChange > -1 && m_ShelfTab != shelfChange)
        {
            m_ShelfTab = shelfChange;
            m_DirtyShelf = true;
        }
        else if (!change && IsChangeTab(m_ShelfTab, true))
        {
            m_ShelfTab = 0;
            m_DirtyShelf = true;
        }
    }

    // The user id of the row a widget is, or is in; -1 when it is none.
    protected int TabOf(Widget w, array<Widget> tabs)
    {
        while (w)
        {
            int at = tabs.Find(w);
            if (at > -1)
                return tabs[at].GetUserID();
            w = w.GetParent();
        }
        return -1;
    }

    // The goods record of a class: its pristine line when the class is
    // stocked in more than one condition (design §11.1).
    protected OZT_WireGoods GoodsOf(string type)
    {
        if (!m_Data)
            return null;
        OZT_WireGoods first = null;
        for (int i = 0; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (g.ClassName != type)
                continue;
            if (g.Level == 0)
                return g;
            if (!first)
                first = g;
        }
        return first;
    }

    protected OZT_WireGoods GoodsAt(string type, int level)
    {
        if (!m_Data)
            return null;
        for (int i = 0; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (g.ClassName == type && g.Level == level)
                return g;
        }
        return null;
    }

    // Does the player hold the licence these goods are (design §11.2)?
    protected bool HoldsLicence(string type)
    {
        if (!m_Data || !m_Data.Licences)
            return false;
        string prefix = OZT_Const.LICENCE_PREFIX;
        if (type.IndexOf(prefix) != 0)
            return false;
        string id = type.Substring(prefix.Length(), type.Length() - prefix.Length());
        return m_Data.Licences.Find(id) > -1;
    }

    // What the stock line has left beside what the cart already buys of it;
    // -1: unlimited.
    protected int StockLeft(OZT_WireGoods g)
    {
        if (!g || g.Stock < 0)
            return -1;
        int left = g.Stock - m_Cart.Buying(g.ClassName, g.Level);
        if (left < 0)
            left = 0;
        return left;
    }

    protected OZT_WireOwn OwnOf(int low, int high)
    {
        if (!m_Data)
            return null;
        for (int i = 0; i < m_Data.Yours.Count(); i++)
        {
            OZT_WireOwn own = m_Data.Yours[i];
            if (own.Low == low && own.High == high)
                return own;
        }
        return null;
    }

    // The trader's goods of the category in view, from the shelf's stand-ins.
    protected void FillShelf()
    {
        m_DirtyShelf = false;
        DropHover();
        if (!m_Data || !m_ShelfGrid.IsReady())
            return;
        int t0 = TickCount(0);
        m_ShelfGrid.Clear();
        string cat = m_TabIds[m_ShelfTab];

        // What the shelf shows is asked once a record (under a slot's filter
        // the question asks the engine).
        for (int i = 0; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (!ShelfShows(g, cat))
                continue;
            OZT_Tile t = ShelfTile(g, i);
            if (!t)
                continue;
            m_ShelfGrid.AddLazy(t);
        }
        int tRecords = TickCount(t0);
        m_ShelfGrid.Arrange();
        int tPacked = TickCount(t0);
        ShelfReveal(true);
        if (m_Loading)
            PaintLoading();
        if (OZ_Log.IsDebug())
        {
            string said = "shelf laid: records " + OZT_Open.Ms(tRecords) + " ms, packing " + OZT_Open.Ms(tPacked - tRecords);
            said = said + " ms, pictures " + OZT_Open.Ms(TickCount(t0) - tPacked) + " ms, ";
            said = said + m_ShelfGrid.Count().ToString() + " tile(s)";
            OZT_Log.Dbg(said);
        }
    }

    // A shelf tile for a record, packed by the class's size alone: the picture
    // and the widgets are made when the tile comes near the view (ShelfReveal).
    // Null when the game has no such class.
    protected OZT_Tile ShelfTile(OZT_WireGoods g, int index)
    {
        string pic = g.ClassName;
        if (g.Look != "")
            pic = g.Look;
        int tw;
        int th;
        if (!OZT_Tile.ConfigSize(pic, tw, th))
            return null;
        OZT_Tile t = new OZT_Tile(OZT_Tile.SHELF, g.ClassName, null);
        t.SizeTo(tw, th);
        t.Price = g.Buy;
        t.Index = index;
        t.Level = g.Level;
        // A stocked line shows what it holds; a sold-out one lies dark.
        if (g.Stock > 0)
            t.Count = g.Stock;
        else if (g.Stock == 0)
            t.SoldOut = true;
        if (g.Look != "")
            t.Owned = HoldsLicence(g.ClassName);
        return t;
    }

    // Goods that came after the shelf was laid (a page of the open): their
    // tiles after the ones there, packed where the packing stopped.
    protected void ShelfAppend(int from)
    {
        if (!m_Data || !m_ShelfGrid.IsReady())
            return;
        string cat = m_TabIds[m_ShelfTab];
        int added = 0;
        for (int i = from; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (!ShelfShows(g, cat))
                continue;
            OZT_Tile t = ShelfTile(g, i);
            if (!t)
                continue;
            m_ShelfGrid.AddLazy(t);
            added++;
        }
        if (added == 0)
            return;
        m_ShelfGrid.ArrangeMore();
        m_ShelfMore = true;
    }

    // The shelf's tiles take their records' numbers again where they lie: a part
    // of the open that left the shelf its shape (MergeOpen). Laying the shelf
    // anew cost every viewer of a big book a third of a second for each deal
    // anyone made there (load test, 2026-11-13).
    protected void RepaintShelf()
    {
        if (!m_Data || !m_ShelfGrid)
            return;
        int t0 = TickCount(0);
        int moved = 0;
        for (int i = 0; i < m_ShelfGrid.Count(); i++)
        {
            OZT_Tile t = m_ShelfGrid.Get(i);
            if (t.Index < 0 || t.Index >= m_Data.Goods.Count())
                continue;
            OZT_WireGoods g = m_Data.Goods[t.Index];
            int count = 0;
            bool soldOut = false;
            if (g.Stock > 0)
                count = g.Stock;
            else if (g.Stock == 0)
                soldOut = true;
            // Not `owned`: that is a word of the language, and a local of that
            // name does not compile.
            bool held = t.Owned;
            if (g.Look != "")
                held = HoldsLicence(g.ClassName);
            if (t.Price == g.Buy && t.Count == count && t.SoldOut == soldOut && t.Owned == held)
                continue;
            t.Price = g.Buy;
            t.Count = count;
            t.SoldOut = soldOut;
            t.Owned = held;
            moved++;
            if (t.Root)
                t.PaintMarks();
            // The card and the words of the selection said the old numbers.
            if (t == m_Hover)
                DropHover();
            if (IsPick(t))
                m_PaintPickSoon = true;
        }
        if (OZ_Log.IsDebug())
            OZT_Log.Dbg("shelf repainted in " + OZT_Open.Ms(TickCount(t0)) + " ms, " + moved.ToString() + " tile(s) moved");
    }

    // The picture of a shelf record: as it comes, with what the price book gives
    // it, a worn stock line in its condition, a licence by its look.
    protected EntityAI ShelfPicture(OZT_WireGoods g)
    {
        if (g.Look != "")
            return m_Shelf.Of(g.Look);
        if (g.Level > 0)
            return m_Shelf.LookWorn(g.ClassName, g.Level);
        return m_Shelf.LookOf(g.ClassName, g.With, null);
    }

    // The shelf's tiles near the view get their pictures and widgets, the ones
    // far from it give them up; at most SHELF_BUILDS a frame, the rest on the
    // next. A big book's shelf had all of them made in one frame: 3404 tiles,
    // 5.3 s of a stopped game (load test, 2026-11-13).
    protected void ShelfReveal(bool force)
    {
        if (!m_Data || !m_ShelfGrid || !m_ShelfGrid.IsReady())
            return;
        int from;
        int to;
        m_ShelfGrid.Band(from, to);
        if (!force && !m_ShelfMore && from == m_ShelfFrom && to == m_ShelfTo)
            return;
        m_ShelfFrom = from;
        m_ShelfTo = to;
        m_ShelfMore = false;
        int built = 0;
        for (int i = 0; i < m_ShelfGrid.Count(); i++)
        {
            OZT_Tile t = m_ShelfGrid.Get(i);
            bool near = t.Row + t.H > from && t.Row < to;
            if (!near)
            {
                if (t.Root)
                    m_ShelfGrid.UnbuildTile(t);
                continue;
            }
            if (t.Root)
                continue;
            if (built >= SHELF_BUILDS)
            {
                m_ShelfMore = true;
                continue;
            }
            if (t.Index < 0 || t.Index >= m_Data.Goods.Count())
                continue;
            t.Item = ShelfPicture(m_Data.Goods[t.Index]);
            if (!t.Item || !m_ShelfGrid.BuildTile(t))
                continue;
            Relight(t, true);
            built++;
        }
    }

    // Goods the shelf shows in this tab (or under a slot's filter).
    protected bool ShelfShows(OZT_WireGoods g, string cat)
    {
        if (g.Buy <= 0)
            return false;
        // Under a slot's filter: what fits the slot, from every category,
        // and what the thing would take beside what it holds.
        if (m_Filter)
        {
            if (!OZT_Fit.SlotOf(m_Filter.HostType, g.ClassName, m_Filter.SlotName))
                return false;
            return EngineTakes(g.ClassName, m_Filter);
        }
        if (cat != "")
            return g.Category == cat;
        // Money is not goods: the notes stand only in their own tab.
        return g.Category != OZT_Const.CATEGORY_CHANGE;
    }

    // Everything the player carries, one tile each: what the trader pays for
    // with its price, the rest marked as not taken. Money is not shown: it is
    // counted in the footer. What the cart sells whole is in the cart.
    protected void FillOwn()
    {
        m_DirtyOwn = false;
        DropHover();
        if (!m_Data || !m_OwnGrid.IsReady())
            return;
        m_OwnGrid.Clear();
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;
        string cat = m_OwnIds[m_OwnTab];
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        InventoryLocation loc = new InventoryLocation();
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI e = all[i];
            if (!e || e == player)
                continue;
            string type = e.GetType();
            // Money is counted in the footer; the change tab shows the notes
            // and nothing else.
            bool note = m_Data.Notes.Find(type) > -1;
            bool changing = cat == OZT_Const.CATEGORY_CHANGE;
            if (note != changing)
                continue;
            // What is worn or held has a slot's tile; a part of another thing
            // is that thing's business (phase 3). The grid is the bags.
            e.GetInventory().GetCurrentInventoryLocation(loc);
            int place = loc.GetType();
            if (place == InventoryLocationType.ATTACHMENT || place == InventoryLocationType.HANDS)
                continue;
            if (cat != "")
            {
                OZT_WireGoods g = GoodsOf(type);
                if (!g || g.Category != cat)
                    continue;
            }
            OZT_Tile t = OwnTile(e);
            if (!t)
                continue;
            // Only what sells: what the trader takes as it is, and what he
            // would take emptied.
            if (s_OwnSellable && t.Why != 0 && t.Why != 2)
                continue;
            if (m_OwnGrid.Add(t))
                Relight(t, true);
        }
        m_OwnGrid.Arrange();
    }

    // The head of the player's things says which of the two it shows.
    protected void PaintOwnFilter()
    {
        string key = "STR_OZT_UI_YOURS_META";
        int color = OZT_Look.Muted();
        if (s_OwnSellable)
        {
            key = "STR_OZT_UI_YOURS_SELLABLE";
            color = OZT_Look.Accent();
        }
        OZT_Ui.Say(layoutRoot, "OwnFilterBtnText", OZT_Ui.Words(key));
        OZT_Ui.Tint(layoutRoot, "OwnFilterBtnText", color);
    }

    // What a thing of the player's brings sold as it is: with what hangs on it
    // and what is loaded in it, as far as the trader takes that (`open` says
    // it; the quote says it again for the deal as a whole).
    protected int Brings(OZT_WireOwn own)
    {
        if (own.Tree > 0)
            return own.Tree;
        return own.Price;
    }

    // The tile of one thing of the player's: what the trader pays for it, or
    // why it cannot be sold. Null for what the cart sells whole.
    protected OZT_Tile OwnTile(EntityAI e)
    {
        int low;
        int high;
        e.GetNetworkID(low, high);
        OZT_WireOwn own = OwnOf(low, high);
        OZT_Tile t = new OZT_Tile(OZT_Tile.OWN, e.GetType(), e);
        t.Low = low;
        t.High = high;
        if (own)
        {
            t.Price = Brings(own);
            t.Why = own.Why;
            t.Count = own.Units;
        }
        else
        {
            t.Why = 1;
            if (OZT_Stack.Countable(e))
                t.Count = OZT_Stack.Units(e);
        }

        OZT_CartLine line = m_Cart.SellOf(e);
        if (line)
        {
            if (line.Count == 0)
                return null;
            // A part of the stack is in the cart: what stays shows its count;
            // what the trader would pay for it is the server's to say, and
            // nobody has asked.
            t.Count = t.Count - line.Count;
            t.Price = 0;
            // A note is worth its face value whatever the count, and `open`
            // said what one is worth.
            OZT_WireGoods money = GoodsOf(t.Type);
            if (money && money.Category == OZT_Const.CATEGORY_CHANGE)
                t.Price = money.Sell * t.Count;
        }
        return t;
    }

    protected void ClearSlots()
    {
        for (int t = 0; t < m_SlotTiles.Count(); t++)
            m_SlotTiles[t].Unbuild();
        m_SlotTiles.Clear();
        for (int s = 0; s < m_SlotWidgets.Count(); s++)
        {
            if (m_SlotWidgets[s])
                m_SlotWidgets[s].Unlink();
        }
        m_SlotWidgets.Clear();
    }

    // The character's slots as the game's config lists them (the list the
    // vanilla inventory reads, so a slot another mod adds is here too), and the
    // hands: seven to a column from the right edge of the block, the live view
    // of the character taking what is left.
    protected void FillSlots()
    {
        m_DirtySlots = false;
        DropHover();
        Widget host = layoutRoot.FindAnyWidget("CharSlots");
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!m_Data || !host || !player)
            return;
        ClearSlots();

        array<string> labels = new array<string>();
        array<string> icons = new array<string>();
        array<int> ids = new array<int>();
        array<EntityAI> worn = new array<EntityAI>();
        array<string> names = new array<string>();
        GetGame().ConfigGetTextArray(SLOT_LIST, names);
        for (int n = 0; n < names.Count(); n++)
        {
            string path = "CfgSlots " + names[n];
            if (!GetGame().ConfigIsExisting(path))
                continue;
            string icon;
            GetGame().ConfigGetText(path + " ghostIcon", icon);
            string slot;
            GetGame().ConfigGetText(path + " name", slot);
            int id = InventorySlots.GetSlotIdFromString(slot);
            string label = InventorySlots.GetSlotDisplayName(id);
            if (label.IndexOf("#") == 0)
                label = Widget.TranslateString(label);
            labels.Insert(label);
            icons.Insert(icon);
            ids.Insert(id);
            worn.Insert(player.GetInventory().FindAttachment(id));
        }
        labels.Insert(OZT_Ui.Words("STR_OZT_UI_HANDS"));
        icons.Insert("set:dayz_inventory image:hands");
        ids.Insert(InventorySlots.INVALID);
        worn.Insert(player.GetHumanInventory().GetEntityInHands());

        // Units of the layout: the block is 440 wide and 442 high, a slot 64
        // by 58, six between. Beyond four columns the slots narrow, so that
        // the character keeps 160.
        int count = labels.Count();
        int cols = (count + SLOT_ROWS - 1) / SLOT_ROWS;
        float slotW = 64;
        if (cols > 4)
            slotW = Math.Floor((280 - 6 * cols) / cols);
        float block = cols * (slotW + 6);
        float px = OZT_Ui.Px();
        float viewW = Math.Round((440 - block) * px);
        float viewH = Math.Round(442 * px);
        if (m_View)
            m_View.SetSize(viewW, viewH);
        Widget bed = layoutRoot.FindAnyWidget("CharBed");
        if (bed)
            bed.SetSize(viewW, viewH);

        float w = Math.Round(slotW * px);
        float h = Math.Round(58 * px);
        float hold = Math.Round(45 * px);
        for (int i = 0; i < count; i++)
        {
            int col = i / SLOT_ROWS;
            int row = i - col * SLOT_ROWS;
            Widget sw = OZT_Ui.Make(SLOT_LAYOUT, host, SLOT_ROOT);
            if (!sw)
                continue;
            m_SlotWidgets.Insert(sw);
            float x = (440 - block + 6 + col * (slotW + 6)) * px;
            float y = row * 64 * px;
            sw.SetPos(Math.Round(x), Math.Round(y));
            sw.SetSize(w, h);
            Widget bg = sw.FindAnyWidget("SlotBg");
            if (bg)
            {
                bg.SetPos(1, 1);
                bg.SetSize(w - 2, h - 2);
            }
            OZT_Ui.Say(sw, "SlotName", labels[i]);
            ImageWidget img = ImageWidget.Cast(sw.FindAnyWidget("SlotIcon"));
            if (img && icons[i] != "")
                img.LoadImageFile(0, StaticGUIUtils.VerifyIconImageString(StaticGUIUtils.IMAGESETGROUP_INVENTORY, icons[i]));

            Widget into = sw.FindAnyWidget("SlotHold");
            // The slot of what is tried on shows the goods, lit: a tile of the
            // shelf, so a double click on it buys them.
            string triedHere = "";
            if (m_Try && m_Try.IsOn() && ids[i] != InventorySlots.INVALID)
                triedHere = m_Try.TriedOn(ids[i]);
            if (triedHere != "")
            {
                EntityAI stand = m_Shelf.Of(triedHere);
                OZT_WireGoods tg = GoodsOf(triedHere);
                if (stand && tg && into)
                {
                    OZT_Tile tried = new OZT_Tile(OZT_Tile.SHELF, triedHere, stand);
                    tried.Price = tg.Buy;
                    if (tried.Build(into))
                    {
                        tried.Fill(w, hold);
                        tried.SetPicked(true);
                        m_SlotTiles.Insert(tried);
                        if (img)
                            img.Show(false);
                    }
                    continue;
                }
            }

            EntityAI on = worn[i];
            if (!on || m_Data.Notes.Find(on.GetType()) > -1)
                continue;
            OZT_Tile tile = OwnTile(on);
            if (!tile || !into || !tile.Build(into))
                continue;
            tile.Fill(w, hold);
            Relight(tile, true);
            m_SlotTiles.Insert(tile);
            if (img)
                img.Show(false);
        }
    }

    protected void FillCarts()
    {
        m_DirtyCarts = false;
        DropHover();
        if (!m_BuyGrid.IsReady() || !m_SellGrid.IsReady())
            return;
        m_BuyGrid.Clear();
        for (int b = 0; b < m_Cart.Buys.Count(); b++)
        {
            OZT_CartLine buy = m_Cart.Buys[b];
            // Drawn as it will come: what it comes with and what the deal
            // fits to it. A part bought onto the player's own thing is drawn
            // alone.
            array<string> comes = null;
            OZT_WireGoods bought = GoodsOf(buy.Type);
            if (bought && !buy.IsOnto())
                comes = bought.With;
            EntityAI stand;
            OZT_WireGoods sold = GoodsOf(buy.Type);
            if (sold && sold.Look != "")
                stand = m_Shelf.Of(sold.Look);
            else if (buy.Level > 0)
                stand = m_Shelf.LookWorn(buy.Type, buy.Level);
            else
                stand = m_Shelf.LookOf(buy.Type, comes, buy.Fits);
            if (!stand)
                continue;
            OZT_Tile bt = new OZT_Tile(OZT_Tile.BUY, buy.Type, stand);
            bt.Index = b;
            bt.Level = buy.Level;
            if (m_BuyGrid.Add(bt))
                Relight(bt, true);
        }
        m_BuyGrid.Arrange();

        m_SellGrid.Clear();
        for (int s = 0; s < m_Cart.Sells.Count(); s++)
        {
            OZT_CartLine sell = m_Cart.Sells[s];
            if (!sell.Item)
                continue;
            OZT_Tile st = new OZT_Tile(OZT_Tile.SELL, sell.Type, sell.Item);
            st.Index = s;
            st.Low = sell.Low;
            st.High = sell.High;
            if (m_SellGrid.Add(st))
                Relight(st, true);
        }
        m_SellGrid.Arrange();
        PaintCarts();
    }

    // The carts' numbers: every line's price and count, the two totals.
    protected void PaintCarts()
    {
        bool known = true;
        int toPay = 0;
        for (int b = 0; b < m_BuyGrid.Count(); b++)
        {
            OZT_Tile bt = m_BuyGrid.Get(b);
            if (bt.Index >= m_Cart.Buys.Count())
                continue;
            OZT_CartLine buy = m_Cart.Buys[bt.Index];
            bt.Price = buy.Shown();
            bt.Count = 0;
            if (buy.Count > 1)
                bt.Count = buy.Count;
            bt.Parts = buy.Fits.Count();
            bt.PaintMarks();
            toPay = toPay + buy.Shown();
        }
        int toGet = 0;
        for (int s = 0; s < m_SellGrid.Count(); s++)
        {
            OZT_Tile st = m_SellGrid.Get(s);
            if (st.Index >= m_Cart.Sells.Count())
                continue;
            OZT_CartLine sell = m_Cart.Sells[st.Index];
            st.Price = sell.Shown();
            st.Count = 0;
            if (sell.Units > 0)
            {
                st.Count = sell.Count;
                if (sell.Count == 0)
                    st.Count = sell.Units;
            }
            st.PaintMarks();
            if (sell.Shown() < 0)
                known = false;
            else
                toGet = toGet + sell.Shown();
        }

        int buys = m_Cart.Buys.Count();
        int sells = m_Cart.Sells.Count();
        OZT_Ui.Show(layoutRoot, "BuyEmpty", buys == 0);
        OZT_Ui.Show(layoutRoot, "SellEmpty", sells == 0);
        string buyMeta = "";
        if (buys > 0)
            buyMeta = buys.ToString();
        string sellMeta = "";
        if (sells > 0)
            sellMeta = sells.ToString();
        OZT_Ui.Say(layoutRoot, "BuyMeta", buyMeta);
        OZT_Ui.Say(layoutRoot, "SellMeta", sellMeta);
        OZT_Ui.Say(layoutRoot, "BuyTotal", OZT_Ui.Sum(toPay));
        if (known)
            OZT_Ui.Say(layoutRoot, "SellTotal", OZT_Ui.Sum(toGet));
        else
            OZT_Ui.Say(layoutRoot, "SellTotal", "...");
    }

    // What stays with the player of the things they sell, as the server
    // counted it: parts that come off (the trader does not take them, or the
    // player keeps them) and loaded rounds that come back. "" when nothing.
    protected string StaysWords(OZT_DealRes res)
    {
        string s = "";
        if (res.Kept > 0)
            s = OZT_Ui.Words1("STR_OZT_UI_STAYS_PARTS", res.Kept.ToString());
        if (res.KeptRounds > 0)
        {
            if (s != "")
                s = s + ". ";
            s = s + OZT_Ui.Words1("STR_OZT_UI_STAYS_ROUNDS", res.KeptRounds.ToString());
        }
        return s;
    }

    // The deal panel: the server's answer to the last quote, or what the
    // screen is waiting for.
    protected void PaintDeal()
    {
        int balance = 0;
        if (m_Data)
            balance = m_Data.Balance;

        string title = OZT_Ui.Words("STR_OZT_UI_DEAL");
        string net = "0";
        int netColor = OZT_Look.Muted();
        string sold = "0";
        string bought = "0";
        string afterLabel = OZT_Ui.Words("STR_OZT_UI_AFTER");
        string after = OZT_Ui.Sum(balance);
        int afterColor = OZT_Look.Text();
        string space = OZT_Ui.Words("STR_OZT_UI_EMPTY_DEAL");
        int spaceColor = OZT_Look.Muted();
        bool stands = false;

        if (!m_Cart.IsEmpty() && !m_Quote)
        {
            net = "...";
            space = OZT_Ui.Words("STR_OZT_UI_ASKING");
        }
        else if (!m_Cart.IsEmpty())
        {
            if (m_Quote.Sold > 0)
                sold = "+" + OZT_Ui.Sum(m_Quote.Sold);
            if (m_Quote.Bought > 0)
                bought = "-" + OZT_Ui.Sum(m_Quote.Bought);
            // Above zero the player pays.
            int sum = m_Quote.Net;
            if (sum > 0)
            {
                title = OZT_Ui.Words("STR_OZT_UI_YOU_PAY");
                net = "-" + OZT_Ui.Sum(sum);
                netColor = OZT_Look.Accent();
            }
            else if (sum < 0)
            {
                title = OZT_Ui.Words("STR_OZT_UI_YOU_GET");
                net = "+" + OZT_Ui.Sum(-sum);
                netColor = OZT_Look.Good();
            }
            else if (m_QuoteOk)
            {
                title = OZT_Ui.Words("STR_OZT_UI_EVEN");
                netColor = OZT_Look.Text();
            }

            if (m_QuoteOk)
            {
                stands = true;
                after = OZT_Ui.Sum(m_Quote.Balance);
                space = OZT_Ui.Words("STR_OZT_UI_FITS");
                string kept = StaysWords(m_Quote);
                if (kept != "")
                {
                    space = kept;
                    spaceColor = OZT_Look.Text();
                }
                if (m_Quote.InHands != "")
                {
                    string what = OZT_Ui.NameOf(m_Shelf.Of(m_Quote.InHands));
                    space = OZT_Ui.Words1("STR_OZT_UI_TO_HANDS", what);
                    spaceColor = OZT_Look.Alert();
                }
                // Cash first, then the bank: the bank's share is said first.
                if (m_Quote.Bank > 0)
                    space = OZT_Ui.Words1("STR_OZT_UI_FROM_BANK", OZT_Ui.Sum(m_Quote.Bank)) + " " + space;
                else if (m_Quote.Bank < 0)
                    space = OZT_Ui.Words1("STR_OZT_UI_TO_BANK", OZT_Ui.Sum(-m_Quote.Bank));
            }
            else
            {
                space = RefusalWords(m_QuoteWhy, m_Quote.Left);
                spaceColor = OZT_Look.Bad();
                if (m_QuoteWhy == OZT_Const.NO_MONEY)
                {
                    afterLabel = OZT_Ui.Words("STR_OZT_UI_SHORT");
                    after = OZT_Ui.Sum(sum - balance);
                    afterColor = OZT_Look.Bad();
                }
            }
        }

        OZT_Ui.Say(layoutRoot, "DealTitle", title);
        OZT_Ui.Say(layoutRoot, "DealNet", net);
        OZT_Ui.Tint(layoutRoot, "DealNet", netColor);
        OZT_Ui.Say(layoutRoot, "DealSell", sold);
        OZT_Ui.Say(layoutRoot, "DealBuy", bought);
        OZT_Ui.Say(layoutRoot, "DealAfterLabel", afterLabel);
        OZT_Ui.Say(layoutRoot, "DealAfter", after);
        OZT_Ui.Tint(layoutRoot, "DealAfter", afterColor);
        RichTextWidget verdict = RichTextWidget.Cast(layoutRoot.FindAnyWidget("DealSpace"));
        if (verdict)
        {
            // The colour first: a wrapped text takes the colour it has when
            // the text is set (measured: set afterwards, it showed on the
            // next text).
            verdict.SetColor(spaceColor);
            verdict.SetText(space);
        }

        // Confirm wears the accent only while the server's quote stands.
        TextWidget caption = TextWidget.Cast(layoutRoot.FindAnyWidget("ConfirmBtnText"));
        if (stands)
        {
            OZT_Ui.Tint(layoutRoot, "ConfirmBtnBg", OZT_Look.Accent());
            OZT_Ui.Tint(layoutRoot, "ConfirmBtnText", OZT_Look.Dark());
        }
        else
        {
            OZT_Ui.Tint(layoutRoot, "ConfirmBtnBg", OZT_Look.Raised());
            OZT_Ui.Tint(layoutRoot, "ConfirmBtnText", OZT_Look.Muted());
        }
        // Dark letters on the accent are thin at this size; the heavier weight
        // of the same face reads.
        if (caption)
            caption.SetBold(stands);
    }

    // ---- the selection ---------------------------------------------------------------------------

    protected string KeyOf(OZT_Tile t)
    {
        if (t.Kind == OZT_Tile.OWN || t.Kind == OZT_Tile.SELL)
            return t.Low.ToString() + ":" + t.High.ToString();
        if (t.Kind == OZT_Tile.SHELF && t.Level > 0)
            return t.Type + "|L" + t.Level.ToString();
        return t.Type;
    }

    protected bool IsPick(OZT_Tile t)
    {
        if (t.Kind != m_PickKind)
            return false;
        return KeyOf(t) == m_PickKey;
    }

    protected void PickTile(OZT_Tile t)
    {
        m_PickKind = t.Kind;
        m_PickKey = KeyOf(t);
        m_PickType = t.Type;
        AimParts(t);
        PaintTry();
        RepaintPicks(m_ShelfGrid);
        RepaintPicks(m_OwnGrid);
        RepaintPicks(m_BuyGrid);
        RepaintPicks(m_SellGrid);
        for (int s = 0; s < m_SlotTiles.Count(); s++)
            Relight(m_SlotTiles[s], false);
        PaintPick(t);
    }

    // Is the tile the thing the parts strip shows? It stays lit while a part
    // of it is picked in the strip, so the player sees what is being changed.
    protected bool IsHeld(OZT_Tile t)
    {
        if (!m_PartsAt || !t || t.Kind == OZT_Tile.PART)
            return false;
        if (m_PartsAt.Kind == OZT_Tile.OWN || m_PartsAt.Kind == OZT_Tile.SELL)
        {
            if (t.Kind != OZT_Tile.OWN && t.Kind != OZT_Tile.SELL)
                return false;
            return t.Low == m_PartsAt.Low && t.High == m_PartsAt.High;
        }
        if (m_PartsAt.Kind == OZT_Tile.BUY)
            return t.Kind == OZT_Tile.BUY && t.Index == m_PartsAt.Line;
        return t.Kind == m_PartsAt.Kind && t.Type == m_PartsAt.Type;
    }

    // Paints a tile's light anew where its pick or its hold changed (`always`:
    // a tile just made). True: the tile is the pick.
    protected bool Relight(OZT_Tile t, bool always)
    {
        bool on = IsPick(t);
        bool kept = IsHeld(t);
        if (always || on != t.IsPicked() || kept != t.Held)
        {
            t.Held = kept;
            t.SetPicked(on);
        }
        return on;
    }

    // Is the selected thing still on the screen?
    protected bool PickStands()
    {
        // While the shelf shows what fits a slot, the thing whose slot it is
        // stays selected, though its tile may be off the filtered shelf.
        if (m_Filter)
            return true;
        if (PickIn(m_ShelfGrid) || PickIn(m_OwnGrid) || PickIn(m_BuyGrid) || PickIn(m_SellGrid))
            return true;
        if (m_Parts)
        {
            for (int p = 0; p < m_Parts.TileCount(); p++)
            {
                if (IsPick(m_Parts.TileOfIndex(p)))
                    return true;
            }
        }
        for (int s = 0; s < m_SlotTiles.Count(); s++)
        {
            if (IsPick(m_SlotTiles[s]))
                return true;
        }
        return false;
    }

    protected bool PickIn(OZT_TileGrid grid)
    {
        for (int i = 0; i < grid.Count(); i++)
        {
            if (IsPick(grid.Get(i)))
                return true;
        }
        return false;
    }

    protected void Unpick()
    {
        m_PickKind = 0;
        m_PickKey = "";
        m_PickType = "";
        PaintTry();
        OZT_Ui.Say(layoutRoot, "SelName", "");
        OZT_Ui.Say(layoutRoot, "SelPrice", "");
        OZT_Ui.Say(layoutRoot, "SelSub", "");
        m_PartsAt = null;
        ClearFilter();
        m_DirtyParts = true;
        m_PaintPickSoon = true;
    }

    // The thing the strip shows becomes the selection again: a part taken off
    // left the strip with its tile.
    protected void PickHost()
    {
        if (!m_PartsAt)
            return;
        m_PickKind = m_PartsAt.Kind;
        if (m_PartsAt.Kind == OZT_Tile.OWN || m_PartsAt.Kind == OZT_Tile.SELL)
            m_PickKey = m_PartsAt.Low.ToString() + ":" + m_PartsAt.High.ToString();
        else
            m_PickKey = m_PartsAt.Type;
        m_PickType = m_PartsAt.Type;
        m_PaintPickSoon = true;
    }

    protected void RepaintPick()
    {
        m_PaintPickSoon = false;
        RepaintPicks(m_ShelfGrid);
        RepaintPicks(m_OwnGrid);
        RepaintPicks(m_BuyGrid);
        RepaintPicks(m_SellGrid);
        OZT_Tile found = PickedIn(m_BuyGrid);
        if (!found)
            found = PickedIn(m_SellGrid);
        if (!found)
            found = PickedIn(m_OwnGrid);
        if (!found)
            found = PickedIn(m_ShelfGrid);
        for (int s = 0; s < m_SlotTiles.Count(); s++)
        {
            OZT_Tile worn = m_SlotTiles[s];
            bool lit = Relight(worn, false);
            if (lit && !found)
                found = worn;
        }
        if (found)
            PaintPick(found);
    }

    protected OZT_Tile PickedIn(OZT_TileGrid grid)
    {
        for (int i = 0; i < grid.Count(); i++)
        {
            if (IsPick(grid.Get(i)))
                return grid.Get(i);
        }
        return null;
    }

    protected void RepaintPicks(OZT_TileGrid grid)
    {
        for (int i = 0; i < grid.Count(); i++)
            Relight(grid.Get(i), false);
    }

    // The selected thing in words: its name, the sum that applies here and
    // whose sum it is, the game's own description.
    protected void PaintPick(OZT_Tile t)
    {
        // A shelf tile far from the view has no picture of its own yet.
        EntityAI named = t.Item;
        if (!named && t.Kind == OZT_Tile.SHELF)
            named = m_Shelf.Of(t.Type);
        string name = OZT_Ui.NameOf(named);
        string price = "";
        string sub = "";
        if (t.Kind == OZT_Tile.SHELF || t.Kind == OZT_Tile.BUY)
        {
            sub = OZT_Ui.Words("STR_OZT_UI_PRICE");
            if (t.Price > 0)
                price = OZT_Ui.Sum(t.Price);
        }
        else if (t.Why == 1)
        {
            sub = OZT_Ui.Words("STR_OZT_UI_WHY_NOT_TAKEN");
        }
        else if (t.Why == 2)
        {
            sub = OZT_Ui.Words("STR_OZT_UI_WHY_NOT_EMPTY");
            if (t.Price > 0)
                price = OZT_Ui.Sum(t.Price);
        }
        else
        {
            sub = OZT_Ui.Words("STR_OZT_UI_PAYS");
            if (t.Price > 0)
                price = OZT_Ui.Sum(t.Price);
        }
        OZT_Ui.Say(layoutRoot, "SelName", name);
        OZT_Ui.Say(layoutRoot, "SelPrice", price);
        OZT_Ui.Say(layoutRoot, "SelSub", sub);
        PaintTry();
    }

    // ---- the parts strip ---------------------------------------------------------------------------------

    // The strip shows the thing just selected; a part picked in the strip
    // leaves it where it is.
    protected void AimParts(OZT_Tile t)
    {
        if (t.Kind == OZT_Tile.PART)
            return;
        // While the shelf shows what fits a slot, a click on its goods
        // chooses and leaves the strip where it is (design §3.4).
        if (m_Filter && t.Kind == OZT_Tile.SHELF)
            return;
        OZT_PartsAt at = new OZT_PartsAt();
        at.Kind = t.Kind;
        at.Item = t.Item;
        at.Type = t.Type;
        at.Low = t.Low;
        at.High = t.High;
        if (t.Kind == OZT_Tile.BUY || t.Kind == OZT_Tile.SELL)
            at.Line = t.Index;
        m_PartsAt = at;
        ClearFilter();
        m_DirtyParts = true;
    }

    // The strip for a BUY line: after a part was fitted to new goods.
    protected void AimLine(int line)
    {
        if (line < 0 || line >= m_Cart.Buys.Count())
            return;
        OZT_PartsAt at = new OZT_PartsAt();
        at.Kind = OZT_Tile.BUY;
        at.Line = line;
        at.Type = m_Cart.Buys[line].Type;
        at.Item = m_Shelf.Of(at.Type);
        m_PartsAt = at;
        m_PickKind = OZT_Tile.BUY;
        m_PickKey = at.Type;
        m_PickType = at.Type;
        m_DirtyParts = true;
        m_PaintPickSoon = true;
    }

    protected void FillParts()
    {
        m_DirtyParts = false;
        if (!m_Parts)
            return;
        bool gone = !m_PartsAt || !m_Data;
        if (!gone && m_PartsAt.Kind == OZT_Tile.BUY && m_PartsAt.Line >= m_Cart.Buys.Count())
            gone = true;
        if (!gone && m_PartsAt.Kind == OZT_Tile.SELL)
        {
            // The line of a sold thing is found by the thing: lines move.
            m_PartsAt.Line = -1;
            for (int s = 0; s < m_Cart.Sells.Count(); s++)
            {
                if (m_Cart.Sells[s].Item == m_PartsAt.Item)
                    m_PartsAt.Line = s;
            }
            if (m_PartsAt.Line < 0)
                m_PartsAt.Kind = OZT_Tile.OWN;
        }
        if (gone)
        {
            m_Parts.Clear();
            SayParts();
            return;
        }
        array<ref OZT_PartsEntry> entries = new array<ref OZT_PartsEntry>();
        OZT_PartsReader.Read(m_PartsAt, m_Data, m_Cart, m_Shelf, entries);
        m_Parts.Lay(entries);
        int lit = -1;
        if (m_Filter)
        {
            for (int i = 0; i < m_Parts.Count(); i++)
            {
                OZT_PartsEntry e = m_Parts.Entry(i);
                if (e.What == OZT_PartsEntry.EMPTY && e.Host == m_Filter.Host && e.HostFit == m_Filter.HostFit && e.SlotName == m_Filter.SlotName)
                    lit = i;
            }
        }
        m_Parts.Light(lit);
        SayParts();
    }

    // The line under the strip: what its gestures do here.
    protected void SayParts()
    {
        string hint = "";
        if (m_Filter)
        {
            hint = OZT_Ui.Words1("STR_OZT_UI_PARTS_FILTER", SlotWords(m_Filter));
        }
        else if (m_PartsAt && m_Parts && m_Parts.Count() == 0)
        {
            hint = OZT_Ui.Words("STR_OZT_UI_PARTS_NONE");
        }
        else if (m_PartsAt)
        {
            if (m_PartsAt.Kind == OZT_Tile.SELL)
                hint = OZT_Ui.Words("STR_OZT_UI_PARTS_HINT_SELL");
            else if (m_PartsAt.Kind == OZT_Tile.OWN)
                hint = OZT_Ui.Words("STR_OZT_UI_PARTS_HINT_OWN");
            else
                hint = OZT_Ui.Words("STR_OZT_UI_PARTS_HINT_GOODS");
        }
        OZT_Ui.Say(layoutRoot, "PartsHint", hint);
    }

    protected string SlotWords(OZT_PartsEntry e)
    {
        string label = InventorySlots.GetSlotDisplayName(e.SlotId);
        if (label.IndexOf("#") == 0)
            label = Widget.TranslateString(label);
        if (label == "")
            label = e.SlotName;
        return label;
    }

    // The shelf's head: the trader's name, or the slot the shelf is
    // filtered for.
    protected void SayShelfMeta()
    {
        if (!m_Data)
            return;
        if (m_Filter)
        {
            OZT_Ui.Say(layoutRoot, "ShelfMeta", OZT_Ui.Words1("STR_OZT_UI_PARTS_FOR", SlotWords(m_Filter)));
            OZT_Ui.Tint(layoutRoot, "ShelfMeta", OZT_Look.Accent());
            return;
        }
        OZT_Ui.Say(layoutRoot, "ShelfMeta", m_Data.Name);
        OZT_Ui.Tint(layoutRoot, "ShelfMeta", OZT_Look.Muted());
    }

    // A click on an empty slot: the shelf shows what fits it.
    protected void AimFilter(OZT_PartsEntry slot, int cell, bool toggle = true)
    {
        if (!m_PartsAt || !slot)
            return;
        if (m_PartsAt.Kind == OZT_Tile.SELL)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_SELLING"), OZT_Look.Alert());
            return;
        }
        if (m_Filter && m_Filter.Host == slot.Host && m_Filter.HostFit == slot.HostFit && m_Filter.SlotName == slot.SlotName)
        {
            if (toggle)
                ClearFilter();
            return;
        }
        m_Filter = slot;
        if (m_Parts)
            m_Parts.Light(cell);
        m_DirtyShelf = true;
        SayShelfMeta();
        SayParts();
    }

    protected void ClearFilter()
    {
        if (!m_Filter)
            return;
        m_Filter = null;
        if (m_Parts)
            m_Parts.Light(-1);
        m_DirtyShelf = true;
        SayShelfMeta();
        SayParts();
    }

    // Goods onto an empty slot: a part fitted to goods bought in this deal,
    // or bought onto the player's own thing.
    protected void FitGoods(string type, OZT_PartsEntry slot)
    {
        if (!slot || !m_PartsAt || !m_Data)
            return;
        if (!OZT_Fit.SlotOf(slot.HostType, type, slot.SlotName) || !EngineTakes(type, slot))
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_NO_FIT"), OZT_Look.Alert());
            return;
        }
        OZT_WireGoods part = GoodsOf(type);
        if (!part || part.Buy <= 0)
            return;
        int kind = m_PartsAt.Kind;
        if (kind == OZT_Tile.SELL)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_SELLING"), OZT_Look.Alert());
            return;
        }
        if (kind == OZT_Tile.SHELF)
        {
            OZT_WireGoods host = GoodsOf(m_PartsAt.Type);
            if (!host || host.Buy <= 0)
                return;
            int fresh = m_Cart.BuyOne(m_PartsAt.Type, host.Buy);
            m_Cart.AddFit(fresh, type, slot.SlotName, slot.HostFit, part.Buy);
            AimLine(fresh);
        }
        else if (kind == OZT_Tile.BUY)
        {
            int one = m_Cart.SplitOne(m_PartsAt.Line);
            m_Cart.AddFit(one, type, slot.SlotName, slot.HostFit, part.Buy);
            AimLine(one);
        }
        else if (kind == OZT_Tile.OWN)
        {
            if (!slot.Host)
                return;
            int low;
            int high;
            slot.Host.GetNetworkID(low, high);
            // One part bought onto one slot: a second choice replaces the first.
            int prior = OZT_PartsReader.OntoLine(m_Cart, low, high, slot.SlotName);
            if (prior > -1)
                m_Cart.BuyCount(prior, 0);
            m_Cart.BuyOnto(type, part.Buy, low, high, slot.SlotName);
            // What stands there goes to the sale by default; taken out of the
            // sale again, it comes off into the bags with the deal.
            bool stood = slot.What == OZT_PartsEntry.PART && slot.State != OZT_PartsEntry.ONTO;
            if (stood && SellLineOf(slot.Low, slot.High) < 0)
                AddSell(slot.Low, slot.High, true);
        }
        ClearFilter();
        CartChanged();
    }

    // The thing's own script has its word too (an AK takes a bayonet or a
    // suppressor, never both). Asked of a local thing that already holds what
    // the deal puts there -- the goods' look, or a twin of the player's own
    // thing with the parts the cart buys onto it -- as the server asks it
    // (OZT_Deal.CheckFits). A sold thing never gets here.
    protected bool EngineTakes(string type, OZT_PartsEntry slot)
    {
        EntityAI part = m_Shelf.Of(type);
        int slotId = InventorySlots.GetSlotIdFromString(slot.SlotName);
        if (!part || slotId == InventorySlots.INVALID)
            return false;
        EntityAI host = null;
        EntityAI twin = null;
        int kind = m_PartsAt.Kind;
        if (kind == OZT_Tile.SHELF)
        {
            OZT_WireGoods sg = GoodsOf(m_PartsAt.Type);
            array<string> scomes = null;
            if (sg)
                scomes = sg.With;
            host = m_Shelf.LookHost(m_PartsAt.Type, scomes, null, slot.HostFit);
        }
        else if (kind == OZT_Tile.BUY)
        {
            if (m_PartsAt.Line < 0 || m_PartsAt.Line >= m_Cart.Buys.Count())
                return false;
            OZT_CartLine line = m_Cart.Buys[m_PartsAt.Line];
            OZT_WireGoods lg = GoodsOf(line.Type);
            array<string> lcomes = null;
            if (lg && !line.IsOnto())
                lcomes = lg.With;
            host = m_Shelf.LookHost(line.Type, lcomes, line.Fits, slot.HostFit);
        }
        else if (kind == OZT_Tile.OWN && slot.Host)
        {
            twin = OntoTwin(slot.Host, slotId);
            host = twin;
        }
        bool takes = false;
        if (host)
        {
            InventoryLocation loc = new InventoryLocation();
            loc.SetAttachment(host, part, slotId);
            takes = GameInventory.LocationTestAddEntity(loc, false, true, false, false, true, false);
        }
        if (twin)
            GetGame().ObjectDelete(twin);
        return takes;
    }

    // A local twin of a thing the player carries, with its parts and with the
    // parts the cart already buys onto it; the caller deletes it. The slot
    // being filled is left empty -- what stands there and what the cart chose
    // for it are what a new choice replaces -- and so is every part the cart
    // sells.
    protected EntityAI OntoTwin(EntityAI real, int skipSlot)
    {
        EntityAI twin = EntityAI.Cast(GetGame().CreateObjectEx(real.GetType(), "0 0 0", ECE_LOCAL));
        if (!twin)
            return null;
        twin.DisableSimulation(true);
        int n = real.GetInventory().AttachmentCount();
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = real.GetInventory().GetAttachmentFromIndex(i);
            if (!part || !part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            if (il.GetSlot() == skipSlot)
                continue;
            int pl;
            int ph;
            part.GetNetworkID(pl, ph);
            if (SellLineOf(pl, ph) > -1)
                continue;
            EntityAI copy = twin.GetInventory().CreateAttachmentEx(part.GetType(), il.GetSlot());
            if (copy)
                OZT_Shelf.CopyParts(part, copy, 1);
        }
        int low;
        int high;
        real.GetNetworkID(low, high);
        for (int b = 0; b < m_Cart.Buys.Count(); b++)
        {
            OZT_CartLine line = m_Cart.Buys[b];
            if (line.OnLow != low || line.OnHigh != high)
                continue;
            int sid = InventorySlots.GetSlotIdFromString(line.OnSlot);
            if (sid != InventorySlots.INVALID && sid != skipSlot)
                twin.GetInventory().CreateAttachmentEx(line.Type, sid);
        }
        return twin;
    }

    // A double click on a part, or a part dropped on a side: keep it, sell
    // it again, sell it on its own, take a fitted part off.
    protected void PartAct(OZT_PartsEntry e, int zone)
    {
        if (!e || !m_PartsAt)
            return;
        // A part of the player's own thing dropped on their things comes off
        // into the bags, whatever the trader would pay for it.
        bool ownPart = m_PartsAt.Kind == OZT_Tile.OWN && e.What == OZT_PartsEntry.PART;
        if (ownPart && zone == ZONE_OWN && e.State != OZT_PartsEntry.ONTO)
        {
            TakePartOff(e.Low, e.High);
            return;
        }
        if (e.State == OZT_PartsEntry.INCLUDED)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_INCLUDED"), OZT_Look.Alert());
            return;
        }
        if (e.State == OZT_PartsEntry.RIDES)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_RIDES"), OZT_Look.Alert());
            return;
        }
        if (e.State == OZT_PartsEntry.NOT_TAKEN)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_NOT_TAKEN"), OZT_Look.Alert());
            return;
        }
        if (e.State == OZT_PartsEntry.FITTED)
        {
            m_Cart.DropFit(m_PartsAt.Line, e.Fit);
            PickHost();
            CartChanged();
            return;
        }
        if (e.State == OZT_PartsEntry.ONTO)
        {
            m_Cart.BuyCount(e.Line, 0);
            PickHost();
            CartChanged();
            return;
        }
        if (m_PartsAt.Kind == OZT_Tile.SELL)
        {
            int rounds = 0;
            if (e.What == OZT_PartsEntry.ROUNDS)
                rounds = 1;
            bool kept = e.State == OZT_PartsEntry.KEPT;
            bool want = !kept;
            if (zone == ZONE_OWN)
                want = true;
            else if (zone == ZONE_SELL)
                want = false;
            if (want != kept)
            {
                m_Cart.ToggleKeep(m_PartsAt.Line, e.Low, e.High, rounds);
                CartChanged();
            }
            return;
        }
        if (m_PartsAt.Kind == OZT_Tile.OWN && e.What == OZT_PartsEntry.PART)
        {
            // Sold on its own, or, when it is already, out of the deal again.
            int line = SellLineOf(e.Low, e.High);
            if (line > -1)
            {
                m_Cart.Unsell(line);
                CartChanged();
                return;
            }
            if (zone == ZONE_NONE || zone == ZONE_SELL)
                AddSell(e.Low, e.High);
        }
    }

    // The player's own thing, as the game's inventory moves it: dropped on the
    // player's things it comes off the character (or out of the hands) into
    // the bags; dropped on the character it is put on, in a free slot. The
    // engine's predictive moves, as the vanilla inventory makes them; the
    // screen follows when the things change (WatchThings).
    protected void MoveOwn(int low, int high, bool wear)
    {
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(low, high));
        if (!me || !item || item.GetHierarchyRootPlayer() != me)
            return;
        if (SellLineOf(low, high) > -1)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_IN_DEAL"), OZT_Look.Alert());
            return;
        }
        InventoryLocation at = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(at))
            return;
        bool onBody = false;
        if (at.GetType() == InventoryLocationType.HANDS)
            onBody = true;
        else if (at.GetType() == InventoryLocationType.ATTACHMENT && at.GetParent() == me)
            onBody = true;

        if (wear)
        {
            if (onBody)
                return;
            if (!me.GetInventory().CanAddAttachment(item))
            {
                SayInfo(OZT_Ui.Words("STR_OZT_UI_NO_SLOT"), OZT_Look.Alert());
                return;
            }
            me.PredictiveTakeEntityToTargetAttachment(me, item);
            return;
        }
        if (!onBody)
            return;
        InventoryLocation to = new InventoryLocation();
        if (!me.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, to))
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_NO_ROOM"), OZT_Look.Alert());
            return;
        }
        me.PredictiveTakeEntityToInventory(FindInventoryLocationType.CARGO, item);
    }

    // Was the drop on an empty slot of the strip of a thing the player carries?
    protected bool OwnSlotUnder()
    {
        if (!m_ActDropEntry || !m_PartsAt || m_PartsAt.Kind != OZT_Tile.OWN)
            return false;
        return m_ActDropEntry.What == OZT_PartsEntry.EMPTY && m_ActDropEntry.Host != null;
    }

    // A part of the player's own thing taken off into the bags, as the game's
    // inventory takes it: a magazine out of the weapon in the hands through the
    // weapon manager, anything else by a predictive move to a free cargo place.
    protected void TakePartOff(int low, int high)
    {
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        EntityAI part = EntityAI.Cast(GetGame().GetObjectByNetworkId(low, high));
        if (!me || !part || part.GetHierarchyRootPlayer() != me)
            return;
        if (SellLineOf(low, high) > -1)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_IN_DEAL"), OZT_Look.Alert());
            return;
        }
        InventoryLocation src = new InventoryLocation();
        if (!part.GetInventory().GetCurrentInventoryLocation(src))
            return;
        if (src.GetType() != InventoryLocationType.ATTACHMENT)
            return;
        InventoryLocation dst = new InventoryLocation();
        if (!me.GetInventory().FindFreeLocationFor(part, FindInventoryLocationType.CARGO, dst))
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_NO_ROOM"), OZT_Look.Alert());
            return;
        }
        Weapon_Base wpn = Weapon_Base.Cast(src.GetParent());
        if (wpn && Magazine.Cast(part) && me.GetHumanInventory().GetEntityInHands() == wpn)
        {
            me.GetWeaponManager().DetachMagazine(dst);
            return;
        }
        me.PredictiveTakeToDst(src, dst);
    }

    // The player's own loose part put onto an empty slot of a thing they carry,
    // as the game's inventory puts it: a magazine into the weapon in the hands
    // through the weapon manager, anything else by a predictive move, when the
    // engine says the slot takes it.
    protected void PutPartOn(int low, int high, OZT_PartsEntry slot)
    {
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        EntityAI part = EntityAI.Cast(GetGame().GetObjectByNetworkId(low, high));
        EntityAI host = slot.Host;
        if (!me || !part || !host || part == host)
            return;
        if (part.GetHierarchyRootPlayer() != me || host.GetHierarchyRootPlayer() != me)
            return;
        if (SellLineOf(low, high) > -1)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_IN_DEAL"), OZT_Look.Alert());
            return;
        }
        Weapon_Base wpn = Weapon_Base.Cast(host);
        Magazine mag = Magazine.Cast(part);
        if (wpn && mag && me.GetHumanInventory().GetEntityInHands() == wpn)
        {
            me.GetWeaponManager().AttachMagazine(mag);
            return;
        }
        int slotId = slot.SlotId;
        if (slotId == InventorySlots.INVALID || !host.GetInventory().CanAddAttachmentEx(part, slotId))
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_PARTS_NO_FIT"), OZT_Look.Alert());
            return;
        }
        me.PredictiveTakeEntityToTargetAttachmentEx(host, part, slotId);
    }

    protected int SellLineOf(int low, int high)
    {
        for (int i = 0; i < m_Cart.Sells.Count(); i++)
        {
            if (m_Cart.Sells[i].Low == low && m_Cart.Sells[i].High == high)
                return i;
        }
        return -1;
    }

    // ---- what the player does ----------------------------------------------------------------------

    protected OZT_Tile TileOf(Widget w)
    {
        OZT_Tile t = m_ShelfGrid.Of(w);
        if (!t)
            t = m_OwnGrid.Of(w);
        if (!t)
            t = m_BuyGrid.Of(w);
        if (!t)
            t = m_SellGrid.Of(w);
        if (!t && m_Parts)
            t = m_Parts.TileAt(w);
        if (t)
            return t;
        // A slot's tile: the widget itself or anything inside it.
        while (w)
        {
            for (int i = 0; i < m_SlotTiles.Count(); i++)
            {
                if (m_SlotTiles[i].Root == w)
                    return m_SlotTiles[i];
            }
            if (w == m_CharPanel)
                return null;
            w = w.GetParent();
        }
        return null;
    }

    protected bool IsInside(Widget w, Widget panel)
    {
        while (w)
        {
            if (w == panel)
                return true;
            w = w.GetParent();
        }
        return false;
    }

    protected int ZoneOf(Widget w)
    {
        if (IsInside(w, m_BuyPanel))
            return ZONE_BUY;
        if (IsInside(w, m_SellPanel))
            return ZONE_SELL;
        if (IsInside(w, m_ShelfPanel))
            return ZONE_SHELF;
        if (IsInside(w, m_OwnPanel))
            return ZONE_OWN;
        if (IsInside(w, m_CharPanel))
            return ZONE_CHAR;
        return ZONE_NONE;
    }

    protected void AskAct(OZT_Tile t, int zone)
    {
        // A double click may be told twice, by the presses and by the engine's
        // own event: the second telling of one gesture is dropped.
        int now = GetGame().GetTime();
        string told = t.Kind.ToString() + ":" + KeyOf(t) + ":" + zone.ToString();
        if (zone == ZONE_NONE && told == m_ToldKey && now - m_ToldAt < TOLD_TWICE_MS)
            return;
        m_ToldKey = told;
        m_ToldAt = now;
        OZT_Log.Dbg("act: " + told);
        m_ActEntry = null;
        m_ActDropEntry = null;
        if (t.Kind == OZT_Tile.PART && m_Parts)
            m_ActEntry = m_Parts.EntryOfTile(t);
        m_ActKind = t.Kind;
        m_ActType = t.Type;
        m_ActLow = t.Low;
        m_ActHigh = t.High;
        m_ActIndex = t.Index;
        m_ActZone = zone;
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Data)
            return super.OnMouseButtonDown(w, x, y, button);

        // While a dropdown is open: a row takes its category, the list's own
        // scrollbar scrolls, a press anywhere else only shuts it. What is
        // taken is taken in Update, with the button up.
        if (m_PickOpen != PICK_NONE)
        {
            int row = TabOf(w, m_PickRows);
            string openPrefix = "Own";
            if (m_PickOpen == PICK_SHELF)
                openPrefix = "Shelf";
            if (row < 0 && IsInside(w, layoutRoot.FindAnyWidget(openPrefix + "PickList")))
                return false;
            m_PickWant = row;
            m_PickWantSide = m_PickOpen;
            m_PickShutSoon = true;
            return true;
        }
        if (IsInside(w, layoutRoot.FindAnyWidget("ShelfPick")))
        {
            OpenPick(PICK_SHELF);
            return true;
        }
        if (IsInside(w, layoutRoot.FindAnyWidget("OwnPick")))
        {
            OpenPick(PICK_OWN);
            return true;
        }

        if (w && w == m_View)
        {
            int my;
            GetMousePos(m_TurnFrom, my);
            m_Turning = true;
            return true;
        }

        if (m_Parts)
        {
            int cell = m_Parts.EntryAt(w);
            OZT_PartsEntry slot = m_Parts.Entry(cell);
            if (slot && slot.What == OZT_PartsEntry.EMPTY)
            {
                AimFilter(slot, cell);
                return true;
            }
            // A filled slot of the player's own thing filters as well: goods
            // that would take its place (the owner, 2026-11-12). The press goes
            // on to the part itself, so a double click still acts on it.
            bool ownCard = m_PartsAt && m_PartsAt.Kind == OZT_Tile.OWN;
            if (slot && ownCard && slot.What == OZT_PartsEntry.PART && slot.Host)
                AimFilter(slot, cell, false);
        }

        OZT_Tile t = TileOf(w);
        if (!t)
            return super.OnMouseButtonDown(w, x, y, button);

        int now = GetGame().GetTime();
        string key = KeyOf(t);
        bool twice = false;
        if (t.Kind == m_PressKind && key == m_PressKey && now - m_PressAt < DOUBLE_MS)
            twice = true;
        m_PressKind = t.Kind;
        m_PressKey = key;
        m_PressAt = now;
        OZT_Log.Dbg("press: " + t.Kind.ToString() + ":" + key + " twice=" + twice.ToString());
        PickTile(t);
        if (twice)
        {
            m_PressKey = "";
            AskAct(t, ZONE_NONE);
        }
        m_Held = t;
        GetMousePos(m_HeldX, m_HeldY);
        return true;
    }

    // The engine's own double click. Whether the second press of one also
    // comes as a press is not the same for every tile (measured on the stand),
    // so both roads lead to AskAct, which takes one gesture once.
    override bool OnDoubleClick(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Data)
            return super.OnDoubleClick(w, x, y, button);
        OZT_Tile t = TileOf(w);
        if (!t)
            return super.OnDoubleClick(w, x, y, button);
        OZT_Log.Dbg("double click: " + t.Kind.ToString() + ":" + KeyOf(t));
        PickTile(t);
        m_PressKey = "";
        AskAct(t, ZONE_NONE);
        return false;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && w.GetName() == "ConfirmBtn")
        {
            ConfirmDeal();
            return true;
        }
        if (w && w.GetName() == "ClearBtn")
        {
            ClearDeal();
            return true;
        }
        if (w && w.GetName() == "ListBtn")
        {
            ToggleList();
            return true;
        }
        if (w && w.GetName() == "OwnFilterBtn")
        {
            s_OwnSellable = !s_OwnSellable;
            PaintOwnFilter();
            m_DirtyOwn = true;
            return true;
        }
        if (w && w.GetName() == "TryOffBtn")
        {
            EndTry();
            return true;
        }
        if (w && w.GetName() == "TryBtn")
        {
            ToggleTry();
            return true;
        }
        return super.OnClick(w, x, y, button);
    }

    // Keys come through the mission (OZT_TradeMission): a menu's own key
    // event goes only to the widget that holds the keyboard.
    void KeyPressed(int key)
    {
        if (m_Qty && m_Qty.IsOpen())
        {
            if (key == KeyCode.KC_F)
                m_Qty.Accept();
            return;
        }
        if (key == KeyCode.KC_P)
        {
            ToggleList();
            return;
        }
        if (m_List && m_List.IsOpen())
            return;
        if (key == KeyCode.KC_F)
            ConfirmDeal();
        else if (key == KeyCode.KC_R)
            ClearDeal();
    }

    // ---- the card and the price list -------------------------------------------------------------------

    // The row of the open dropdown under the cursor, lit; no card while a list
    // is open.
    protected void PickHoverFrame()
    {
        int at = TabOf(GetWidgetUnderCursor(), m_PickRows);
        if (at == m_PickHover)
            return;
        m_PickHover = at;
        PaintPickRows();
    }

    // The wheel over a shut dropdown's button steps to the next or the
    // previous category: as quick as a tab was.
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (m_PickOpen == PICK_NONE && m_Data && wheel != 0)
        {
            int side = PICK_NONE;
            if (IsInside(w, layoutRoot.FindAnyWidget("ShelfPick")))
                side = PICK_SHELF;
            else if (IsInside(w, layoutRoot.FindAnyWidget("OwnPick")))
                side = PICK_OWN;
            if (side != PICK_NONE)
            {
                int now = m_OwnTab;
                if (side == PICK_SHELF)
                    now = m_ShelfTab;
                int next = now + 1;
                if (wheel > 0)
                    next = now - 1;
                ChooseTab(side, next);
                return true;
            }
        }
        return super.OnMouseWheel(w, x, y, wheel);
    }

    protected void DropHover()
    {
        m_Hover = null;
        m_HoverFor = 0;
        if (m_Card)
            m_Card.Hide();
    }

    // The card of the tile the cursor rests on: not while a button is held,
    // something is dragged, or a window is over the screen.
    protected void HoverFrame(float dt)
    {
        if (!m_Card)
            return;
        bool over = false;
        if (m_Qty && m_Qty.IsOpen())
            over = true;
        if (m_List && m_List.IsOpen())
            over = true;
        OZT_Tile t;
        if (!over && !m_Held && !m_Dragged && !LeftDown())
            t = TileOf(GetWidgetUnderCursor());
        if (t != m_Hover)
        {
            DropHover();
            m_Hover = t;
            return;
        }
        if (!t)
            return;
        m_HoverFor = m_HoverFor + dt;
        if (m_HoverFor >= CARD_AFTER && !m_Card.IsShown())
            OpenCard(t);
        m_Card.Frame();
    }

    // The card of a tile, with what the goods come with at this trader, by
    // the game's names.
    protected void OpenCard(OZT_Tile t)
    {
        // The tile's own record: a stock line has one per condition.
        OZT_WireGoods entry = null;
        if (t.Kind == OZT_Tile.SHELF && m_Data && t.Index >= 0 && t.Index < m_Data.Goods.Count())
        {
            if (m_Data.Goods[t.Index].ClassName == t.Type)
                entry = m_Data.Goods[t.Index];
        }
        else if (t.Kind == OZT_Tile.BUY)
        {
            entry = GoodsAt(t.Type, t.Level);
        }
        if (!entry)
            entry = GoodsOf(t.Type);
        string comes = "";
        if (entry && entry.With)
        {
            for (int w = 0; w < entry.With.Count(); w++)
            {
                if (comes != "")
                    comes = comes + ", ";
                comes = comes + OZT_Ui.NameOf(m_Shelf.Of(entry.With[w]));
            }
        }
        m_Card.Open(t, entry, comes);
    }

    void ToggleList()
    {
        if (!m_List || !m_Data)
            return;
        if (m_List.IsOpen())
        {
            m_List.Shut();
            return;
        }
        if (m_Qty && m_Qty.IsOpen())
            return;
        DropHover();
        m_List.Open(m_Data, m_Shelf);
    }

    // ---- drag and drop ---------------------------------------------------------------------------------
    //
    // The screen follows the mouse itself: a press on a tile, the button
    // followed frame by frame, the widget under the cursor at the release
    // (measured in phase 2a). The engine's own drag (a widget flagged
    // DRAGGABLE, events to the menu) was built first and worked, at a price:
    // the press must be answered "not handled" or no drag begins, every
    // widget on a tile must ignore the pointer or the press lands on what is
    // not draggable, and the engine moves the tile itself under the cursor
    // inside its scrolled grid. None of that is needed here.

    protected void DragFrame()
    {
        if (!m_Held)
            return;
        int mx;
        int my;
        GetMousePos(mx, my);
        if (LeftDown())
        {
            if (!m_Dragged)
            {
                int dx = mx - m_HeldX;
                int dy = my - m_HeldY;
                if (dx * dx + dy * dy < DRAG_FROM * DRAG_FROM)
                    return;
                BeginDrag(m_Held);
            }
            if (m_Ghost)
                m_Ghost.SetPos(mx - m_GrabX, my - m_GrabY);
            int zone = ZoneOf(GetWidgetUnderCursor());
            if (zone != m_DragZone)
            {
                m_DragZone = zone;
                PaintZones();
            }
            return;
        }

        // Released.
        if (m_Dragged)
        {
            Widget under = GetWidgetUnderCursor();
            int onto = ZoneOf(under);
            string name = "nothing";
            if (under)
                name = under.GetName();
            OZT_Log.Dbg("drop: " + m_Dragged.Type + " on '" + name + "', zone " + onto.ToString());
            // A drop on nothing asks for nothing.
            if (onto != ZONE_NONE)
            {
                AskAct(m_Dragged, onto);
                if (m_Parts)
                    m_ActDropEntry = m_Parts.Entry(m_Parts.EntryAt(under));
            }
            EndDrag();
        }
        m_Held = null;
    }

    // The ghost takes the tile's picture and size and hangs on the cursor by
    // the point the tile was taken at.
    protected void BeginDrag(OZT_Tile t)
    {
        m_Dragged = t;
        m_DragZone = ZONE_NONE;
        m_PressKey = "";
        OZT_Log.Dbg("drag: a tile of kind " + t.Kind.ToString() + ", " + t.Type);
        if (!m_Ghost || !t.Root)
            return;
        float tx;
        float ty;
        float tw;
        float th;
        t.Root.GetScreenPos(tx, ty);
        t.Root.GetScreenSize(tw, th);
        m_GrabX = m_HeldX - tx;
        m_GrabY = m_HeldY - ty;
        m_Ghost.SetSize(tw, th);
        if (m_GhostPic)
        {
            m_GhostPic.SetItem(t.Item);
            if (t.Item)
                m_GhostPic.SetView(t.Item.GetViewIndex());
        }
        m_Ghost.Show(true);
    }

    protected void EndDrag()
    {
        m_Dragged = null;
        m_DragZone = ZONE_NONE;
        m_Held = null;
        if (m_GhostPic)
            m_GhostPic.SetItem(null);
        if (m_Ghost)
            m_Ghost.Show(false);
        PaintZones();
    }

    // Would a drop of the dragged tile do something there?
    protected bool ZoneTakes(int kind, int zone)
    {
        if (kind == OZT_Tile.SHELF)
            return zone == ZONE_BUY || zone == ZONE_OWN || zone == ZONE_CHAR;
        if (kind == OZT_Tile.OWN)
            return zone == ZONE_SELL || zone == ZONE_SHELF || zone == ZONE_OWN || zone == ZONE_CHAR;
        if (kind == OZT_Tile.BUY)
            return zone != ZONE_BUY && zone != ZONE_NONE;
        if (kind == OZT_Tile.SELL)
            return zone != ZONE_SELL && zone != ZONE_NONE;
        if (kind == OZT_Tile.PART)
            return zone == ZONE_OWN || zone == ZONE_SELL;
        return false;
    }

    // The bed of the grid a drop would land in is lit while the tile is over it.
    protected void PaintZones()
    {
        int lit = ZONE_NONE;
        if (m_Dragged && ZoneTakes(m_Dragged.Kind, m_DragZone))
            lit = m_DragZone;
        PaintZone("BuyCells", lit == ZONE_BUY);
        PaintZone("SellCells", lit == ZONE_SELL);
        PaintZone("ShelfCells", lit == ZONE_SHELF);
        PaintZone("OwnCells", lit == ZONE_OWN);
    }

    protected void PaintZone(string bed, bool on)
    {
        if (on)
            OZT_Ui.Tint(layoutRoot, bed, OZT_Look.BedLit());
        else
            OZT_Ui.Tint(layoutRoot, bed, OZT_Look.Bed());
    }

    // ---- the deal ------------------------------------------------------------------------------------------

    // A double click, or a drop where it means something.
    protected void RunAct()
    {
        int kind = m_ActKind;
        m_ActKind = 0;
        if (!m_Data || m_DealFlying)
            return;
        if (m_ActZone != ZONE_NONE && !ZoneTakes(kind, m_ActZone))
            return;

        if (kind == OZT_Tile.PART)
        {
            PartAct(m_ActEntry, m_ActZone);
            return;
        }
        // Goods onto an empty slot of the strip: dropped there, or a double
        // click while the shelf shows what fits the slot.
        if (kind == OZT_Tile.SHELF)
        {
            OZT_PartsEntry onto = m_ActDropEntry;
            if (!onto || onto.What != OZT_PartsEntry.EMPTY)
                onto = null;
            if (!onto && m_ActZone == ZONE_NONE)
                onto = m_Filter;
            if (onto)
            {
                FitGoods(m_ActType, onto);
                return;
            }
        }

        if (kind == OZT_Tile.SHELF)
            AddBuy(m_ActType, m_ActIndex);
        else if (kind == OZT_Tile.OWN && OwnSlotUnder())
            PutPartOn(m_ActLow, m_ActHigh, m_ActDropEntry);
        else if (kind == OZT_Tile.OWN && (m_ActZone == ZONE_OWN || m_ActZone == ZONE_CHAR))
            MoveOwn(m_ActLow, m_ActHigh, m_ActZone == ZONE_CHAR);
        else if (kind == OZT_Tile.OWN)
            AddSell(m_ActLow, m_ActHigh);
        else if (kind == OZT_Tile.BUY)
        {
            m_Cart.BuyCount(m_ActIndex, 0);
            CartChanged();
        }
        else if (kind == OZT_Tile.SELL)
        {
            m_Cart.Unsell(m_ActIndex);
            CartChanged();
        }
    }

    // What the notes the cart gives come to, less the notes it takes: what is
    // still to be taken in other notes. Below zero the cart takes more notes
    // than it gives, and the rest is paid like any purchase.
    protected int NotesLeft()
    {
        int left = 0;
        for (int s = 0; s < m_Cart.Sells.Count(); s++)
        {
            OZT_CartLine given = m_Cart.Sells[s];
            OZT_WireGoods gs = GoodsOf(given.Type);
            if (!gs || gs.Category != OZT_Const.CATEGORY_CHANGE)
                continue;
            int units = given.Count;
            if (units <= 0)
                units = given.Units;
            left = left + gs.Sell * units;
        }
        for (int b = 0; b < m_Cart.Buys.Count(); b++)
        {
            OZT_CartLine taken = m_Cart.Buys[b];
            OZT_WireGoods gb = GoodsOf(taken.Type);
            if (!gb || gb.Category != OZT_Const.CATEGORY_CHANGE)
                continue;
            left = left - gb.Buy * taken.Count;
        }
        return left;
    }

    protected void AddBuy(string type, int index = -1)
    {
        OZT_WireGoods g = null;
        if (m_Data && index >= 0 && index < m_Data.Goods.Count() && m_Data.Goods[index].ClassName == type)
            g = m_Data.Goods[index];
        if (!g)
            g = GoodsOf(type);
        if (!g || g.Buy <= 0)
            return;
        // A licence (design §11.2): once, and not one the player holds.
        if (g.Look != "")
        {
            if (HoldsLicence(type) || m_Cart.Buying(type, 0) > 0)
            {
                SayInfo(OZT_Ui.Words("STR_OZT_NO_HAS_LICENCE"), OZT_Look.Alert());
                return;
            }
            m_Cart.Buy(type, 1, g.Buy, 1, 0);
            CartChanged();
            return;
        }
        // A stock line (design §11.1): no more than it holds.
        int left = StockLeft(g);
        if (left == 0)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_SOLD_OUT"), OZT_Look.Alert());
            return;
        }
        m_QtyLevel = g.Level;
        // Under the unit rule the count is units, and the player says how
        // many: a full stack to begin with, five stacks at most at a time.
        if (g.Rule == OZT_Const.RULE_UNIT)
        {
            EntityAI stand = m_Shelf.Of(type);
            int full = 1;
            if (stand && OZT_Stack.Countable(stand))
                full = OZT_Stack.Max(stand);
            if (full < 1)
                full = 1;
            int most = full * 5;
            if (most > OZT_Deal.MAX_UNITS)
                most = OZT_Deal.MAX_UNITS;
            if (left > 0 && most > left)
                most = left;
            if (full > most)
                full = most;
            if (g.Category == OZT_Const.CATEGORY_CHANGE)
            {
                // A note: as many as the notes given leave room for, one when
                // they leave none (the owner's example: a 100 given, the 20
                // taken five times).
                most = OZT_Deal.MAX_UNITS;
                full = NotesLeft() / g.Buy;
                if (full < 1)
                    full = 1;
                m_Qty.ToBuy(type, stand, g.Buy, most, full, true);
                return;
            }
            m_Qty.ToBuy(type, stand, g.Buy, most, full, false);
            return;
        }
        m_Cart.Buy(type, 1, g.Buy, OZT_Deal.MAX_PIECES, g.Level);
        CartChanged();
    }

    // The quantity dialog's OK.
    void QtyChosen(bool selling, string type, int low, int high, int count)
    {
        if (!m_Data || m_DealFlying || count < 1)
            return;
        if (!selling)
        {
            OZT_WireGoods g = GoodsAt(type, m_QtyLevel);
            if (!g)
                g = GoodsOf(type);
            if (!g || g.Buy <= 0)
                return;
            m_Cart.Buy(type, count, g.Buy, OZT_Deal.MAX_UNITS, g.Level);
            CartChanged();
            return;
        }
        OZT_WireOwn own = OwnOf(low, high);
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(low, high));
        if (!own || !item)
            return;
        m_Cart.Sell(item, low, high, count, own.Units, Brings(own));
        CartChanged();
    }

    // `quiet`: a sale another gesture makes on its own way (the old part a
    // bought one replaces) -- no word if the trader does not take the thing,
    // no question how many, and no CartChanged: the gesture calls it once, so
    // what that call says (a line taken out of the deal) stays on the screen.
    protected void AddSell(int low, int high, bool quiet = false)
    {
        OZT_WireOwn own = OwnOf(low, high);
        if (!own)
        {
            if (!quiet)
                SayInfo(OZT_Ui.Words("STR_OZT_UI_WHY_NOT_TAKEN"), OZT_Look.Alert());
            return;
        }
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(low, high));
        if (!item)
            return;
        // A thing with something inside goes with all of it or not at all: it
        // may be put into the deal, and the quote says how much is in the way.
        // More than one unit: the player says how many.
        if (own.Units > 1 && !quiet)
        {
            // Notes are given one to begin with: a note is broken more often
            // than a whole wad.
            if (m_Data.Notes.Find(item.GetType()) > -1)
                m_Qty.ToSell(item, low, high, own.Units, Brings(own), 1, true);
            else
                m_Qty.ToSell(item, low, high, own.Units, Brings(own), own.Units, false);
            return;
        }
        m_Cart.Sell(item, low, high, 0, own.Units, Brings(own));
        if (!quiet)
            CartChanged();
    }

    protected void CartChanged()
    {
        string pruned = PruneParts();
        m_PruneSaid = pruned;
        m_Quote = null;
        m_QuoteTries = 0;
        m_DirtyOwn = true;
        m_DirtySlots = true;
        m_DirtyCarts = true;
        m_DirtyParts = true;
        if (m_Cart.IsEmpty())
            m_QuoteIn = -1;
        else
            QuoteSoon(QUOTE_AFTER);
        if (pruned != "")
            SayInfo(pruned, OZT_Look.Alert());
        else
            SayInfo(OZT_Ui.Words("STR_OZT_UI_HELLO"), OZT_Look.Text());
        PaintDeal();
    }

    // After every change of the cart: a part chosen for a slot the deal has
    // made unavailable -- a light bought for a rifle whose rail handguard the
    // deal replaces with a wooden one -- leaves the deal, and the screen names
    // it (the owner, 2026-11-12). The server's own question (OZT_Deal
    // .CheckFits), asked in passes: what does not go on yet is tried again
    // after the rest (a light waits for the rail bought with it), and what
    // never goes on is out. Before that, the other side of the same rule:
    // what would take off a part the game holds on (PruneOffs). The rule is
    // the engine's for every part and slot, no class is named. Returns the
    // words for what left, or "".
    protected string PruneParts()
    {
        array<string> held = new array<string>();
        array<string> unsold = new array<string>();
        array<string> unkept = new array<string>();
        array<string> offToo = new array<string>();
        PruneOffs(held, unsold, offToo);
        PruneKeeps(unkept);
        array<string> lost = new array<string>();
        PruneOntos(lost);
        PruneFits(lost);
        string say = Said("", "STR_OZT_UI_PARTS_DROPPED", lost);
        say = Said(say, "STR_OZT_UI_OLD_HELD", held);
        say = Said(say, "STR_OZT_UI_SALE_HELD", unsold);
        say = Said(say, "STR_OZT_UI_KEEP_HELD", unkept);
        say = Said(say, "STR_OZT_UI_OFF_TOO", offToo);
        return say;
    }

    // One sentence more for what left the deal: the key's words with the names.
    protected string Said(string say, string key, array<string> names)
    {
        if (names.Count() == 0)
            return say;
        string list = "";
        for (int i = 0; i < names.Count(); i++)
        {
            if (i > 0)
                list = list + ", ";
            list = list + names[i];
        }
        string line = OZT_Ui.Words1(key, list);
        if (say == "")
            return line;
        return say + " " + line;
    }

    // Parts the deal would take off a thing the player keeps while the game
    // holds them on (OZT_Offs; the server's CheckOffs): a part sold off it, the
    // old part a bought one pushes out -- the light that stays on a rifle holds
    // its rail handguard. What holds it on comes off with it, into the bags,
    // as the server does it (OZT_Deal.CheckOffs), and is named. A part nothing
    // frees stays where it is: what is bought into its slot leaves the cart,
    // and so does its own sale.
    protected void PruneOffs(array<string> held, array<string> unsold, array<string> offToo)
    {
        m_OffToo.Clear();
        array<EntityAI> hosts = new array<EntityAI>();
        array<ref array<EntityAI>> leaves = new array<ref array<EntityAI>>();
        for (int s = 0; s < m_Cart.Sells.Count(); s++)
        {
            OZT_CartLine sold = m_Cart.Sells[s];
            if (sold.Item && sold.Count == 0)
                OZT_Offs.AddLeave(hosts, leaves, sold.Item);
        }
        for (int b = 0; b < m_Cart.Buys.Count(); b++)
        {
            OZT_CartLine onto = m_Cart.Buys[b];
            if (!onto.IsOnto())
                continue;
            EntityAI host = EntityAI.Cast(GetGame().GetObjectByNetworkId(onto.OnLow, onto.OnHigh));
            int sid = InventorySlots.GetSlotIdFromString(onto.OnSlot);
            if (!host || sid == InventorySlots.INVALID)
                continue;
            EntityAI there = host.GetInventory().FindAttachment(sid);
            if (there)
                OZT_Offs.AddLeave(hosts, leaves, there);
        }
        for (int h = 0; h < hosts.Count(); h++)
        {
            array<EntityAI> stuck = new array<EntityAI>();
            OZT_Offs.Stuck(hosts[h], leaves[h], null, stuck);
            if (stuck.Count() == 0)
                continue;
            array<EntityAI> holders = new array<EntityAI>();
            if (OZT_Offs.Holders(hosts[h], leaves[h], holders))
            {
                for (int o = 0; o < holders.Count(); o++)
                {
                    m_OffToo.Insert(holders[o]);
                    offToo.Insert(OZT_Ui.NameOf(holders[o]));
                }
                continue;
            }
            for (int k = 0; k < stuck.Count(); k++)
                LetStay(hosts[h], stuck[k], held, unsold);
        }
    }

    protected void LetStay(EntityAI host, EntityAI part, array<string> held, array<string> unsold)
    {
        InventoryLocation il = new InventoryLocation();
        if (!part.GetInventory().GetCurrentInventoryLocation(il))
            return;
        int hl;
        int hh;
        host.GetNetworkID(hl, hh);
        string slot = InventorySlots.GetSlotName(il.GetSlot());
        bool replaced = false;
        int onto = OZT_PartsReader.OntoLine(m_Cart, hl, hh, slot);
        while (onto > -1)
        {
            held.Insert(OZT_Ui.NameOf(m_Shelf.Of(m_Cart.Buys[onto].Type)));
            m_Cart.BuyCount(onto, 0);
            replaced = true;
            onto = OZT_PartsReader.OntoLine(m_Cart, hl, hh, slot);
        }
        int pl;
        int ph;
        part.GetNetworkID(pl, ph);
        int line = SellLineOf(pl, ph);
        if (line < 0)
            return;
        // The sale the replacement put there goes without a word of its own.
        if (!replaced)
            unsold.Insert(OZT_Ui.NameOf(part));
        m_Cart.Unsell(line);
    }

    // A part kept off a sold thing while the game holds it on (an Atlas bipod
    // never comes off): the thing's sale leaves the cart, so nothing the
    // player meant to keep goes to the trader. The thing's other parts are the
    // trader's to take off first, in any order. A part the trader does not
    // take comes off the same way; the server answers for that one.
    protected void PruneKeeps(array<string> unkept)
    {
        for (int s = m_Cart.Sells.Count() - 1; s >= 0; s--)
        {
            OZT_CartLine line = m_Cart.Sells[s];
            if (!line.Item || line.Keep.Count() == 0)
                continue;
            array<EntityAI> hosts = new array<EntityAI>();
            array<ref array<EntityAI>> leaves = new array<ref array<EntityAI>>();
            for (int k = 0; k < line.Keep.Count(); k++)
            {
                OZT_WireKeep keep = line.Keep[k];
                if (keep.Rounds != 0)
                    continue;
                EntityAI kept = EntityAI.Cast(GetGame().GetObjectByNetworkId(keep.Low, keep.High));
                if (kept && !RidesKept(line, kept))
                    OZT_Offs.AddLeave(hosts, leaves, kept);
            }
            bool stuck = false;
            for (int h = 0; h < hosts.Count(); h++)
            {
                array<EntityAI> may = new array<EntityAI>();
                OZT_Offs.PartsOf(hosts[h], may);
                array<EntityAI> held = new array<EntityAI>();
                OZT_Offs.Stuck(hosts[h], leaves[h], may, held);
                if (held.Count() > 0)
                    stuck = true;
            }
            if (!stuck)
                continue;
            unkept.Insert(OZT_Ui.NameOf(line.Item));
            m_Cart.Unsell(s);
        }
    }

    // Does the part hang on another part the line keeps? Then it goes with
    // that one and does not come off on its own.
    protected bool RidesKept(OZT_CartLine line, EntityAI part)
    {
        InventoryLocation il = new InventoryLocation();
        EntityAI at = part;
        while (at && at != line.Item)
        {
            if (!at.GetInventory().GetCurrentInventoryLocation(il))
                return false;
            if (il.GetType() != InventoryLocationType.ATTACHMENT)
                return false;
            EntityAI up = il.GetParent();
            if (!up || up == line.Item)
                return false;
            int ul;
            int uh;
            up.GetNetworkID(ul, uh);
            if (line.Keeps(ul, uh, 0))
                return true;
            at = up;
        }
        return false;
    }

    // Would `host` take a thing of `type` in that slot now, as the engine sees it?
    protected bool Takes(EntityAI host, string type, int slotId)
    {
        EntityAI part = m_Shelf.Of(type);
        if (!host || !part || slotId == InventorySlots.INVALID)
            return false;
        InventoryLocation loc = new InventoryLocation();
        loc.SetAttachment(host, part, slotId);
        return GameInventory.LocationTestAddEntity(loc, false, true, false, false, true, false);
    }

    // A local twin of a carried thing as the deal leaves it: its parts but the
    // ones the cart sells and the ones in `freed` (the slots parts are bought
    // onto, whose old parts the deal sells or puts in the bags). The caller
    // deletes it.
    protected EntityAI BareTwin(EntityAI real, array<int> freed)
    {
        EntityAI twin = EntityAI.Cast(GetGame().CreateObjectEx(real.GetType(), "0 0 0", ECE_LOCAL));
        if (!twin)
            return null;
        twin.DisableSimulation(true);
        int n = real.GetInventory().AttachmentCount();
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = real.GetInventory().GetAttachmentFromIndex(i);
            if (!part || !part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            if (freed.Find(il.GetSlot()) > -1)
                continue;
            if (m_OffToo.Find(part) > -1)
                continue;
            int pl;
            int ph;
            part.GetNetworkID(pl, ph);
            if (SellLineOf(pl, ph) > -1)
                continue;
            EntityAI copy = twin.GetInventory().CreateAttachmentEx(part.GetType(), il.GetSlot());
            if (copy)
                OZT_Shelf.CopyParts(part, copy, 1);
        }
        return twin;
    }

    // Parts bought onto the player's own things, thing by thing.
    protected void PruneOntos(array<string> lost)
    {
        array<int> drop = new array<int>();
        array<int> doneLow = new array<int>();
        array<int> doneHigh = new array<int>();
        for (int b = 0; b < m_Cart.Buys.Count(); b++)
        {
            OZT_CartLine first = m_Cart.Buys[b];
            if (!first.IsOnto())
                continue;
            bool seen = false;
            for (int d = 0; d < doneLow.Count(); d++)
            {
                if (doneLow[d] == first.OnLow && doneHigh[d] == first.OnHigh)
                    seen = true;
            }
            if (seen)
                continue;
            doneLow.Insert(first.OnLow);
            doneHigh.Insert(first.OnHigh);
            EntityAI host = EntityAI.Cast(GetGame().GetObjectByNetworkId(first.OnLow, first.OnHigh));
            if (!host)
                continue;

            array<int> lines = new array<int>();
            array<int> slots = new array<int>();
            for (int k = 0; k < m_Cart.Buys.Count(); k++)
            {
                OZT_CartLine l = m_Cart.Buys[k];
                if (l.OnLow != first.OnLow || l.OnHigh != first.OnHigh)
                    continue;
                lines.Insert(k);
                slots.Insert(InventorySlots.GetSlotIdFromString(l.OnSlot));
            }
            EntityAI twin = BareTwin(host, slots);
            if (!twin)
                continue;
            array<bool> placed = new array<bool>();
            for (int p = 0; p < lines.Count(); p++)
                placed.Insert(false);
            bool progress = true;
            while (progress)
            {
                progress = false;
                for (int q = 0; q < lines.Count(); q++)
                {
                    if (placed[q])
                        continue;
                    string qType = m_Cart.Buys[lines[q]].Type;
                    if (!Takes(twin, qType, slots[q]))
                        continue;
                    twin.GetInventory().CreateAttachmentEx(qType, slots[q]);
                    placed[q] = true;
                    progress = true;
                }
            }
            GetGame().ObjectDelete(twin);
            for (int r = 0; r < lines.Count(); r++)
            {
                if (!placed[r])
                    drop.Insert(lines[r]);
            }
        }
        // The last first, so the indices before them hold.
        drop.Sort();
        for (int x = drop.Count() - 1; x >= 0; x--)
        {
            lost.Insert(OZT_Ui.NameOf(m_Shelf.Of(m_Cart.Buys[drop[x]].Type)));
            m_Cart.BuyCount(drop[x], 0);
        }
    }

    // Parts fitted to goods bought in this deal, line by line.
    protected void PruneFits(array<string> lost)
    {
        for (int b = 0; b < m_Cart.Buys.Count(); b++)
        {
            OZT_CartLine line = m_Cart.Buys[b];
            if (line.IsOnto() || line.Fits.Count() == 0)
                continue;
            EntityAI fresh = EntityAI.Cast(GetGame().CreateObjectEx(line.Type, "0 0 0", ECE_LOCAL));
            if (!fresh)
                continue;
            fresh.DisableSimulation(true);
            OZT_WireGoods g = GoodsOf(line.Type);
            if (g && g.With)
            {
                array<string> taken = new array<string>();
                for (int w = 0; w < g.With.Count(); w++)
                {
                    string ws = OZT_Fit.SlotFor(line.Type, g.With[w], taken);
                    if (ws == "")
                        continue;
                    taken.Insert(ws);
                    fresh.GetInventory().CreateAttachmentEx(g.With[w], InventorySlots.GetSlotIdFromString(ws));
                }
            }
            int count = line.Fits.Count();
            array<EntityAI> made = new array<EntityAI>();
            array<bool> placed = new array<bool>();
            for (int i = 0; i < count; i++)
            {
                made.Insert(null);
                placed.Insert(false);
            }
            bool progress = true;
            while (progress)
            {
                progress = false;
                for (int f = 0; f < count; f++)
                {
                    if (placed[f])
                        continue;
                    OZT_CartFit fit = line.Fits[f];
                    EntityAI on = fresh;
                    if (fit.On >= 0)
                    {
                        on = null;
                        if (fit.On < count && placed[fit.On])
                            on = made[fit.On];
                    }
                    int sid = InventorySlots.GetSlotIdFromString(fit.SlotName);
                    if (!on || !Takes(on, fit.Type, sid))
                        continue;
                    made[f] = on.GetInventory().CreateAttachmentEx(fit.Type, sid);
                    if (!made[f])
                        continue;
                    placed[f] = true;
                    progress = true;
                }
            }
            GetGame().ObjectDelete(fresh);
            // The last first: a fit dropped takes what hangs on it along.
            for (int x = count - 1; x >= 0; x--)
            {
                if (placed[x] || x >= line.Fits.Count())
                    continue;
                lost.Insert(OZT_Ui.NameOf(m_Shelf.Of(line.Fits[x].Type)));
                m_Cart.DropFit(b, x);
            }
        }
    }

    void ClearDeal()
    {
        if (m_DealFlying || m_Cart.IsEmpty())
            return;
        m_Cart.Clear();
        CartChanged();
    }

    // Sends the deal the last quote was for, with the net it gave.
    void ConfirmDeal()
    {
        // Not while the pages of an open come: the deal's own part of the open
        // would cross them.
        if (m_DealFlying || m_Streaming || m_Cart.IsEmpty())
            return;
        if (!m_Quote || !m_QuoteOk || m_QuoteFor != m_Cart.Revision)
            return;
        string json = m_Cart.Request(m_Trader, m_Quote.Net);
        if (json == "")
            return;
        m_DealFlying = true;
        SayInfo(OZT_Ui.Words("STR_OZT_UI_ASKING"), OZT_Look.Muted());
        OZ_Rpc.ServiceRequest(OZT_Const.SERVICE, OZT_Const.OP_DEAL, json);
    }
}
