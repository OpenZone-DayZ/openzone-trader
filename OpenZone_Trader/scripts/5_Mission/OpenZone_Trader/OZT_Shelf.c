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
class OZT_Shelf
{
    protected ref map<string, EntityAI> m_ByClass;

    void OZT_Shelf()
    {
        m_ByClass = new map<string, EntityAI>();
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
    }
}
