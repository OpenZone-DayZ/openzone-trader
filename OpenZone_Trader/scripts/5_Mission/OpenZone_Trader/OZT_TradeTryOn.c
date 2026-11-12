// Trying on (design §3.7): the character shown wearing goods that are not
// bought. Nothing reaches the server and nothing is created that anybody else
// could see: the picture is a client-local copy of the character, dressed in
// client-local copies of what the player wears and in the goods.
//
// Measured in phase 2a (docs/measurements/2026-11-11/client-answers.md): the
// preview widget's own dummy cannot be dressed; a copy of the player's class
// made with ECE_LOCAL can, in 5-10 ms, and costs nothing per frame. Its face
// is natural only while it stands at the player's place with its simulation
// on, so that is where it stands: the trade screen covers the world, and the
// copy is deleted when the trying ends. While it exists it is one more entry
// of the client's ClientData.m_PlayerBaseList.
//
// Several things at once (the owner, 2026-11-12): one copy, one tried thing per
// slot of the character; a thing tried on a slot replaces what the player
// wears there or what was tried there before, taking one off gives the slot
// back what the player wears, and the last one off ends the trying.

class OZT_TryOn
{
    protected PlayerPreviewWidget m_View;
    protected PlayerBase m_Copy;
    // What is tried on, and the slot of the character each took: one per slot.
    protected ref array<string> m_Types;
    protected ref array<int> m_Slots;

    void OZT_TryOn(PlayerPreviewWidget view)
    {
        m_View = view;
        m_Types = new array<string>();
        m_Slots = new array<int>();
    }

    bool IsOn()
    {
        return m_Copy != null;
    }

    int Count()
    {
        return m_Types.Count();
    }

    bool Has(string type)
    {
        return m_Types.Find(type) > -1;
    }

    // The class tried on a slot of the character; "" when none is.
    string TriedOn(int slot)
    {
        int at = m_Slots.Find(slot);
        if (at < 0)
            return "";
        return m_Types[at];
    }

    // The slot names a class may sit on, from its own config.
    protected static void SlotsOf(string type, array<string> names)
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

    // The first slot of the character the class names; INVALID when it names
    // none: such goods are not worn.
    static int SlotFor(string type)
    {
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        if (!me)
            return InventorySlots.INVALID;
        array<string> names = new array<string>();
        SlotsOf(type, names);
        for (int i = 0; i < names.Count(); i++)
        {
            int slot = InventorySlots.GetSlotIdFromString(names[i]);
            if (slot == InventorySlots.INVALID)
                continue;
            if (me.GetInventory().HasAttachmentSlot(slot))
                return slot;
        }
        return InventorySlots.INVALID;
    }

    // What `from` wears, made anew on `to`, each thing on the slot it stands
    // on; then the same for what hangs on each, `depth` levels down. Cargo is
    // not copied: it is not seen.
    protected void CopyGear(EntityAI from, EntityAI to, int depth)
    {
        int n = from.GetInventory().AttachmentCount();
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = from.GetInventory().GetAttachmentFromIndex(i);
            if (!part)
                continue;
            if (!part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            EntityAI twin = to.GetInventory().CreateAttachmentEx(part.GetType(), il.GetSlot());
            if (twin)
                OZT_Shelf.ShowIfMagazine(to, twin);
            if (twin && depth > 0)
                CopyGear(part, twin, depth - 1);
        }
    }

    // One more thing on the character, on its slot. False when the class names
    // no slot of the character or the game made no copy.
    bool Put(string type)
    {
        if (!m_View)
            return false;
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        int slot = SlotFor(type);
        if (!me || slot == InventorySlots.INVALID)
            return false;

        if (!m_Copy)
        {
            m_Copy = PlayerBase.Cast(GetGame().CreateObjectEx(me.GetType(), me.GetPosition(), ECE_LOCAL));
            if (!m_Copy)
                return false;
            m_Copy.SetOrientation(me.GetOrientation());
            CopyGear(me, m_Copy, 2);
            m_View.SetPlayer(m_Copy);
            m_View.SetModelPosition("0 0 0.605");
        }
        // What stands on the slot goes first: the player's own, or what was
        // tried there before.
        Forget(slot);
        EntityAI on = m_Copy.GetInventory().FindAttachment(slot);
        if (on)
            GetGame().ObjectDelete(on);
        EntityAI made = m_Copy.GetInventory().CreateAttachmentEx(type, slot);
        if (!made)
        {
            OZT_Log.Dbg("trying on: " + type + " was not made on the copy");
            Restore(me, slot);
            if (m_Types.Count() == 0)
                Drop();
            return false;
        }
        m_Types.Insert(type);
        m_Slots.Insert(slot);
        return true;
    }

    // One thing off: its slot gets back what the player wears there. The last
    // one off ends the trying.
    void TakeOff(string type)
    {
        int at = m_Types.Find(type);
        if (at < 0 || !m_Copy)
            return;
        int slot = m_Slots[at];
        m_Types.RemoveOrdered(at);
        m_Slots.RemoveOrdered(at);
        if (m_Types.Count() == 0)
        {
            Drop();
            return;
        }
        EntityAI on = m_Copy.GetInventory().FindAttachment(slot);
        if (on)
            GetGame().ObjectDelete(on);
        Restore(PlayerBase.Cast(GetGame().GetPlayer()), slot);
    }

    protected void Forget(int slot)
    {
        int at = m_Slots.Find(slot);
        if (at < 0)
            return;
        m_Slots.RemoveOrdered(at);
        m_Types.RemoveOrdered(at);
    }

    // What the player wears on a slot, made again on the copy with its parts.
    protected void Restore(PlayerBase me, int slot)
    {
        if (!me || !m_Copy)
            return;
        EntityAI worn = me.GetInventory().FindAttachment(slot);
        if (!worn)
            return;
        EntityAI twin = m_Copy.GetInventory().CreateAttachmentEx(worn.GetType(), slot);
        if (twin)
            CopyGear(worn, twin, 1);
    }

    // Everything off: back to the player as they are.
    void Drop()
    {
        m_Types.Clear();
        m_Slots.Clear();
        if (!m_Copy)
            return;
        // The widget must not be left holding what is deleted.
        DayZPlayer me = GetGame().GetPlayer();
        if (m_View)
        {
            m_View.SetPlayer(me);
            m_View.SetModelPosition("0 0 0.605");
            if (me)
                m_View.UpdateItemInHands(me.GetHumanInventory().GetEntityInHands());
        }
        GetGame().ObjectDelete(m_Copy);
        m_Copy = null;
    }
}
