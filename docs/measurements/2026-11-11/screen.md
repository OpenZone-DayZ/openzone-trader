# The trade screen on a live client, 2026-11-11

DayZ 1.30.164289, the diag build on the stand; `OpenZone_Core`, `OpenZone_Trader` and its
stand tools; the stand's own files (`examples/stand`). The client is the MCP's own, a window
of 1600x900, English. Everything below was done with real mouse input (`tools/click.py`,
`tools/drag.py`) and real keys, and read from screenshots, the client's script log (the
screen's `dbg:` lines) and the server's `deal:` lines. The player's id in those lines is
left out.

The kit: the probe's `strip` and `dress`, forty notes of 50 in the wallet (2 000), a canteen
and a pile of twelve 7.62x39 rounds in the pants. The screen is opened by the server:
`oz_trader {"op": "show"}`.

## What the screen did

| # | done | the screen | the server |
|---|---|---|---|
| S.1 | `show` | the screen opens over the world: the shelf (ten goods, tabs ALL and three categories), the player's things in the bags, the worn things on fifteen slot tiles (fourteen slots of the game's list and the hands), the live view of the character, money 2 000 | `open` answered |
| S.2 | double click on the shelf's canteen | a line in BUY, to pay 120; the deal panel: YOU PAY -120, money after 1 880, everything fits; Confirm lit | `quote` answered |
| S.3 | click on Confirm | "The deal is done", money 1 880, a second canteen among the player's things, the carts empty | `net 120 (items): +Canteenx1=120` |
| S.4 | the shelf's helmet dragged into BUY, the player's canteen dragged into SELL | BUY 900, SELL 60; YOU PAY -840, you sell +60, you buy -900, money after 1 160 | `quote` answered |
| S.5 | key F | the deal is done; the helmet is on the head (the slot tile Head and the live view), money 1 160 | `net 840 (items): -Canteenx1=60 +Mich2001Helmetx1=900` |
| S.6 | the worn helmet dragged from its slot tile into SELL, double click on the shelf's helmet | SELL 450, BUY 900; YOU PAY -450, money after 710, everything fits; the slot tile Head is empty while the helmet is in the cart | `quote` answered: the slot the same deal frees counts as free |
| S.7 | key F | the deal is done; a new helmet on the head, money 710 | `net 450 (items): -Mich2001Helmetx1=450 +Mich2001Helmetx1=900` |
| S.8 | double click on the shelf's rifle (2 500) with 2 000 | BUY 2 500; YOU PAY -2 500, "Short by 500" in red, "Not enough money"; Confirm grey | `quote` refused: `STR_OZT_NO_MONEY` |
| S.9 | key R | the carts empty, the things back in their grids, the deal panel at rest | nothing asked |
| S.10 | double click on the player's pile of twelve rounds | the quantity dialog: HOW MANY TO SELL, 12, "60 for all of it", total 60 | nothing asked: the whole stack's sum is `open`'s |
| S.11 | in that dialog: a click on the slider, minus three times, SELL | SELL holds one round, to receive 5; YOU GET +5; the pile among the player's things shows 11 | `quote` answered: one round of twelve brings its share, 5 |
| S.12 | double click on the shelf's nails (5 a unit) | the quantity dialog: HOW MANY TO BUY, 99 (a full stack), "5 each", total 495 | nothing asked: a bought total is the price of one times the count |
| S.13 | key F in the dialog, then the pile sold whole through its dialog (ALL, SELL), key F | BUY nails 99 for 495, SELL rounds 12 for 60; YOU PAY -435, money after 1 565; then the deal is done | `net 435 (items): -Ammo_762x39x12=60 +Nailx99=495` |
| S.14 | a tab of the shelf, a drag on the live view, key Esc | the tab's goods only; the character turns with the mouse; the screen closes, the cursor goes, the HUD is back | nothing asked |

No `SCRIPT (E)` line and no `ERROR` line in the client's or the server's log of any of
these runs; the boots' verdicts were a pass.

Not covered by these runs: walking after the screen has closed (the calls that give the
controls back are the probe menu's, measured in phase 2a; the gamepad was left alone, the
owner was at the machine); the screen in Ukrainian; the screen at 3840x1600 and at
1920x1080; a deal refused for want of room, seen from the screen; a second client.

## What was learned on the way

- **An item's picture is drawn over its low siblings.** With the tile's labels at the
  priorities their order gave them (3 to 7) and the picture at 1, the price was hidden
  behind the model wherever the model reached. At 610 the labels draw over it. The vanilla
  icon has its picture at 151 and its count at 606 (`gui/layouts/inventory_new/medium/icon.layout`).
- **A picture inside something high needs a window of its own.** The layout generator
  refuses an `ItemPreviewWidget` under a middle frame at 300 or more (the skill's
  measurement), so the quantity dialog, which has to cover the tiles' pictures and holds
  one itself, is a layout of its own on the workspace: root 952, frame 1, picture 201,
  the vanilla tooltip's shape. It hears its own events as the handler of its root
  (`OZT_QtyDialog : ScriptedWidgetEventHandler`) and is unlinked when the screen closes.
  Seen: its box covers the pictures under it.
- **The engine's own drag was built and worked, and is not used.** With the tile's root
  flagged `DRAGGABLE` three things had to hold, each found by a drag that did nothing: the
  picture on the tile must ignore the pointer (a press on a picture that takes the pointer
  lands on a widget that is not draggable); the ghost's picture too (it lay under the
  cursor and was named as the receiver of every drop); and `OnMouseButtonDown` must answer
  `false` for the tile (a press the menu takes for itself never becomes a drag). The engine
  also moves the dragged tile itself inside its scrolled grid. The screen now follows the
  mouse itself: the press, the button frame by frame, `GetWidgetUnderCursor()` at the
  release. Both were seen to work; the second asks nothing of the engine.
- **The engine tells a double click twice**: the second press comes as a press, and
  `OnDoubleClick` comes with it. The screen takes one gesture once.
- **A wrapped text takes the colour it has when its text is set**: `SetColor` after
  `SetText` showed on the next text. The colour is set first now (not seen on the stand
  yet: the fix is younger than the last run).
- **A trap of this machine, not of the screen.** The owner's taskbar stands at the left
  edge of the desktop, 80 pixels wide, and the client's window opens in the top left
  corner, under it. Real clicks at client x below 77 went to the taskbar: for an hour the
  "second press of a double click" was lost for every tile of a grid's first column, and
  only there. `WindowFromPoint` named the taskbar. The window is moved clear of it before
  any real click now (`SetWindowPos`, no activation).
