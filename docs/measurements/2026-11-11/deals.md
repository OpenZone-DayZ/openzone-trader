# Trader, phase 1: scripted deals of 2026-11-11

Stand: DayZ 1.30.164289 diag, `serverDZ_trader.cfg` (instance 79); CF, the core, the MCP
bridge, the trader, the probe; one client. The files are `examples/stand/`. Each row is one
`oz_trader` command and what it answered.

Rows are numbered by the task of the plan (`docs/2026-11-11-phase-1-plan.md`) that sent
them; the rows of Task 7 carry the plan's own numbers.

## Task 3: the balance, the paying rule, prices, a broken line

Scene: the kit of phase 0, three fives and two fifties in the jacket's wallet, one 500 in the
backpack's wallet, four loose fives and a canteen in the trousers.

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| 3.1 | the ready line and the revision | `{"op": "status"}` | `trader loaded: traders=1 goods=11 money=items gates=none problems=0 revision=1` | yes |
| 3.2 | the balance and the stacks | `{"op": "balance"}` | `provider=items unit=5 balance=635 stacks: 5x3 5x4 50x2 500x1` | yes |
| 3.3 | pay 50 | `{"op": "pay", "amount": "50"}` | `pay 50: spend 50x1/2; change 0 =` | yes |
| 3.4 | pay 120 | `{"op": "pay", "amount": "120"}` | `pay 120: spend 5x4/4 50x2/2; change 0 =` | yes |
| 3.5 | pay 40 | `{"op": "pay", "amount": "40"}` | `pay 40: spend 50x1/2; change 10 = 5x2` | yes |
| 3.6 | pay 150 | `{"op": "pay", "amount": "150"}` | `pay 150: spend 500x1/1; change 350 = 50x7` | yes |
| 3.7 | pay more than there is | `{"op": "pay", "amount": "640"}` | `pay 640: the notes do not cover it` | yes |
| 3.8 | a fraction-rule price | `{"op": "price", "class": "Canteen"}` | `Canteen rule='fraction' buy(1)=120 sell(1 of 1, level 0)=60` | yes |
| 3.9 | a unit-rule price | `{"op": "price", "class": "Ammo_762x39", "count": "30"}` | `Ammo_762x39 rule='unit' buy(30)=300 sell=(the player carries none)` | yes |
| 3.10 | goods of a category this trader does not deal in | `{"op": "price", "class": "TunaCan"}` | `TunaCan rule='' buy(1)=0 sell=(the player carries none)` | yes |
| 3.11 | wear the canteen | `{"op": "damage", "class": "Canteen", "health": "0.6"}` | `Canteen health level 1` | yes |
| 3.12 | the worn price, rounded to the smallest note | `{"op": "price", "class": "Canteen"}` | `Canteen rule='fraction' buy(1)=120 sell(1 of 1, level 1)=50` | yes |
| 3.13 | a class the files do not know | `{"op": "price", "class": "NoSuchThing"}` | `price: trader or class not in the files (trader='stand' class='NoSuchThing')` | yes |
| 3.14 | two broken lines in the goods file (`Paper` at 7, a `NoSuchThing`) | `{"op": "reload"}` | `reloaded: traders=1 goods=11 money=items gates=none problems=2 revision=2`; two WARNING lines: `Goods: Buy of 'Paper' is 7, not a multiple of the smallest note (5); using 5` and `Goods: 'NoSuchThing' in category 'gear' is skipped: no such class`; the file itself untouched | yes |
| 3.15 | the goods file put back | `{"op": "reload"}` | `reloaded: ... problems=0 revision=3` | yes |
| 3.16 | `"CooldownMs": 700000` in the settings | `{"op": "reload"}` | `reloaded: traders=1 goods=11 money=items gates=none problems=1 revision=4`; `WARNING: Settings: CooldownMs 700000 is outside 0..600000, using 1000`; the loader rewrote the file: `"CooldownMs": 1000`, 29 lines in the serializer's layout | yes |
| 3.17 | the settings put back | `{"op": "reload"}` | `reloaded: ... problems=0 revision=5` | yes |

The verdict of this boot was taken before row 3.14: pass, no warning, the counters
`traders=1 goods=11 problems=0` met.

## Task 4: an object trader from the file, and `open`

The first build of this task placed the stall with the engine's `ECE_PLACE_ON_SURFACE`, and
the run showed that it does not give one place for one request: at mission start the stall
stood on the ground 2.5 m aside of the asked point (origin at `<4690.19, 340.77, 10192.47>`
for an asked `<4690, 339, 10190>`), and after a reload at the asked point with its origin at
ground level, half buried (the same in the logs of four boots and some twenty reloads). A
second build lifted the object by the depth of `ClippingInfo` below its origin: 3.57 m for
this stall, which showed its foundation above the runway. The build that stands is the third:
the object is created at `Pos` exactly, with the flags of the game's object spawner, and the
stand's file names the origin the engine itself had chosen for a stall on this runway
(1.77 m above it).

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| 4.1 | where the stall stands after the start | log, `point '` | `point 'stand' (Land_City_Stand_Grocery) stands at <4690.000000, 340.769989, 10195.000000>` (the file then said `10195`) | yes |
| 4.2 | the same after a reload | `{"op": "reload"}`, log | `reloaded: ... problems=0 revision=2`; the same place in the log; one stall in the world, at that position; from outside it stands on the runway, door at ground level | yes |
| 4.3 | the file changed to `Pos` `[4690.0, 340.77, 10198.0]`, `Radius` 8 (the character at `10192` was inside the stall: its body lies south of its origin) | `{"op": "reload"}`, log | `reloaded: ... revision=3`; `stands at <4690.000000, 340.769989, 10198.000000>`; one stall | yes |
| 4.4 | the catalogue | `{"op": "open"}` | `open ok=yes trader=stand goods=10 categories=3 provider=items unit=5 balance=635 bytes=1317 revision=3` | yes |
| 4.5 | a trader that is not in the file | `{"op": "open", "trader": "nobody"}` | `open ok=no why=STR_OZT_NO_CLOSED` | yes |
| 4.6 | from 32 m | teleport to `4690 339 10230`, `{"op": "open"}` | `open ok=no why=STR_OZT_NO_TOO_FAR` | yes |
| 4.7 | back at the stall | teleport to `4690 339 10192`, `{"op": "open"}` | `open ok=yes ... balance=635 bytes=1317 revision=3` | yes |
| 4.8 | one more reload | `{"op": "reload"}` | `reloaded: ... revision=4`; still one stall, `4690.000000 340.769989 10198.000000` | yes |

The body of `open` begins `{"Revision":2,"Trader":"stand","Name":"Стендовий торговець","Radius":5.0,"Provider":"items","Physical":1,"Unit":5,...`
(read after row 4.2) and is 1317 bytes: two parts of the core's 900-byte transport.

## Task 5: the inventory model

After the restart there is exactly one stall, at the place the file names: the one of the
last boot was never saved. Scene: the kit and a canteen in the trousers. `fit` asks the model
alone and lays things in the order they are listed.

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| 5.1 | three small things | `{"op": "fit", "buy": "Paper:3"}` | `fit: placed=3 unplaced=0`, `Paper: CARGO M65Jacket_Black r0 c3`, `r0 c4`, `r0 c5` | yes |
| 5.2 | the same, made | `{"op": "fit", "buy": "Paper:3", "make": "1"}` | the same places, `made=3 where planned=3` | yes |
| 5.3 | two helmets | `{"op": "fit", "buy": "Mich2001Helmet:2"}` | `Mich2001Helmet: SLOT Headgear` and `Mich2001Helmet: CARGO CargoPants_Black r0 c2 turned` | yes |
| 5.4 | three rifles | `{"op": "fit", "buy": "AKM:3"}` | `placed=1 unplaced=2`: `AKM: SLOT Melee`, twice `AKM: NO PLACE` | yes |
| 5.5 | two rifles | `{"op": "fit", "buy": "AKM:2"}` | `placed=2 unplaced=0`: `AKM: SLOT Melee`, `AKM: HANDS` | yes |
| 5.6 | two new stacks of notes | `{"op": "fit", "notes": "OZT_ProbeNote50:2"}` | `OZT_ProbeNote50: CARGO OZT_ProbeWallet r0 c0` and `r0 c2` | yes |
| 5.7 | a helmet, made | `{"op": "fit", "buy": "Mich2001Helmet:1", "make": "1"}` | `Mich2001Helmet: SLOT Headgear`, `made=1 where planned=1` | yes |
| 5.8 | the client against the server | `dump 1` | `SAME`, 28 items | yes |
| 5.9 | every free cell filled | `ozt_probe` `{"op": "fill", "into": "all"}` | `fill: 53 Paper made in 7 holder(s), 60 cell(s) refused` | yes |
| 5.10 | one thing into full bags | `{"op": "fit", "buy": "Paper:1"}` | `placed=1 unplaced=0`, `Paper: HANDS` | yes |
| 5.11 | two things into full bags | `{"op": "fit", "buy": "Paper:2"}` | `placed=0 unplaced=2`, twice `Paper: NO PLACE` | yes |
| 5.12 | a note into full bags | `{"op": "fit", "notes": "OZT_ProbeNote:1"}` | `OZT_ProbeNote: CARGO OZT_ProbeWallet r0 c0` | yes |
| 5.13 | a canteen into full bags | `{"op": "fit", "buy": "Canteen:1"}` | `Canteen: HANDS` | yes |
| 5.14 | the same with the carried canteen leaving | `{"op": "fit", "buy": "Canteen:1", "gone": "Canteen"}` | `Canteen: CARGO CargoPants_Black r0 c0` | yes |
| 5.15 | the same, made | `{"op": "fit", "buy": "Canteen:1", "gone": "Canteen", "make": "1"}` | the same place, `made=1 where planned=1` | yes |
| 5.16 | the client against the server | `dump 2` | `SAME`, 81 items; the canteen in the trousers is a new one (`022604`, the one of `dump 1` was `022505`) | yes |
| 5.17 | nothing left by the plans | `world_entities` for `Canteen` and `Paper`, 10 m | none of either lying loose | yes |

The verdict after these rows: pass, no warning, no error.

## Task 6: the first deals

Scene: the money scene of Task 3 (635 in four stacks, a canteen in the trousers).

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| 6.1 | a quote | `{"op": "quote", "buy": "BandageDressing:1"}` | `quote ok=yes net=40 paid=40 received=0 balance=595 hands= left=0` | yes |
| 6.2 | a quote changes nothing | `{"op": "balance"}` | `provider=items unit=5 balance=635 stacks: 5x3 5x4 50x2 500x1` | yes |
| 6.3 | the deal | `{"op": "deal", "buy": "BandageDressing:1"}` | `deal ok=yes net=40 paid=40 received=0 balance=595 hands= left=0` | yes |
| 6.4 | the notes after it | `{"op": "balance"}` | `provider=items unit=5 balance=595 stacks: 5x4 5x5 50x1 500x1` | yes |
| 6.5 | the client against the server | `dump 1` | `SAME`, 29 items; a second `BandageDressing` in the jacket at `r0 c3` | yes |
| 6.6 | the log | `deal:` | one line: `deal: <uid> at 'stand' net 40 (items): +BandageDressingx1=40` | yes |

## Task 7: the scripted deals of the phase

One boot, one sequence, the rows in the plan's order with the plan's numbers. Scene: the kit;
three fives and two fifties in the jacket's wallet, four loose fives in the trousers, a 500
and nine 500s in the backpack's wallet (5135 in all); a canteen, a pile of twelve rounds and
a can of tuna in the trousers.

`{"op": "balance"}` before row 1: `provider=items unit=5 balance=5135 stacks: 5x3 5x4 50x2 500x1 500x9` (as expected).

| # | what is asked | command | answer | stacks afterwards | as expected? |
|---|---|---|---|---|---|
| 1 | a bandage: change in notes | `{"op": "deal", "buy": "BandageDressing:1"}` | `deal ok=yes net=40 paid=40 received=0 balance=5095 hands= left=0` | `5x4 5x5 50x1 500x1 500x9` | yes |
| 2 | four papers: paid exactly | `{"op": "deal", "buy": "Paper:4"}` | `deal ok=yes net=20 paid=20 received=0 balance=5075 hands= left=0` | `5x5 50x1 500x1 500x9` | yes |
| 3 | three rifles: short of money | `{"op": "deal", "buy": "AKM:3"}` | `deal ok=no why=STR_OZT_NO_MONEY net=7500 paid=0 received=0 balance=0 hands= left=0` | unchanged | yes |
| 4 | the canteen sold | `{"op": "deal", "sell": "Canteen"}` | `deal ok=yes net=-60 paid=0 received=60 balance=5135 hands= left=0` | `5x7 50x2 500x1 500x9` | yes |
| 5 | eight rounds of the pile sold | `{"op": "deal", "sell": "Ammo_762x39:8"}` | `deal ok=yes net=-40 paid=0 received=40 balance=5175 hands= left=0`; the pile `ammo 4/20` | `5x15 50x2 500x1 500x9` | yes |
| 6 | thirty rounds bought | `{"op": "deal", "buy": "Ammo_762x39:30"}` | `deal ok=yes net=300 paid=300 received=0 balance=4875 hands= left=0`; the old pile `ammo 20/20`, a new one `CARGO M65Jacket_Black r2 c0`, `ammo 14/20` (number 0 in the engine's order) | `5x15 50x6 500x9` | yes |
| 7 | a helmet bought | `{"op": "deal", "buy": "Mich2001Helmet:1"}` | `deal ok=yes net=900 paid=900 received=0 balance=3975 hands= left=0`; the helmet `ATT ... slot=Headgear` | `5x15 50x8 500x7` | yes |
| - | the client against the server | `dump 2` | `SAME`, 35 items (`server-dump-after-seven-deals.txt`, `client-dump-after-seven-deals.txt`) | | yes |
| - | a quote for the scope | `{"op": "quote", "buy": "PSO1Optic:1"}` | `quote ok=yes net=600 paid=600 received=0 balance=3375 hands= left=0` | | yes |
| - | the scope at 650 in the file | `{"op": "reload"}` | `reloaded: traders=1 goods=11 money=items gates=none problems=0 revision=2` | | yes |
| - | the deal the client showed at 600 | `{"op": "deal", "buy": "PSO1Optic:1", "net": "600"}` | `deal ok=no why=STR_OZT_NO_CHANGED net=650 paid=0 received=0 balance=0 hands= left=0` | unchanged | yes |
| - | the file put back | `{"op": "reload"}` | `reloaded: ... problems=0 revision=3` | | yes |
| 8 | goods this trader does not deal in | `{"op": "deal", "buy": "TunaCan:1"}` | `deal ok=no why=STR_OZT_NO_CHANGED net=0 ...` | unchanged | yes |
| 9 | the same sold | `{"op": "deal", "sell": "TunaCan"}` | `deal ok=no why=STR_OZT_NO_NOT_TAKEN net=0 ...` | unchanged | yes |
| 10 | a thing sold here and not bought back | `{"op": "deal", "sell": "Paper"}` | `deal ok=no why=STR_OZT_NO_NOT_TAKEN net=0 ...` | unchanged | yes |
| 11 | a backpack with things in it | `{"op": "deal", "sell": "TaloonBag_Blue"}` | `deal ok=no why=STR_OZT_NO_NOT_EMPTY ... left=4` | unchanged | yes |
| 12 | a rifle with five parts | `{"op": "deal", "sell": "AKM"}` | `deal ok=no why=STR_OZT_NO_NOT_EMPTY ... left=5` | unchanged | yes |
| 13 | a loaded magazine | `{"op": "deal", "sell": "Mag_AKM_30Rnd"}` | `deal ok=no why=STR_OZT_NO_NOT_EMPTY ... left=30` | unchanged | yes |
| 14 | a canteen lying on the ground | `{"op": "deal", "sellnear": "Canteen"}` | `deal ok=no why=STR_OZT_NO_NOT_YOURS ...` | unchanged | yes |
| 15 | a trader that is not in the file | `{"op": "deal", "trader": "nobody", "buy": "Paper:1"}` | `deal ok=no why=STR_OZT_NO_CLOSED ...` | unchanged | yes |
| 16 | from 32 m | teleport to `4690 339 10230`, `{"op": "deal", "buy": "Paper:1"}` | `deal ok=no why=STR_OZT_NO_TOO_FAR ...` | unchanged | yes |
| - | after nine refusals | `{"op": "balance"}`, `dump 3` | `balance=3975 stacks: 5x15 50x8 500x7`; `SAME`, 35 items | | yes |
| 17 | the scope sold off the rifle on the shoulder | `{"op": "deal", "sell": "PSO1Optic"}` | `deal ok=yes net=-300 paid=0 received=300 balance=4275 hands= left=0`; the rifle `attachments=4`, its state machine idle | `5x15 50x14 500x7` | yes |
| 18 | a canteen worn | `{"op": "damage", "class": "Canteen", "health": "0.6"}` | `Canteen health level 1` | | yes |
| 19 | the worn canteen sold | `{"op": "deal", "sell": "Canteen"}` | `deal ok=yes net=-50 paid=0 received=50 balance=4325 hands= left=0` | `5x15 50x15 500x7` | yes |
| 20 | the helmet ruined | `{"op": "damage", "class": "Mich2001Helmet", "health": "0"}` | `Mich2001Helmet health level 4` | | yes |
| 21 | the ruined helmet sold | `{"op": "deal", "sell": "Mich2001Helmet"}` | `deal ok=no why=STR_OZT_NO_NOT_TAKEN ...` | unchanged | yes |
| 22 | `"CooldownMs": 600000` | `{"op": "reload"}` | `reloaded: ... problems=0 revision=4` | | yes |
| 23 | a deal inside the cooldown | `{"op": "deal", "buy": "Paper:1"}` | `deal ok=no why=STR_OZT_NO_BUSY net=0 paid=0 received=0 balance=0 hands= left=0` | unchanged | yes |
| 24 | the cooldown off, `"WearBought": 0` | `{"op": "reload"}` | `reloaded: ... problems=0 revision=5` | | yes |
| 25 | the same deal | `{"op": "deal", "buy": "Paper:1"}` | `deal ok=yes net=5 paid=5 received=0 balance=4320 hands= left=0` | `5x14 50x15 500x7` | yes |
| 26 | a rifle with nothing worn | `{"op": "quote", "buy": "AKM:1"}` | `quote ok=yes net=2500 paid=2500 received=0 balance=1820 hands=AKM left=0` | | yes |
| 27 | the settings put back | `{"op": "reload"}` | `reloaded: ... problems=0 revision=6` | | yes |
| 28 | the same quote | `{"op": "quote", "buy": "AKM:1"}` | `quote ok=yes net=2500 paid=2500 received=0 balance=1820 hands= left=0` | | yes |
| - | a canteen into the trousers, every free cell filled | `ozt_probe` `create`, `{"op": "fill", "into": "all"}` | the canteen at `CARGO CargoPants_Black r0 c2`; `fill: 45 Paper made in 7 holder(s), 54 cell(s) refused` | | yes |
| 29 | two things into full bags | `{"op": "deal", "buy": "Paper:2"}` | `deal ok=no why=STR_OZT_NO_SPACE net=10 paid=0 received=0 balance=0 hands= left=2` | unchanged | yes |
| 30 | a canteen and two piles sold, two papers and a canteen bought | `{"op": "deal", "sell": "Canteen,Ammo_762x39#0,Ammo_762x39#1", "buy": "Paper:2,Canteen:1"}` | `deal ok=yes net=-100 paid=0 received=100 balance=4420 hands= left=0`; `hands hold null` | `5x14 50x17 500x7` | yes |
| 31 | a canteen sold, four papers and a canteen bought | `{"op": "deal", "sell": "Canteen", "buy": "Paper:4,Canteen:1"}` | `deal ok=yes net=80 paid=80 received=0 balance=4340 hands=Canteen left=0`; `hands hold <id> Canteen` | `5x8 50x16 500x7` | yes |
| 32 | the canteen sold out of the hands | `{"op": "deal", "sell": "Canteen"}` | `deal ok=yes net=-60 paid=0 received=60 balance=4400 hands= left=0` | `5x10 50x17 500x7` | yes |
| 33 | one thing into full bags | `{"op": "deal", "buy": "Paper:1"}` | `deal ok=yes net=5 paid=5 received=0 balance=4395 hands=Paper left=0`; `hands hold <id> Paper` | `5x9 50x17 500x7` | yes |
| 34 | one more, the hands busy | `{"op": "deal", "buy": "Paper:1"}` | `deal ok=no why=STR_OZT_NO_SPACE net=5 paid=0 received=0 balance=0 hands= left=1` | unchanged | yes |
| 35 | a canteen into full bags | `{"op": "quote", "buy": "Canteen:1"}` | `quote ok=no why=STR_OZT_NO_SPACE net=120 paid=0 received=0 balance=0 hands= left=1` | | yes |
| 36 | two bandages sold, a canteen bought | `{"op": "deal", "sell": "BandageDressing,BandageDressing#1", "buy": "Canteen:1"}` | `deal ok=yes net=80 paid=80 received=0 balance=4315 hands= left=0`; the canteen `CARGO M65Jacket_Black r0 c2` | `5x3 50x16 500x7` | yes |
| - | the client against the server | `dump 4` | `SAME`, 84 items (`server-dump-full-bags.txt`, `client-dump-full-bags.txt`) | | yes |

After the run: the verdict is pass with no warning and no error, so no `deal defect`; the log
holds fourteen `deal:` lines, one for each deal that stood (rows 1, 2, 4, 5, 6, 7, 17, 19, 25,
30, 31, 32, 33, 36); no paper lies loose within ten metres, and the one canteen that does is
the one put on the ground for row 14 (deleted afterwards; the character stripped; the stand's
three files equal `examples/stand/`).

One thing was done differently from the plan: the comparison the plan asks for after row 3
was not taken on its own; the one after row 7 covers it.

## What the run shows

- **Paid:** rows 1 and 2 (row 2 exactly: the loose fives are spent whole and their cells are free).
- **Short of money:** row 3, refused with the server's net, nothing moved.
- **Change in notes:** row 1 (a fifty spent, two fives back onto the fives in the wallet); rows 6 and 7 the same with 500s.
- **One overflow to the hands:** row 33.
- **Two overflows refused:** row 29, with the count.
- **Price changed:** the scope at 650 in the file against 600 shown: refused as `changed`, with the server's net.
- **The order of things** (decision 11 of the plan): row 30, the canteen is laid first though the papers are named first, and nothing goes to the hands; row 31, laid once more with the canteen set aside for the hands, so that four papers take its cells.
- **Sold and bought in one deal:** row 36, a canteen takes the cells of two bandages; row 30, the cells of the canteen and the piles sold.
- **Refusals that touch nothing:** rows 8-16, 21, 23, 34, and the comparison after them.
- **Differed from the plan:** nothing in the answers. What the plan had wrong before this run is in the plan's execution record: the stall's placement (Task 4) and the profile's mod list (Task 1).

## After the owner's two decisions

The same day the owner changed two of the plan's own decisions, and the code with them:
prices are rounded always in the trader's favour ("we want no money dupes"), and what a deal
brings joins only a stack in the same condition ("what differs in condition must not be
stacked or averaged"). One more boot on the changed code. Rows 3.14 and 3.15 above were
measured before the change; their answers now are rows A.4 and A.5.

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| A.1 | prices that are exact multiples, as before | `price` for `Canteen`, for 30 rounds; `pay 40` | `buy(1)=120 sell(1 of 1, level 0)=60`; `buy(30)=300`; `pay 40: spend 50x1/2; change 10 = 5x2` | yes |
| A.2 | a worn canteen | `damage` to 0.6, `{"op": "price", "class": "Canteen"}` | `sell(1 of 1, level 1)=50` (51 goes down) | yes |
| A.3 | the same canteen at 950 of 1000 | `ozt_probe` `{"op": "qty", "item": "canteen", "set": "950"}`, `price` | `sell(1 of 1, level 1)=45`: 48.45 goes down to 45, where the nearest multiple is 50 | yes |
| A.4 | the goods file with `Paper` at `Buy` 7, `Sell` 7 and a `NoSuchThing` | `{"op": "reload"}` | `reloaded: ... problems=3 revision=2`; `Buy of 'Paper' is 7, not a multiple of the smallest note (5); using 10`, `Sell of 'Paper' is 7, not a multiple of the smallest note (5); using 5`, `'NoSuchThing' in category 'gear' is skipped: no such class` | yes |
| A.5 | the file put back; then `CooldownMs` 700000 and back | three reloads | `problems=0 revision=3`; `problems=1 revision=4`; `problems=0 revision=5` | yes |
| A.6 | the trader's `BuyFactor` at 1.01 | `{"op": "reload"}`, `price` for `Canteen`, for 30 rounds | `revision=6`; `buy(1)=125` (121.2 goes up, where the nearest multiple is 120); `buy(30)=450` (one round at 10.1 goes up to 15, then thirty of them) | yes |
| A.7 | the file put back | `{"op": "reload"}` | `problems=0 revision=7` | yes |
| A.8 | Task 6's first deal again | `quote` and `deal` of a bandage, `balance` | `net=40 ... balance=595`, `stacks: 5x4 5x5 50x1 500x1` | yes |
| A.9 | a pile of four rounds, worn | `ozt_probe` `create` with `ammo` 4, `{"op": "damage", "class": "Ammo_762x39", "health": "0.6"}` | `Ammo_762x39 health level 1` | yes |
| A.10 | eight rounds bought | `{"op": "deal", "buy": "Ammo_762x39:8"}` | `deal ok=yes net=80 paid=80 received=0 balance=515 hands= left=0`; the worn pile still `ammo 4/20`; a new pile `CARGO M65Jacket_Black r0 c4`, `ammo 8/20` | yes |
| A.11 | four more | `{"op": "deal", "buy": "Ammo_762x39:4"}` | `deal ok=yes net=40 paid=40 received=0 balance=475 hands= left=0`; the new pile `ammo 12/20`, the worn one still `ammo 4/20`; `stacks: 5x5 50x9` | yes |
| A.12 | the client against the server | `dump 2` | `SAME`, 29 items | yes |
| A.13 | the 36 rows of Task 7 again, on a fresh scene | the same commands in the same order | every answer, balance and stack list as in the table of Task 7 above; the comparison after row 36 `SAME`, 84 items | yes |

The log of this boot holds seventeen `deal:` lines (three of rows A.8-A.11, fourteen of
Task 7), no error of the mod and no `deal defect`. Its verdict fails on the warning budget
alone: the four WARNING lines of rows A.4 and A.5, which those rows provoke on purpose.

## A money loop, stacks, magazines, and places sold and bought

The same day the owner asked for a warning when goods sell for more than they cost, and for a
check of a deal that sells a worn thing and buys another for its slot. Writing the warning
found two money presses of the same family; both were closed first. Two boots on the changed
code: the first for every row below up to B.23, the second for the final wording of the
warning. Rows of the plan that were run again say so.

### The warning, and a stack priced as a piece (Task 3 Steps 10-11, whole)

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| B.1 | the rows of Steps 10-11 as they were | the same commands in the same order | every answer as in rows 3.8-3.13 and A.2-A.7: `sell(1 of 1, level 1)=50`, then `=45`; `problems=3 revision=2`, `problems=0 revision=3`, `problems=1 revision=4`, `problems=0 revision=5`; with the buy factor at 1.01 `buy(1)=125` and `buy(30)=450`; `problems=0 revision=7` | yes |
| B.2 | the canteen at `Sell` 130, `Buy` 120 | `{"op": "reload"}` | `reloaded: ... problems=1 revision=8`, one WARNING naming the canteen as a money loop | yes |
| B.3 | the file back; the trader's `SellFactor` at 2.1 | `{"op": "reload"}` | `problems=7 revision=9`: the canteen, the helmet, the backpack, the rifle, the magazine, the scope and the round; not the bandage (42 goes down to 40, its cost) | yes |
| B.4 | the file back | `{"op": "reload"}` | `problems=0 revision=10` | yes |
| B.5 | thirty nails, the goods line `Nail` with `Buy` 100, `Sell` 50 and no rule | `ozt_probe` `create` with `qty` 30; `{"op": "reload"}`; `{"op": "price", "class": "Nail"}` | the pile `qty 30/99`; `problems=0 revision=11`; `Nail rule='' buy(1)=100 sell(30 of 30, level 0)=15` (50 x 30 / 99 is 15.15) | yes |
| B.6 | twenty of them, and one | `price` with `count` 20, then 1 | `sell(20 of 30, level 0)=10`; `sell(1 of 30, level 0)=0` | yes |
| B.7 | twelve rounds, `Ammo_762x39` with `Buy` 200, `Sell` 100 and no rule | `price`, then with `count` 1 | `Ammo_762x39 rule='' buy(1)=200 sell(12 of 12, level 0)=60`; `sell(1 of 12, level 0)=5` | yes |
| B.8 | the file back | `{"op": "reload"}` | `problems=0 revision=12` | yes |

Before the change the two lines with `count` 1 would have answered 50 and 100, the price of the
whole stack for one unit of it. That is read from the code (the old `SellPrice` multiplied by
the count under the unit rule only, and `Fraction` answered 1 for every magazine class), not
measured: no build with the hole was booted.

### A magazine, and places sold and bought (Task 6 Step 5, whole)

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| B.9 | the first rows of the step as they were | the same commands | `balance=595`, `stacks: 5x4 5x5 50x1 500x1`; the worn pile still `ammo 4/20`, the new one `ammo 8/20` and then `12/20` in the jacket; `balance=475`, `stacks: 5x5 50x9` | yes |
| B.10 | how the game makes a magazine | `ozt_probe` `{"op": "create", "class": "Mag_AKM_30Rnd", "into": "pants", "cell": "free", "as": "rawmag"}` | `ammo 30/30` | a fact |
| B.11 | a magazine bought | `{"op": "deal", "buy": "Mag_AKM_30Rnd:1"}` | `deal ok=yes net=180 paid=180 received=0 balance=2295 hands= left=0`; `find`: `CARGO M65Jacket_Black r0 c5`, `ammo 0/30` | yes |
| B.12 | and sold back | `{"op": "deal", "sell": "Mag_AKM_30Rnd"}` | `deal ok=yes net=-90 paid=0 received=90 balance=2385 hands= left=0`; `stacks: 5x17 50x6 500x4` | yes |
| B.13 | the worn backpack sold, a backpack bought | `{"op": "deal", "sell": "TaloonBag_Blue", "buy": "TaloonBag_Blue:1"}` | `deal ok=yes net=200 paid=200 received=0 balance=2185 hands= left=0`; `find`: `ATT ... slot=Back`, entity 022728 where 022682 stood | yes |
| B.14 | a worn helmet sold, a helmet bought | `fit` with `make`, then `{"op": "deal", "sell": "Mich2001Helmet", "buy": "Mich2001Helmet:1"}` | the first on `SLOT Headgear` (022729); `deal ok=yes net=450 paid=450 received=0 balance=1735 hands= left=0`; `find`: `slot=Headgear`, 022730 | yes |
| B.15 | a bare rifle on the shoulder sold, a rifle bought | `fit` with `make`, then `{"op": "deal", "sell": "AKM", "buy": "AKM:1"}` | the first on `SLOT Shoulder` (022731); `deal ok=yes net=1250 paid=1250 received=0 balance=485 hands= left=0`; `find`: `slot=Shoulder`, 022732; `stacks: 5x7 50x9` | yes |
| B.16 | the client against the server | `dump 3` | `dump 3 items=19 hands=none state=1`, `SAME` | yes |
| B.17 | the canteen in the hands sold, a canteen bought, with every bag full | `move` to the hands, `fill` (77 papers, 22 cells refused), `hands` (022709, `hand state=2`), then `{"op": "deal", "sell": "Canteen", "buy": "Canteen:1"}` | `deal ok=yes net=60 paid=60 received=0 balance=425 hands=Canteen left=0`; `hands hold 022814 Canteen`, `inventory processing=no`, `CanDeleteItems=yes`, `hand state=2 idle=yes`; `stacks: 5x5 50x8` | yes |
| B.18 | the client against the server | `dump 4` | `dump 4 items=96 hands=Canteen state=2`, `SAME` | yes |
| B.19 | the new canteen sold out of the hands | `{"op": "deal", "sell": "Canteen"}` | `deal ok=yes net=-60 paid=0 received=60 balance=485 hands= left=0`; `hands hold null`, `hand state=1` | yes |
| B.20 | one more bought into them | `{"op": "deal", "buy": "Canteen:1"}` | `deal ok=yes net=120 paid=120 received=0 balance=365 hands=Canteen left=0`; `hands hold 022815 Canteen`, `hand state=2`; `stacks: 5x3 50x7` | yes |
| B.21 | the client against the server | `dump 5` | `dump 5 items=96 hands=Canteen state=2`, `SAME`; kept as `server-dump-places-sold-and-bought.txt` and `client-dump-places-sold-and-bought.txt` | yes |
| B.22 | a fresh scene: every cargo full but the empty backpack; the backpack sold and a paper bought | `fit`, `quote`, `deal` | `fit`: `Paper: CARGO TaloonBag_Blue r0 c0` (the only room); `quote ok=yes net=-195 paid=0 received=195 balance=195 hands=Paper left=0`; the deal the same; `hands hold 022951 Paper`; the backpack `GONE`; `stacks: 5x9 50x3`; `dump 6 items=72 hands=Paper state=2`, `SAME` | yes |
| B.23 | rows B.13-B.17 once more on a fresh scene, for the screenshot | the same four deals, with 2000 in the wallet | `balance=1800`, `1350`, `100`, `40`; `hands hold 023070 Canteen`, `hand state=2` | yes |

Each time the plan asked the engine for its gates about a place that was still taken, by the
thing the same deal sells, and the commit deleted that thing and created the new one there in
one call: on the character's `Back`, `Headgear` and `Shoulder` slots and in the hands, twice
each (rows B.13-B.17 and B.23). The hand state machine followed on both sides. Row B.22 is the
other side of the same rule: the cells of a container that is sold are nobody's.

The first screenshot of this run was a black frame: the machine's monitor was switched off,
the game went on rendering and the capture did not. Row B.23 is the same scene built again
once it was on.

The log of the first boot holds sixteen `deal:` lines, no error of the mod and no
`deal defect`; its verdict, taken before row B.1, is a pass.

### The wording of the warning (the second boot)

The first boot's WARNING lines gave the prices of the file alone, which read oddly when the
loop comes from a trader's factor (`Sell 60, Buy 120`). They now name both sums.

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| B.24 | the canteen at `Sell` 130 | `{"op": "reload"}` | `problems=1 revision=2`; `WARNING: Goods: 'Canteen' sells for 130 at 'stand' and costs 120 at 'stand': a money loop (Sell 130, Buy 120 in the file; the factors are counted)` | yes |
| B.25 | the file back; `SellFactor` 2.1 | `{"op": "reload"}` | `problems=7 revision=3`: the canteen 125 against 120, the helmet 945 against 900, the backpack 420 against 400, the rifle 2625 against 2500, the magazine 185 against 180, the scope 630 against 600, the round 10.5 against 10 | yes |
| B.26 | the file back | `{"op": "reload"}` | `problems=0 revision=4` | yes |

The verdict of the second boot, taken before row B.24, is a pass with no warning. A reload
needs no client: these three rows ran with nobody connected.

### A banknote in the price book (the third boot)

Found re-reading the cross-checks: nothing stopped a banknote of the server from standing in
the price book, where a note bought below its value, or sold above it, is a money press. A
note class there is now skipped at load.

| # | what is asked | command | answer | as expected? |
|---|---|---|---|---|
| B.27 | the goods file with `{ "ClassName": "OZT_ProbeNote500", "Buy": 400 }` added | `{"op": "reload"}` | `reloaded: traders=1 goods=11 money=items gates=none problems=1 revision=2`; `WARNING: Goods: 'OZT_ProbeNote500' in category 'gear' is skipped: it is one of this server's banknotes, and money is not goods` | yes |
| B.28 | the file back | `{"op": "reload"}` | `problems=0 revision=3` | yes |

The verdict of the third boot, taken before row B.27, is a pass with no warning.

Not covered here either: a second client watching such a deal; a thing bought into a
container bought in the same deal (it is not planned: a backpack and things for it, with full
bags, are two deals).
