// The trader pages' class fields in the window's live class search (the core's OZ_ClassPick,
// design docs/specs/2026-11-13-openzone-vpp-class-search-design.md in the series' docs): what
// each edit box takes, and whether it holds a list.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    override void ClassFields(OZ_ClassPick pick)
    {
        super.ClassFields(pick);
        // Goods: the line's class and what comes with it (classes, comma between).
        pick.Bind("TrdGdCls", OZ_ClassIndex.ITEM);
        pick.Bind("TrdGdWithNew", OZ_ClassIndex.ITEM, true);
        // Money: a banknote's class and the wallets filled first (classes, comma between).
        pick.Bind("TrdMnNoteCls", OZ_ClassIndex.ITEM);
        pick.Bind("TrdMnHolders", OZ_ClassIndex.ITEM, true);
        // Licences: the class the shelf draws for a licence.
        pick.Bind("TrdLcDefLook", OZ_ClassIndex.ITEM);
        // Points: what stands at a point, a building or a character.
        pick.Bind("TrdClass", OZ_ClassIndex.OBJECT);
        pick.Bind("TrdNewClass", OZ_ClassIndex.OBJECT);
    }
}

#endif
#endif
#endif
