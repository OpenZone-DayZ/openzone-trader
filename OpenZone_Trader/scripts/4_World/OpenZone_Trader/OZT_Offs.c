// What the game lets come off a thing (the owner, 2026-11-12: one rule for
// every part, not a fix for handguards).
//
// Taking a part off a thing, the game asks two questions: the part's own
// CanDetachAttachment and the thing's CanReleaseAttachment -- the pair every
// vanilla action that takes a part off asks (ActionDetach, ActionTakeItem),
// and the place a mod puts its own rule. Vanilla's: a rail handguard does not
// come off a weapon that carries a light, an Atlas bipod never comes off, a
// burlap sack is not taken off one's own head.
//
// A deal takes parts off in an order of its own, so the questions are asked of
// a local twin of the thing, a part at a time: a part the deal takes off first
// frees the one it held. A light sold in the same deal lets the rail come off;
// a light that stays does not. The server refuses a deal that would leave a
// part stuck (OZT_Deal.CheckOffs); the screen takes such a line out before
// that (OZT_TradeMenu.PruneOffs).
class OZT_Offs
{
    // How deep a twin copies parts: the rules read the thing's own parts and,
    // at most, what hangs on them.
    static const int DEPTH = 2;

    // Groups a part under the thing it hangs on: `hosts[i]` loses the parts in
    // `leaves[i]`. A part that is not in a slot of a thing asks nothing here.
    static void AddLeave(array<EntityAI> hosts, array<ref array<EntityAI>> leaves, EntityAI part)
    {
        if (!part)
            return;
        InventoryLocation il = new InventoryLocation();
        if (!part.GetInventory().GetCurrentInventoryLocation(il))
            return;
        if (il.GetType() != InventoryLocationType.ATTACHMENT)
            return;
        EntityAI host = il.GetParent();
        if (!host)
            return;
        int at = hosts.Find(host);
        if (at < 0)
        {
            hosts.Insert(host);
            leaves.Insert(new array<EntityAI>());
            at = hosts.Count() - 1;
        }
        if (leaves[at].Find(part) < 0)
            leaves[at].Insert(part);
    }

    // Every part in the thing's own slots.
    static void PartsOf(EntityAI host, array<EntityAI> into)
    {
        int n = host.GetInventory().AttachmentCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = host.GetInventory().GetAttachmentFromIndex(i);
            if (part)
                into.Insert(part);
        }
    }

    // `host` is a real thing; `leave` are parts in its own slots the deal takes
    // off and that have to come off; `may` (or null) are parts in its slots the
    // deal may take off on the way -- the other parts of a thing that is sold,
    // the trader's to take off in any order. Fills `stuck` with the parts of
    // `leave` that never come off.
    static void Stuck(EntityAI host, array<EntityAI> leave, array<EntityAI> may, array<EntityAI> stuck)
    {
        if (!host || !leave)
            return;

        // A slot the thing has locked holds its part, whatever the order: a
        // twin has no locks to ask.
        array<EntityAI> want = new array<EntityAI>();
        InventoryLocation il = new InventoryLocation();
        for (int l = 0; l < leave.Count(); l++)
        {
            EntityAI one = leave[l];
            if (!one || !one.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            if (host.GetInventory().GetSlotLock(il.GetSlot()))
                stuck.Insert(one);
            else
                want.Insert(one);
        }
        if (want.Count() == 0)
            return;

        // A character is asked as it is: no twin of one is made, and no rule
        // of the game orders the clothes on it among themselves.
        if (Man.Cast(host))
        {
            for (int w = 0; w < want.Count(); w++)
            {
                if (!ComesOff(host, want[w]))
                    stuck.Insert(want[w]);
            }
            return;
        }

        array<EntityAI> helpers = new array<EntityAI>();
        if (may)
        {
            for (int m = 0; m < may.Count(); m++)
            {
                if (may[m] && want.Find(may[m]) < 0)
                    helpers.Insert(may[m]);
            }
        }

        // A part at a time, until every wanted one is off or nothing more
        // comes off.
        array<EntityAI> freed = new array<EntityAI>();
        while (StillOn(want, freed) > 0)
        {
            EntityAI loose = NextOff(host, want, helpers, freed);
            if (!loose)
                break;
            freed.Insert(loose);
        }
        for (int s = 0; s < want.Count(); s++)
        {
            if (freed.Find(want[s]) < 0)
                stuck.Insert(want[s]);
        }
    }

    // The parts of a thing the player keeps that hold a part of `leave` on:
    // with them off as well, everything in `leave` comes off. Found one at a
    // time, each asked of the engine like the rest (a light holds a rifle's
    // rail handguard on). A deal takes them off into the bags, first (the
    // owner, 2026-11-12: a part is replaced in one gesture). False: nothing
    // frees it -- a part that never comes off, a locked slot, a character's
    // clothes.
    static bool Holders(EntityAI host, array<EntityAI> leave, array<EntityAI> holders)
    {
        array<EntityAI> all = new array<EntityAI>();
        for (int l = 0; l < leave.Count(); l++)
            all.Insert(leave[l]);
        array<EntityAI> stuck = new array<EntityAI>();
        Stuck(host, all, null, stuck);
        if (stuck.Count() == 0)
            return true;
        if (Man.Cast(host))
            return false;
        array<EntityAI> parts = new array<EntityAI>();
        PartsOf(host, parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            EntityAI candidate = parts[i];
            if (all.Find(candidate) > -1)
                continue;
            all.Insert(candidate);
            array<EntityAI> still = new array<EntityAI>();
            Stuck(host, all, null, still);
            bool helps = still.Count() < stuck.Count();
            if (still.Find(candidate) > -1)
                helps = false;
            if (!helps)
            {
                all.RemoveOrdered(all.Count() - 1);
                continue;
            }
            holders.Insert(candidate);
            if (still.Count() == 0)
                return true;
            stuck = still;
        }
        return false;
    }

    // The game's own pair. A magazine in a weapon is the one exception: the
    // game lets it out only of a weapon in the hands (the weapon manager's
    // question), which a twin never is and a deal does not need -- the trader
    // takes the magazine out with its own move.
    static bool ComesOff(EntityAI host, EntityAI part)
    {
        if (Magazine.Cast(part) && Weapon_Base.Cast(host))
            return true;
        if (!part.CanDetachAttachment(host))
            return false;
        return host.CanReleaseAttachment(part);
    }

    protected static int StillOn(array<EntityAI> want, array<EntityAI> freed)
    {
        int left = 0;
        for (int i = 0; i < want.Count(); i++)
        {
            if (freed.Find(want[i]) < 0)
                left++;
        }
        return left;
    }

    // A twin of the thing without the parts already off: the first wanted part
    // that would come off it now, else the first helper; null: none would.
    protected static EntityAI NextOff(EntityAI host, array<EntityAI> want, array<EntityAI> helpers, array<EntityAI> freed)
    {
        // Out of sight under the world on a client; at the thing on the server,
        // where its own twins of things stand (OZT_InvModel.FreshOf).
        vector at = "0 0 0";
        if (GetGame().IsDedicatedServer())
            at = host.GetPosition();
        EntityAI twin = EntityAI.Cast(GetGame().CreateObjectEx(host.GetType(), at, ECE_LOCAL | ECE_NOPERSISTENCY_WORLD));
        if (!twin)
            return null;
        twin.DisableSimulation(true);

        array<EntityAI> todo = new array<EntityAI>();
        array<int> slots = new array<int>();
        int n = host.GetInventory().AttachmentCount();
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = host.GetInventory().GetAttachmentFromIndex(i);
            if (!part || freed.Find(part) > -1)
                continue;
            if (!part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            todo.Insert(part);
            slots.Insert(il.GetSlot());
        }

        // Made in passes: a part whose own rule asks for another (a light for
        // a rail) is made after it, whatever order the thing lists them in.
        array<EntityAI> reals = new array<EntityAI>();
        array<EntityAI> copies = new array<EntityAI>();
        bool progress = true;
        while (progress)
        {
            progress = false;
            for (int t = 0; t < todo.Count(); t++)
            {
                EntityAI orig = todo[t];
                if (reals.Find(orig) > -1)
                    continue;
                int slot = slots[t];
                EntityAI copy = twin.GetInventory().CreateAttachmentEx(orig.GetType(), slot);
                if (!copy)
                    continue;
                int deeper = DEPTH - 1;
                CopyParts(orig, copy, deeper);
                reals.Insert(orig);
                copies.Insert(copy);
                progress = true;
            }
        }

        EntityAI found = FirstOff(twin, want, freed, reals, copies);
        if (!found)
            found = FirstOff(twin, helpers, freed, reals, copies);
        GetGame().ObjectDelete(twin);
        return found;
    }

    protected static EntityAI FirstOff(EntityAI twin, array<EntityAI> from, array<EntityAI> freed, array<EntityAI> reals, array<EntityAI> copies)
    {
        for (int i = 0; i < from.Count(); i++)
        {
            EntityAI p = from[i];
            if (freed.Find(p) > -1)
                continue;
            int at = reals.Find(p);
            if (at < 0)
                continue;
            EntityAI copy = copies[at];
            if (ComesOff(twin, copy))
                return p;
        }
        return null;
    }

    // Recursion by locals only (dayz-modding, enforce-script.md: an object's
    // field handed to the next call from inside the loop would overwrite this
    // call's parameters).
    protected static void CopyParts(EntityAI from, EntityAI to, int depth)
    {
        if (depth <= 0)
            return;
        int n = from.GetInventory().AttachmentCount();
        InventoryLocation il = new InventoryLocation();
        for (int i = 0; i < n; i++)
        {
            EntityAI part = from.GetInventory().GetAttachmentFromIndex(i);
            if (!part || !part.GetInventory().GetCurrentInventoryLocation(il))
                continue;
            int slot = il.GetSlot();
            EntityAI copy = to.GetInventory().CreateAttachmentEx(part.GetType(), slot);
            int deeper = depth - 1;
            if (copy && deeper > 0)
                CopyParts(part, copy, deeper);
        }
    }
}
