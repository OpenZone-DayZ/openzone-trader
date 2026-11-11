# OpenZone Trader, Phase 3b: Buying a Thing With Parts, and Parts Onto a Thing — Plan and Record

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Goods can come with parts (a pistol with its magazine), a bought thing can be fitted with more parts in the same deal, and a part can be bought straight onto a thing the player already carries. Every part is paid at its own price and arrives attached, taking no room in the bags.

**Architecture:** The server only, driven by scripted deals; the screen keeps working as it is (goods that come with parts show their sum and say what comes with them) and gets the parts strip in phase 3c. The price book learns `With`; the request learns `Fit` (parts for a bought thing, a flat list with the index of what each hangs on) and `OnLow`/`OnHigh` (a carried thing the bought part goes onto). One new class, `OZT_Fit`, answers "which slot" from config; the plan asks the engine's gates with stand-ins; the commit attaches parents first. Phase 0 measured the calls (`answers.md` rows 17 to 19: a creation straight in a slot, a part on a part three levels down, a weapon's magazine only through `SpawnAttachedMagazine`).

**Tech Stack:** Enforce Script on DayZ 1.30.164289 (diag), `OpenZone_Core`, the `dayz` MCP server, the stand verbs `oz_trader` and `ozt_probe`.

**Design:** `E:\openzone\docs\specs\2026-11-10-openzone-trader-design.md` §3.4 ("Fitting a part"), §4.2, §6.2, §6.5, §8.1 (`With`).

## Global Constraints

The phase 1 plan's, unchanged (`docs/2026-11-11-phase-1-plan.md`, "Global Constraints"), and the owner's rule for everything that touches a price: no money out of nothing. What this part adds, and how each is closed:

- **Nothing comes free.** A part that comes with goods is charged at its own price at that trader; the goods cost the sum. A part free of charge that some trader pays for would be a money press, and a sum of classes cannot make a loop while no class alone does (the load-time check of decision 14 stays as it is).
- **A part comes with goods only where it is sold.** At a trader who does not sell the part (no price to buy, or a category he does not deal in) the goods are sold bare: every thing a trader hands out stays inside the categories he deals in, which is what the loop check counts.
- **A magazine that comes with a weapon, or is fitted to one, holds no rounds** (decision 16).

---

## Decisions

Numbered on from the phase 3a plan's thirty-one.

32. **`With`.** A goods line may name classes that come attached to it. A class that does not exist, has no price to buy in the book, is the goods itself, is named twice or does not fit the goods by config is left out with a warning (in memory; the file is the admin's). At a trader a `With` class comes with the goods when that trader sells it.
33. **`Fit`.** A buy line may carry parts for the thing it buys: a flat list of (class, slot name `SlotName` or "", the index `On` of the entry it hangs on, -1 for the bought thing). An entry hangs on an earlier entry only. Each is goods of this trader, bought at its own price. A line with fits buys one piece, and at most sixteen parts.
34. **Onto a carried thing.** A buy line may name a carried thing (`OnLow`, `OnHigh`) and a slot (`SlotName`): its class is then the part. The thing must be the player's and must not be sold in this deal; the slot must be free, or freed by this deal (what stands in it is sold). Such a line buys one piece and may carry fits of its own.
35. **Which slot.** The slot the request names; otherwise the first slot name of the part's `inventorySlot[]` that the host's `attachments[]` holds and the plan has not given away. A magazine goes onto a weapon whose `magazines[]` names it, in the slot `magazine`.
36. **Does it fit.** Config says whether there is a slot. Then the engine's gates are asked with stand-ins (`GameInventory.LocationTestAddEntity`, the item check and the script check, no occupancy): the carried thing itself for a part that goes onto it, a local entity of the class for goods that do not exist yet. Occupancy is the plan's own. A part that does not fit refuses the deal as `STR_OZT_NO_CHANGED`: the screen that offered it holds a stale picture.
37. **The commit.** A bought thing is created at its place, then its parts are attached, a parent before what hangs on it: a magazine onto a weapon with `Weapon_Base.SpawnAttachedMagazine(type, WeaponWithAmmoFlags.NONE)` and emptied, anything else with `CreateAttachmentEx(type, slot)`. Parts onto carried things are attached after the deletions, so a slot the deal frees is free. A part the engine refuses is made by the engine's own search, then at the player's feet, with a `deal defect` line: a paid thing is never lost.
38. **What `open` says of goods.** `Buy` is what one costs at this trader with what comes with it; `With` lists what comes with it there.

---

## File structure

| file | change |
|---|---|
| `scripts/3_Game/OpenZone_Trader/OZT_GoodsFile.c` | `OZT_Goods.With` |
| `scripts/3_Game/OpenZone_Trader/OZT_Wire.c` | `OZT_WireFit`; `OZT_WireBuy.Fit`, `OnLow`, `OnHigh`, `SlotName`; `OZT_WireGoods.With` |
| `scripts/4_World/OpenZone_Trader/OZT_Fit.c` | new: which slot a part takes on a host, by config |
| `scripts/4_World/OpenZone_Trader/OZT_Config.c` | `With` checked at load |
| `scripts/4_World/OpenZone_Trader/OZT_Book.c` | `OZT_BookEntry.With`; `Comes`, `BuyWith` |
| `scripts/4_World/OpenZone_Trader/OZT_Deal.c` | buys with parts: price, slots, gates, the commit |
| `scripts/4_World/OpenZone_Trader/OZT_Service.c` | `open`: the sum and `With` |
| `OpenZone_Trader_Bridge/.../OZT_Verb.c` | `buy=Class:n+Part>Sub`, `on=`, `slot=`; the op `fits` |
| `scripts/5_Mission/OpenZone_Trader/OZT_TradeCard.c`, `strings.csv` | the card says what goods come with |
| `examples/stand/OZ_Trader_Goods.json`, `dayz-mcp.toml` | a pistol that comes with its magazine, goggles and a battery: fifteen goods |

---

### Task 1: The book, the wire and the slots

**Interfaces produced:**

```c
class OZT_Goods { ...; ref array<string> With; }
class OZT_WireFit { string ClassName; string SlotName; int On; }  // On: -1 the bought thing, else an earlier entry
class OZT_WireBuy { ...; ref array<ref OZT_WireFit> Fit; int OnLow; int OnHigh; string SlotName; }
class OZT_WireGoods { ...; ref array<string> With; }

class OZT_Fit
{
    static void HostSlots(string type, array<string> names);      // attachments[] of the class
    static void PartSlots(string type, array<string> names);      // inventorySlot of the class
    static bool IsMagazine(string type);                          // a magazine, not a pile of rounds
    static bool WeaponTakes(string weapon, string magazine);
    static string SlotFor(string host, string part, array<string> taken);   // "" when none
}

class OZT_Book
{
    static void Comes(OZT_Point trader, OZT_BookEntry e, out array<string> parts);
    static int BuyWith(OZT_Point trader, OZT_BookEntry e, int count);
}
```

- [x] **Step 1:** `OZT_GoodsFile.c`, `OZT_Wire.c`: the fields above, copied null-safe.
- [x] **Step 2:** `OZT_Fit.c`; `OZT_InvModel.SlotNamesOf` reads through it.
- [x] **Step 3:** `OZT_Config.CheckGoods`: decision 32. `OZT_Book`: `With` in the entry, `Comes`, `BuyWith`.
- [x] **Step 4:** `OZT_Service.c`: `open` fills `Buy` with the sum and `With`.
- [x] **Step 5:** `OZT_Verb.c`: the op `fits` (`host=` a class or a carried thing with `carried=1`, `part=`, `slot=`): the slot by config and the engine's answer.

The first build did not compile: a field named `Slot` in a World class is refused, `Slot` is a type of the game there (`Variable name 'Slot' already used as type name`). The fields are `SlotName`, on the wire too.

### Task 2: The deal

- [x] **Step 1:** `ReadBuys`: a line's parts (what comes with it, then its fits), each priced; decisions 33 to 35; the line's sum.
- [x] **Step 2:** the plan: `CheckFits` asks the gates (decision 36); a line with `On` takes no place.
- [x] **Step 3:** the commit: `AttachParts` (decision 37).
- [x] **Step 4:** `OZT_Verb.c`: `buy=Class:n+Part+Part>Sub`, `on=Class[#nth]`, `slot=`.
- [x] **Step 5:** the stand's files: `MakarovIJ70` (650/325, `With` its magazine), `Mag_IJ70_8Rnd` (60/30), `NVGoggles` (1 500/750), `Battery9V` (20/10); `expect.counters goods = 15`.
- [x] **Step 6:** Boot; the kit and money (twenty notes of 500); the rows below. Run once, 2026-11-11, on the second build: see "As the stand answered" under the table.

| args (`oz_trader`) | expected |
|---|---|
| `{"op":"fits","host":"AKM","part":"PSO1Optic"}` | the slot `weaponOpticsAK`, the engine yes |
| `{"op":"fits","host":"AKM","part":"Mag_IJ70_8Rnd"}` | no slot: the rifle's `magazines[]` does not name it |
| `{"op":"fits","host":"MakarovIJ70","part":"Mag_IJ70_8Rnd"}` | the slot `magazine`, the engine yes |
| `{"op":"open"}` | the pistol's `Buy` 710 and `With` its magazine; the rifle's 2 500 |
| `{"op":"deal","buy":"MakarovIJ70:1"}` | net 710; a pistol with its magazine on it, no rounds in the magazine; the weapon reads its magazine and is idle |
| `{"op":"deal","buy":"AKM:1+PSO1Optic+Mag_AKM_30Rnd"}` (the kit's rifle sold first) | net 2 500 + 600 + 180 = 3 280; the rifle with both parts on it |
| `{"op":"deal","buy":"Mich2001Helmet:1+NVGoggles>Battery9V"}` | net 900 + 1 500 + 20 = 2 420; the helmet on the head, the goggles on it, the battery in the goggles |
| `{"op":"deal","sell":"Mag_AKM_30Rnd#<the rifle's>","buy":"Mag_AKM_30Rnd:1","on":"AKM"}` | the old magazine sold (240), a new empty one on the rifle (180): net -60; the rifle reads its magazine on both sides |
| `{"op":"deal","buy":"PSO1Optic:1","on":"AKM"}` with the scope still on the rifle | refused `STR_OZT_NO_CHANGED`: the slot is taken |
| `{"op":"quote","buy":"AKM:1+Mag_IJ70_8Rnd"}` | refused `STR_OZT_NO_CHANGED`: it does not fit |
| `{"op":"quote","buy":"AKM:2+PSO1Optic"}` | refused `STR_OZT_NO_INTERNAL`: a line with fits buys one |
| a goods file with `With` naming a class that is not in the book, then `reload` | `problems=1`, the warning names both classes, the goods are sold bare |

- [x] **Step 7:** Commit (the server half and the card's line; the record below is owed).

**As the stand answered** (one boot, verdict a pass with `goods=15 problems=0`; the rows not yet copied into `deals.md`):

| what was sent | answer |
|---|---|
| `fits` AKM / PSO1Optic | `slot by config 'weaponOpticsAK'; the engine's gates yes, with occupancy yes` (a local stand-in of the rifle) |
| `fits` AKM / Mag_IJ70_8Rnd | `slot by config ''` |
| `fits` MakarovIJ70 / Mag_IJ70_8Rnd | `'magazine'; gates yes, with occupancy yes` |
| `fits` the carried AKM (a scope on it) / PSO1Optic, `carried=1` | `'weaponOpticsAK'; gates yes, with occupancy no` |
| `fits` Mich2001Helmet / NVGoggles; NVGoggles / Battery9V | `'NVG'` and `'BatteryD'`, gates yes both |
| `{"op":"deal","buy":"MakarovIJ70:1"}` | `ok=yes net=710`; the pistol lies in the jacket with its magazine on it, 0 of 12 rounds; `GetMagazine` names it, idle |
| `{"op":"deal","sell":"AKM","buy":"AKM:1+PSO1Optic+Mag_AKM_30Rnd"}` | `ok=yes net=1490 ... sold=1790 kept=3`: 3 280 for the new rifle with both parts, 1 790 for the old; the new one on the shoulder, scope and an empty magazine on it; `GetMagazine` names it, idle, stable state 4 |
| `{"op":"deal","buy":"Mich2001Helmet:1+NVGoggles>Battery9V"}` | `ok=yes net=2420`; the helmet on the head, the goggles on it, the battery in the goggles |
| a comparison after the three (`dump 21`) | SAME, 31 lines (`*-dump-task3b-bought-with-parts.txt`) |
| `{"op":"deal","buy":"PSO1Optic:1","on":"AKM"}` (a scope stands there) | `ok=no why=STR_OZT_NO_CHANGED` |
| `{"op":"quote","buy":"AKM:1+Mag_IJ70_8Rnd"}` | `ok=no why=STR_OZT_NO_CHANGED` |
| `{"op":"quote","buy":"AKM:2+PSO1Optic"}` | `ok=no why=STR_OZT_NO_INTERNAL` |
| `{"op":"deal","sell":"Mag_AKM_30Rnd","buy":"Mag_AKM_30Rnd:1","on":"AKM"}` | `ok=yes net=90 ... sold=90`: the rifle's own magazine (empty) sold, a new one bought onto the rifle; the rifle names the new one, idle, stable state 4 |
| `{"op":"deal","buy":"Battery9V:1","on":"PSO1Optic"}` | `ok=yes net=20`; the battery stands in the scope on the rifle on the shoulder |
| a comparison after those (`dump 22`) | SAME, 32 lines (`*-dump-task3b-parts-onto-own-things.txt`) |
| the pistol's `With` set to `Mag_FNX45_15Rnd`, `NoSuchPart`, `PSO1Optic`; `reload` | `problems=3`: "it has no price to buy in the price book", "no such class", "it does not fit, or its slot is taken by a part named before it"; the pistol then quotes 650, bare |
| the file put back; `reload` | `problems=0` |

The rows of the table above that were not sent: `open`'s answer read for the pistol's `Buy` and `With` (the deal's 710 says the sum is charged; the screen's shelf was not looked at).

**Stopped here on the owner's word (2026-11-11, "we pause for now").** Owed when the work goes on: Task 3 whole; a clean boot for the verdict (this boot's log holds the three warnings of the last rows); the rows into `deals.md`.

### Task 3: The card, and the record

- [x] **Step 1:** `OZT_TradeCard.c`, `strings.csv`: a line "comes with" naming the parts. Written and compiled on the client (it joined the stand); not yet looked at.
- [ ] **Step 2:** On a live client, both languages: the pistol's tile reads 710, its card names the magazine; bought, it arrives with the magazine on it.
- [ ] **Step 3:** `deals.md`: a section "Phase 3b"; the design's §4.2, §6.2 and §8.1; the skills.
- [ ] **Step 4:** Commit.

---

## What this part does not do

- The screen fits nothing yet: no parts strip, no filtered list (3c). Fits and parts onto carried things are the verb's.
- A bundle has no price of its own: the sum is the price (the design's "unless the entry sets one" is not built: a price below the sum is a discount on parts and would have to pass the loop check as one).
- A part that comes with goods cannot be swapped for another in the same line: a fit into a slot that an included part takes refuses the deal.
