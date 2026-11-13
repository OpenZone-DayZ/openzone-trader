# Load test: a big book of distinct items and five big traders (2026-11-13)

The owner asked how the trader holds a book of 5000 positions and several such traders, with a
roster as varied as possible (no item twice). Everything below was measured on the full stand
(the series, VPP Admin Tools, TraderPlus, instance 79), diag 1.30, one player, with the timing
lines this test added at debug level (`OZT_Open.Answer`, `OZT_Service.Handle`, `OZT_Stock.Save`,
`OZT_Module` start; on the client `TakeOpen` and the shelf's first lay-out). `TickCount` ticks at
10 MHz, so the numbers are wall time inside one call.

## The book

The roster is every inventory item with a display name the running mod set declares: vanilla,
the series, VPP, TraderPlus, and three item packs fetched for the test (MMG - Mightys Military
Gear, WindstridesClothingPack, SNAFU Weapons; MuchStuffPack was fetched too, but its
MuchFramework does not compile on 1.30: `CanReleaseCargo`'s argument is named differently from
the game's). Body parts, money and statics left out: **3398 distinct items, no class twice**, in
17 categories by kind; with the stand's own 105 lines, 3503 lines. 5000 distinct items were not
reached: the class dump of this mod set holds 3869 items with a name and a size.

- `OZ_Trader_Goods.json` 1 024 119 B (about 300 B a line, every line with a `Stock` block)
- five points: load1 carries all 17 categories (3398 items), load2 weapons (759), load3 clothing
  and gear (1376), load4 food and medicine (174), load5 everything else (1089); each its own stock
- the generator: `make_load.py` in the session's scratchpad (seeded: the same files every run)

## Server

| step | before the fix | after |
|---|---|---|
| start: the three files read and checked | 76.6 ms | 84.7 ms |
| start: the book | 9.9 ms | 11.4 ms |
| start: the five shelves (6796 lines) | **1064.7 ms** | 43.5 ms |
| open load4, 180 records, 27 KB | 21.4 ms | 4.8 ms |
| open load2, 765 records, 118 KB | 339.3 ms | 20.3 ms |
| open load5, 1095 records, 168 KB | 657.3 ms | 28.9 ms |
| open load3, 1382 records, 213 KB | 1099.7 ms | 52.0 ms |
| open load1, 3404 records, 520 KB | **6018.5 ms** | 83-124 ms |
| quote | 2-3 ms | 1.5 ms |
| deal, with the stock file rewritten | 7.4 ms | 6.6 ms |
| the stock file of 3398 lines (408 KB) written | 4.3-5.3 ms | same |

An open is built inside one server frame, so before the fix the whole server stood for 6 s each
time anybody opened load1 (a vanilla login, the stand's yardstick of a freeze, is 1.8 s). Of the
6018 ms, 5988 were the goods loop; the JSON took 6.7 ms. The cause: the stock shelf was three
parallel lists searched line by line (`OZT_StockShelf.IndexOf`), and an open asks it several times
a record; loading a shelf from its file, `GoodsOf` and `Destock` searched lists the same way.
Quadratic: 2.5x the records cost 5.5x the time, 5000 would have been about 13 s. The fix keeps
the lists (the file's order) and adds a map from condition and class to the line; `GoodsOf` and
`Destock` use maps too. After it, the open is linear, about 0.028 ms a record (5000: ~140 ms).

The per-deal rewrite of a whole stock file is NOT a problem at this size: 4 ms for 3398 lines.
(TraderPlus keeps its stock as one small file per trader and category,
`TraderPlusDatabase\ID_<id>\<category>.json`, read from disk and written back on every deal;
our one file per stock costs the same order of time here.)

## Client (one player, the screen of load1, server fixed)

Before the client's fixes:

| step | time |
|---|---|
| the open's answer, 520 615 B in the core's 900-byte parts | 8.6 s after the ask (first open), 3.1-4.7 s later |
| reading the JSON | 10-13 ms |
| laying the shelf (ALL tab, 3404 tiles) | **5.3 s** the first time, 2.0-2.2 s again |

- The shelf is laid in one client frame: the player's game stands for 5 s.
- The ALL tab of 3404 tiles is drawn wrong: tiles over tiles, many pack items white.
- Every change of a stock (anybody's deal at a point that sells from it) makes every client with
  that trader open ask the whole open again: 520 KB and a new shelf each, measured twice in a row
  after two deals.
- Not measured live: saving the goods from the admin window. The page sends the whole file in
  one request, the core takes at most 256 KB per client request (`OZ_Module.REQPART_MAX_BYTES`),
  and this file is 1 MB: by the code, the save is refused.
- A category name typed with the capital Ukrainian `І` shows a box on the trade screen's tab
  ("□нструменти й різне"): the font has no such glyph, and names from the JSON are not mapped to
  the Latin `I` the series' stringtables use.

After them (same stand, same book; the debug lines `open answered`, `shelf laid: records /
packing / pictures`, `shelf repainted`, `own side, carts and parts filled`):

| step | before | after |
|---|---|---|
| the open's answer | 520 615 B | **123 746 B** |
| it arrives after the ask | 8.6 s first, 3.1-4.7 s later | 3.1-3.4 s (built in 0.12-0.16 s) |
| reading it | 10-13 ms | 12-13 ms |
| laying the ALL tab (3362 tiles) | 5.3 s first, 2.0-2.2 s again | **131 ms** first (records 16, packing 62, pictures 54), **71 ms** again (6, 61, 4) |
| a tab of 306 weapons | - | 9 ms |
| another player's deal at that stock | the whole open again, 520 KB and a new shelf | **2 285 B**; read and merged 2.3 ms, shelf repainted 1.3 ms, own side 6 ms |
| the player's own deal | the whole open again | 2 194 B, 30 ms after the ask; repainted in under 0.1 ms |
| a slot's filter over 3404 records | not measured | 32 ms the first time, 10 ms after |

What changed:

1. **A record is one string** (`OZT_WireRows`, 3_Game): `ClassName|cat|Buy|Sell|Stock|Level|Rule|
   Group|With|Name|Look`, the category by its index in `RowCats`, "-" for an empty field, empty
   fields at the end left off. The answer is 4.2x shorter.
2. **A part of the open.** `OZT_OpenReq.Part = 1` with `Only` (classes) answers those classes'
   records and the player's side whole. `OZT_StockNews.Tell` sends it to everybody else at a
   point of the stock (service op `stock`); the dealer's screen asks it for `OZT_DealRes.Moved`;
   a licence bought asks the whole open. The screen merges it in place (`MergeOpen`, the order
   and the colour groups kept), and when the shelf keeps its shape (the same records in the same
   places, shown and grouped alike, the same licences) its tiles take the new numbers where they
   lie (`RepaintShelf`) instead of a new lay.
3. **The shelf is laid lazily.** Tiles are packed by the classes' `itemSize` from config, kept by
   class (`OZT_Tile.ConfigSize`); widgets and local pictures are made only for the rows in view
   and a view's height around it, at most 40 a frame, and given up far from it (`ShelfReveal`,
   `OZT_TileGrid.Band`). The ALL tab is drawn right, and scrolling to the end of the 3362 tiles
   builds them on the way (checked on screen down to the last tile).
4. **Packing.** The packer looked for a tall tile from the first row with enough free cells and
   walked the whole shelf for each one; now each tile shape starts from where the last tile of
   that shape went (cells only fill, so it cannot fit higher). Same layout, 300 ms -> 61 ms.
5. **The slot filter** keeps what config said of each class (`OZT_Fit`: the host's slots, the
   part's slots, the kind, a weapon's magazines).

The transfer: the core sends all parts in one frame and the client appends them as they come,
so the wait is the engine's pace for guaranteed messages: 6.6 KB (load4, 8 parts) in 47 ms,
27 KB (load2) in 0.5 s, 52 KB (load3) in 1.1 s, 124 KB (load1) in 3.0-3.2 s on every try. About
45-50 KB/s; the server is free meanwhile, the player waits. Bytes are the lever.

## Left to do

1. The first open of a big book still waits about 3 s on the engine's pace. A client cache of the
   open per trader (sent again only when the book changed; the stock as a short list of numbers)
   would spare it. A design question, not started.
2. Packing the ALL tab of 3362 tiles takes 61 ms of one frame on every switch to it.
3. Saving goods from the admin window under the core's cap (per category, or the cap).
4. Capital `І` in names from the files.
