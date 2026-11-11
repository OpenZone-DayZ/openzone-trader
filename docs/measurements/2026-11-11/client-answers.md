# Trader, phase 2a: the client facts of 2026-11-11

Stand: DayZ 1.30.164289 diag, `serverDZ_trader.cfg` (instance 79); CF, the core, the MCP
bridge, the trader, the probe; one client in a 1600x900 window. Three boots of the same day.
The probe's own screen (`OZT_ProbeMenu`, a `UIScriptedMenu` with a hand-written layout) was
driven through `control.txt`; every command and its answer is in `client-results-run1.txt`,
`client-results-run2.txt` and `client-results-run3.txt`, the slot list in `client-slots.txt`.
A real mouse drag came from `tools/drag.py`. Plan and record:
`docs/2026-11-11-phase-2a-plan.md`.

The questions are design §12 items 8, 9, 14 and the slot list of 12.

## The answers in one table

| # | question | answer |
|---|---|---|
| 8a | Do the engine's drag and drop events work between widgets of our own menu? | Yes, all of them, on the menu itself, with no handler set on any widget. |
| 8b | Does a `PlayerPreviewWidget` show the real player in our menu, turned by the mouse? | Yes. |
| 8c | Can an item's picture be turned a quarter? | Yes, with the forced flip the vanilla cargo icons use. |
| 9a | What do client-local preview entities cost? | 60 take 84 ms, 150 take 186 ms the first time and 15 ms again; one slow frame, then the frame time is what it was. |
| 9b | What else do they do? | Nothing on the server. Near the player they lie in the world and in the vicinity panel; far away they do neither. |
| 9c | Does a local copy put together from parts show its parts? | Yes, three levels deep; a weapon's magazine only after `ShowMagazine()`. |
| 12 | What is the slot list on the client, with a slot a mod added? | The vanilla recipe gives all fourteen, the mod's among them, each with its ghost icon. |
| 14 | Can the character be shown wearing a thing it does not have? | Yes, on a client-local copy of the character (5-10 ms, no frame cost). Not on the widget's own dummy. The copy's face is pale unless the copy stands, simulated, at the player's place. |

## 8a. Drag and drop in a menu of our own

A tile is made draggable with `SetFlags(WidgetFlags.DRAGGABLE)` at run time (the layout
property is `draggable 1`). Nothing else is set up: the menu is the handler of its own layout.

| what was dragged | onto | what the menu was told, in order |
|---|---|---|
| a panel with a picture inside (`TileEngine`) | a plain panel (`DropRight`) | `OnMouseButtonDown(w=TileEngine)`; `OnDrag(w=TileEngine, 248, 132)` once, at the point of the press; `OnDraggingOver` and `OnDragging` eleven times each (`reciever` is the widget under the cursor: the layout root while over panels that ignore the pointer, then `DropRight`); `OnMouseButtonUp(w=DropRight)`; `OnDropReceived(w=TileEngine, 433, 158, reciever=DropRight)`; `OnDrop(w=TileEngine, 433, 158, reciever=DropRight)` |
| an `ItemPreviewWidget` itself (`TilePreview`) | `DropLeft` | the same, with `w=TilePreview` |
| the tile | nothing (between the zones) | the same, with `reciever=OZT_ProbeMenuRoot` in `OnDropReceived` and `OnDrop`: never null |
| the tile | the other tile | `reciever=TilePreview` |
| a tile lying in a `WrapSpacerWidget` that clips its children | `DropRight` | the same events, `w` the tile, `reciever=DropRight` |

- Coordinates are pixels of the screen. One real drag of 12 steps gave 11 calls of each
  moving event.
- What the menu's events return changes nothing: the same calls with `false` and with `true`.
- The engine moves the dragged widget itself: while held, the tile was drawn with its top
  left corner at the cursor. After the drop it stood where it had started, by itself.
- A tile that a spacer lays out is not moved: it stayed in its cell for the whole drag, and
  nothing followed the cursor. The events came all the same. So in a grid the thing under the
  cursor has to be drawn by the screen.
- The vanilla way (`WidgetEventHandler.RegisterOnDrag` and its kin, which call
  `SetHandler` on the widget) takes those widgets' events away from the menu: the tile's
  `OnDrag` and `OnDrop` went to the registered functions (the drop without a receiver), and
  `OnDraggingOver` and `OnDropReceived` went to the registered functions only while the
  receiver was the registered panel. Over anything else, and for `OnDragging` always, the
  menu was still told. A drop on a panel that was not registered reached the menu's own
  `OnDropReceived`.
- A drag made by hand needs no flag: `OnMouseButtonDown(w=TilePoll)` on a plain panel, the
  button followed with `GetMouseState(MouseState.LEFT) & 0x80000000` (122 frames for the
  same drag), a ghost widget that ignores the pointer moved with `SetPos`, and at the
  release both `GetWidgetUnderCursor()` and the menu's own `OnMouseButtonUp(w=...)` named the
  widget under the cursor (`DropRight`, and the other tile when released over it).

One oddity, not the probe's: during a drag held for six seconds the menu counted three more
pairs of button events at another point of the screen (877, 46). Something else clicked there.

## 8b, 8c. The previews

- `PlayerPreviewWidget.SetPlayer(GetGame().GetPlayer())`, `SetModelPosition("0 0 0.605")`:
  the player stands in the widget with everything it wears, the rifle on its back and the
  pistol in the holster included.
- It is turned with `SetModelOrientation(Vector(0, yaw, 0))`. Followed from the mouse the
  vanilla way (the x of `GetMousePos` while the button is down), a drag of 120 px turned it by
  120 degrees. `GetModelOrientation()` does not give back what was set: after `(0, 90, 0)` it
  answered `-90 0 0`.
- The widget keeps a dummy of its own: `GetDummyPlayer()` answered a `SurvivorM_Boris` that
  is not the player and has no network id, and whose inventory counts no attachment although
  the picture shows them all.
- `ItemPreviewWidget.SetItem(entity)` with `SetView(entity.GetViewIndex())` draws a real
  thing of the player with what is on it. `SetForceFlipEnable(true)` with `SetForceFlip(true)`
  draws it turned a quarter and fitted to the widget; with `SetForceFlip(false)` it lies as
  it is.
- All of it under the priority rule the skill holds: every priority of the layout is below
  100.

## 9. Client-local entities

Made with `GetGame().CreateObjectEx(type, pos, ECE_LOCAL)`, one class each, classes taken
evenly from every public class under `Inventory_Base`, `Weapon_Base` and `Magazine_Base`
(finding them in the config took 12 ms). Each on a tile with an `ItemPreviewWidget`.

| run | made | time to create | time for the tiles | frames afterwards |
|---|---|---|---|---|
| idle, the probe's screen up | | | | 6.2 ms average, 8 ms the longest |
| 60, far | 60 of 60 | 84 ms | 3 ms | 6.4 ms average; one frame of 158 ms |
| 150, far | 150 of 150 | 186 ms | 6 ms | 6.6 ms average; one frame at the 300 ms cap |
| the same 150 again | 150 of 150 | 15 ms | 5 ms | 6.3 ms average; the longest 41 ms |
| 300, far | 300 of 300 | 201 ms | 13 ms | 7.7 ms average; one frame at the cap |
| 60, far, simulation off | 60 of 60 | 85 ms | 2 ms | 6.4 ms average; one frame of 157 ms |
| 60, near (classes loaded before) | 60 of 60 | 30 ms | 2 ms | |

- The cost is the first loading of a class and its model; made again, 150 take 15 ms.
- Drawing 150 pictures costs about a third of a millisecond a frame, 300 about one and a half.
- `DisableSimulation(true)` on each changed neither number.
- The server saw none of them: `world_entities` at their place listed nothing of theirs.
- Far ("0 0 0", where the game's own script console keeps its preview entity) they are in
  nobody's way. Near, at the player's feet, they lie in the world, the action cursor names
  them, and the vanilla inventory lists them in VICINITY, the clothes among them with their
  cargo grids open.
- Deleted with `ObjectDelete`, every one.

Put together from parts, all local:

- A rifle with `GetInventory().CreateAttachment` for the scope, the suppressor, the stock,
  the handguard and the magazine: five parts made, 1-2 ms. The picture showed four; the
  magazine was there and was not drawn. After `Weapon_Base.ShowMagazine()`, and equally after
  `ForceSyncSelectionState()`, it was.
- A vest with pouches and a holster, and a pistol with its magazine in the holster: all made,
  2 ms, and the picture showed the pouches, the holster and the pistol in it.

## 12. The slots

Read as the vanilla inventory reads them (`playercontainer.c`): the names of
`CfgVehicles SurvivorBase InventoryEquipment playerSlots`, then for each `CfgSlots <name>`
its `name`, its `ghostIcon`, `InventorySlots.GetSlotIdFromString(name)` and
`InventorySlots.GetSlotDisplayName(id)`.

Fourteen slots, all in `CfgSlots`, all with an icon: the thirteen of the game and
`Slot_OZT_ProbeWear`, the slot the probe's own config adds, last in the list. The icon loads
into an `ImageWidget` with `LoadImageFile(0, StaticGUIUtils.VerifyIconImageString(...))`.
Display names are string-table keys for the game's slots and plain text for the mod's.
`GetInventory().FindAttachment(id)` on the client named what the player wears on each. The
list is `client-slots.txt`.

Not measured: the reference server's own slot mods. They were not on this machine; the
probe's slot stands in for them.

## 14. Trying on

- On the widget's own dummy: `CreateAttachmentEx` on the dummy made the helmet (its
  inventory then counted one attachment) and the picture did not change.
- On a copy of our own: `CreateObjectEx(<the player's class>, pos, ECE_LOCAL)`, every thing
  the player wears made anew on the same slot with `CreateAttachmentEx`, the parts of each
  two levels down, then the tried thing on the first slot it names (what stood there deleted
  first), then `SetPlayer(copy)`. Fourteen things on the copy in 5-10 ms; the picture showed
  the character in a helmet the player does not have; a rifle took the place of the copy's
  rifle. The frame time did not move.
- While it exists the copy is one more entry of `ClientData.m_PlayerBaseList` (1 became 2);
  a frame after its deletion the list was back at 1. Anything on the client that walks that
  list meets it.
- It stays where it is put: 300 m above the player it had not fallen four seconds later,
  with its simulation on.
- Its face: natural when the copy stood at the player's feet with its simulation on, twice.
  Pale grey in every other case tried: at the corner of the map; 300 m above; 3, 10, 20, 30
  and 40 m below; 3 m above; and at the player's feet with its simulation off. Waiting forty
  seconds changed nothing. The clothes looked the same everywhere.
- At the player's feet the copy is in the player's own view: with a see-through screen its
  body showed at the edges of the picture of the world.

## What it means for the screen

1. The engine's drag and drop is used as it is: tiles are draggable, the menu takes
   `OnDrag`, `OnDraggingOver`, `OnDropReceived` and `OnDrop`, and a drop whose receiver is not
   a place of ours is a drop on nothing. No registry, no handler on a widget.
2. What is seen under the cursor is drawn by the screen: a tile in a grid does not move. One
   ghost widget at the top of the layout, shown in `OnDrag`, moved in `OnDragging`, hidden in
   `OnDrop`.
3. The trader's shelf is drawn from client-local entities kept far from every player, made
   for the category in view, kept while the screen is open and deleted when it closes (the
   owner, 2026-11-11: "we keep them, that is fine"). A pool of 150 is a fifth of a second
   once.
4. A picture of an item "as the deal would leave it" is a local copy with local parts; a
   weapon with a magazine is told `ShowMagazine()`.
5. The slot tiles come from the vanilla recipe, so another mod's slot is there by itself.
6. TRY ON works on a copy. Where the copy stands is still to settle, for its face: at the
   player's place it looks right and is in the player's world; anywhere else it is out of the
   way and pale.
