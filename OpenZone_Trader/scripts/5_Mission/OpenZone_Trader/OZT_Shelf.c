// What the trader's shelf is drawn from.
//
// A trader's goods do not exist until they are bought, and the game draws a
// thing from an entity and from nothing else (it has no item icons). So the
// shelf holds one client-local entity per class: made with ECE_LOCAL, it has no
// network id, and neither the server nor another client ever sees it (measured
// in phase 2a: docs/measurements/2026-11-11/client-answers.md). At a player's
// feet such an entity would lie in the world and in the vicinity panel, so
// they stand at the corner of the map, where the game's own script console
// keeps its preview, with their simulation off. The screen deletes them when
// it closes.
//
// LOOKS. Goods that come with parts, and goods the deal fits parts to, are drawn
// as they will come: one more local entity of the class with those parts made
// on it the way the try-on doll is dressed (CreateAttachmentEx on a local
// entity). One per look, keyed by what is on it; a look is never changed, a new
// key makes a new one, and all go when the screen closes.
class OZT_Shelf
{
    protected ref map<string, EntityAI> m_ByClass;
    protected ref map<string, EntityAI> m_Looks;
    protected ref map<string, ref array<EntityAI>> m_LookParts;

    void OZT_Shelf()
    {
        m_ByClass = new map<string, EntityAI>();
        m_Looks = new map<string, EntityAI>();
        m_LookParts = new map<string, ref array<EntityAI>>();
    }

    // The entity that stands for a class. Null: the game makes no such thing.
    EntityAI Of(string type)
    {
        EntityAI e;
        if (m_ByClass.Find(type, e))
            return e;
        e = EntityAI.Cast(GetGame().CreateObjectEx(type, "0 0 0", ECE_LOCAL));
        if (e)
            e.DisableSimulation(true);
        m_ByClass.Set(type, e);
        return e;
    }

    // The key of a look: the class, what it comes with, and every fit as
    // host:slot=class.
    protected string LookKey(string type, array<string> comes, array<ref OZT_CartFit> fits)
    {
        string key = type;
        if (comes)
        {
            for (int w = 0; w < comes.Count(); w++)
                key = key + "+" + comes[w];
        }
        if (fits)
        {
            for (int f = 0; f < fits.Count(); f++)
            {
                OZT_CartFit kf = fits[f];
                key = key + "|" + kf.On.ToString();
                key = key + ":" + kf.SlotName + "=" + kf.Type;
            }
        }
        return key;
    }

    // The goods as they will come: `type` with what it comes with (`comes`, the
    // price book's list) and the parts the deal fits to it (`fits`, a fit's
    // host an earlier fit or the goods themselves). The plain stand-in when
    // nothing is on it.
    EntityAI LookOf(string type, array<string> comes, array<ref OZT_CartFit> fits)
    {
        string key = LookKey(type, comes, fits);
        if (key == type)
            return Of(type);

        EntityAI look;
        if (m_Looks.Find(key, look) && look)
            return look;
        look = EntityAI.Cast(GetGame().CreateObjectEx(type, "0 0 0", ECE_LOCAL));
        if (!look)
            return Of(type);
        look.DisableSimulation(true);
        m_Looks.Set(key, look);

        if (comes)
        {
            array<string> taken = new array<string>();
            for (int k = 0; k < comes.Count(); k++)
            {
                string slot = OZT_Fit.SlotFor(type, comes[k], taken);
                if (slot == "")
                    continue;
                taken.Insert(slot);
                Attach(look, comes[k], slot);
            }
        }
        // Index for index with `fits`, so a fit finds its host by `On`.
        ref array<EntityAI> made = new array<EntityAI>();
        if (fits)
        {
            for (int i = 0; i < fits.Count(); i++)
            {
                OZT_CartFit fit = fits[i];
                EntityAI host = look;
                if (fit.On >= 0)
                {
                    host = null;
                    if (fit.On < made.Count())
                        host = made[fit.On];
                }
                EntityAI part = null;
                if (host)
                    part = Attach(host, fit.Type, fit.SlotName);
                made.Insert(part);
            }
        }
        m_LookParts.Set(key, made);
        return look;
    }

    // What a part fitted in the deal hangs on, inside the look: the look
    // itself for -1, else the part made for that fit. Null when there is none.
    EntityAI LookHost(string type, array<string> comes, array<ref OZT_CartFit> fits, int hostFit)
    {
        EntityAI look = LookOf(type, comes, fits);
        if (!look || hostFit < 0)
            return look;
        array<EntityAI> made;
        if (!m_LookParts.Find(LookKey(type, comes, fits), made) || !made)
            return null;
        if (hostFit >= made.Count())
            return null;
        return made[hostFit];
    }

    // A local twin of a real thing with its parts, `depth` levels down, the
    // caller's to delete. Recursion by locals only (dayz-modding: an object's
    // field handed to the next call from inside the loop overwrites this
    // call's parameters).
    static void CopyParts(EntityAI from, EntityAI to, int depth)
    {
        int n = from.GetInventory().AttachmentCount();
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = from.GetInventory().GetAttachmentFromIndex(i);
            if (!part || !part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            EntityAI copy = to.GetInventory().CreateAttachmentEx(part.GetType(), il.GetSlot());
            if (copy && depth > 0)
                CopyParts(part, copy, depth - 1);
        }
    }

    // A part made on `host` in the slot named: the slot by its id, or -- for a
    // weapon's magazine, which the game's own SpawnAttachedMagazine makes this
    // way -- wherever the inventory puts it.
    protected EntityAI Attach(EntityAI host, string type, string slotName)
    {
        EntityAI part = null;
        int slot = InventorySlots.GetSlotIdFromString(slotName);
        if (slot != InventorySlots.INVALID)
            part = host.GetInventory().CreateAttachmentEx(type, slot);
        if (!part)
            part = host.GetInventory().CreateAttachment(type);
        if (!part)
        {
            OZT_Log.Dbg("look: " + type + " was not made on " + host.GetType() + " (" + slotName + ")");
            return null;
        }
        ShowIfMagazine(host, part);
        return part;
    }

    // A weapon draws an attached magazine only once its own state machine
    // says it has one (Weapon_Base.SyncSelectionState); a local copy's never
    // runs, so the magazine is attached and hidden until this shows it.
    static void ShowIfMagazine(EntityAI host, EntityAI part)
    {
        Weapon_Base wpn = Weapon_Base.Cast(host);
        if (wpn && Magazine.Cast(part))
            wpn.ShowMagazine();
    }

    int Count()
    {
        return m_ByClass.Count();
    }

    void Clear()
    {
        for (int i = 0; i < m_ByClass.Count(); i++)
        {
            EntityAI e = m_ByClass.GetElement(i);
            if (e)
                GetGame().ObjectDelete(e);
        }
        m_ByClass.Clear();
        for (int j = 0; j < m_Looks.Count(); j++)
        {
            EntityAI l = m_Looks.GetElement(j);
            if (l)
                GetGame().ObjectDelete(l);
        }
        m_Looks.Clear();
        m_LookParts.Clear();
    }
}
