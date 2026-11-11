# OpenZone Trader

A trader for DayZ in the OpenZone family of mods. The trade screen puts the trader's stock
on the left, your own things on the right and your character with its slots in the middle;
a deal is one cart that buys and sells at once. Money is items of any class or a bank mod
behind a small glue pbo; goods and whole traders can be opened by faction, post and research.
Class prefix `OZT_`; runs with `OpenZone_Core` alone.

## Status: phase 1 done, nothing to play yet

The server half works for plain items: the three JSON files, the price book, item money,
the inventory model and the deal, driven on the test stand by scripted deals
([docs/measurements](docs/measurements)). There is no trade screen yet, so there is still
nothing to play; the screen is phase 2. The plans are in [docs](docs); the design note
lives beside the repositories of the series, in
`docs/specs/2026-11-10-openzone-trader-design.md`.

`OpenZone_Trader_Bridge`, `OpenZone_TraderProbe` and `OpenZone_TraderProbe_Bridge` are stand
tools. They are never published and no server should load them.

## Українською

Торговець для DayZ із серії модів OpenZone. На екрані обміну ліворуч товар торговця,
праворуч ваші речі, посередині ваш персонаж зі слотами; угода — один кошик, у якому
водночас купують і продають. Гроші — предмети будь-якого класу або банківський мод через
невелику склейку; товари й самих торговців можна відкривати за фракцією, посадою та
дослідженнями.

Стан: фазу 1 завершено. Серверна половина працює для простих речей: файли налаштувань,
прайс, гроші предметами, модель інвентаря та угода; на стенді її перевірено сценарними
угодами. Екрана обміну ще немає, тож грати ще нема в що.

## Licence

CC BY-NC-SA 4.0 with the additional permission in [NOTICE](NOTICE).
