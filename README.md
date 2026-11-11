# OpenZone Trader

A trader for DayZ in the OpenZone family of mods. The trade screen puts the trader's stock
on the left, your own things on the right and your character with its slots in the middle;
a deal is one cart that buys and sells at once. Money is items of any class or a bank mod
behind a small glue pbo; goods and whole traders can be opened by faction, post and research.
Class prefix `OZT_`; runs with `OpenZone_Core` alone.

## Status: the trade screen works for plain items; no trader in the world yet

The server half works for plain items: the three JSON files, the price book, item money,
the inventory model and the deal, driven on the test stand by scripted deals
([docs/measurements](docs/measurements)). The trade screen works for plain items too: the
shelf, your own things and your character's slots, a deal put together by double click or
by dragging, how many of a stack, the server's verdict before you confirm, a card for the
thing under the cursor, the trader's price list, and goods tried on your character before
you buy them
([screen.md](docs/measurements/2026-11-11/screen.md)). It is opened by a stand command:
the trader's action in the world and items with parts come with their own phases, so
there is still nothing to play. The plans are in [docs](docs); the
design note lives beside the repositories of the series, in
`docs/specs/2026-11-10-openzone-trader-design.md`.

`OpenZone_Trader_Bridge`, `OpenZone_TraderProbe` and `OpenZone_TraderProbe_Bridge` are stand
tools. They are never published and no server should load them.

## Українською

Торговець для DayZ із серії модів OpenZone. На екрані обміну ліворуч товар торговця,
праворуч ваші речі, посередині ваш персонаж зі слотами; угода — один кошик, у якому
водночас купують і продають. Гроші — предмети будь-якого класу або банківський мод через
невелику склейку; товари й самих торговців можна відкривати за фракцією, посадою та
дослідженнями.

Стан: серверна половина працює для простих речей (файли налаштувань, прайс, гроші
предметами, модель інвентаря та угода), її перевірено на стенді сценарними угодами. Екран
обміну теж працює для простих речей: полиця торговця, ваші речі, слоти персонажа, угода
подвійним клацанням або перетягуванням, кількість зі стосу, вердикт сервера до
підтвердження, картка речі під курсором, прайс торговця і примірка товару на персонажі
до купівлі. Відкривається він поки що
командою стенда: дія торговця у світі та речі з частинами — попереду, тож грати ще нема
в що.

## Licence

CC BY-NC-SA 4.0 with the additional permission in [NOTICE](NOTICE).
