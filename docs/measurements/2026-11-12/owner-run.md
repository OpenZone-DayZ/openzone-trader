# The owner's run, 2026-11-12

The owner drove the trade screen on the stand (DayZDiag 1.30, `serverDZ_trader.cfg`,
window 1920x1080) and reported what did not work; each row is a report or a request, what
was found, what changed and how it was checked. "Owner" in the last column means the owner's
own hands in the game; "stand" means the stand's verbs (`oz_trader`, `ozt_probe`) and logs.

| | report or request | found | changed | checked |
|---|---|---|---|---|
| R.1 | a click on the AKM's magazine slot shows nothing to fit, though a magazine fits | the strip of parts is read recursively; the call for a fitted scope was handed `e.Type` and `e.Item` (fields of the entry) from inside the loop, and came back with the loop's own `hostType` set to the scope's class: every slot after the scope was the scope's, and the filter asked the scope for a magazine | the loop reads its host from locals and hands the recursion locals (`OZT_TradeParts.ReadGoods`) | owner: the drum magazine was picked and bought on a rifle with a scope (client log 08:53:27, deal 08:54:08) |
| R.2 | -- (the engine fact behind R.1) | the stand verb `oz_trader op=recur`: a string, an int and an object parameter each came back holding the inner call's value when the argument was a field (`k.Name`, `k.Num`, `k.Kid`): "top.0 top.1", "101 102", "root.0 root.1"; through a local copy "top top" | recorded in dayz-modding (enforce-script.md) | stand |
| R.3 | more parts for the AKM in the trader | -- | fourteen goods in `examples/stand`: PSO-1-1, Kashtan and Kobra optics, the suppressor, the bayonet, three stocks, three handguards, the universal light, the Palm and the drum magazines | stand: `goods=29 problems=0`, every class known |
| R.4 | goods with parts on them keep the bare picture | a tile is drawn from one stand-in per class | a look per set of parts (`OZT_Shelf.LookOf`): a local entity with what the goods come with and what the deal fits, on the shelf and in the basket | owner: "the parts are drawn"; the magazine was not (R.5) |
| R.5 | the magazine is not drawn on the picture | attached (no "look:" refusal in the client log), but a weapon shows a magazine only when its state machine says it has one (`SyncSelectionState` -> `ShowMagazine`), and a local copy's never runs | `ShowMagazine()` after a magazine is made on a look, and on the try-on copy | build ready, owner's look pending |
| R.6 | the AKM took a suppressor and a bayonet at once in the trader; the game allows one | the gates were asked of a bare stand-in, part by part, on the screen and on the server alike | each part asked of a local thing holding the parts before it, then made on it: on the server for goods (`FreshOf`) and for things carried (`TwinOf`, every part bought onto the same thing in one sequence), on the screen of the looks and of a twin; the slot filter shows only what the thing would take | stand: `quote buy=AKM:1+AK_Suppressor+AK_Bayonet` refused `changed`, the log "the gates refuse AK_Bayonet in the slot weaponBayonetAK of AKM"; `AKM:1+AK_Suppressor+PSO1Optic>Battery9V+Mag_AKM_30Rnd` quoted 4100 |
| R.7 | more money for testing | -- | a stack of 100 notes of 500 into the stand player's bag (`ozt_probe create` + `qty set=100`) | stand: qty 100/500 |
| R.8 | try several things on at once, and one button that takes them all off | trying on was one thing; selecting other goods ended it | one tried thing per slot of the character on one copy; TRY ON on goods already tried takes them off; TAKE ALL OFF in the character's header; selecting goods leaves the trying as it is | build ready, owner's look pending |
| R.9 | make a thing that stands in the world a trader, not only a new one of ours (later: Sidorovich on a Zone map) | -- | kind `map` (design §9.1) and the op `adopt`; the pane's AIMED THING BECOMES A TRADER sends what the camera points at | stand: `adopt` on our own stall answered `STR_OZT_ADMIN_TAKEN`, on a stock lying on the ground `STR_OZT_ADMIN_NO_THING`; a real map thing not yet (the props by the stand have no class, and their model is what the pane's ray reads) |
| R.10 | a wooden stock and handguard lie by the stall after buying an AK with parts | not from that deal: the deal of 09:09:40 logs the AKM with its six parts and no refused part, and the two lay at 4690.000 339.019 10192.000 since before the owner's first deal (a listing at 08:46). They are row D.28 of `2026-11-11/deals.md`: `deal sell=AKM refuse=2` sent the three kept parts of the sold rifle to the ground at the player's feet on purpose, and the stand's world saved them | -- (the owner picked them up) | stand: `world_entities` before and after; the deal log |
| R.11 | more clothing in the trader | -- | a category `clothes` ("Одяг") of 61 vanilla classes, every one checked against the mission's types.xml: tops, trousers, boots, helmets and hats, masks and glasses, gloves, vests, backpacks, armbands; both stand traders deal in it | stand: `goods=90 problems=0` |
| R.12 | the things the player wears cannot be taken off in the trade screen | a tile of a worn thing is the player's own tile, which went only into the deal; and the screen noticed a change of things by ids, counts and health, never by place, so a thing moved into a bag would not have shown | a tile of the player's own dropped on YOUR ITEMS comes off the character (or out of the hands) into the bags, dropped on the character it is put on in a free slot: the engine's predictive moves, as the vanilla inventory makes them; refused, said, when the thing is in the deal, when no bag has room or no slot is free; the screen's signature of the player's things now holds each thing's place | build ready, owner's hands pending |

## The TraderPlus bank (phase 6's glue)

TraderPlus (Workshop 2458896948, its build of 2026-11-09) loaded on the stand beside the
glue `OpenZone_Trader_TraderPlus`. Ostap was set to `"Provider": "traderplus"` (and, for the
run, a radius of 50 and the stand flag `friend`). The account file is TraderPlus's own,
`<profile>\TraderPlus\TraderPlusBankDatabase\Account_<uid>.json`.

| | asked | answered | the account file |
|---|---|---|---|
| B.1 | `open trader=npc` | `provider=traderplus unit=1 balance=0` (a new account starts at TraderPlus's DefaultStartCurrency, 0) | made by TraderPlus, MoneyAmount 0; set to 20000 by hand |
| B.2 | `quote trader=npc buy=TaloonBag_Blue:1` | `net=400 balance=19600` | 20000 |
| B.3 | `deal` of the same | `ok=yes net=400 balance=19600`; log "traderplus bank: 'Survivor' paid 400 (deal), 19600 left" | 19600 |
| B.4 | `deal trader=npc sell=BandageDressing` | `net=-20 received=20 balance=19620`; log "got 20 (deal), 19620 now" | 19620 |
| B.5 | the account set to 100, `deal buy=TaloonBag_Blue:1` | `ok=no why=STR_OZT_NO_MONEY` | 100, then set back to 20000 |

TraderPlus on the stand logs two errors of its own at the start ("TypeConverter not found
for type 'int'" from its logger module's networked variable, against the stand's CF) and 43
warnings "Scripted variables corrupted upon CivilianSedan_..." from cars the stand's world
saved without its CarLock. Neither is the glue's; the trader loads `problems=0`.

## Input on the stand

A physical pad in the first XInput slot (its packet counter climbing about 1000 a second at
rest) left the client deaf to the keyboard, the mouse and the MCP's virtual pad, as on
2026-11-09; unplugged, input came back (`npc.md`, after N.22).
