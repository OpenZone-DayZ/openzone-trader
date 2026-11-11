// Shared by both sides of the probe: files under $profile:OpenZone_TraderProbe
// and the two bits of text every report needs.
class OZT_ProbeLog
{
    static const string DIR = "$profile:OpenZone_TraderProbe";

    static void EnsureDir()
    {
        MakeDirectory(DIR);
    }

    static void Append(string fileName, string line)
    {
        FileHandle fh = OpenFile(DIR + "/" + fileName, FileMode.APPEND);
        if (!fh)
            return;
        FPrintln(fh, line);
        CloseFile(fh);
    }

    static void WriteAll(string fileName, array<string> lines)
    {
        FileHandle fh = OpenFile(DIR + "/" + fileName, FileMode.WRITE);
        if (!fh)
            return;
        for (int i = 0; i < lines.Count(); i++)
            FPrintln(fh, lines.Get(i));
        CloseFile(fh);
    }

    static string B(bool v)
    {
        if (v)
            return "yes";
        return "no";
    }

    // One decimal, as text.
    static string R1(float v)
    {
        float r = Math.Round(v * 10) / 10;
        return r.ToString();
    }
}
