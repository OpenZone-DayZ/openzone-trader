# Phase 7 on the stand, 2026-11-12: stock, licences, the TraderPlus import

DayZDiag 1.30, `serverDZ_trader.cfg`, the client at 1920x1080 in English. "Stand" means the
stand's verbs (`oz_trader`, `ozt_probe`) and logs; "screen" means a screenshot of the live
client, sent to the owner. Prices are in the stand's notes, the smallest of which is 5.

## Stock (7a)

| | asked | answered |
|---|---|---|
| S.1 | the AKM line `Stock {3, 0.95, 0.25}` at a stock of 2: one, then two | 2375 for one (2500 x 0.95), 4875 for two (the second at the stock the first leaves, 2500); a deal for more than the line holds refused `STR_OZT_NO_STOCK` |
| S.2 | the M4A1 line `Stock {2, 0.9}` at a stock of 2: two | 5700 (3000 x 0.9 + 3000) |
| S.3 | `UniversalLight` with `Stock {0, Coefficient 1}` ("only what players sell") | no line on the shelf until one was sold; sold at 85 (worn), it came onto the shelf as its own worn line at 170 and was bought back worn (the card: WORN) |
| S.4 | `Ammo_762x39` with `Stock {300}`, priced by the unit | the line counts rounds; 280 left after a deal of 20 |
| S.5 | a stocked line with nothing in it | one record with `Stock` 0: a red 0, a red corner and a muted price on the tile; it does not go into the cart; the card reads "In stock" with the count otherwise |

## Licences (7b)

| | asked | answered |
|---|---|---|
| L.1 | `quote buy=M4A1` (its line `Requires` the licence `weapons`) without the licence | `STR_OZT_NO_LOCKED` |
| L.2 | buy `@licence:weapons` (5000) | paid, nothing created, the licence on the character (`TraderLicences/<uid>.json`) |
| L.3 | buy it again | `STR_OZT_NO_HAS_LICENCE` |
| L.4 | `quote buy=M4A1` with the licence | 2700 |
| L.5 | the screen | the licence's tile wears its name and a green corner when held; its card shows the name, the price and "Licence: you hold it" and, since `d42c543`, no weight, condition, description or size |

## The TraderPlus import (7c)

Sample files in TraderPlus's own layout under `$profile:TPImportTest`, written by
`tools/tp_samples.py`: two licences (`Car Licence`, `Weapon Licence`), worn and damaged
accepted at 0.75 and 0.5, badly damaged not; six currencies, one of a class the game does not
know; three traders (a character of ID 1 with clothes, a stall of ID 2, a TraderPlus sign of
ID 1); five categories (one named `Car Licence`); ID 2 requires `Weapon Licence`; stock files
for both IDs, one line with its class in lower case; the stand's own bank account holding
`Weapon Licence`.

| | asked | answered |
|---|---|---|
| I.1 | `oz_trader op=import root=$profile:TPImportTest` (dry run) | the three files and `report.txt` in `OpenZone\TraderPlusImport`, nothing applied. Report: licences 2; notes 7 classes, 1 unknown skipped; categories 5, 1 behind a licence by its name; goods lines 11; 1 class listed twice (the first kept); 1 line with MaxStock 0 imported without a sell price; 2 piles priced by the unit; 1 vehicle left to TraderPlus (`Hatchback_02`); 1 unknown class skipped; traders 1 character and 2 objects, 1 with currencies of its own; 1 decoration object not taken; 7 stock lines |
| I.2 | the converted files | `TunaCan` 40/20 unlimited (`0.5` sell = half the buy); `Apple` `Stock {10, 0.95}`; `SodaCan_Cola` `Stock {0, 1}` and sell 0; `Ammo_762x39` traded 20 at 100/50: 5 a round, `Stock {200, 0.95^(1/20) = 0.99744}`; `Mag_AKM_30Rnd` unlimited, sold whole; `@licence:weapon_licence` 5000 and `@licence:car_licence` 8000 in Documents; the category `car_licence` `Requires` licence `car_licence`; the points `tp-0` (npc, Sidorovich, three clothes), `tp-1` (Kevin, `Access` licence `weapon_licence`), `tp-2` (the sign, named by its role); `tp-0` and `tp-2` sell from stock `tp1` |
| I.3 | apply | our three files written with the core's backups, `loaded again: traders=3 goods=11 problems=2`, 1 licence put on the character, `tp1.json` (Apple 4 pristine and 2 worn, 60 rounds from "ammo_762x39 3 0") and `tp2.json` (AKM 2 pristine and 1 damaged, a light, 20 rounds) |
| I.4 | the two problems of I.3 | the sell price of a round, 2 (50 for 20 floored), is no multiple of the smallest note: the load turned it to 0. The import now makes every price a multiple of the smallest note itself (buy up, sell down) and names the lines: "prices made multiples of the smallest note (5): 2 (Ammo_762x39 Ammo_9x19); of them the trader now pays nothing for: 2"; the next apply loaded with `problems=0` |
| I.5 | the screen at Sidorovich | tabs Food, Ammo, Documents, Change; tuna 40 unlimited, apples 4 at 10 and 2 worn at 10 (7.5 rounded up), the cola sold out (a red 0), 60 rounds at 5, the licences at 5 000 (held: a green corner) and 8 000; the character wears the red T-shirt, the jeans and the shoes of `Clothes` |
| I.6 | the screen at Kevin | AKM 2 at 2 850 (3000 x 0.95), the damaged one at 1 500 (3000 x 0.5); the player's AKM brings 1 425 (1500 x 0.95), the worn light 75 (100 x 0.75); the category `Car Licence` locked: no tab on the shelf since `55cd89a` (I.9) |

## Two players at one shelf, tabs, the sellable filter

| | asked | answered |
|---|---|---|
| N.1 | another player's deal, played by `oz_trader op=stock stock=tp2 class=AKM level=0 delta=-1` while Kevin's screen is open | the shelf changed with no gesture of the player: AKM 2 at 2 850 became 1 at 3 000; the player's own AKM now brings 1 500 (the curve at a stock of 1) |
| N.2 | the last pristine AKM in the player's cart; the same op again | the cart line came off, the line under the deal read "Some of the goods are gone: the deal has changed", the shelf shows the damaged AKM only |
| N.3 | the sellable filter at Kevin, then at the stand trader | the head of YOUR ITEMS turns to "only what can be sold" (orange): two lights at Kevin; at the stand trader the batteries and the nails go, the rounds, the lights and the handguard stay |
| N.4 | a container in the bags that the trader would take emptied (reason 2) | not seen live: the engine refuses anything into a container that lies in another's cargo (`CanReceiveItemIntoCargo` false, its children not accessible; a hoodie and a first aid kit in the backpack), so the stand tools cannot fill one there; in play it is filled in the hands first. The server's answer marks such a thing reason 2 (`tree.Inside > 0`) and the filter keeps reasons 0 and 2 |
| N.5 | the licence `car_licence` given while Kevin's screen is open, then news of his stock | the shelf's strip was made again: Weapons, Ammo, Car Licence, Change, the canister on the shelf. Before it the player's strip already had Car Licence (the trader takes canisters) and had no Ammo (he pays nothing for rounds after I.4) |
