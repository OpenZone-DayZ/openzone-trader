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
```

Four mods ship: `OpenZone_Trader`, and three glues that each hard-require another mod:
`OpenZone_Trader_VPP` (the TRADERS pane; needs VPP Admin Tools and the core's OpenZone_VPP),
`OpenZone_Trader_Research` (the research gate; needs OpenZone Research) and
`OpenZone_Trader_TraderPlus` (the TraderPlus bank as money; needs TraderPlus). The stand tools
(`OpenZone_Trader_Bridge`, `OpenZone_TraderProbe`, `OpenZone_TraderProbe_Bridge`) never ship.

None of them has a Workshop item yet, so there is no `meta.cpp`: the first
`workshop_publish` creates the item and its id goes into `meta.cpp` here.

Publishing, in order, and only on the owner's word: `mod_build` (packs and signs; the
series key goes into the ignored `keys/` first) -> `.\package.ps1` (puts the rest in place;
`-Check` only reports) -> `workshop_publish("<Mod>")` from the `dayz` MCP.

The TRADERS pane was compiled and driven with VPP Admin Tools on 2026-11-12, see
`docs/measurements/2026-11-12/npc.md` N.20-N.22.
