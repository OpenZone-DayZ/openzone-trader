// The words of the gates (design §8.2), as the files carry them. Who answers
// them is a provider (OZT_Gates, 4_World); a kind nobody answers is not an
// error, it passes.
//
// A condition: `Kind` (the admin's word: "faction", "post", "leader",
// "research", or any kind a glue brings), `Arg` (alternatives joined by "|"),
// `Not` 1 to turn it round. All conditions of one list must hold.
//
// A discount: a fixed `Percent` while every condition of `When` holds; or a
// percent the provider of `By` computes for the player, capped by `Max`. The
// best single discount counts. A discount is never given on the word of a side
// that is not there.
//
// Keys a nested entry lacks read zero (the loader leaves defaults unset): Not
// 0 is "as written", Percent 0 and Max 0 give nothing.

class OZT_Condition
{
    string Kind = "";
    string Arg = "";
    int Not = 0;

    OZT_Condition Copy()
    {
        OZT_Condition c = new OZT_Condition();
        c.Kind = Kind;
        c.Arg = Arg;
        c.Not = Not;
        return c;
    }

    // A list into another, the empty ones left out.
    static void CopyAll(array<ref OZT_Condition> from, array<ref OZT_Condition> into)
    {
        if (!from || !into)
            return;
        for (int i = 0; i < from.Count(); i++)
        {
            if (from[i] && from[i].Kind != "")
                into.Insert(from[i].Copy());
        }
    }
}

class OZT_Discount
{
    float Percent = 0;
    ref array<ref OZT_Condition> When;
    ref OZT_Condition By;
    float Max = 0;

    OZT_Discount Copy()
    {
        OZT_Discount c = new OZT_Discount();
        c.Percent = Percent;
        c.When = new array<ref OZT_Condition>();
        OZT_Condition.CopyAll(When, c.When);
        if (By && By.Kind != "")
            c.By = By.Copy();
        c.Max = Max;
        return c;
    }

    // The most this discount can ever give: what the loop check counts.
    float Most()
    {
        if (By)
            return Max;
        return Percent;
    }

    static void CopyAll(array<ref OZT_Discount> from, array<ref OZT_Discount> into)
    {
        if (!from || !into)
            return;
        for (int i = 0; i < from.Count(); i++)
        {
            if (from[i])
                into.Insert(from[i].Copy());
        }
    }
}
