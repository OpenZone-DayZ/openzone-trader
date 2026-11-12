# Traders in the world: the trade action, the points in the sync packet, a trader's character — 2026-11-12

Phase 4, the first half: what stands at a point can be traded with by its own action,
and a point may be a trader's character (`Kind` "npc"). DayZ 1.30.164289, the diag build on
the stand; `OpenZone_Core`, `OpenZone_Trader` and its stand tools; the stand's files
(`examples/stand`): the stall of phase 1 and Ostap, a `SurvivorM_Boris` dressed in four
things, standing on the runway at 4687 339 10195.5, facing south, radius 3, gear and ammo,
`NoExchange` 1. The client is the MCP's own, 1920x1080, the Ukrainian strings. Real keys
through the MCP (`client_type`, `client_key`); the player moved by the bridge's teleport.
The player's id in the server's lines is left out.

The engine facts this rests on were read in the game's scripts before the build (the
design, §9.2 and §12 item 10): a character the server makes is never selected for a player,
so `PlayerBase.OnSelectPlayer` never runs for it -- its modifiers never tick, it has no
action manager, and the engine gets no callbacks to aim the infected's senses at it.

## What was done and what came of it

| # | done | seen | as expected? |
|---|---|---|---|
| N.1 | the first boot with an npc point | the character made 1.4 s after the stall (the first character of a boot loads what a character needs); `point 'npc' (SurvivorM_Boris) stands at <4687, 339, 10195.5>` | yes |
| N.2 | the same boot's outfit check | three of the four things reported "no class" with an empty name, the fourth worn. The file was read right (a log of the parsed items named all four); the check had recursed with `it.Inside` from inside a loop over its own array parameter, and the parameter came back pointing at the inner list. Isolated in one more boot, five variants (dayz-modding, enforce-script.md); the check walks a work list now and the ready line reads `problems=0` | no, then yes |
| N.3 | the client after the build with the points in the sync packet | `points: 1 from the sync packet` in its script log, then 2 with the npc point | yes |
| N.4 | `world_action OZT_ActionTrade` on the stall (the action through the engine's own gate on the server) | the action started and ended; the trade screen opened on the client | yes |
| N.5 | the crosshair on the stall's wall 6 m from its point, then on the open window | on the wall "[F] Торгувати"; through the broken window nothing (the ray reaches no part of the stall within reach) | yes |
| N.6 | key F on the wall | `trade action: show 'stand'`; the screen with ALL, gear, weapons, ammo and the change tab: the stall's entry has no `NoExchange`, which reads 0, so it changes notes | yes |
| N.7 | the crosshair on Ostap's head, key F twice (the first press did not reach the game) | "[F] Торгувати"; `trade action: show 'npc'`; the screen named Остап with ALL, gear and ammo, no weapons tab and no change tab; the player's rifle red (not taken there) | yes |
| N.8 | Tab beside Ostap, 1.7 m away | the vicinity panel empty | yes |
| N.9 | a plain `SurvivorM_Boris` made by the bridge beside him; the crosshair on each in turn | the plain one: "[F] Проверить пульс [УДЕРЖИВАЙТЕ]"; Ostap: "[F] Торгувати" only. A bandage in hand changes nothing on either (bandaging another needs them bleeding, so that is no test) | yes |
| N.10 | the server restarted | one `SurvivorM_Boris` within 1 m of the point and within 10 m: no second Ostap, and the two plain survivors the bridge had made were gone too -- a character with no player is not saved with the world | yes |
| N.11 | twenty more npc points (a test file, five by four, 3 m apart), `reload` | all twenty-one made in 42 ms, about 2 ms each; standing, dressed, facing north | yes |
| N.12 | the server's frames over 10 s (`oz_trader {"op": "fps"}`) | no player: 2 points 0.337 ms a frame, 22 points 0.377 ms. A player beside the grid: 2 points 0.521 ms, 22 points 0.792 ms -- about 13 µs a frame for each trader near a player, under 1 % of a 30 fps frame for twenty | yes |
| N.13 | `oz_trader {"op": "hurt"}`: a 7.62 bullet to the torso of a trader and of a plain survivor beside it | `trader SurvivorM_Boris 100 -> 100; control SurvivorM_Boris 100 -> 0` | yes |
| N.14 | an infected put 1.5 m from Ostap, the player 60 m away | nothing could be seen at that distance at night in rain; 13 s later it was still within 15 m of where it was put. Not measured: whether an infected goes for a trader. If one does, it does him no harm (N.13) | open |
| N.15 | the admin section `traders`, called by the stand verb as the pane will call it (`oz_trader {"op": "admin"}`, the stand's player as the admin): `place` with Id adm1, kind npc, class SurvivorF_Linda | a third point in the file at the player's position, facing where the player faced (30.8), every category of the price book; made at once; the answer the list of three, with `Standing` 1 | yes |
| N.16 | `outfit:adm1` | the point's outfit: the AKM on the shoulder with its handguard, magazine, buttstock, suppressor and scope, the backpack, jacket, boots, pants, the plate carrier with its holster and pouches, the bandage in the hands; Linda made again in all of it, the prompt "Торгувати" on her | yes |
| N.17 | `set:adm1` with a name, radius 4, one category, factors 1.1 and 0.9, `NoExchange` 1 | the file holds them | yes |
| N.18 | `place` with the taken Id adm1; `set:adm1` as an npc of a building's class; `move:nope` | `STR_OZT_ADMIN_BAD_ID`, `STR_OZT_ADMIN_BAD_CLASS`, `STR_OZT_ADMIN_NO_POINT`, nothing written | yes |
| N.19 | `remove:adm1`, then `respawn` | the point gone from the file; no SurvivorF_Linda within 3 m of where she stood | yes |
| N.20 | the pane TRADERS itself, with VPP Admin Tools (Workshop 1828439124, its build of 2026-10-29) subscribed later the same day, real mouse and keyboard | the glue compiles on the server and the client (no `SCRIPT (E)`); TRADERS stands after PLAYERS in the OpenZone window; the list shows both points from the section; a row fills the form (`npc`, Остап, KIND npc, SurvivorM_Boris, radius 3, factors 1 and 1, `gear,ammo`, CHANGES NOTES no, "at 4687 339 10195.5, facing 180, wears 4") | yes |
| N.21 | in the pane: PLACE HERE with id `probe1`, class Land_City_Stand_Grocery, KIND object; then MOVE HERE from 8 m away; then TURN AS I | a stall at the feet (4700 339 10186), three rows, "traders: place done"; the stall at the new feet (4706 339.02 10180), none left at the old place; "turn:probe1 done" (the character had not turned, so the yaw stayed -165.095) | yes |
| N.22 | KIND npc, class SurvivorF_Linda, categories `food,weapons`, CHANGES NOTES no, SAVE POINT; then OUTFIT FROM ME; then REMOVE twice | SurvivorF_Linda where the stall stood, the file: Kind npc, NoExchange 1, Categories [food, weapons]; "wears 7", the file holds the character's AKM with five parts, the bag and the jacket; the first REMOVE asks again ("press REMOVE again to remove probe1"), the second removes it: two rows, no one within 3 m | yes |

N.20-N.22 left two things for the rework of the VPP window the owner plans (2026-11-12): the
list draws the engine's red scroll bar and red selection, where the core's lists set the
style NoScrollBar; and after REMOVE the line under the form still describes the removed
point until another row is picked. RESPAWN ALL and REFRESH were not pressed in the pane
(the window was closed by then); the ops behind them are N.19 and the answer to every op.

Input on this stand died twice (2026-11-09, 2026-11-12) while a physical pad sat in the
first XInput slot (its packet counter climbing about 1000 a second at rest): keyboard,
mouse and the MCP's virtual pad all did nothing. Unplugging it brought input back. VPP's
admin toggle started off this time: Home answered "[VPPAT] Can't preform that action", End
and then Home opened the bar.

No `SCRIPT (E)` line and no `ERROR` line in either log of these runs. The verdict of the
boots read "fail" only for the stand's own count (`traders` 2 where the profile said 1),
corrected in `dayz-mcp.toml`.

## What the code does, in short

- `OZT_Points` makes an npc point's character with `CreateObject(class, pos, false, false,
  true)`, puts it at `Pos` facing `Yaw`, turns its damage off (`SetAllowDamage(false)`) and
  dresses it through the core's `OZ_LoadoutApply.To` from the point's `Outfit` (the shape
  of the core's loadout item; a key a nested entry lacks reads zero, so `Health01` 0 is
  left alone). It sends the core's sync packet again after every respawn.
- `OZT_Points.Fill` puts the points in the sync packet: "id|class|x|y|z|radius" joined by
  ";", in values under 900 characters, and their count. `OZT_ClientPoints` (3_Game) holds
  them on the client for the action's condition; the class is the one the thing really has.
- `OZT_ActionTrade` (interact, in `PlayerBase.SetActions`, `CCTCursor`, no lock on the
  target): the client offers it on a thing of a point's class within a metre of its
  position, the player within the point's radius; the server checks the things it made
  itself, runs again from the nearest point that reaches the player when the target is
  gone, refuses with a notice (`trader.open`, shown as a notification), and opens the
  screen with `OZ_Rpc.Show`.
- `OZT_Npc.c`: a trader's character takes no damage (`EEOnDamageCalculated`), is no target
  of the AI (`CanBeTargetedByAI`), cannot be restrained, fed or given drink, shows no
  inventory and lets nothing off it; and every action through `CCTMan` -- every action on
  another character -- refuses it. The server knows its characters by reference; a client
  by the points and by the character having no identity.
- `NoExchange` replaces `Exchange` (the change tab of the night before): a trader changes
  notes unless it is 1, because a key a trader's entry lacks reads zero.
