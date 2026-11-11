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
    static const string TAB_LAYOUT = "OpenZone_Trader/gui/layouts/ozt_trade_tab.layout";
    static const string TAB_ROOT   = "OZT_TradeTab";

    // The tab strips, in layout units (ozt_trade.json): where a strip starts
    // and where the grid under it ends; a tab's height in one row and in
    // several; how wide a tab must be for its name.
    static const float STRIP_TOP   = 48;
    static const float GRID_BOTTOM = 492;
    static const float TAB_HIGH    = 44;
    static const float TAB_LOW     = 30;
    static const float TAB_LETTER  = 7;
    static const float TAB_AIR     = 24;
    static const float TAB_LEAST   = 72;
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
    protected ref array<string> m_TabIds;
    protected ref array<Widget> m_ShelfTabs;
    protected ref array<Widget> m_OwnTabs;
    protected int m_ShelfTab;
    protected int m_OwnTab;

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

    // What must be laid anew at the next quiet moment.
    protected bool m_DirtyShelf;
    protected bool m_DirtyOwn;
    protected bool m_DirtyCarts;
    protected bool m_DirtySlots;

    // The character's slots and the tiles of what is worn in them.
    protected ref array<Widget> m_SlotWidgets;
    protected ref array<ref OZT_Tile> m_SlotTiles;

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
        m_ShelfTabs = new array<Widget>();
        m_OwnTabs = new array<Widget>();
        m_SlotWidgets = new array<Widget>();
        m_SlotTiles = new array<ref OZT_Tile>();
        m_QuoteIn = -1;
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
        HoverFrame(timeslice);

        if (!LeftDown() && !m_Dragged)
        {
            if (m_ActKind != 0)
                RunAct();
            if (m_DirtyShelf)
                FillShelf();
            if (m_DirtyOwn)
                FillOwn();
            if (m_DirtySlots)
                FillSlots();
            if (m_DirtyCarts)
                FillCarts();
            if (m_DirtyParts)
                FillParts();
            if (m_PaintPickSoon)
                RepaintPick();
            if (m_PickKind != 0 && !PickStands())
                Unpick();
        }

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

    protected void AskOpen()
    {
        OZT_OpenReq req = new OZT_OpenReq();
        req.Trader = m_Trader;
        string json;
        string err;
        if (!JsonFileLoader<OZT_OpenReq>.MakeData(req, json, err, false))
            return;
        m_OpenFlying = true;
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
                SayInfo(OZT_Ui.Words(error), OZT_Look.Bad());
                return;
            }
            TakeOpen(json);
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

    protected void TakeOpen(string json)
    {
        OZT_OpenRes res = new OZT_OpenRes();
        string err;
        if (!JsonFileLoader<OZT_OpenRes>.LoadData(json, res, err))
        {
            OZT_Log.Error("the answer of open could not be read: " + err);
            SayInfo(OZT_Ui.Words(OZT_Const.NO_INTERNAL), OZT_Look.Bad());
            return;
        }
        bool first = m_Data == null;
        m_Data = res;
        if (first)
        {
            MakeTabs();
            SayInfo(OZT_Ui.Words("STR_OZT_UI_HELLO"), OZT_Look.Text());
        }
        SayShelfMeta();
        OZT_Ui.Say(layoutRoot, "OwnMoney", OZT_Ui.Sum(res.Balance));
        m_DirtyParts = true;

        if (m_Cart.Prune(res) > 0)
            m_Quote = null;
        m_DirtyShelf = true;
        m_DirtyOwn = true;
        m_DirtySlots = true;
        m_DirtyCarts = true;
        m_Seen = ThingsSignature();
        if (!m_Cart.IsEmpty() && !m_Quote)
            QuoteSoon(QUOTE_AFTER);
        PaintDeal();
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
            EndTry();
            m_Cart.Clear();
            m_Quote = null;
            m_DirtyOwn = true;
            m_DirtySlots = true;
            m_DirtyCarts = true;
            SayInfo(OZT_Ui.Words("STR_OZT_UI_DONE"), OZT_Look.Good());
            PaintDeal();
            AskOpen();
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
        AskOpen();
    }

    protected int ThingsSignature()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return 0;
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        int sig = all.Count();
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
        if (on)
        {
            OZT_Ui.Say(layoutRoot, "TryBtnText", OZT_Ui.Words("STR_OZT_UI_TAKE_OFF"));
            OZT_Ui.Say(layoutRoot, "CharMeta", OZT_Ui.Words("STR_OZT_UI_TRYING"));
            OZT_Ui.Tint(layoutRoot, "CharMeta", OZT_Look.Accent());
        }
        else
        {
            OZT_Ui.Say(layoutRoot, "TryBtnText", OZT_Ui.Words("STR_OZT_UI_TRY_ON"));
            OZT_Ui.Say(layoutRoot, "CharMeta", "");
        }
    }

    // The button: on with the selected goods, or off.
    protected void ToggleTry()
    {
        if (!m_Try)
            return;
        if (m_Try.IsOn())
        {
            EndTry();
            return;
        }
        if (m_PickType == "" || !m_Try.Put(m_PickType))
            return;
        m_DirtySlots = true;
        PaintTry();
    }

    // Trying ends when something else is selected, when the goods are taken
    // off, when a deal is made and when the screen closes.
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

    protected void MakeTabs()
    {
        m_TabIds.Clear();
        array<string> names = new array<string>();
        m_TabIds.Insert("");
        names.Insert(OZT_Ui.Words("STR_OZT_UI_ALL"));
        for (int c = 0; c < m_Data.Categories.Count(); c++)
        {
            m_TabIds.Insert(m_Data.Categories[c].Id);
            // The change tab is the server's, not an admin's category: the
            // screen names it in the player's language.
            if (m_Data.Categories[c].Id == OZT_Const.CATEGORY_CHANGE)
                names.Insert(OZT_Ui.Words("STR_OZT_UI_CHANGE"));
            else
                names.Insert(m_Data.Categories[c].Name);
        }
        float shelfStrip = MakeStrip("ShelfTabs", names, m_ShelfTabs);
        float ownStrip = MakeStrip("OwnTabs", names, m_OwnTabs);
        // The grid starts below its strip and ends where the layout ends it.
        float px = OZT_Ui.Px();
        float top = Math.Round(STRIP_TOP * px);
        float gap = Math.Round(4 * px);
        float bottom = Math.Round(GRID_BOTTOM * px);
        m_ShelfGrid.Reframe(top + shelfStrip + gap, bottom);
        m_OwnGrid.Reframe(top + ownStrip + gap, bottom);
        PaintTabs();
    }

    // The tabs of a strip, in as many rows as their names need: one row
    // while every tab has room for the longest name, more when the admin's
    // categories are many (the owner, 2026-11-11: categories are the admin's
    // own). Tabs of a row share its width. Returns the strip's height in
    // pixels; the grid below gives that height up.
    protected float MakeStrip(string strip, array<string> names, array<Widget> into)
    {
        Widget host = layoutRoot.FindAnyWidget(strip);
        if (!host)
            return 0;
        for (int old = 0; old < into.Count(); old++)
        {
            if (into[old])
                into[old].Unlink();
        }
        into.Clear();

        float px = OZT_Ui.Px();
        float gap = Math.Round(4 * px);
        float all = OZT_TileGrid.COLS * Math.Floor(OZT_TileGrid.CELL_UNITS * px);
        int n = names.Count();
        if (n < 1)
            return 0;

        // The longest name by its letters: about seven units a letter in the
        // tab's font, and some air on both sides.
        int longest = 0;
        for (int k = 0; k < n; k++)
        {
            int letters = names[k].LengthUtf8();
            if (letters > longest)
                longest = letters;
        }
        float need = (longest * TAB_LETTER + TAB_AIR) * px;
        float least = TAB_LEAST * px;
        if (need < least)
            need = least;
        int perRow = Math.Floor((all + gap) / (need + gap));
        if (perRow < 1)
            perRow = 1;
        int rows = (n + perRow - 1) / perRow;
        int inRow = (n + rows - 1) / rows;
        float h = Math.Round(TAB_HIGH * px);
        if (rows > 1)
            h = Math.Round(TAB_LOW * px);

        for (int i = 0; i < n; i++)
        {
            Widget tab = OZT_Ui.Make(TAB_LAYOUT, host, TAB_ROOT);
            if (!tab)
                continue;
            int row = i / inRow;
            int col = i - row * inRow;
            int here = n - row * inRow;
            if (here > inRow)
                here = inRow;
            float w = Math.Floor((all - gap * (here - 1)) / here);
            tab.SetUserID(i);
            tab.SetPos(col * (w + gap), row * (h + gap));
            tab.SetSize(w, h);
            OZT_Ui.Say(tab, "TabText", names[i]);
            into.Insert(tab);
        }
        float tall = rows * h + (rows - 1) * gap;
        host.SetSize(all, tall);
        return tall;
    }

    protected void PaintTabs()
    {
        PaintStrip(m_ShelfTabs, m_ShelfTab);
        PaintStrip(m_OwnTabs, m_OwnTab);
    }

    protected bool IsChangeTab(int tab)
    {
        if (tab < 0 || tab >= m_TabIds.Count())
            return false;
        return m_TabIds[tab] == OZT_Const.CATEGORY_CHANGE;
    }

    // The change tab is one for both sides: the trader's notes on the shelf
    // and the player's own beside them. Opened on one side it opens on the
    // other; left on one side, the other goes back to everything.
    protected void FollowChange(int tab, bool fromShelf)
    {
        bool change = IsChangeTab(tab);
        if (fromShelf)
        {
            if (change && m_OwnTab != tab)
            {
                m_OwnTab = tab;
                m_DirtyOwn = true;
                m_DirtySlots = true;
            }
            else if (!change && IsChangeTab(m_OwnTab))
            {
                m_OwnTab = 0;
                m_DirtyOwn = true;
                m_DirtySlots = true;
            }
            return;
        }
        if (change && m_ShelfTab != tab)
        {
            m_ShelfTab = tab;
            m_DirtyShelf = true;
        }
        else if (!change && IsChangeTab(m_ShelfTab))
        {
            m_ShelfTab = 0;
            m_DirtyShelf = true;
        }
    }

    protected void PaintStrip(array<Widget> tabs, int current)
    {
        for (int i = 0; i < tabs.Count(); i++)
        {
            Widget tab = tabs[i];
            if (!tab)
                continue;
            bool on = tab.GetUserID() == current;
            OZT_Ui.Show(tab, "TabLine", on);
            if (on)
            {
                OZT_Ui.Tint(tab, "TabText", OZT_Look.Text());
                OZT_Ui.Tint(tab, "TabBg", OZT_Look.TileLit());
            }
            else
            {
                OZT_Ui.Tint(tab, "TabText", OZT_Look.Muted());
                OZT_Ui.Tint(tab, "TabBg", OZT_Look.Raised());
            }
        }
    }

    // The index of the tab a widget is, or is in; -1 when it is none.
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

    protected OZT_WireGoods GoodsOf(string type)
    {
        if (!m_Data)
            return null;
        for (int i = 0; i < m_Data.Goods.Count(); i++)
        {
            if (m_Data.Goods[i].ClassName == type)
                return m_Data.Goods[i];
        }
        return null;
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
        m_ShelfGrid.Clear();
        string cat = m_TabIds[m_ShelfTab];
        for (int i = 0; i < m_Data.Goods.Count(); i++)
        {
            OZT_WireGoods g = m_Data.Goods[i];
            if (g.Buy <= 0)
                continue;
            // Under a slot's filter: what fits the slot, from every category.
            if (m_Filter)
            {
                if (!OZT_Fit.SlotOf(m_Filter.HostType, g.ClassName, m_Filter.SlotName))
                    continue;
            }
            else if (cat != "" && g.Category != cat)
            {
                continue;
            }
            else if (cat == "" && g.Category == OZT_Const.CATEGORY_CHANGE)
            {
                // Money is not goods: the notes stand only in their own tab.
                continue;
            }
            EntityAI e = m_Shelf.Of(g.ClassName);
            if (!e)
                continue;
            OZT_Tile t = new OZT_Tile(OZT_Tile.SHELF, g.ClassName, e);
            t.Price = g.Buy;
            t.Index = i;
            if (m_ShelfGrid.Add(t))
                t.SetPicked(IsPick(t));
        }
        m_ShelfGrid.Arrange();
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
        string cat = m_TabIds[m_OwnTab];
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
            if (t && m_OwnGrid.Add(t))
                t.SetPicked(IsPick(t));
        }
        m_OwnGrid.Arrange();
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
            if (m_Try && m_Try.IsOn() && ids[i] != InventorySlots.INVALID && ids[i] == m_Try.SlotId())
            {
                EntityAI stand = m_Shelf.Of(m_Try.Tried());
                OZT_WireGoods tg = GoodsOf(m_Try.Tried());
                if (stand && tg && into)
                {
                    OZT_Tile tried = new OZT_Tile(OZT_Tile.SHELF, m_Try.Tried(), stand);
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
            tile.SetPicked(IsPick(tile));
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
            EntityAI stand = m_Shelf.Of(buy.Type);
            if (!stand)
                continue;
            OZT_Tile bt = new OZT_Tile(OZT_Tile.BUY, buy.Type, stand);
            bt.Index = b;
            if (m_BuyGrid.Add(bt))
                bt.SetPicked(IsPick(bt));
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
                st.SetPicked(IsPick(st));
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
        if (m_Try && m_Try.IsOn() && m_Try.Tried() != t.Type)
            EndTry();
        RepaintPicks(m_ShelfGrid);
        RepaintPicks(m_OwnGrid);
        RepaintPicks(m_BuyGrid);
        RepaintPicks(m_SellGrid);
        for (int s = 0; s < m_SlotTiles.Count(); s++)
        {
            OZT_Tile held = m_SlotTiles[s];
            bool lit = IsPick(held);
            if (lit != held.IsPicked())
                held.SetPicked(lit);
        }
        PaintPick(t);
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
        EndTry();
        PaintTry();
        OZT_Ui.Say(layoutRoot, "SelName", "");
        OZT_Ui.Say(layoutRoot, "SelPrice", "");
        OZT_Ui.Say(layoutRoot, "SelSub", "");
        m_PartsAt = null;
        ClearFilter();
        m_DirtyParts = true;
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
            OZT_Tile held = m_SlotTiles[s];
            bool lit = IsPick(held);
            if (lit != held.IsPicked())
                held.SetPicked(lit);
            if (lit && !found)
                found = held;
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
        {
            OZT_Tile t = grid.Get(i);
            bool on = IsPick(t);
            if (on != t.IsPicked())
                t.SetPicked(on);
        }
    }

    // The selected thing in words: its name, the sum that applies here and
    // whose sum it is, the game's own description.
    protected void PaintPick(OZT_Tile t)
    {
        string name = OZT_Ui.NameOf(t.Item);
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
    protected void AimFilter(OZT_PartsEntry slot, int cell)
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
        if (!OZT_Fit.SlotOf(slot.HostType, type, slot.SlotName))
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
            m_Cart.BuyOnto(type, part.Buy, low, high, slot.SlotName);
        }
        ClearFilter();
        CartChanged();
    }

    // A double click on a part, or a part dropped on a side: keep it, sell
    // it again, sell it on its own, take a fitted part off.
    protected void PartAct(OZT_PartsEntry e, int zone)
    {
        if (!e || !m_PartsAt)
            return;
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

        int tab = TabOf(w, m_ShelfTabs);
        if (tab > -1)
        {
            if (tab != m_ShelfTab)
            {
                m_ShelfTab = tab;
                m_DirtyShelf = true;
                FollowChange(tab, true);
                PaintTabs();
            }
            return true;
        }
        tab = TabOf(w, m_OwnTabs);
        if (tab > -1)
        {
            if (tab != m_OwnTab)
            {
                m_OwnTab = tab;
                m_DirtyOwn = true;
                m_DirtySlots = true;
                FollowChange(tab, false);
                PaintTabs();
            }
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
        OZT_WireGoods entry = GoodsOf(t.Type);
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
            return zone == ZONE_SELL || zone == ZONE_SHELF;
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
            AddBuy(m_ActType);
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

    protected void AddBuy(string type)
    {
        OZT_WireGoods g = GoodsOf(type);
        if (!g || g.Buy <= 0)
            return;
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
        m_Cart.Buy(type, 1, g.Buy, OZT_Deal.MAX_PIECES);
        CartChanged();
    }

    // The quantity dialog's OK.
    void QtyChosen(bool selling, string type, int low, int high, int count)
    {
        if (!m_Data || m_DealFlying || count < 1)
            return;
        if (!selling)
        {
            OZT_WireGoods g = GoodsOf(type);
            if (!g || g.Buy <= 0)
                return;
            m_Cart.Buy(type, count, g.Buy, OZT_Deal.MAX_UNITS);
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

    protected void AddSell(int low, int high)
    {
        OZT_WireOwn own = OwnOf(low, high);
        if (!own)
        {
            SayInfo(OZT_Ui.Words("STR_OZT_UI_WHY_NOT_TAKEN"), OZT_Look.Alert());
            return;
        }
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(low, high));
        if (!item)
            return;
        // A thing with something inside goes with all of it or not at all: it
        // may be put into the deal, and the quote says how much is in the way.
        // More than one unit: the player says how many.
        if (own.Units > 1)
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
        CartChanged();
    }

    protected void CartChanged()
    {
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
        SayInfo(OZT_Ui.Words("STR_OZT_UI_HELLO"), OZT_Look.Text());
        PaintDeal();
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
        if (m_DealFlying || m_Cart.IsEmpty())
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
