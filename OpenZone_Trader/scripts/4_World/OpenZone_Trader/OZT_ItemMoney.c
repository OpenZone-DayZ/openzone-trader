// Item money: banknotes are items, by class, each unit worth what the settings
// say.
//
// The owner's rules (2026-11-10). Paying: notes are taken from the smallest
// value upward until the sum is covered; then, from the smallest again, every
// taken note the sum does not need is put back; what is still overpaid comes
// back as change. Change and payouts are made of the largest notes that make
// the sum.

// One stack of notes the player carries.
class OZT_NoteStack
{
    EntityAI Item;
    int Value;      // of one unit
    int Units;      // in the stack now
    int Spend;      // units a payment takes from it
}

class OZT_ItemMoney : OZT_MoneyProvider
{
    override string Id()
    {
        return OZT_Const.PROVIDER_ITEMS;
    }

    override bool Physical()
    {
        return true;
    }

    override int Unit()
    {
        return OZT_Config.Get().Unit();
    }

    override int Balance(PlayerBase player)
    {
        array<ref OZT_NoteStack> stacks;
        Stacks(player, null, stacks);
        int sum = 0;
        for (int i = 0; i < stacks.Count(); i++)
            sum = sum + stacks[i].Value * stacks[i].Units;
        return sum;
    }

    // What one unit of this class is worth; 0 when it is not a note.
    static int ValueOf(string type)
    {
        array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
        for (int i = 0; i < notes.Count(); i++)
        {
            if (notes[i].ClassName == type)
                return notes[i].Value;
        }
        return 0;
    }

    // Every note stack the player carries, at any depth, the hands included:
    // the smallest value first, and within one value the smaller stack first,
    // so that paying empties whole stacks and frees their cells. `skip` are
    // items that leave in the same deal. A ruined note is not money.
    static void Stacks(PlayerBase player, array<EntityAI> skip, out array<ref OZT_NoteStack> stacks)
    {
        stacks = new array<ref OZT_NoteStack>();
        array<EntityAI> all = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        for (int i = 0; i < all.Count(); i++)
        {
            EntityAI e = all[i];
            if (!e || e == player)
                continue;
            if (skip && skip.Find(e) > -1)
                continue;
            int value = ValueOf(e.GetType());
            if (value <= 0 || e.IsRuined())
                continue;
            OZT_NoteStack s = new OZT_NoteStack();
            s.Item = e;
            s.Value = value;
            s.Units = OZT_Stack.Units(e);
            s.Spend = 0;
            if (s.Units <= 0)
                continue;
            int at = 0;
            while (at < stacks.Count() && Before(stacks[at], s))
                at++;
            stacks.InsertAt(s, at);
        }
    }

    private static bool Before(OZT_NoteStack a, OZT_NoteStack b)
    {
        if (a.Value != b.Value)
            return a.Value < b.Value;
        return a.Units <= b.Units;
    }

    // Marks in every stack how many units a payment of `amount` takes
    // (`Spend`) and returns the overpaid sum, which comes back as change; -1
    // when the notes do not cover the amount. `stacks` as Stacks() ordered them.
    static int Pay(array<ref OZT_NoteStack> stacks, int amount)
    {
        int sum = 0;
        for (int i = 0; i < stacks.Count(); i++)
        {
            OZT_NoteStack s = stacks[i];
            s.Spend = 0;
            if (sum >= amount)
                continue;
            int need = amount - sum;
            int units = need / s.Value;
            if (units * s.Value < need)
                units++;
            if (units > s.Units)
                units = s.Units;
            s.Spend = units;
            sum = sum + units * s.Value;
        }
        if (sum < amount)
            return -1;

        // From the smallest again: put back every note the sum does not need.
        for (int k = 0; k < stacks.Count(); k++)
        {
            OZT_NoteStack b = stacks[k];
            int excess = sum - amount;
            int back = excess / b.Value;
            if (back > b.Spend)
                back = b.Spend;
            b.Spend = b.Spend - back;
            sum = sum - back * b.Value;
        }
        return sum - amount;
    }

    // A sum in the largest notes: counts[i] units of Settings.Notes[i]. False
    // when the notes cannot make it (it is not a multiple of the smallest).
    static bool Change(int amount, out array<int> counts)
    {
        counts = new array<int>();
        array<ref OZT_Note> notes = OZT_Config.Get().Settings().Notes;
        for (int z = 0; z < notes.Count(); z++)
            counts.Insert(0);
        int rest = amount;
        for (int i = notes.Count() - 1; i >= 0; i--)
        {
            int n = rest / notes[i].Value;
            counts.Set(i, n);
            rest = rest - n * notes[i].Value;
        }
        return rest == 0;
    }
}
