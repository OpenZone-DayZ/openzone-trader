# Trader probe: answers of 2026-11-11

Stand: DayZ 1.30.164289 diag, `serverDZ_trader.cfg` (instance 79), mods: the MCP bridge and
the probe, one client. Evidence is `#<n>` in the `results-task<k>.txt` beside this file.

| §12 | question | answer | evidence | what the design does with it |
|---|---|---|---|---|
| 3 | Does a map built from item locations agree with the engine on every cell of every cargo the character carries? | Yes. Seven cargos, 149 cells, asked with a 1x1 probe entity and the occupancy test alone: no cell where the map and the engine differ, with the hands empty and with an item in them. | task3 #3, #4 | The plan's occupancy map is built from each item's own location and needs no per-cell question to the engine. |
| 3 | Which cargos does the character carry, at what depth, and which report their children accessible? | Worn or attached to a worn item (accessible): jacket 6x4, trousers 5x4, backpack 6x5, the plate carrier's pouches 5x3. Lying in a cargo (NOT accessible): the wallet 6x4 in the jacket, the wallet 6x4 and the protector case 3x4 in the backpack. The vest, the holster and the boots have no cargo. | task3 #3 | `AreChildrenAccessible` is false for every holder that lies in a cargo. Whether the server may still create there is measured in task 4. |
| 3 | Does the item in the hands change the picture (a reserved cell)? | No. An item CREATED in the hands reserves no cell: the same 149 cells, no mismatch. An item TAKEN into the hands is a different case, measured in task 6. | task3 #4 | Nothing for a created item. |
| 3 | Does the client hold the same inventory as the server (the diff)? | Yes, line for line, both times (23 and 24 items). Network ids, places, sizes and amounts are equal on both sides a few seconds after the kit is created. | `dump 1`, `dump 2`; `server-dump-scene.txt` against `client-dump-scene.txt` | The client's picture can be trusted as the base of the cart; the server still decides. |

Seen in passing while the kit was built (task3 #2):

- All 23 items of the kit were created in one script call, parts included. `CreateAttachment` and
  `Weapon_Base.SpawnAttachedMagazine` both worked on a pistol that already lay in the backpack's
  cargo, so an item lying in a cargo takes parts from the server.
- `SpawnAttachedMagazine(type, WeaponWithAmmoFlags.NONE)` attaches a full magazine.
