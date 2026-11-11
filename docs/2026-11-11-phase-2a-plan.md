# OpenZone Trader, Phase 2a: The Client Facts — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Measure on a live client the four things the trade screen's design takes on trust (design §12 items 8, 9, 14 and the slot list of 12): drag and drop between widgets of our own menu, the character and item previews, the cost and the side effects of client-local preview entities, the character's slot list, and a client-local copy of the character for trying things on. Write the answers down, correct the design, and only then plan the screen (phase 2b).

**Architecture:** The stand probe gains a client half. `OpenZone_TraderProbe` gets a scripted menu of its own, `OZT_ProbeMenu`, with one hand-written layout that holds a region for each question. The stand drives it through the file channel phase 0 made (`control.txt` in the client's profile, read once a second): a line `ui <command>` is carried out by the menu, and its answer is written to `client_ui.ack` and, in full, to `client_results.log`. A real mouse drag, which no MCP tool does, comes from `tools/drag.py`. Nothing changes on the server, and nothing of the trader itself.

**Tech Stack:** Enforce Script on DayZ 1.30.164289 (diag build), client side (`5_Mission`); the `dayz` MCP server (`client_*`, `ui_tree`, `ui_cursor`, `client_shot`); `tools/drag.py` on the MCP server's own window helpers; git.

**Design:** `E:\openzone\docs\specs\2026-11-10-openzone-trader-design.md` — §3 (the trade screen), §12 items 8, 9, 12 and 14, §13 row 2.

## Global Constraints

- Game build: DayZ 1.30.164289, the diag executable; the stand is `E:\dayzmod\testserver`, port 2302, world instance 79 (`serverDZ_trader.cfg`). The client window is 1600x900: one layout unit is 900/1080 of a pixel there, and the layout below is written in units.
- The repository holds English and Ukrainian only. No Russian anywhere in it. Code comments in this phase are English.
- The probe is a stand tool: prefix `OZT_Probe`, never published, no string table (its few texts are English and are not the mod's).
- Do not invent DayZ API. Every engine call below was read in the 1.30.164289 scripts; the file and line stand beside each in "The engine's side, as read". If the compiler disagrees with a name, look it up with `knowledge_find` / `knowledge_show(body=True, layer='core')` before changing anything.
- What a step measures is written down as it came, also when it is not what this plan expected. An expectation here is what the scripts suggest, not a fact: a different answer is a finding, and it goes into the answers file in the same words as a matching one.
- A menu takes the game's focus and gives it back: `OnShow` and `OnHide` call their `super`; the input excludes and the restriction that `OnShow` adds are taken off in `OnHide`, and only if this menu added them. An unbalanced focus freezes the player's input.
- An `ItemPreviewWidget` or `PlayerPreviewWidget` draws only under the measured priority rule (skill `dayz-modding`, gui-layouts §13): its own priority and every ancestor's below 100, or its own exactly 201. The layout keeps everything below 100.
- Every client-local entity the probe makes is deleted by the probe: when its command is repeated, when the menu closes, and at mission end. Nothing the probe makes may reach the server; each task checks that with `world_entities`.
- Real input (`client_key`, `tools/drag.py`) takes the foreground and needs the machine's monitor switched on; so does a screenshot (measured 2026-11-11: with the monitor off the capture is a black frame). Ask the owner before the first run of a task if it is off.
- Lines the probe prints into the script log never contain the whole words `ERROR` or `WARNING`: the stand's verdict counts them. The reports go to files.
- Absolute paths appear only in `dayz-mcp.local.toml`, which is not committed, and in this plan's commands.
- Work is committed on `main` (the owner, 2026-11-11). Nothing is pushed and nothing goes to the Workshop without the owner's word. Commit author: `Alexandru Covalschi`, already set locally. A brief given to a subagent says "Do NOT push" in those words.
- Enforce Script rules that bite here: no ternary; no statement continued with a leading `+`; about ten parts to an expression at most; a compound boolean is computed into a local before it is stored in a field; one declaration of a name per function, loop counters included; `map.Set` to insert or update; a field is never named like a type; integer division truncates; `%` is for ints only; `Length()` and `Substring()` count bytes.
- Every stand run ends with the telling screenshots sent to the owner with `SendUserFile`.

## What is asked, and what an answer decides

| # | question (design §12) | decides |
|---|---|---|
| 8a | In a menu of ours, do `OnDrag`, `OnDragging`, `OnDraggingOver`, `OnDrop` and `OnDropReceived` fire between two of our widgets, on which handler, with which widgets and coordinates? | whether the screen's drag is the engine's (`DRAGGABLE` tiles and `OnDropReceived`) or drawn by us from the mouse state (§3.6) |
| 8b | Does a `PlayerPreviewWidget` show the real player in our menu, and can it be turned? | the middle column of the screen (§3.2) |
| 8c | Can an `ItemPreviewWidget` show an item turned a quarter, the way it lies in a cargo? | how a turned item is drawn in a grid (§3.3) |
| 9a | What do 60 and 150 client-local preview entities cost: the time to create them, the frames while their models load, the frame time afterwards? | whether the trader's shelf is drawn from a pool of local entities (§3.3) and how large the pool may be |
| 9b | What else do they do: are they seen in the vicinity, by the server, by other scripts? Where must they stand? | where the pool lives |
| 9c | Does a local copy assembled from parts (a rifle with optics and a magazine, a vest with pouches and a holstered pistol) show with its parts? | the picture of an item "as the deal would leave it" (§3.3) |
| 12 | What is the character's slot list on the client, with a slot a mod added: names, ids, display names, ghost icons? | the slot tiles (§3.1, §3.2) |
| 14 | Can a client-local copy of the character be dressed in copies of the player's gear and one more thing, and shown in a `PlayerPreviewWidget`? What does it cost, and what does it leave behind? | whether TRY ON is built as designed (§3.7) |

## The stand

Phase 1's stand, unchanged (the phase 1 plan, "The stand"): the diag server and client, CF, the core, the MCP bridge, `@OpenZone_Trader`, `@OpenZone_TraderProbe`, and the two stand-only bridges on the server. The probe's own wear slot (`Slot_OZT_ProbeWear`, in its `config.cpp` since phase 0) stands in for a slot another mod adds.

### The stand cycle

1. `mcp__dayz__client_stop()` and `mcp__dayz__server_stop()` if something still runs.
2. `mcp__dayz__project_open(path="E:/openzone/openzone-trader")`
3. `mcp__dayz__mod_lint()` — expected: no refusals, no warnings (the probe's layouts are linted too).
4. `mcp__dayz__mod_build()` then `mcp__dayz__job_wait(job_id=<id>, timeout=300)` — expected: four mods packed.
5. `rm -f /e/dayzmod/testserver/profiles/OpenZone_TraderProbe/results.log /e/dayzmod/testserver/clientprofile/OpenZone_TraderProbe/client_results.log /e/dayzmod/testserver/clientprofile/OpenZone_TraderProbe/client_ui.ack`
6. `mcp__dayz__server_start()` then `mcp__dayz__job_wait(job_id=<id>, timeout=420)` — expected: ready by the line `trader loaded`.
7. `mcp__dayz__log_verdict(source="server")` — expected: pass.
8. `mcp__dayz__world_ready()`
9. `mcp__dayz__client_start()` then `mcp__dayz__job_wait(job_id=<id>, timeout=240)` — expected: one player; the summary names the client's pid, which `tools/drag.py` needs.
10. `mcp__dayz__world_teleport(pos="4690 339 10192")`, then the kit: `mcp__dayz__world_exec(verb="ozt_probe", args={"op": "strip"})` and `{"op": "dress"}`.

The probe's menu is compiled on the server too (`5_Mission` is, and the probe guards nothing with `NO_GUI`), so a compile error shows in step 6 or 7 as `Can't compile`; read it with `mcp__dayz__log_tail(source="server", pattern="OZT_", n=600)`. What only the client can get wrong (a layout that does not load, a widget that is not found) is said in `client_ui.ack` and `client_results.log`.

### A command, its answer, a drag, a picture

- **A command** is one line written into `E:\dayzmod\testserver\clientprofile\OpenZone_TraderProbe\control.txt`: `ui <words> #<k>`, with a number `<k>` that was not used before in this boot (the client carries out a line it has not seen). With Bash: `printf 'ui open #1\n' > /e/dayzmod/testserver/clientprofile/OpenZone_TraderProbe/control.txt`.
- **Its answer** is in `client_ui.ack` beside it, one line that begins with the command, at most two seconds later: `sleep 2; cat /e/dayzmod/testserver/clientprofile/OpenZone_TraderProbe/client_ui.ack`. The full report of every command is appended to `client_results.log` in the same folder.
- **Where a widget is**: `mcp__dayz__ui_find(name="<widget name>")` gives its rectangle in pixels of the client area.
- **A drag**, from the middle of one rectangle to the middle of another:

```bash
/e/dayzmod/dayz-agentic-modding-mcp/.venv/Scripts/python.exe -I /e/openzone/openzone-trader/tools/drag.py <pid> <x1> <y1> <x2> <y2>
```

It takes the foreground, like `client_key`, and refuses when Windows will not give it.
- **A picture**: `mcp__dayz__client_shot()`; send the telling ones with `SendUserFile`.

`2026-11-11` below is the date of the stand run as `YYYY-MM-DD`. The answers go to `docs/measurements/2026-11-11/client-answers.md`.

## File Structure

```
openzone-trader/
  tools/drag.py                                              a real mouse drag in the client window (Task 1)
  OpenZone_TraderProbe/
    gui/layouts/ozt_probe_menu.layout                        the probe's screen: a region for each question (Task 1)
    gui/layouts/ozt_probe_tile.layout                        one item tile, made as often as needed (Task 4)
    gui/layouts/ozt_probe_slot.layout                        one slot tile: ghost icon and name (Task 5)
    scripts/3_Game/OpenZone_TraderProbe/OZT_ProbeLog.c       unchanged
    scripts/5_Mission/OpenZone_TraderProbe/
      OZT_ProbeClient.c                                      routes `ui ...` lines to the menu (Task 1)
      OZT_ProbeMission.c                                     makes the menu for its id (Task 1)
      OZT_ProbeMenu.c                                        the menu: opening, closing, the command table (Task 1)
      OZT_ProbeMenuDrag.c                                    question 8a (Task 2)
      OZT_ProbeMenuPreviews.c                                questions 8b, 8c (Task 3)
      OZT_ProbeMenuItems.c                                   questions 9a, 9b, 9c (Task 4)
      OZT_ProbeMenuSlots.c                                   question 12 (Task 5)
      OZT_ProbeMenuTryOn.c                                   question 14 (Task 6)
  docs/measurements/2026-11-11/client-answers.md                    the answers (Tasks 2-7)
```

Each question has a file of its own, a class of static functions that takes the menu, so that one question's code can be read without the others'.

---
## The engine's side, as read

Read in the 1.30.164289 scripts (`.dayz-mcp/corpus/scripts` of this repository) before any code was written:

- `UIScriptedMenu` (`3_game/tools/uiscriptedmenu.c`) is not a `ScriptedWidgetEventHandler`; it declares its own `bool OnDrag(Widget w, int x, int y)`, `OnDragging`, `OnDraggingOver`, `OnDrop` and `OnDropReceived(Widget w, int x, int y, Widget reciever)`, `OnMouseButtonDown(Widget w, int x, int y, int button)` and `OnMouseButtonUp` (:373-567). Its `OnShow` calls `LockControls()` (focus for mouse, keyboard and pad, and the cursor) and hooks the player's death to `Close()`; `OnHide` undoes both (:173-192).
- `WidgetFlags.DRAGGABLE` (`1_core/proto/enwidgets.c:84`), `Widget.SetFlags` / `ClearFlags` (:128, :131), `GetWidgetUnderCursor()` (:184), `GetMousePos(out int x, out int y)` and `GetMouseState(MouseState)` (`1_core/proto/ensystem.c:329`, :326).
- `WidgetEventHandler.RegisterOnDrag`, `RegisterOnDrop`, `RegisterOnDraggingOver`, `RegisterOnDropReceived(Widget w, Managed eventHandler, string functionName)` (`5_mission/gui/widgeteventhandler.c:91-117`): each calls `w.SetHandler(this)`; drag and drop pass `Param3<Widget, int, int>`, dragging over and drop received pass `Param4<Widget, int, int, Widget>` and are looked up by the receiver (:288-367).
- `PreviewWidget` (`3_game/gameplay.c:276`): `SetModelOrientation`, `GetModelOrientation`, `SetModelPosition`, `GetModelPosition`. `ItemPreviewWidget` (:286): `SetItem(EntityAI)`, `GetItem()`, `GetView()`, `SetView(int)`, `SetForceFlipEnable(bool)`, `SetForceFlip(bool)`. `PlayerPreviewWidget` (:305): `SetPlayer(DayZPlayer)`, `GetDummyPlayer()`, `UpdateItemInHands(EntityAI)`, `Refresh()`.
- The vanilla character panel (`5_mission/gui/inventorynew/playerpreview.c`): `SetPlayer(g_Game.GetPlayer())`, `SetModelPosition("0 0 0.605")`, the yaw in `orientation[1]` moved by the mouse's x while `GetMouseState(MouseState.LEFT) & 0x80000000`.
- A preview entity in vanilla: `CreateObjectEx(object, "0 0 0", ECE_LOCAL | ECE_CREATEPHYSICS | ECE_TRACE)`, then `DisableSimulation(true)` (`5_mission/gui/scriptconsoleitemstab.c:696-702`, a developer build's code); `ECE_LOCAL = 1073741824` (`3_game/ce/centraleconomy.c:25`). The vanilla cargo icon turns a picture with `SetForceFlipEnable(true)` and `SetForceFlip(flip)` (`5_mission/gui/inventorynew/containeditems/icon.c:1440`).
- The slot list (`5_mission/gui/inventorynew/inherited/playercontainer.c:58-105`): `ConfigGetTextArray("CfgVehicles SurvivorBase InventoryEquipment playerSlots", ...)`, `CfgSlots <name>` with `ghostIcon` and `name`, `InventorySlots.GetSlotIdFromString`, `GetSlotDisplayName`, `StaticGUIUtils.VerifyIconImageString`.
- `Weapon_Base.ShowMagazine()` and `ForceSyncSelectionState()` (`4_world/entities/firearms/weapon_base.c:2133`, :454); `GameInventory.CreateAttachment(string)`, `CreateAttachmentEx(string, int slotId)`, `FindAttachment(int slot)` (`3_game/systems/inventory/inventory.c:209-224`); `ClientData.m_PlayerBaseList` (`3_game/client/clientdata.c:7`); `CGame.GetTickTime()`, `ConfigGetChildrenCount`, `ConfigGetChildName`, `IsKindOf(string, string)` (`3_game/global/game.c`).

What our own skills already held on these subjects (opening a menu, the focus balance, the preview's priority rule, the window drag of the VPP pane) is in `dayz-modding`, `references/gui-layouts.md`; none of the eight questions was answered there.

---

### Task 1: The probe's screen, opened from the stand

**Files:**
- Create: `tools/drag.py`
- Create: `OpenZone_TraderProbe/gui/layouts/ozt_probe_menu.layout`, `ozt_probe_tile.layout`, `ozt_probe_slot.layout`
- Create: `OpenZone_TraderProbe/scripts/5_Mission/OpenZone_TraderProbe/OZT_ProbeMenu.c` (`OZT_ProbeQuestion`, `OZT_ProbeMenu`)
- Modify: `OZT_ProbeClient.c` (the `ui` lines, `Ack`, `Note`), `OZT_ProbeMission.c` (`CreateScriptedMenu`)

**Interfaces:**
- Produces: the menu id `OZT_ProbeMenu.ID` (137); `OZT_ProbeClient.Ack(line, answer)` and `Note(text)`; the base class of a question: `Attach(menu, root)`, `OnAttach()`, `string Command(array<string> words)` ("" when the word is not its own), `Frame(float dt)`, `Stop()`; the menu's `ResetFrames()` and the command `frames` / `frames reset`.

- [x] **Step 1: Write the three layouts and the menu, lint**

`mcp__dayz__mod_lint()` — expected: three layouts, no refusal, no warning.

- [x] **Step 2: The stand cycle; open the screen**

`printf 'ui open #1\n' > .../control.txt` — expected in `client_ui.ack`: `ui open #1 -> open: the screen is up`; `mcp__dayz__ui_menu()` names `OZT_ProbeMenu` with the cursor shown. A screenshot.

### Task 2: Drag and drop (question 8a)

**Files:**
- Create: `OZT_ProbeMenuDrag.c` (`OZT_ProbeQDrag`: commands `drag reset`, `drag report`, `drag returns 0|1`, `drag registered`)

- [x] **Step 1: A drag of the engine's tile onto a panel**

`ui drag reset`, then `tools/drag.py <pid> 248 132 433 158` (the middle of `TileEngine` to the middle of `DropRight`; `mcp__dayz__ui_find(name=...)` gives the rectangles), then `ui drag report`. Expected, from the scripts: `OnDrag` and `OnDrop` at least; whether `OnDraggingOver` and `OnDropReceived` reach a menu is the question.

- [x] **Step 2: The variants**

The preview widget itself onto the left panel; the tile onto nothing; the tile onto the other tile; the same with `drag returns 1`; the plain tile (`TilePoll`) by hand; `drag registered` and the drag again, onto the registered panel and onto the other one; a drag held for six seconds (`drag.py ... 12 6000`) with a screenshot while it is held; a tile of the items grid after `items draggable`, dragged out of its grid, with a screenshot while it is held.

### Task 3: The previews (questions 8b, 8c)

**Files:**
- Create: `OZT_ProbeMenuPreviews.c` (`OZT_ProbeQPreviews`: `player on|rot <deg>|refresh|state`, `item <slot>|flip 0|1|off|turn x y z|pos x y z`)

- [x] **Step 1:** `ui player on`, `ui player rot 90`, a screenshot; a real drag across `PlayerTurn`, `ui player state`.
- [x] **Step 2:** `ui item Shoulder`, `ui item flip 1`, a screenshot; `ui item flip 0`, a screenshot.

### Task 4: Client-local entities (question 9)

**Files:**
- Create: `OZT_ProbeMenuItems.c` (`OZT_ProbeQItems`: `items <n> near|far [still]`, `items where|draggable|keep|drop|clear`, `kit rifle [show|sync]`, `kit vest`, `kit clear`)

- [x] **Step 1:** `ui frames reset`, wait, `ui frames` (the idle frame time). `ui items 60 far`, wait, `ui frames`; a screenshot; `mcp__dayz__world_entities(pos="0 0 0", radius=50)` — expected: nothing of theirs.
- [x] **Step 2:** `ui items 150 far` twice, `ui items 300 far`, `ui items 60 far still`, each with `ui frames` after it.
- [x] **Step 3:** `ui items 60 near`, `ui items keep`, `ui close`; a screenshot of the world; the vanilla inventory (`mcp__dayz__client_key(name="tab")`), a screenshot of VICINITY; `ui open`, `ui items drop`.
- [x] **Step 4:** `ui kit rifle`, `ui kit rifle show`, `ui kit rifle sync`, `ui kit vest`, a screenshot of each.

### Task 5: The slots (question 12)

**Files:**
- Create: `OZT_ProbeMenuSlots.c` (`OZT_ProbeQSlots`: `slots`)

- [x] **Step 1:** `ui slots` — expected: the thirteen slots of the game and the probe's `Slot_OZT_ProbeWear`; `client_slots.txt`; a screenshot.

### Task 6: Trying on (question 14)

**Files:**
- Create: `OZT_ProbeMenuTryOn.c` (`OZT_ProbeQTryOn`: `tryon dummy <Class>`, `tryon copy <Class> near|far|above|under|dy<m> [still]`, `tryon where`, `tryon rot <deg>`, `tryon clear`)

- [x] **Step 1:** `ui tryon dummy Mich2001Helmet`, a screenshot.
- [x] **Step 2:** `ui tryon copy Mich2001Helmet far`, `ui frames`, a screenshot; `ui tryon copy AKM far`; `ui tryon clear` twice.
- [x] **Step 3:** the copy in other places, a screenshot of each: `near`, `above`, `above still`, `under still`, `dy-3 still`, `dy-10 still`, `dy-20 still`, `dy3 still`, `near still`, `dy-3`, `dy-40`, `near`.

### Task 7: The answers, the design, the skills

**Files:**
- Create: `docs/measurements/2026-11-11/client-answers.md`, `client-results-run*.txt`, `client-slots.txt`
- Modify: `E:\openzone\docs\specs\2026-11-10-openzone-trader-design.md` (§3.3, §3.4, §3.6, §3.7, §12 items 8, 9, 12, 14, §13 row 2, §14)
- Modify: `E:\dayzmod\dayz-agentic-modding-mcp\skills\dayz-modding\references\gui-layouts.md`; `E:\dayzmod\dayz-modding-skill\openzone\references\ui.md`

- [x] **Step 1:** Write `client-answers.md` from the three logs: one table of answers, the evidence under it, and what it means for the screen.
- [x] **Step 2:** Correct the design where an answer differs from what it assumed, and mark the four items of §12 as measured.
- [x] **Step 3:** The engine facts into `dayz-modding` with neutral names (`python -I openzone/scripts/check-generic.py` must print nothing), the pointer to the probe's screen into `openzone`. Stage this task's own lines only.
- [x] **Step 4:** Commit on `main`. Nothing is pushed.

---

## Execution record

Carried out on 2026-11-11, in the session that wrote it, on `main`. This plan was not written ahead and then followed: its questions and its method were fixed first, the probe was written against the scripts, and the tasks above were put down as the stand answered them. The code is in the files the tasks name; it is not repeated here. The answers are `docs/measurements/2026-11-11/client-answers.md`.

**Three boots.** The first ran Tasks 1-3, Task 4 Steps 1, 2 and 4, Task 5 and Task 6 Steps 1-2. The second added what the first asked for: `still`, `keep`, `drop` and `draggable` for the items, `show` and `sync` for the rifle, more places for the copy of the character. The third added a free height for that copy (`dy<m>`), to find where its face stops being pale.

**What the scripts did not tell, and the stand did.**

- A menu is told every drag and drop event of its widgets without any handler being set, the receiver included. The scripts show only the vanilla inventory's registry, which this makes unnecessary.
- A widget that a spacer lays out is not moved by the engine's drag; a free one is.
- A magazine made on a local weapon is not drawn until the weapon is told.
- The preview widget's own dummy cannot be dressed: what is created on it is not drawn.
- A local copy of the character has a pale face everywhere but at the player's feet with its simulation on. Twelve places and states were tried for that one finding; the cause was not found, only the rule.

**What differed from the plan's text.**

- The header first said the menu's code is hidden from the server's compile. It is not: the probe guards nothing, `5_Mission` is compiled on the server, and all three boots passed their verdicts with it.
- The first try of the vicinity screenshot showed the world, not the inventory: the key was sent while the probe's screen was still closing. Sent again, it opened.
- During the six-second drag the menu counted three more pairs of button events at another point of the screen. They were not the probe's; who clicked there was not found.
- The reference server's slot mods were not on the machine. The probe's own slot stands in for them, as the plan's stand says.

**Not covered.** Double click, the mouse wheel and hovering were not measured here (our skill holds them from other screens). A second client. A gamepad. The cost on a slower machine than this one: every time above is from one machine, and the frame times are those of an empty stand.

**The stand afterwards.** Stopped, the character stripped, nothing left lying. The three files of the trader are the examples.
