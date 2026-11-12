# Fill one edit box of the game client with the real mouse and keyboard: click
# it, empty it, type the new text. A stand tool, like click.py: the MCP types
# into a field (client_type) but has no way to empty one first, and an admin
# form arrives filled from the row picked on its list.
#
#   <mcp>/.venv/Scripts/python.exe -I tools/field.py <pid> <x> <y> [text] [erase]
#
# <x> <y> are pixels of the CLIENT AREA, inside the box. `erase` is how many
# Backspaces follow the End key (default 40, more than any field of the
# series holds). Text is typed as US scancodes by the MCP's own helper, so
# Latin letters, digits and the usual punctuation only: Cyrillic has no key
# there and is refused before anything is sent.
import sys
import time

from dayz_mcp import winui

MOVE = winui.MOUSEEVENTF_ABSOLUTE | winui.MOUSEEVENTF_MOVE
END = 0x4F
BACKSPACE = 0x0E


def absolute(hwnd, x, y):
    sx, sy = winui.client_to_screen(hwnd, x, y)
    u = winui.user32
    return winui.virtual_to_absolute(
        sx, sy,
        u.GetSystemMetrics(winui.SM_XVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_YVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_CXVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_CYVIRTUALSCREEN),
    )


def main(argv):
    if len(argv) < 4:
        sys.exit("usage: field.py <pid> <x> <y> [text] [erase]")
    pid, x, y = (int(a) for a in argv[1:4])
    text = argv[4] if len(argv) > 4 else ""
    erase = int(argv[5]) if len(argv) > 5 else 40
    bad = winui.unsupported_characters(text)
    if bad:
        sys.exit("no US key for: %s" % " ".join(bad))

    refusal = winui._require_focus(pid)
    if refusal:
        sys.exit("refused: %s" % refusal.error)
    hwnd = winui.find_window(pid)
    winui._mouse_event(MOVE, *absolute(hwnd, x, y))
    time.sleep(0.2)
    winui._mouse_event(winui.MOUSEEVENTF_LEFTDOWN)
    time.sleep(0.06)
    winui._mouse_event(winui.MOUSEEVENTF_LEFTUP)
    time.sleep(0.25)
    winui._tap(END, extended=True)
    for _ in range(erase):
        winui._tap(BACKSPACE)
    if text:
        result = winui.type_text(pid, text)
        if not result.ok:
            sys.exit("typing refused: %s" % result.error)
    print("field at (%d, %d): erased %d, typed %r" % (x, y, erase, text))


if __name__ == "__main__":
    main(sys.argv)
