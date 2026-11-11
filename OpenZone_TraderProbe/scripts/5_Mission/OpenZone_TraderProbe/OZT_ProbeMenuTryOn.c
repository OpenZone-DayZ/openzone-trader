// Question 14: the character shown wearing a thing it does not have.
//
// Two ways are tried. The preview widget keeps a dummy of its own, a stand-in
// the engine dresses like the player (PlayerPreviewWidget.GetDummyPlayer): the
// thing can be put on that. Or the probe makes a client-local character of the
// player's class, dresses it in copies of what the player wears, adds the thing
// and gives it to the widget.
//
//   tryon dummy <Class>          the thing onto the widget's own dummy
//   tryon copy <Class> near|far|above|under [still]
//                                a local copy of the character with the thing;
//                                `near` stands at the player's feet, `far` at
//                                the corner of the map, `above` 300 m over the
//                                player, `under` 30 m below, `dy<m>` that many
//                                metres above (below when negative); `still`
//                                turns its simulation off
//   tryon where                  where the copy stands now
//   tryon rot <deg>              turn the picture
//   tryon clear                  back to the player as it is
class OZT_ProbeQTryOn : OZT_ProbeQuestion
{
    protected PlayerPreviewWidget m_View;
    protected PlayerBase m_Copy;

    override void OnAttach()
    {
        m_View = PlayerPreviewWidget.Cast(m_Root.FindAnyWidget("TryOnView"));
    }

    override void Stop()
    {
        DropCopy();
    }

    protected void DropCopy()
    {
        if (!m_Copy)
            return;
        // The widget must not be left holding what is deleted.
        if (m_View)
            m_View.SetPlayer(GetGame().GetPlayer());
        GetGame().ObjectDelete(m_Copy);
        m_Copy = null;
    }

    override string Command(array<string> words)
    {
        if (words[1] != "tryon")
            return "";
        if (!m_View)
            return "tryon: the layout has no TryOnView";
        string sub = Word(words, 2, "clear");
        if (sub == "dummy")
            return OnDummy(Word(words, 3, ""));
        if (sub == "copy")
            return OnCopy(Word(words, 3, ""), Word(words, 4, "far"), Word(words, 5, "") == "still");
        if (sub == "where")
        {
            if (!m_Copy)
                return "tryon where: no copy";
            return "tryon where: the copy stands at " + V(m_Copy.GetPosition());
        }
        if (sub == "rot")
        {
            m_View.SetModelOrientation(Vector(0, Word(words, 3, "0").ToFloat(), 0));
            return "tryon rot: orientation=" + V(m_View.GetModelOrientation());
        }
        DropCopy();
        m_View.SetPlayer(GetGame().GetPlayer());
        m_View.Refresh();
        DayZPlayer dummy = m_View.GetDummyPlayer();
        string s = "tryon clear: the player as it is";
        if (dummy)
            s = s + ", the dummy wears " + dummy.GetInventory().AttachmentCount().ToString();
        s = s + "; players the client lists=" + ClientData.m_PlayerBaseList.Count().ToString();
        return s;
    }

    // ---- the widget's own dummy ----------------------------------------------------

    protected string OnDummy(string type)
    {
        if (type == "")
            return "tryon dummy: a class name";
        DropCopy();
        DayZPlayer me = GetGame().GetPlayer();
        m_View.SetPlayer(me);
        m_View.SetModelPosition("0 0 0.605");
        DayZPlayer dummy = m_View.GetDummyPlayer();
        if (!dummy)
            return "tryon dummy: the widget has no dummy";
        int before = dummy.GetInventory().AttachmentCount();
        float t0 = GetGame().GetTickTime();
        string put = PutOn(dummy, type);
        float t1 = GetGame().GetTickTime();
        m_Menu.ResetFrames();

        string s = "tryon dummy " + type + ": " + put + " | dummy=" + dummy.GetType() + " id=" + dummy.GetNetworkIDString();
        s = s + " same as the player=" + OZT_ProbeLog.B(dummy == me);
        s = s + " worn before=" + before.ToString() + " after=" + dummy.GetInventory().AttachmentCount().ToString();
        s = s + " ms=" + OZT_ProbeLog.R1((t1 - t0) * 1000);
        return s;
    }

    // ---- a character of our own ------------------------------------------------------

    protected string OnCopy(string type, string place, bool still)
    {
        if (type == "")
            return "tryon copy: a class name, then near, far, above or under";
        DropCopy();
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        if (!me)
            return "tryon copy: no player";
        vector at = "0 0 0";
        if (place == "near")
            at = me.GetPosition();
        if (place == "above")
            at = me.GetPosition() + "0 300 0";
        if (place == "under")
            at = me.GetPosition() - "0 30 0";
        // dy<metres>: straight above or below the player, by that much.
        if (place.IndexOf("dy") == 0)
        {
            float dy = place.Substring(2, place.Length() - 2).ToFloat();
            at = me.GetPosition();
            at[1] = at[1] + dy;
        }
        int listed = ClientData.m_PlayerBaseList.Count();

        float t0 = GetGame().GetTickTime();
        m_Copy = PlayerBase.Cast(GetGame().CreateObjectEx(me.GetType(), at, ECE_LOCAL));
        if (!m_Copy)
            return "tryon copy: the game made no local " + me.GetType();
        if (still)
            m_Copy.DisableSimulation(true);
        int worn = me.GetInventory().AttachmentCount();
        int copied = CopyGear(me, m_Copy, 2);
        string put = PutOn(m_Copy, type);
        m_View.SetPlayer(m_Copy);
        m_View.SetModelPosition("0 0 0.605");
        float t1 = GetGame().GetTickTime();
        m_Menu.ResetFrames();

        string s = "tryon copy " + type + " " + place + ": " + put + " | copy=" + m_Copy.GetType() + " id=" + m_Copy.GetNetworkIDString();
        s = s + " still=" + OZT_ProbeLog.B(still);
        s = s + " at " + V(m_Copy.GetPosition()) + " | the player wears " + worn.ToString();
        s = s + ", things made on the copy (parts of parts counted)=" + copied.ToString();
        s = s + " | ms=" + OZT_ProbeLog.R1((t1 - t0) * 1000);
        s = s + " | players the client lists: " + listed.ToString() + " -> " + ClientData.m_PlayerBaseList.Count().ToString();
        return s;
    }

    // What `from` wears, made anew on `to`, each thing on the slot it stands
    // on; then the same for what hangs on each, `depth` levels down. Cargo is
    // not copied: it is not seen.
    protected int CopyGear(EntityAI from, EntityAI to, int depth)
    {
        int made = 0;
        int n = from.GetInventory().AttachmentCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = from.GetInventory().GetAttachmentFromIndex(i);
            if (!part)
                continue;
            InventoryLocation il = new InventoryLocation();
            if (!part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            EntityAI twin = to.GetInventory().CreateAttachmentEx(part.GetType(), il.GetSlot());
            if (!twin)
            {
                OZT_ProbeClient.Note("not copied: " + part.GetType() + " on slot " + InventorySlots.GetSlotName(il.GetSlot()) + " of " + to.GetType());
                continue;
            }
            made++;
            if (depth > 0)
                made = made + CopyGear(part, twin, depth - 1);
        }
        return made;
    }

    // The thing onto the first slot of the holder that it names; what stands
    // there goes first. Says what happened.
    protected string PutOn(EntityAI who, string type)
    {
        array<string> names = new array<string>();
        SlotsOf(type, names);
        for (int i = 0; i < names.Count(); i++)
        {
            int slot = InventorySlots.GetSlotIdFromString(names[i]);
            if (slot == InventorySlots.INVALID)
                continue;
            if (!who.GetInventory().HasAttachmentSlot(slot))
                continue;
            string took = "";
            EntityAI on = who.GetInventory().FindAttachment(slot);
            if (on)
            {
                took = ", in place of " + on.GetType();
                GetGame().ObjectDelete(on);
            }
            EntityAI made = who.GetInventory().CreateAttachmentEx(type, slot);
            if (made)
                return "put on slot " + names[i] + took;
            return "NOT put on slot " + names[i] + took;
        }
        EntityAI loose = who.GetInventory().CreateInInventory(type);
        if (loose)
            return "it names no slot of the character; made in the inventory";
        return "it names no slot of the character and was not made";
    }

    // The slot names a class may sit on, from its own config.
    protected void SlotsOf(string type, array<string> names)
    {
        array<string> roots = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
        for (int i = 0; i < roots.Count(); i++)
        {
            string path = roots[i] + " " + type + " inventorySlot";
            if (!GetGame().ConfigIsExisting(path))
                continue;
            if (GetGame().ConfigGetType(path) == CT_ARRAY)
            {
                GetGame().ConfigGetTextArray(path, names);
                return;
            }
            string one;
            GetGame().ConfigGetText(path, one);
            if (one != "")
                names.Insert(one);
            return;
        }
    }
}
