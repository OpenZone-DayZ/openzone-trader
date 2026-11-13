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

| step | time |
|---|---|
| the open's answer, 520 615 B in the core's 900-byte parts | 8.6 s after the ask (first open), 3.1-4.7 s later |
| reading the JSON | 10-13 ms |
| laying the shelf (ALL tab, 3404 tiles) | **5.3 s** the first time, 2.0-2.2 s again |

- The transfer runs at about 60 KB/s and dominates the wait; the server is free meanwhile.
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

## Left to do

1. Stock news with the changed lines only, patched into the client's open, instead of a new open
   for everybody.
2. A shorter open: a record as one short string, the category by its index (about 4x smaller).
3. The shelf laid by the rows in view (or per category), not all tiles in one frame; the ALL tab.
4. Saving goods from the admin window under the core's cap (per category, or the cap).
5. Capital `І` in names from the files.
