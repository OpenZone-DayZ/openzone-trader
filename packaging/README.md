# packaging

What a published `@Mod` folder needs besides the packed pbo. The `@Mod` folders are build
output and are not in git; `package.ps1` assembles them after `mod_build`.

```
packaging/<Mod>/mod.cpp     the name, author, version and description DayZ shows in
                            the launcher and the mod list
packaging/<Mod>/meta.cpp    the Workshop item id, once the item exists -- without it the
                            next upload CREATES a second item instead of updating
packaging/<Mod>.workshop.bbcode
                            the Workshop listing, English and Ukrainian in one field
                            (at most 8000 BYTES: a Cyrillic letter is two)
packaging/<Mod>.workshop.png
                            the preview image the item's page shows, 1024x512
packaging/<Mod>.shot<N>.<en|uk>.jpg
                            the item's screenshot strip, in order, English and Ukrainian;
                            flat beside the preview, never in packaging/<Mod>/, which
                            package.ps1 copies into the mod
```

The strip goes up with `workshop_publish("<Mod>", previews=[...], content=False)`. Every
call ADDS pictures, so send each one once; taking one down is done on the item's page.

Four mods ship: `OpenZone_Trader`, and three glues that each hard-require another mod:
`OpenZone_Trader_VPP` (the TRADERS pane; needs VPP Admin Tools and the core's OpenZone_VPP),
`OpenZone_Trader_Research` (the research gate; needs OpenZone Research) and
`OpenZone_Trader_TraderPlus` (the TraderPlus bank as money; needs TraderPlus). The stand tools
(`OpenZone_Trader_Bridge`, `OpenZone_TraderProbe`, `OpenZone_TraderProbe_Bridge`) never ship.

All four have had Workshop items since 2026-11-13: OpenZone_Trader 3838849089,
OpenZone_Trader_VPP 3838849242, OpenZone_Trader_Research 3838849326,
OpenZone_Trader_TraderPlus 3838849413. Each id lives in `packaging/<Mod>/meta.cpp`;
`package.ps1` puts it into the `@Mod` folder, so the next upload updates the item.

Publishing, in order, and only on the owner's word: `mod_build` (packs and signs; the
series key goes into the ignored `keys/` first) -> `.\package.ps1` (puts the rest in place;
`-Check` only reports) -> `workshop_publish("<Mod>")` from the `dayz` MCP.

The TRADERS pane was compiled and driven with VPP Admin Tools on 2026-11-12, see
`docs/measurements/2026-11-12/npc.md` N.20-N.22.
