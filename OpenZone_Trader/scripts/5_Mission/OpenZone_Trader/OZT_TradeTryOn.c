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

class OZT_TryOn
{
    protected PlayerPreviewWidget m_View;
    protected PlayerBase m_Copy;
    protected string m_Type;
    protected int m_Slot;

    void OZT_TryOn(PlayerPreviewWidget view)
    {
        m_View = view;
        m_Slot = InventorySlots.INVALID;
    }

    bool IsOn()
    {
        return m_Copy != null;
    }

    // The class that is tried on, and the slot of the character it took.
    string Tried()
    {
        return m_Type;
    }

    int SlotId()
    {
        return m_Slot;
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
            if (twin && depth > 0)
                CopyGear(part, twin, depth - 1);
        }
    }

    // The character in the goods. False when the class names no slot of the
    // character or the game made no copy.
    bool Put(string type)
    {
        Drop();
        if (!m_View)
            return false;
        PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
        int slot = SlotFor(type);
        if (!me || slot == InventorySlots.INVALID)
            return false;

        m_Copy = PlayerBase.Cast(GetGame().CreateObjectEx(me.GetType(), me.GetPosition(), ECE_LOCAL));
        if (!m_Copy)
            return false;
        m_Copy.SetOrientation(me.GetOrientation());
        CopyGear(me, m_Copy, 2);
        // What stands on the slot goes first.
        EntityAI on = m_Copy.GetInventory().FindAttachment(slot);
        if (on)
            GetGame().ObjectDelete(on);
        EntityAI made = m_Copy.GetInventory().CreateAttachmentEx(type, slot);
        if (!made)
        {
            OZT_Log.Dbg("trying on: " + type + " was not made on the copy");
            Drop();
            return false;
        }
        m_Type = type;
        m_Slot = slot;
        m_View.SetPlayer(m_Copy);
        m_View.SetModelPosition("0 0 0.605");
        return true;
    }

    // Back to the player as they are.
    void Drop()
    {
        m_Type = "";
        m_Slot = InventorySlots.INVALID;
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
