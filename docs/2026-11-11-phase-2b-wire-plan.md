# OpenZone Trader, Phase 2b, Part 1: The Wire for the Screen — Plan and Record

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the service `trader` what a trade screen needs and the stand cannot do without: the player's own things with what each brings, a quote that changes nothing, and a way to tell a client to open.

**Architecture:** Two decisions, both made here because phase 2a's facts allow them. The screen does not price and does not plan: the server's `quote` is what it shows before Confirm (the net, what is paid or received, the balance afterwards, the one thing for the hands, the count that does not fit), so there is one copy of every rule and the answer the player sees is the answer a deal gets. And the server prices the player's own things: `open` lists every carried entity the trader pays for, with its price as it is now, so the screen marks the rest as not taken without knowing the price book's rules.

**Tech Stack:** Enforce Script on DayZ 1.30.164289 (diag), `OpenZone_Core` (service registry, `OZ_Rpc.Show`), the `dayz` MCP server, the stand verb `oz_trader`.

**Design:** `E:\openzone\docs\specs\2026-11-10-openzone-trader-design.md` — §4.1 (opening), §4.2 (the request), §5 (the model). Facts: `docs/measurements/2026-11-11/client-answers.md`.

## Global Constraints

The phase 1 plan's, unchanged (`docs/2026-11-11-phase-1-plan.md`, "Global Constraints"): English and Ukrainian only; prefix `OZT_`; no invented API; a refusal is a string-table key; work on `main`; nothing pushed without the owner's word; a subagent's brief says "Do NOT push".

One thing changed since: the phase 1 plan's code blocks are no longer the source of these files. Phase 2a and this plan edit the repository directly, so the phase 1 assembler must not be run with its sync again.

---

### Task 1: `open` names the player's own things; `quote`; `show`

**Files:**
- Modify: `OpenZone_Trader/scripts/3_Game/OpenZone_Trader/OZT_Const.c` (`OP_QUOTE`, `SHOW_PREFIX`)
- Modify: `OpenZone_Trader/scripts/3_Game/OpenZone_Trader/OZT_Wire.c` (`OZT_WireOwn`, `OZT_OpenRes.Yours`)
- Modify: `OpenZone_Trader/scripts/4_World/OpenZone_Trader/OZT_Deal.c` (`InTheWay` static; the gap between quotes)
- Modify: `OpenZone_Trader/scripts/4_World/OpenZone_Trader/OZT_Service.c` (the list; the route of `quote`)
- Modify: `OpenZone_Trader_Bridge/scripts/5_Mission/OpenZone_Trader_Bridge/OZT_Verb.c` (`show`; `quote` through the handler, `twice`; `yours=` in the detail of `open`)

**Interfaces:**
- Produces, on the wire:
  - `OZT_WireOwn { int Low; int High; int Price; int Why; int Units; }`: an entity by the halves of its network id; `Price` for the whole of it as it is; `Why` 0 (sells) or 2 (not empty); `Units` above zero for a stack, of which a part may be sold.
  - `OZT_OpenRes.Yours`: `array<ref OZT_WireOwn>`, every carried entity whose price here is above zero, in the engine's order of enumeration. What is not listed is not taken.
  - Operation `quote` (`OZT_Const.OP_QUOTE`): the body of `deal`; the answer of `deal` with nothing changed and `Net` not compared; `STR_OZT_NO_BUSY` when the same player asked less than `OZT_Deal.QUOTE_GAP_MS` (100) ago.
  - `OZT_Const.SHOW_PREFIX` (`"trader:"`): what `OZ_Rpc.Show` carries is this prefix and the trader's id.
- Produces, in script: `static int OZT_Deal.InTheWay(EntityAI e)`.

- [x] **Step 1: The constants and the records**

In `OZT_Const.c`, after `OP_DEAL`:

```c
    static const string OP_QUOTE = "quote";

    // What the server tells a client to show (OZ_Rpc.Show): this prefix, then
    // the trader's id.
    static const string SHOW_PREFIX = "trader:";
```

In `OZT_Wire.c`, before `class OZT_OpenRes`:

```c
// One thing the player carries that this trader pays for: the entity by the
// two halves of its network id, and what it brings as it is now. What is not in
// the list is not taken here.
class OZT_WireOwn
{
    int Low = 0;
    int High = 0;
    // For the whole of it: the thing, or the stack as it stands.
    int Price = 0;
    // 0: sells. 2: not empty, something hangs on it, lies in it or is loaded
    // into it (phase 3 sells such a thing as a tree).
    int Why = 0;
    // Above zero: a stack of this many units, of which a part may be sold.
    int Units = 0;
}
```

and in `OZT_OpenRes` the field `ref array<ref OZT_WireOwn> Yours;`, made in the constructor like the other lists.

- [x] **Step 2: A quote for clients**

In `OZT_Deal.c`: `InTheWay` becomes `static int InTheWay(EntityAI e)`; beside `s_Last`:

```c
    // A quote changes nothing, but it lays a whole plan: one client may ask
    // for one only so often.
    static const int QUOTE_GAP_MS = 100;

    protected static ref map<string, int> s_LastQuote = new map<string, int>();
```

`Reset()` clears it too; and `CheckPlayer` ends, after the cooldown of a commit:

```c
        else
        {
            int asked;
            int now = GetGame().GetTime();
            if (s_LastQuote.Find(Uid(), asked) && now - asked < QUOTE_GAP_MS)
                return OZT_Const.NO_BUSY;
            s_LastQuote.Set(Uid(), now);
        }
        return "";
```

- [x] **Step 3: The list and the route**

In `OZT_Service.c`, after the goods of `open`:

```c
        // What the player carries that this trader pays for, each with what it
        // brings as it is now. The screen marks everything else as not taken.
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI own = all[i];
            if (!own || own == player)
                continue;
            OZT_BookEntry entry = OZT_Book.EntryOf(own.GetType());
            int units = OZT_Stack.Units(own);
            int price = OZT_Book.SellPrice(trader, entry, own, units);
            if (price <= 0)
                continue;
            OZT_WireOwn wo = new OZT_WireOwn();
            int low;
            int high;
            own.GetNetworkID(low, high);
            wo.Low = low;
            wo.High = high;
            wo.Price = price;
            if (OZT_Deal.InTheWay(own) > 0)
                wo.Why = 2;
            if (OZT_Stack.Countable(own))
                wo.Units = units;
            res.Yours.Insert(wo);
        }
```

and in `Handle`, after `deal`:

```c
        // The same request, with nothing changed and the net not compared:
        // what the screen shows before Confirm is the server's own answer.
        if (op == OZT_Const.OP_QUOTE)
            return OZT_Deal.Run(player, json, false, ok, error);
```

- [x] **Step 4: The verb**

In `OZT_Verb.c`: the op `show` (`OZ_Rpc.Show(identity, OZT_Const.SHOW_PREFIX + id)`, in `KnownOps()` too); the detail of `open` ends with `yours=<n> (sells <a>, not empty <b>)`; `quote` goes through `Ask(player, OZT_Const.OP_QUOTE, json, ok, why)`, and with `twice=1` asks again in the same call and appends the second answer.

- [x] **Step 5: Boot and ask**

The stand cycle of phase 1, then the kit and three things more:

```json
{"op": "strip"}
{"op": "dress"}
{"op": "create", "class": "OZT_ProbeNote50", "into": "wallet_jacket", "cell": "free", "qty": "4", "as": "n50"}
{"op": "create", "class": "Canteen", "into": "pants", "cell": "free", "as": "canteen"}
{"op": "create", "class": "Ammo_762x39", "into": "pants", "cell": "free", "ammo": "12", "as": "pile"}
```

| args (`oz_trader`) | expected detail |
|---|---|
| `{"op": "open"}` | `open ok=yes trader=stand goods=10 ... balance=200 ... yours=7 (sells 4, not empty 3)`: the canteen, the pile, the bandage and the scope sell; the rifle (five parts), its magazine (thirty rounds) and the backpack (four things) are not empty |
| `{"op": "quote", "buy": "BandageDressing:1"}` | `quote ok=yes net=40 paid=40 received=0 balance=160 hands= left=0` |
| `{"op": "quote", "sell": "Canteen", "buy": "Canteen:1", "twice": "1"}` | `quote ok=yes net=60 ... balance=140 ... \| asked again at once: ok=no why=STR_OZT_NO_BUSY ...` |
| `{"op": "deal", "sell": "Ammo_762x39:5", "buy": "BandageDressing:1"}` | `deal ok=yes net=15 paid=15 received=0 balance=185 hands= left=0` (five rounds at 5, a bandage at 40) |
| `{"op": "show"}` | `show: sent 'trader:stand'` (nothing listens on the client yet) |

- [x] **Step 6: Record, commit**

The rows into `docs/measurements/2026-11-11/deals.md`; the design's §4.1 and §4.2 say what is built.

```bash
git -C /e/openzone/openzone-trader add -A
git -C /e/openzone/openzone-trader commit -m "trader: the wire for the screen: the player's own things priced by the server, a quote for clients, show" -m "Co-Authored-By: Claude Fable 5.2 <noreply@anthropic.com>"
```

---

## Execution record

Carried out on 2026-11-11 in the session that wrote it. Every row of Step 5 answered as printed: `yours=7 (sells 4, not empty 3)`, the two quotes, the refusal of the second quote in one call, the deal of 15, `show` sent. The verdict of the boot was a pass, and the log held no error line. Rows C.1-C.5 of `deals.md`.

## What the screen's plan starts from

Not tasks: what is settled, so that the next plan (the screen itself) can be written against it.

- The screen is `OZT_TradeMenu`, a `UIScriptedMenu` with the id 136, opened by a subscription to the core's `OZ_Show.OnShow` for `trader:<id>` and entered a tick later on the GUI queue, the way the research tree opens (`OZL_MissionGameplay.c`).
- It asks `open` with `OZ_Rpc.ServiceRequest("trader", "open", json)` and hears the answer through `OZ_ClientState.ServiceWatch()`; after every deal it asks `open` again.
- Every change of the cart asks `quote`, not sooner than a quarter of a second after the last change; the deal panel shows the quote's answer or its refusal.
- Layouts are generated from `ui/OpenZone_Trader/*.json` with the core's tokens (`[build] tokens` in `dayz-mcp.toml`); the generator has `preview`, `grid`, `list` and `raw` nodes for what the screen needs.
- Drag and drop, the previews, the shelf's local entities, the slot list and trying on: as `client-answers.md` says under "What it means for the screen".
- Strings go through `strings.csv` and `tools/stringtable.py`, Ukrainian and English; done means the gallery at 3840x1600 and 1920x1080 in both languages.
