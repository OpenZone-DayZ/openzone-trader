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
