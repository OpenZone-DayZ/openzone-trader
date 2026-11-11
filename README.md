# OpenZone Trader

A trader for DayZ in the OpenZone family of mods. The trade screen puts the trader's stock
on the left, your own things on the right and your character with its slots in the middle;
a deal is one cart that buys and sells at once. Money is items of any class or a bank mod
behind a small glue pbo; goods and whole traders can be opened by faction, post and research.
Class prefix `OZT_`; runs with `OpenZone_Core` alone.

## Status: phase 0 done, nothing to play yet

The repository holds a measurement probe for the test stand and no mod. The probe's questions
(creating an item at a chosen cell, reusing the cell of a sold item, moving a kept part off a
sold weapon, banknotes in a wallet) are answered in
[docs/measurements/2026-11-11/answers.md](docs/measurements/2026-11-11/answers.md); the mod
itself starts in phase 1. The plan of phase 0 is
[docs/2026-11-11-phase-0-plan.md](docs/2026-11-11-phase-0-plan.md); the design note lives
beside the repositories of the series, in `docs/specs/2026-11-10-openzone-trader-design.md`.

`OpenZone_TraderProbe` and `OpenZone_TraderProbe_Bridge` are stand tools. They are never
published and no server should load them.

## Українською

Торговець для DayZ із серії модів OpenZone. На екрані обміну ліворуч товар торговця,
праворуч ваші речі, посередині ваш персонаж зі слотами; угода — один кошик, у якому
водночас купують і продають. Гроші — предмети будь-якого класу або банківський мод через
невелику склейку; товари й самих торговців можна відкривати за фракцією, посадою та
дослідженнями.

Стан: фазу 0 завершено. У репозиторії лише вимірювальний зонд для стенда; на його запитання
відповідає `docs/measurements/2026-11-11/answers.md`. Сам мод починається з фази 1, грати ще
нема в що.

## Licence

CC BY-NC-SA 4.0 with the additional permission in [NOTICE](NOTICE).
