# OpenZone Trader, Phase 2b, Part 2: The Trade Screen for Plain Items — Plan and Record

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A player trades plain items with a trader through a screen: sees the shelf and what the trader pays for their own things, puts a deal together by double click or by dragging, sees the server's verdict before confirming, confirms.

**Architecture:** The screen is a `UIScriptedMenu` (id 136) that knows nothing about prices or places. `open` gives it the shelf and the player's own things with what each brings; every change of the cart asks the server for a `quote` and the deal panel shows that answer, line by line; Confirm sends the same request as a `deal` with the net the quote gave. Layouts are generated from `ui/OpenZone_Trader/*.json`; everything the screen places itself (tiles, tabs, slots) is placed in pixels, layout units times the screen's height over 1080.

**Tech Stack:** Enforce Script on DayZ 1.30.164289 (diag), `OpenZone_Core` (`OZ_Show`, `OZ_Rpc.ServiceRequest`, `OZ_ClientState.ServiceWatch`), the `dayz` MCP server (`layout_build`, the stand), `tools/click.py` and `tools/drag.py` for real mouse input.

**Design:** `E:\openzone\docs\specs\2026-11-10-openzone-trader-design.md` §3 (the screen), §4 (a deal on the server), the mockup beside it. Facts: `docs/measurements/2026-11-11/client-answers.md` (phase 2a), `docs/measurements/2026-11-11/screen.md` (this part).

## Global Constraints

The phase 1 plan's, unchanged (`docs/2026-11-11-phase-1-plan.md`, "Global Constraints"): English and Ukrainian only; prefix `OZT_`; no invented API; a refusal is a string-table key; work on `main`; nothing pushed without the owner's word; a subagent's brief says "Do NOT push".

Two of this part's own:

- **The screen does not price.** Every sum it shows is the server's: `open` (the shelf's price for one, what the trader pays for a whole thing), `quote` (each line, the two subtotals, the net, the balance afterwards). The one product it forms is a bought line's price of one times its count before the quote answers, which is the server's own rule for a buy.
- **Nothing that deletes a tile's widgets runs inside an event.** Events set state; grids are laid anew in `Update`, with the mouse button up and no drag in the air.

---

## File structure

| file | what it is |
|---|---|
| `ui/tokens.json` | the trader's own tokens over the core's: the bed of a grid, a tile, a title strip, the gold of a price, the veil, a slot's seam, two sizes for sums |
| `ui/OpenZone_Trader/ozt_trade.json` | the screen: the body of 1840x1032 units, BUY over the shelf, the character, the selection, the deal panel, Confirm and Clear, SELL over the player's things, the ghost of a drag |
| `ui/OpenZone_Trader/ozt_trade_tile.json` | one tile: picture, price, count, condition pip, the corner of a thing that cannot be sold as it is |
| `ui/OpenZone_Trader/ozt_trade_tab.json` | one category tab |
| `ui/OpenZone_Trader/ozt_trade_slot.json` | one slot of the character: ghost icon, name, room for the worn thing's tile |
| `ui/OpenZone_Trader/ozt_trade_qty.json` | the quantity dialog, a window of its own |
| `OpenZone_Trader/gui/layouts/*.layout` | generated from the five above by `layout_build` |
| `scripts/5_Mission/OpenZone_Trader/OZT_TradeMission.c` | the client's entry: `show` opens the screen, service answers and keys go to it |
| `.../OZT_TradeMenu.c` | the screen: its life, the service, the grids, the selection, clicks, drag, the deal |
| `.../OZT_TradeTile.c` | `OZT_Tile` and `OZT_TileGrid` (first fit, twelve cells across) |
| `.../OZT_TradeCart.c` | `OZT_Cart`: the lines, the request, what a quote said for each line |
| `.../OZT_TradeQty.c` | `OZT_QtyDialog` |
| `ui/OpenZone_Trader/ozt_trade_card.json`, `.../OZT_TradeCard.c` | the item card, `OZT_Card` |
| `ui/OpenZone_Trader/ozt_trade_list.json` (and its row), `.../OZT_TradeList.c` | the price list, `OZT_PriceList` |
| `.../OZT_TradeTryOn.c` | trying on, `OZT_TryOn` |
| `.../OZT_Shelf.c` | the shelf's stand-ins: one client-local entity per class |
| `.../OZT_Ui.c` | pixels in a unit, a layout made with the mix-up guard, words, sums, the palette |
| `tools/click.py`, `tools/drag.py`, `tools/hover.py` | real clicks, drags and a resting cursor in the client's window |

## The wire, as this part left it

Added to phase 2b part 1's (`docs/2026-11-11-phase-2b-wire-plan.md`):

- `OZT_OpenRes.Notes` (`array<string>`): the classes that are this trader's money when money is items. The screen counts them as money (the footer) and shows no tile for them.
- `OZT_DealRes.Sold`, `Bought` (`int`): what the trader pays for everything sold and what everything bought costs; `SellPrices`, `BuyPrices` (`array<int>`): each line's own sum, in the order of the request. All four are filled as far as the lines were read, on a refusal too.

---

### Task 1: The screen opens and shows both sides

- [x] `config.cpp`: the `Mission` module. `OZT_Const.MENU_TRADE = 136`.
- [x] `OZT_TradeMission.c`: `OZ_Show.OnShow` for `trader:<id>`, the menu entered a tick later on the GUI queue; `CreateScriptedMenu` with super first; the service's answers and `OnKeyPress` handed to the open screen.
- [x] The layouts and the tokens; `OZT_Ui.Make` checks the root's name after `CreateWidgets` and asks again with backslashes (the series' layout mix-up).
- [x] `open` asked in `OnShow`; the shelf from `Goods` with `Buy > 0`, pictures from `OZT_Shelf`; the player's things from the client's own enumeration joined with `Yours` by network id: what is listed has its price, what is not is marked as not taken, money is left out; tabs ALL and the trader's categories on both sides.
- [x] The character: `PlayerPreviewWidget` on the player, turned by the mouse; the slots from `CfgVehicles SurvivorBase InventoryEquipment playerSlots` and the hands, seven to a column, each worn thing a tile.
- [x] Input excluded, HUD hidden by its two flags, both given back in `OnHide`; the screen closes on Esc and over a player who is dead, unconscious or restrained.

### Task 2: A deal put together and confirmed

- [x] `OZT_Cart`: one bought line per class, one sold line per thing; `Revision` grows with every change.
- [x] Double click (two presses on one tile within 400 ms, or the engine's `OnDoubleClick`, one gesture taken once): shelf to BUY, own to SELL, a cart's tile back out.
- [x] Drag: a press on a tile, six pixels of travel, a ghost of the tile's size hung on the cursor by the point it was taken at, the widget under the cursor at the release. Shelf tiles are taken by BUY, the player's panels and the character; own tiles by SELL and the shelf; a cart's tile leaves by a drop anywhere else.
- [x] `quote` a quarter of a second after the cart last changed, one at a time, asked again on `STR_OZT_NO_BUSY`, dropped when the cart has moved on; the deal panel from its answer: the net, the two subtotals, money after or "short by", the space verdict or the refusal's words; Confirm lit only while a quote stands.
- [x] Confirm (the button or F) sends `deal` with the quote's net; on success the cart is emptied and `open` asked again; on a refusal both are asked again. R clears.
- [x] The screen watches the player's things twice a second (count, ids, units, condition) and asks `open` again when they change: after a deal the new things arrive a moment later than the answer.

### Task 3: How many

- [x] `OZT_QtyDialog`: a slider, minus, plus, ALL, the count, the total, OK and Cancel; F accepts, Esc cancels.
- [x] Selling a stack of more than one unit asks how many (all to begin with). The total of the whole stack is `open`'s; the total of a part is asked from the server, a quote of that one line, a third of a second after the count rests.
- [x] Buying goods priced by the unit asks how many (a full stack to begin with, five stacks at most at a time). Goods bought by the piece take one more per double click.

### Task 4: The card and the price list

- [x] `OZT_Card`: a window of its own that ignores the pointer; it comes up when the cursor has rested on a tile for 0.35 s and goes at a press, a drag, a window over the screen or a grid laid anew. Name, the game's own words for weight and condition, the sum that applies here (the price, the price per unit, what the trader pays, a cart line's total), the game's description, then lines: size, cargo, the stack or the count, the other side of the same trader's book, and in red why the thing cannot be sold.
- [x] `OZT_PriceList`: a window of its own, opened by the button and by P, closed by its button, P and Esc; tabs ALL and the trader's categories; a line per goods of the answer of `open`, so what is locked for the player is not there; the note on wear from the answer's condition factors.

### Task 5: Trying on

- [x] `OZT_TryOn`: a client-local copy of the character at the player's own place, dressed in copies of what the player wears (parts two levels down) and in the goods on the first slot of the character their class names, what stood there removed; the live view shows the copy. `SlotFor` says whether a class can be worn at all.
- [x] The button under the selected thing is there for goods of the shelf or of BUY that the character could wear; it reads TRY ON or TAKE OFF. The tried slot's tile shows the goods, lit; it is a tile of the shelf, so a double click on it buys them.
- [x] The trying ends when another thing is selected, when the selection goes, when the button is pressed again, when a deal is made and when the screen closes.

### Task 6: Record

- [x] `docs/measurements/2026-11-11/screen.md`: twenty-five rows and what was learned.
- [x] The design's §3 and §4 say what is built and what was measured.

---

## Execution record

Carried out on 2026-11-11 in the session that wrote it, in six builds of the stand. Rows S.1 to S.14 of `screen.md` are what the last builds did; the deals the screen made are in the server's log as `net 120`, `net 840`, `net 450` and `net 435`.

What the builds found, in the order they found it:

1. The first build opened the screen, took a double click and confirmed a deal. Prices on narrow tiles were hidden behind the items' pictures.
2. Tiles could not be dragged (the owner saw the same on the stand). The picture on a tile took the pointer, so the press landed on a widget that is not draggable; the ghost's picture took it too and was the receiver of every drop.
3. With both ignoring the pointer the engine's drag worked: a helmet into BUY, a canteen into SELL, and the owner's case from the screen, a worn helmet sold and a helmet bought for its slot in one deal.
4. A double click "lost its second press" for every tile in a grid's first column. Three builds went into it (the engine's own double click as a second road, the screen's own drag instead of the engine's) before `WindowFromPoint` named the cause: the owner's taskbar, 80 pixels wide at the left edge of the desktop, over the left strip of the client's window. The clicks had gone to the taskbar. The window is moved clear of it now.
5. The screen's own drag stayed: it asks nothing of the engine (no flag, no answer a press must give, no tile moved under the cursor). `screen.md` says what the engine's needed.
6. The card and the price list came up as written; one build went into the card's lines, which stood two to a line. The screen was then seen in Ukrainian with both, and at 3840x1600 (rows S.16 to S.20).
7. Trying on came up as written, on phase 2a's recipe (rows S.21 to S.24). The owner asked that the copy be hidden from other players: it is this client's alone, and the server was asked (S.25).

## What is not built

- On the card: what a thing takes and what it fits, what is on it, why it is discounted (design §3.5): they come with parts (phase 3) and gates (phase 5).
- The parts strip, items with parts, containers with things in them: phase 3. Until then a thing that is not empty shows its price and an amber corner, and a double click says why it cannot be sold.
- Trying on shows one thing at a time, and only goods: a thing of the player's own cannot be tried against another slot.
- A bought line's count cannot be lowered by one: its tile leaves the cart whole.
- More than seven tabs on a side narrow the tabs; beyond four columns of slots the slots narrow and their labels are cut.
- The gallery as `ui_gallery` runs it (`openzone-core/CONTRIBUTING.md`): there is no preview index yet. The screen was looked at by hand: 1600x900 and 1920x1080 in English, 1920x1080 and 3840x1600 in Ukrainian.
- A slot's name longer than its tile is cut.
