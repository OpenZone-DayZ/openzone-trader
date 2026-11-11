# OpenZone Trader, Phase 3a: Selling a Thing With What Is On It — Plan and Record

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A thing is sold as a tree: itself, what hangs on it at any depth, and the rounds loaded in it. Each part is paid by its own line of the price book; a part the player keeps, or the trader does not take, comes off and stays with the player; a container is sold when nothing would be left inside it.

**Architecture:** The server only, driven by scripted deals; the screen keeps working as it is and gets its part in phase 3c. One new reader, `OZT_Tree`, turns a sold thing into nodes with a state each (sold, kept by choice, kept because not taken, riding on a kept part) and into lots of rounds. `OZT_Deal` plans the kept parts first, in the room that is free before the sale, then everything else as it does now; the commit moves the kept parts, deletes what is sold and creates what is bought and what comes back, in one script call. Phase 0 measured every engine call this needs (`docs/measurements/2026-11-11/answers.md`, "What the deal does", points 2, 3 and 7).

**Tech Stack:** Enforce Script on DayZ 1.30.164289 (diag), `OpenZone_Core`, the `dayz` MCP server, the stand verbs `oz_trader` and `ozt_probe`.

**Design:** `E:\openzone\docs\specs\2026-11-10-openzone-trader-design.md` §4.3, §5 ("Kept parts are planned first"), §6.1, §6.3, §6.4. Phase 3 is three parts: 3a this one; 3b buying parts, onto goods and onto the player's own things (§6.2); 3c the parts strip and the rest of §3.4 on the screen. 3b and 3c get plans of their own.

## Global Constraints

The phase 1 plan's, unchanged (`docs/2026-11-11-phase-1-plan.md`, "Global Constraints"): English and Ukrainian only; prefix `OZT_`; no invented API; a refusal is a string-table key; work on `main`; nothing pushed without the owner's word; a subagent's brief says "Do NOT push".

The owner's rule for everything that touches a price (2026-11-11): no money out of nothing. This part adds three ways to make some, and closes each in its first task:

- **Rounds are paid as loose rounds are.** A round in a magazine or a weapon brings what the same round brings in a pile of its class: the pile's line of the book, its rule, and the condition factor of the round's own damage. Loading worn rounds into a magazine must not sell them as new.
- **Rounds that come back come back as they were.** Kept rounds are made as piles of their class with the mean health the rounds had. Selling a magazine and keeping its rounds must not repair them.
- **A part is never lost and never doubled.** A kept part is moved, never copied; if the engine refuses its planned place at the commit, it goes to the engine's own choice, then to the ground at the player's feet, and the log says so. A part is in exactly one of four states, and only a sold node is paid.

---

## Decisions

Numbered on from the phase 1 plan's sixteen.

17. **The tree.** The root is the thing a sell line names. Its nodes are its attachments, theirs, and so on. Cargo is not in the tree: what lies inside a thing is a thing of its own (§6.3).
18. **States.** A root must be taken by the trader, or the line is refused as `STR_OZT_NO_NOT_TAKEN`. An attachment under a sold node is: kept when the request names it; kept when the trader pays nothing for it (not in his book, not in a category he deals in, ruined); otherwise sold, and walked in turn. Everything on a kept part rides on it and is neither priced nor touched.
19. **Rounds.** Every sold magazine (not a pile) and every sold weapon is read round by round: the magazine's cartridges, each muzzle's chamber and internal magazine. Rounds are grouped into lots by pile class (`CfgAmmo <type> spawnPileType`) and health level (the game's damage thresholds 0.3, 0.5, 0.7, 1.0). A lot is kept when the request says so for the thing it is loaded in, or when the trader pays nothing for it; otherwise it is sold at the pile's line: under the unit rule the price of one times the count, under any other rule the lot's share of a full pile, times the level's factor, rounded down to the smallest note, lot by lot.
20. **What a container needs.** A sold node (the root or a sold part) with a cargo is sold only when every thing lying in that cargo is sold whole by the same deal. Otherwise `STR_OZT_NO_NOT_EMPTY` with the number of things in the way. Parts and rounds no longer count as "in the way": they are the tree.
21. **Listed twice.** A thing named by a sell line may not hang, by attachments alone, on a thing another line sells: it is that line's part already. Refused as `STR_OZT_NO_INTERNAL` (a screen never sends it). A thing lying in the cargo of a sold thing is a line of its own, as decision 20 wants.
22. **Kept parts are planned first.** They move while their host still exists, so each gets a cell that is free before anything is sold; the cells of what the deal sells are not theirs. The holders that leave or move (sold nodes, kept parts and everything on them) take nothing. A kept part with no cell goes to the hands when they are free now and nothing else needs them.
23. **The fit rule with kept parts.** What has no place is counted together: bought things and kept parts. None: the deal stands. One: it goes to the hands, if they are free. More: when no kept part is among them, the second laying of decision 11 is tried; otherwise the deal is refused as `STR_OZT_NO_SPACE` with the count.
24. **Kept rounds** are placed after the sale, like bought things, as stacks of their own: never merged into a stack the player has (what differs in condition is not stacked), each no larger than a full pile.
25. **The commit's order** (one script call): kept parts move (`LocationCanAddEntity`, then `LocationSyncMoveEntity` and `InventoryInputUserData.SendServerMove`); sold nodes and spent notes are deleted; stacks shrink; bought things, kept rounds and the change are created; stacks grow.
26. **What `open` says of a carried thing.** `Price` stays the thing's own. `Tree` is new: what the thing brings sold with everything on it that the trader takes, nothing kept. `Loaded` and `LoadedPrice` are new: the rounds in it and what they bring. `Why` 2 now means one thing: something lies inside it. A screen that sells a thing whole shows `Tree` before the quote answers.

---

## File structure

| file | change |
|---|---|
| `scripts/3_Game/OpenZone_Trader/OZT_Wire.c` | `OZT_WireKeep`; `OZT_WireSell.Keep`; `OZT_WireOwn.Tree`, `Loaded`, `LoadedPrice` |
| `scripts/4_World/OpenZone_Trader/OZT_Tree.c` | new: `OZT_Node`, `OZT_RoundLot`, `OZT_Tree` |
| `scripts/4_World/OpenZone_Trader/OZT_InvModel.c` | holders that take nothing; a place kept for a moved part; hands taken |
| `scripts/4_World/OpenZone_Trader/OZT_Deal.c` | the sells read as trees; kept parts and kept rounds in the plan and in the commit |
| `scripts/4_World/OpenZone_Trader/OZT_Service.c` | `open` fills the new fields |
| `OpenZone_Trader_Bridge/.../OZT_Verb.c` | `tree`; `keep=` and `keeprounds=` on `quote` and `deal`; the line sums in the answer |
| `scripts/5_Mission/OpenZone_Trader/OZT_TradeMenu.c` | a carried thing's tile and its cart line show `Tree` |
| `OpenZone_Trader/strings.csv` | "not empty" says "inside", not "on it" |

---

### Task 1: The tree, read and priced

**Interfaces produced:**

```c
class OZT_WireKeep { int Low; int High; int Rounds; }     // Rounds 1: keep the rounds loaded in that thing; 0: keep the thing
class OZT_WireSell { ...; ref array<ref OZT_WireKeep> Keep; }

class OZT_Node { EntityAI Item; OZT_Node Up; int State; int Price; }
class OZT_RoundLot { EntityAI In; string Pile; int Level; int Count; float Damage; int Price; bool Kept; }
class OZT_Tree
{
    static const int SOLD = 0; static const int KEPT = 1; static const int NOT_TAKEN = 2; static const int RIDES = 3;
    ref array<ref OZT_Node> Nodes;        // the root first, then every node under it
    ref array<ref OZT_RoundLot> Lots;
    ref array<EntityAI> Gone;             // every sold node
    ref array<EntityAI> Moves;            // every kept part that comes off: the topmost of each kept branch
    int Total;                            // what the trader pays for it
    int Inside;                           // things in the cargo of sold nodes that `whole` does not hold
    static OZT_Tree Read(OZT_Point trader, EntityAI root, array<ref OZT_WireKeep> keep, array<EntityAI> whole);
    string Text();                        // one line for the stand
}
```

- [x] **Step 1:** `OZT_Wire.c`: `OZT_WireKeep` with `Copy()`; `OZT_WireSell.Keep` made in a constructor and copied in `Copy()` (an arrived line may have none: null is read as empty); `OZT_WireOwn.Tree`, `Loaded`, `LoadedPrice`.
- [x] **Step 2:** `OZT_Tree.c`: the walk of decisions 17 to 20; the rounds of a magazine (`GetAmmoCount`, `GetCartridgeAtIndex`) and of a weapon (`GetMuzzleCount`, `IsChamberEmpty`, `GetCartridgeInfo`, `GetInternalMagazineCartridgeCount`, `GetInternalMagazineCartridgeInfo`); the lot's price as decision 19 says, with the full pile from `CfgMagazines <pile> count`.
- [x] **Step 3:** `OZT_Service.c`: `open` reads a tree for every carried thing the trader takes and fills `Tree`, `Loaded`, `LoadedPrice`; `Why` 2 only when `Inside` is above zero.
- [x] **Step 4:** `OZT_Verb.c`: the op `tree` (`{"op":"tree","item":"AKM"}`, with `keep` and `keeprounds` lists of `Class[#nth]`) prints `OZT_Tree.Text()`; `open`'s detail gains nothing (the full answer is in `trader_last.json`).
- [x] **Step 5:** Boot; the kit; the rows below. Every row answered as written (2026-11-11, the first build).

The kit is the probe's (`strip`, `dress`): a rifle on the shoulder with a scope, a suppressor, a buttstock, a handguard and a magazine of thirty rounds; a plate carrier with pouches and a holster with a pistol and its magazine; a backpack with four things. The stand's trader takes the rifle (1 250), its magazine (90), the scope (300) and 7.62x39 rounds (5 a round); he does not take the suppressor, the buttstock, the handguard, the carrier or the pistol.

| args (`oz_trader`) | expected |
|---|---|
| `{"op":"tree","item":"AKM"}` | the rifle sold 1 250, the scope sold 300, the magazine sold 90, thirty rounds sold 150; the suppressor, the buttstock and the handguard kept, not taken; `total=1790`, `moves=3`, `inside=0` |
| `{"op":"tree","item":"AKM","keep":"PSO1Optic"}` | the scope kept by choice; `total=1490`, `moves=4` |
| `{"op":"tree","item":"AKM","keep":"Mag_AKM_30Rnd"}` | the magazine kept, its rounds neither sold nor kept apart (they ride); `total=1550`, `moves=4` |
| `{"op":"tree","item":"AKM","keeprounds":"Mag_AKM_30Rnd"}` | the magazine sold 90, thirty rounds kept; `total=1640` |
| `{"op":"tree","item":"Mag_AKM_30Rnd"}` | the magazine alone: 90 and thirty rounds 150, `total=240` |
| `{"op":"tree","item":"TaloonBag_Blue"}` | the backpack 200, `inside=4` |
| `{"op":"tree","item":"PlateCarrierVest"}` | refused: the trader does not take the root |
| `{"op":"damage","class":"Mag_AKM_30Rnd","rounds":"0.4"}` (every round of the magazine worn, by `Magazine.SetCartridgeDamageAtIndex`), then `{"op":"tree","item":"Mag_AKM_30Rnd"}` | the rounds are one lot of level 1: 30 x 5 x 0.85 = 127.5, paid 125; `total=215` |
| `{"op":"open"}` (after the row above, so with the worn rounds) | `trader_last.json`: the rifle's `Price` 1250, `Tree` 1765, `Why` 0; the magazine's `Tree` 215, `Loaded` 30, `LoadedPrice` 125; the backpack's `Why` 2 |

- [x] **Step 6:** Commit: `trader: a sold thing is read as a tree: parts by their own lines, rounds by the pile's`.

One thing was added while writing the reader: a loaded round whose cartridge type names no pile class (`spawnPileType` empty) can neither be paid nor given back, so it counts into `Inside` and the thing is refused as not empty until it is unloaded.

### Task 2: The plan and the commit

- [ ] **Step 1:** `OZT_InvModel.c`: a second list beside `gone`, `shut`: holders whose cargo takes nothing (they leave or move). `Reserve(OZT_Place place, string type)` marks the cells of a place another model gave. `TakeHands()` marks the hands. `PlaceInHandsNow(string type, out OZT_Place place)` asks the hands as they are (`CanAddEntityInHands`).
- [ ] **Step 2:** `OZT_Deal.c`, the sells: a first pass resolves every line and collects the things sold whole; a second reads each as a stack (as now) or as a tree (decisions 18 to 21). The deal keeps `m_Gone` (every sold node), `m_Moves` (kept parts), `m_KeptLots`.
- [ ] **Step 3:** `OZT_Deal.c`, the plan: before `LayThings`, a model of the inventory as it is, with the moved and the sold holders shut, gives every kept part a cell (decision 22). `LayThings` shuts the same holders, reserves those cells, lays the bought lines, then the kept lots as stacks of their own (decision 24), then the change. The leftover rule is decision 23.
- [ ] **Step 4:** `OZT_Deal.c`, the commit (decision 25). A kept part whose planned place the engine refuses goes to `FindFreeLocationFor` of the player, then to the ground; each is a `deal defect` line. A kept lot's stacks get their units and their health.
- [ ] **Step 5:** `OZT_Verb.c`: `keep=` and `keeprounds=` on `quote` and `deal`; the answer's line gains `sold=` and `lines=`.
- [ ] **Step 6:** Boot; the kit and money; the rows below, each followed by the two dumps (`ozt_probe dump`): the client's inventory is the server's.

| args (`oz_trader`) | expected |
|---|---|
| `{"op":"quote","sell":"AKM"}` | `ok=yes net=-1790`, `left=0`: three parts have cells |
| `{"op":"deal","sell":"AKM"}` | the rifle, the scope and the magazine are gone; the suppressor, the buttstock and the handguard lie in the bags, the same entities (their network ids); balance up by 1 790 |
| `dress`, then `{"op":"deal","sell":"AKM","keep":"PSO1Optic,Mag_AKM_30Rnd"}` | net -1 250; the scope and the magazine in the bags, the magazine with its thirty rounds |
| `dress`, then `{"op":"deal","sell":"AKM","keeprounds":"Mag_AKM_30Rnd"}` | net -1 640; thirty rounds as two piles (20 and 10) of their own |
| `dress`, then `{"op":"deal","sell":"Mag_AKM_30Rnd"}` | the magazine sold off the rifle: 240; the rifle reads no magazine on both sides |
| `dress`, bags filled (`ozt_probe fill`), `{"op":"quote","sell":"AKM"}` | `ok=no why=STR_OZT_NO_SPACE left=3` (or 2 with one in the hands: the row is written from what the stand says, with the reason) |
| `{"op":"quote","sell":"TaloonBag_Blue"}` | `ok=no why=STR_OZT_NO_NOT_EMPTY left=4` |
| `{"op":"deal","sell":"TaloonBag_Blue,<its four things>"}` | sold together, those of them the trader takes; a thing he does not take refuses the deal as `STR_OZT_NO_NOT_TAKEN` |
| `{"op":"quote","sell":"AKM,PSO1Optic"}` | `ok=no why=STR_OZT_NO_INTERNAL`: the scope is the rifle's part already |
| `{"op":"deal","sell":"AKM","buy":"AKM:1"}` | the slot the rifle leaves takes the new rifle (decision of §5); the kept parts in the bags |

- [ ] **Step 7:** Commit: `trader: a deal sells trees: kept parts move first, kept rounds come back as they were, a container goes with what is in it`.

### Task 3: The screen shows the tree's sum

- [ ] **Step 1:** `OZT_TradeMenu.c`: a carried thing's tile shows `Tree`; its cart line's `Whole` is `Tree`; the card's line "the trader pays" says the same.
- [ ] **Step 2:** `strings.csv`: `STR_OZT_UI_WHY_NOT_EMPTY` and `STR_OZT_NO_NOT_EMPTY` speak of what lies inside.
- [ ] **Step 3:** On a live client: the rifle's slot tile shows 1 790; dragged to SELL, the quote says 1 790 and Confirm sells it; the three parts the trader does not take appear among the player's things.
- [ ] **Step 4:** Commit: `trader: the screen sells a thing with what is on it`.

### Task 4: Record

- [ ] `docs/measurements/2026-11-11/deals.md`: a section "Phase 3a: trees" with the rows above as the stand answered them.
- [ ] The design's §4.2, §4.3, §5 and §6 say what is built.
- [ ] The skills: what the stand taught that phase 0 had not.

---

## What this part does not do

- No part is chosen on a screen yet: the screen sells a tree whole, with what the trader does not take kept. Keeping by choice is the verb's until phase 3c.
- Nothing is bought onto anything (3b).
- A kept part is planned by its class, not by what hangs on it: a kept part's own size is its class's, as everywhere in the game.
- A second client watching a kept part move was not put there (phase 0's open point stands).
