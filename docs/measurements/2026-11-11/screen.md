# The trade screen on a live client, 2026-11-11

DayZ 1.30.164289, the diag build on the stand; `OpenZone_Core`, `OpenZone_Trader` and its
stand tools; the stand's own files (`examples/stand`). The client is the MCP's own, a window
of 1600x900, English; the last run a window of 1920x1080 with the Ukrainian strings (the
stand's `language="Russian"`: the game has no Ukrainian, so the mod's original stands in
every column but the English one). Everything below was done with real mouse input (`tools/click.py`,
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
| S.15 | the Ukrainian strings at 1920x1080: S.4's deal put together again, then S.8's | every title, label and line of the screen in Ukrainian and inside its box; the tiles 44 pixels a cell; "Бракує грошей" and "Бракує 500" in red | `quote` answered, then refused |
| S.16 | the cursor rested on the shelf's backpack (`tools/hover.py`), 1920x1080, English and Ukrainian | the card beside the tile, over the pictures of the tiles under it: the name, "about 1 kg" and "pristine" on the game's own chips, Price 400, the game's description, Size in inventory 4 x 5, Cargo 6 x 5, The trader pays for a pristine one 200 | nothing asked |
| S.17 | the cursor rested on a case of the player's that the trader does not take, by the right edge | the card on the left of the tile; The trader pays: a dash; "The trader does not take this" in red | nothing asked |
| S.18 | key P (and the button PRICE LIST) | the price list over the screen: the trader's name, tabs, ten lines with a picture, the name, what the trader sells it for and what he pays ("per unit" under rounds and nails, a dash where he does not take a thing), the note on wear with the server's own factors (85, 60, 35) | nothing asked: built from the answer of `open` |
| S.19 | a tab of the list, key Esc, key P twice, the button PRICE LIST, the list's CLOSE | the tab's three lines; Esc closes the list and leaves the screen; P opens it and closes it; the button opens it and CLOSE closes it | nothing asked |
| S.20 | the screen at 3840x1600, Ukrainian (the window moved off the desktop, the frame captured there) | the body in the middle of the wide screen, 65 pixels a cell, every tile on its grid lines, every text inside its box | `open` answered |
| S.21 | the shelf's helmet selected, the button TRY ON (the player wears no helmet) | the live view shows the character in the helmet, its face as in life; the slot tile Head shows the helmet, lit, with its price; the panel's head says "trying on: not bought yet"; the button reads TAKE OFF | nothing asked, nothing changed: the player's position on the server is the same half a minute later |
| S.22 | another thing of the shelf selected | the trying ends: the character as they are, the slot Head empty, the button TRY ON again (the backpack can be worn too) | nothing asked |
| S.23 | the helmet tried on again, a double click on its tile in the slot Head | the helmet is a line of BUY, 900; YOU PAY -900; the trying goes on; the card comes up for the tried tile as for any | `quote` answered |
| S.24 | key Esc while the helmet is tried on and in the cart | the screen closes; the world is as it was, no second character in it; no error line in either log | nothing asked |
| S.25 | the helmet tried on; the SERVER asked what stands at the player's place (`world_entities`, class `Man`, 10 m; it leaves out only a person with an identity) | the live view shows the character in the helmet | `listed 0 of 0 object(s) within 10m`, `players=1`: the server has no second character there |
| S.26 | phase 3a's build, Ukrainian, 1920x1080; the kit with no money; the screen opened | the rifle's slot tile reads 1 790: the rifle, its scope, its magazine and thirty rounds; the backpack's reads 200 and wears the amber corner | `open` answered: `Tree` 1790 for the rifle |
| S.27 | a double click on the rifle's slot tile | SELL holds the rifle, 1 790; ВИ ОТРИМАЄТЕ +1 790; the verdict reads "Залишаться у вас частини: 3", and the line under the buttons says the same in amber; Confirm lit | `quote` answered: it stands, three parts stay |
| S.28 | key F | "Угоду укладено"; the rifle is gone from its slot and from the character; the suppressor, the buttstock and the handguard are among the player's things, each with the red corner; money 1 790 | `deal: <uid> at 'stand' net -1790 (items): -AKM=1790(3 sold, 3 kept)` |
| S.29 | a double click on the backpack's slot tile (four things lie in it) | SELL holds the backpack, 200; the verdict, red: "Усередині лежать речі, яких немає в угоді: 4"; the panel's title stays УГОДА over a 0; Confirm unlit | `quote` refused: `STR_OZT_NO_NOT_EMPTY`, 4 |
| S.30 | the same in English, a fresh kit with the magazine's rounds worn | the rifle's tile and its line read 1 765; with the backpack beside it: "4 thing(s) lie inside and are not in the deal"; after R and the rifle alone: YOU GET +1 765, "Parts that stay with you: 3" in the verdict and under the buttons | `quote` answered both times |
| S.31 | tiles at the size things take in the inventory (the owner's word), Ukrainian, 1920x1080 | the rifle 8x3, the backpack 4x5, the helmet 4x3, the canteen 2x2, a round 1x1; the shelf scrolls where it did not | `open` answered |
| S.32 | nine categories of an admin's own (a goods file and a trader written for the look) | the tabs in three rows, ALL and three, four, two; every name whole; the grid below starts under the third row; the same strip over the player's things | `open` answered, nine categories |
| S.34 | a later build: the cursor rests on the pistol, which comes with its magazine | the card: ИЖ-70, the price 710, "У комплекті: Магазин ИЖ-70, 12 пат.", "Торговець дає за нову 325" | nothing asked |
| S.35 | a double click on the pistol, F | BUY holds the pistol at 710, ВИ ПЛАТИТЕ -710, "Усе вміщується у ваші речі"; the first F came before the quote and did nothing, the second made the deal | `deal ... +MakarovIJ70x1=710(1 parts)` |
| S.33 | prices written for the look: 9 995, 12 350 and 12 345, 125 000, 1 250 000 | on the shelf "9 995" on a tile one cell wide, "12.4к" twice (rounded up), "125к", "1.3М"; the player's bandage, which the trader pays 12 345 for, "12.3к" (rounded down) | nothing asked |
| S.36 | phase 3c's build with the panel art (`tools/panels.py`, the owner: "very much like the inventory of STALKER 2"), Ukrainian, 1920x1080 | six dark plates in worn steel whose outlines are not true rectangles, a title band on each, the grids and tabs inside; the palette of S.31 under them | `open` answered |
| S.37 | the shelf's helmet selected; a click on the empty NVG slot of its strip; a double click on the goggles | the strip under the character: the NVG and flashlight slots; the shelf filtered to what fits the slot, its head "Підбір: ..." in amber; then BUY holds the helmet with "+1", 2 400, and the quote stands | `quote` answered |
| S.38 | the goggles' empty battery slot in the strip, then a double click on the shelf's 9 V battery (the only tile left), F | "Підбір: Батарея на 9 В"; the line "+2", 2 420; "Угоду укладено", 17 580 | the dump: the helmet on the head, the goggles in its NVG slot, a battery in them; client and server the same (`task3c-fitted-from-screen`) |
| S.39 | a double click on the rifle's slot tile, then a click on its tile in SELL | SELL 1 790; the strip: the handguard, the buttstock and the suppressor red (not taken), the magazine 90, its thirty rounds 150, the scope 300 and the scope's battery slot, the bayonet's empty slot; "Залишаться у вас частини: 3" | `quote` answered |
| S.40 | a double click on the magazine in the strip, again, then on the rounds | the magazine kept with its rounds: 1 550, "частини: 4"; sold again: 1 790; the rounds kept alone: 1 640, "Залишаться у вас частини: 3. Повернуться набої: 30" | `quote` answered each time |
| S.41 | F | "Угоду укладено", 19 220 | the dump: the rifle and its magazine gone, the three parts in the jacket, the thirty rounds back as piles of 20 and 10; the same on both sides (`task3c-sold-keep-rounds`) |
| S.42 | a scope of the player's own in the hands (the bridge put it there), selected; a click on its empty battery slot, a double click on the battery | the strip shows the battery in the scope's slot with the accent edge, BUY a line of 20; F: done, 19 200 | the dump: the battery on the scope in the hands; the same on both sides (`task3c-onto-own-scope`) |
| S.43 | the shelf's helmet bought, the goggles fitted (+1, 2 400), a double click on the fitted goggles | the helmet back to 900. The first build lost the selection with the part's tile and the strip went empty; the next keeps the helmet selected and its strip open (the thing the strip shows is picked again when a part leaves it) | `quote` answered |
| S.44 | the change tab (the owner, 2026-11-11), Ukrainian; the kit of `deals.md` F; a click on the shelf's last tab | "Розмін" ends both strips; one click turns both sides to it: on the shelf the six notes at face value (5 to 500), among the player's things the two stacks, "300 · 3" and "2 000 · 4"; no note on the shelf's УСЕ nor among the things of other tabs; the price list has no such tab and no note among its rows | `open` answered, the category `@change` and six notes |
| S.45 | a double click on the player's 100s, ВІДДАТИ; a double click on the shelf's 20 | the dialog "СКІЛЬКИ ВІДДАТИ" at 1, total 100; SELL holds one 100; the dialog "СКІЛЬКИ ОТРИМАТИ" already at 5 (what the given note leaves room for), total 100 | `quote` answered |
| S.46 | ОТРИМАТИ, F | "РІВНИЙ ОБМІН" 0, Продаєте +100, Купуєте -100, Грошей після угоди 2 300; then "Угоду укладено" | `net 0 (items): -OZT_ProbeNote100x1=100 +OZT_ProbeNote20x5=100`; the 100s 3 to 2, five 20s in the jacket. `dump 51`: SAME (`change-100-into-five-20`) |
| S.47 | English: the tab, a 500 given, the shelf's 100, F | "Change" on both strips; "HOW MANY TO GIVE" / GIVE, "HOW MANY TO TAKE" / TAKE at 5; EVEN TRADE 0, Money after 2 300 | the 100s 3 to 8, the 500s 4 to 3. `dump 54`: SAME (`change-500-into-five-100-en`) |
| S.48 | Ukrainian again on a later build: a 500 given | the rest of the stack reads "1 500 · 3": a note's sum is its face value, so the client says it (before, a stack partly in the deal showed its count and no sum); the deal as S.47. `dump 61`: SAME (`change-500-into-five-100-ua`) | `quote` answered |

No `SCRIPT (E)` line and no `ERROR` line in the client's or the server's log of any of
these runs; the boots' verdicts were a pass.

Not covered by these runs: walking after the screen has closed (the calls that give the
controls back are the probe menu's, measured in phase 2a; the gamepad was left alone, the
owner was at the machine); real input at 3840x1600 (S.20 is a look, the window stood off the
desktop); the price list with more lines than it shows at once; a deal refused for want of
room, seen from the screen; a second client; the parts strip in English (its hints were
seen in English, S.43, the gestures in Ukrainian).

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
  `SetText` showed on the next text. With the colour set first the refusal is red at once
  (S.15).
- **What the game names stays in the client's language.** Slot names, item names and
  descriptions are the game's own strings: on the stand's Ukrainian check they are Russian,
  as they are for a Ukrainian player on a Russian client. Two slot names of that language
  are wider than a slot tile and are cut ("Верхняя одежда", "Нарукавная повязка").
- **The game's own inspect helpers fill any window that names its widgets their way.**
  `InspectMenuNew.UpdateItemInfoWeight(root, item)` and `UpdateItemInfoDamage(root, item)`
  are static; they set `ItemWeightWidget` and `ItemDamageWidget` under the root they are
  given and colour `ItemWeightWidgetBackground` and `ItemDamageWidgetBackground`. The card
  has those four and says weight and condition in the game's words and colours, in the
  client's language, for a real thing and for a shelf's stand-in alike.
- **Rows that size themselves to their text share a line in a wrap spacer**: two short
  rows of the card stood side by side. A row of the spacer's own width, the key at its
  left and the value at its right, stands alone.
- **A spacer takes the pointer unless told otherwise**; the card's are flagged
  `IGNOREPOINTER` when it is made, so that nothing of the card ever lies under the cursor.
- **A card's height is its content's two frames later.** The card is laid out off the
  screen (`SetPos(-4000, 0)`, shown), and two frames on its spacer's `GetScreenSize` is the
  content's height; then it is sized and put beside the tile. No flicker was seen.
- **A window moved off the desktop is still captured.** The client at 3840x1600 was moved
  to x = 3900 as soon as its window existed (`SetWindowPos`, no activation); the MCP's
  screenshot of it was a full, live frame, and the owner's desktop was never covered.
- **Trying on stands where phase 2a said it must.** The copy of the character is made at
  the player's own place, facing as the player faces, with its simulation on: its face was
  natural (S.21), the screen covers it, and the player did not move on the server while it
  stood there. It is deleted when the trying ends and when the screen closes (S.24).
- **The copy is nobody's but this client's** (the owner, 2026-11-11: the doll must be hidden
  from other players). It is made on the client with `ECE_LOCAL`: its network id is `00`
  (phase 2a, `client-results-run*.txt`), and the server, asked while it stood there, had no
  second character at the player's place (S.25). What the server does not have no other
  client is sent. A second client standing beside was not put there to look.
- **A trap of this machine, not of the screen.** The owner's taskbar stands at the left
  edge of the desktop, 80 pixels wide, and the client's window opens in the top left
  corner, under it. Real clicks at client x below 77 went to the taskbar: for an hour the
  "second press of a double click" was lost for every tile of a grid's first column, and
  only there. `WindowFromPoint` named the taskbar. The window is moved clear of it before
  any real click now (`SetWindowPos`, no activation).
