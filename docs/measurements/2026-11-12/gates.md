# Gates: locks, access and discounts — 2026-11-12

Phase 5 (design §8.2). The base mod holds a registry of gate providers and knows no kind by
name; the identity kinds (`faction`, `post`, `leader`) are its own provider over the core's
identity contract; research is a glue pbo (`OpenZone_Trader_Research`); the stand brings a
provider of a made-up family (`stand`, in `OpenZone_Trader_Bridge`), the way a foreign mod
would, whose words the stand verb sets per player (`oz_trader {"op": "flag"}`).

DayZ 1.30.164289 diag; `OpenZone_Core` with the core's identity stub only (no faction mod);
the stand's files (`examples/stand`) with these gates:

- the helmet: 20 % off while `stand vip` holds;
- the goggles: locked unless `stand nvg`;
- the canteen: a percent `stand_pct rep` computes, capped at 30;
- weapons: 10 % off for the post `quartermaster` (the factions family);
- ammo: locked to the faction `duty` (the factions family);
- food: locked to `reputation 50`, a kind nobody answers;
- Ostap: `Access` only with `stand friend`.

Prices were read from `open` (`oz_trader {"op": "open"}`, the full answer in
`trader_last.json`) and from `quote`. The player's id is left out.

| # | done | seen | as expected? |
|---|---|---|---|
| G.1 | the boot | `gates=stand` on the ready line (the factions provider is registered and not there: the core's stub); the boot log: the weapons discount "not given", the ammo lock and the food lock "it passes" (no faction mod; a kind nobody answers) | yes |
| G.2 | `open` at the stall, no flags | helmet 900, goggles 0 (locked: not on the shelf), canteen 120, AKM 2 500 (the post discount not given), ammo 10 (the faction lock passes) | yes |
| G.3 | flags vip, rep 25 and nvg; `open` | helmet 720, goggles 1 500, canteen 90 | yes |
| G.4 | rep 50; `quote` the helmet and the canteen | 805: 720 and 85 -- the computed 50 % capped at 30, 84 rounded up to the note of 5 | yes |
| G.5 | nvg cleared; `quote` the goggles | `STR_OZT_NO_LOCKED` | yes |
| G.6 | `open` at Ostap without friend; then the trade action on him | `STR_OZT_NO_ACCESS`; the action ran and ended (the notice it sends was not seen: the client's frames were black, the owner's monitor off) | yes, the notice unseen |
| G.7 | friend set; `open` at Ostap | answered | yes |
| G.8 | the server's `GatesOff` ["stand"], reload | helmet 900 with vip, goggles 1 500 without nvg, canteen 120: the family's locks pass and its discounts are not given | yes |
| G.9 | the stall's own `GatesOn` ["stand"] over it, reload | at the stall: helmet 720, goggles locked, canteen 85; Ostap (the server's switch) opens without friend | yes |
| G.10 | the helmet's discount set to 60 % for a moment, reload | `WARNING: Goods: 'Mich2001Helmet' sells for 450 at 'stand' and costs 360 at 'stand': a money loop (... and a discount of 60 %)`, `problems=1`; put back | yes |
| G.11 | the research mod and the glue on the stand for one run; the scope locked to the node ts_eco_1, the tuna to a node the tree lacks | `gates=stand,research`; `WARNING: gates: 'TunaCan' Requires: 'research no_such_node' cannot be answered: the research tree has no node 'no_such_node'`; the scope 0 (the player's faction, the default owner clearsky, has not completed it) | yes |
| G.12 | the node completed for clearsky through the research mod's own admin section (`complete:clearsky:ts_eco_1`, by the stand verb) | the scope 600. That node stays completed in the shared stand's research state | yes |

No `SCRIPT (E)` line of ours and no `ERROR` line in either log. Every boot carries the
vanilla diag stack of `PluginItemDiagnostic` (`PluginConfigDebugProfile is not Registred`),
the same before these runs.

## What the code does, in short

- `OZT_GateProvider`: `Family`, `Kinds`, `Answers` (is the side that knows there today),
  `Holds`, `Knows` (at load), `Percent` (a computed discount), `Label`. `OZT_Gates.Provide`:
  the first of a family wins.
- A condition (`Kind`, `Arg` with alternatives joined by "|", `Not`) is answered by the
  provider of its kind if it answers today and its family is on at the trader (the trader's
  `GatesOn`, then its `GatesOff`, then the server's `GatesOff`). Otherwise a lock passes and
  a discount is not given.
- `OZT_Book.BuyFor`: what this player pays -- 0 when locked; the best single discount off
  one piece's price, rounded up to a note, times the count. Notes of the change tab have no
  gates. `open` hides locked goods (no buy price), a deal refuses them (`NO_LOCKED`); a part
  that comes with goods comes only when it is not locked for the player.
- `Access`: the trade action, `open` and a deal refuse with `NO_ACCESS`.
- The money-loop check counts the largest discount goods can get; a computed discount needs
  a `Max` (a missing one reads zero and gives nothing, said at load).
- At the ready tick every condition nobody answers, or whose family is not there, is said,
  and every one a provider does not know is a WARNING; the ready line names the families
  that answer.
