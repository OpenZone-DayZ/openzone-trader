// Which slot a part takes on a thing, read from config: nothing here needs an
// entity, so it can be asked of goods that do not exist yet.
//
// A thing names the slots it has in `attachments[]`; a part names the slots it
// may sit in in `inventorySlot` (one name, or a list). A magazine is the one
// exception: a weapon names the magazines it takes in `magazines[]`, and they
// all sit in the slot `magazine`.
//
// Names are compared without regard to case: configs are not consistent in it.
//
// This answers "is there a slot". Whether the thing's own script lets the part
// in is the engine's to say, and a deal asks it too (OZT_Deal.CheckFits).
class OZT_Fit
{
    static const string MAGAZINE_SLOT = "magazine";

    // What config said of each class, kept: a slot's filter asks every record of
    // the shelf, and each answer was some ten config reads, the same host's slot
    // list among them every time (load test, 2026-11-13: 3404 records, twice per
    // click). Config does not change while a mission runs.
    protected static ref map<string, ref array<string>> s_Host = new map<string, ref array<string>>();
    protected static ref map<string, ref array<string>> s_Part = new map<string, ref array<string>>();
    protected static ref map<string, int> s_Kind = new map<string, int>();
    protected static const int KIND_OTHER = 0;
    protected static const int KIND_MAGAZINE = 1;
    protected static const int KIND_WEAPON = 2;

    static void Reset()
    {
        s_Host.Clear();
        s_Part.Clear();
        s_Kind.Clear();
        s_Mags.Clear();
    }

    protected static array<string> Kept(map<string, ref array<string>> kept, string type, string key)
    {
        array<string> names;
        if (kept.Find(type, names))
            return names;
        names = new array<string>();
        ReadNames(type, key, names);
        kept.Set(type, names);
        return names;
    }

    protected static int KindOf(string type)
    {
        int kind;
        if (s_Kind.Find(type, kind))
            return kind;
        kind = KIND_OTHER;
        if (GetGame().ConfigIsExisting("CfgWeapons " + type))
            kind = KIND_WEAPON;
        else if (GetGame().ConfigIsExisting("CfgMagazines " + type) && !GetGame().IsKindOf(type, "Ammunition_Base"))
            kind = KIND_MAGAZINE;
        s_Kind.Set(type, kind);
        return kind;
    }

    protected static void ReadNames(string type, string key, array<string> names)
    {
        array<string> roots = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
        for (int i = 0; i < roots.Count(); i++)
        {
            string path = roots[i] + " " + type + " " + key;
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

    // The slots a class has for parts.
    static void HostSlots(string type, array<string> names)
    {
        names.InsertAll(Kept(s_Host, type, "attachments"));
    }

    // The slots a class may sit in.
    static void PartSlots(string type, array<string> names)
    {
        names.InsertAll(Kept(s_Part, type, "inventorySlot"));
    }

    // A magazine, not a pile of rounds: CfgMagazines holds both, and a pile is
    // a class of the game's Ammunition_Base.
    static bool IsMagazine(string type)
    {
        return KindOf(type) == KIND_MAGAZINE;
    }

    static bool IsWeapon(string type)
    {
        return KindOf(type) == KIND_WEAPON;
    }

    static bool WeaponTakes(string weapon, string magazine)
    {
        array<string> mags;
        if (!s_Mags.Find(weapon, mags))
        {
            mags = new array<string>();
            GetGame().ConfigGetTextArray("CfgWeapons " + weapon + " magazines", mags);
            for (int m = 0; m < mags.Count(); m++)
            {
                string low = mags[m];
                low.ToLower();
                mags.Set(m, low);
            }
            s_Mags.Set(weapon, mags);
        }
        string want = magazine;
        want.ToLower();
        return mags.Find(want) > -1;
    }
    protected static ref map<string, ref array<string>> s_Mags = new map<string, ref array<string>>();

    protected static bool Holds(array<string> names, string name)
    {
        string want = name;
        want.ToLower();
        for (int i = 0; i < names.Count(); i++)
        {
            string have = names[i];
            have.ToLower();
            if (have == want)
                return true;
        }
        return false;
    }

    // The slot `part` takes on `host`: the first of the part's own slots that
    // the host has and `taken` does not hold. "" when there is none.
    static string SlotFor(string host, string part, array<string> taken)
    {
        if (IsMagazine(part) && IsWeapon(host))
        {
            if (!WeaponTakes(host, part))
                return "";
            if (taken && Holds(taken, MAGAZINE_SLOT))
                return "";
            return MAGAZINE_SLOT;
        }

        array<string> has = new array<string>();
        HostSlots(host, has);
        array<string> sits = new array<string>();
        PartSlots(part, sits);
        for (int i = 0; i < sits.Count(); i++)
        {
            string slot = sits[i];
            if (!Holds(has, slot))
                continue;
            if (taken && Holds(taken, slot))
                continue;
            return slot;
        }
        return "";
    }

    // Does `part` sit in the slot `slot` of `host` at all?
    static bool SlotOf(string host, string part, string slot)
    {
        if (IsMagazine(part) && IsWeapon(host))
        {
            string name = slot;
            name.ToLower();
            return name == MAGAZINE_SLOT && WeaponTakes(host, part);
        }
        return Holds(Kept(s_Part, part, "inventorySlot"), slot) && Holds(Kept(s_Host, host, "attachments"), slot);
    }
}
