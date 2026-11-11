// Question 12: the character's slots as the client's config has them, read the
// way the vanilla inventory builds its equipment grid (playercontainer.c).
//
//   slots                      every slot of `playerSlots` as a tile (its ghost
//                              icon and its name) and as a line of
//                              client_slots.txt: config class, slot name, id,
//                              display name, ghost icon, what the player wears
//                              there now
class OZT_ProbeQSlots : OZT_ProbeQuestion
{
    static const string SLOT = "OpenZone_TraderProbe/gui/layouts/ozt_probe_slot.layout";
    static const string LIST = "CfgVehicles SurvivorBase InventoryEquipment playerSlots";

    protected Widget m_Grid;
    protected ref array<Widget> m_Tiles;

    override void OnAttach()
    {
        m_Grid = m_Root.FindAnyWidget("SlotsGrid");
        m_Tiles = new array<Widget>();
    }

    override void Stop()
    {
        for (int i = 0; i < m_Tiles.Count(); i++)
        {
            if (m_Tiles[i])
                m_Tiles[i].Unlink();
        }
        m_Tiles.Clear();
    }

    override string Command(array<string> words)
    {
        if (words[1] != "slots")
            return "";
        Stop();

        array<string> names = new array<string>();
        GetGame().ConfigGetTextArray(LIST, names);
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        array<string> lines = new array<string>();
        int unknown = 0;
        int noIcon = 0;
        bool probeSlot = false;

        for (int i = 0; i < names.Count(); i++)
        {
            string cfg = names[i];
            string path = "CfgSlots " + cfg;
            if (!GetGame().ConfigIsExisting(path))
            {
                unknown++;
                lines.Insert(cfg + " | not in CfgSlots");
                continue;
            }
            string icon;
            GetGame().ConfigGetText(path + " ghostIcon", icon);
            string slot;
            GetGame().ConfigGetText(path + " name", slot);
            int id = InventorySlots.GetSlotIdFromString(slot);
            string shown = InventorySlots.GetSlotDisplayName(id);
            if (icon == "")
                noIcon++;
            if (cfg == "Slot_OZT_ProbeWear")
                probeSlot = true;

            string worn = "-";
            if (player)
            {
                EntityAI on = player.GetInventory().FindAttachment(id);
                if (on)
                    worn = on.GetType();
            }
            string line = cfg + " | name=" + slot + " | id=" + id.ToString() + " | display=" + shown;
            line = line + " | ghost=" + icon + " | worn=" + worn;
            lines.Insert(line);

            Widget tile = GetGame().GetWorkspace().CreateWidgets(SLOT, m_Grid);
            if (!tile)
                continue;
            m_Tiles.Insert(tile);
            ImageWidget img = ImageWidget.Cast(tile.FindAnyWidget("SlotIcon"));
            if (img && icon != "")
                img.LoadImageFile(0, StaticGUIUtils.VerifyIconImageString(StaticGUIUtils.IMAGESETGROUP_INVENTORY, icon));
            TextWidget text = TextWidget.Cast(tile.FindAnyWidget("SlotName"));
            if (text)
                text.SetText(slot);
        }
        OZT_ProbeLog.WriteAll("client_slots.txt", lines);

        string s = "slots: listed=" + names.Count().ToString() + " tiles=" + m_Tiles.Count().ToString();
        s = s + " not_in_CfgSlots=" + unknown.ToString() + " without_icon=" + noIcon.ToString();
        s = s + " the probe's own slot=" + OZT_ProbeLog.B(probeSlot) + " | client_slots.txt";
        return s;
    }
}
