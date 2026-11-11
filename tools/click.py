# Real mouse clicks inside the game client's window: one, or several in a row
# at one point of the client area. A stand tool: the trade screen's double
# click is two presses within 400 ms, and the MCP's own click takes two
# seconds a press.
#
# It stands on the MCP server's own window helpers (dayz_mcp.winui), so it has
# to be run with that server's Python:
#
#   <mcp>/.venv/Scripts/python.exe -I tools/click.py <pid> <x> <y> [count] [gap_ms]
#
# <pid> is the client's process id (client_start's summary names it). The
# coordinates are pixels of the CLIENT AREA, the ones ui_tree reports. `count`
# is how many presses (default 1), `gap_ms` the pause between the release of
# one and the press of the next (default 120).
#
# Like every real input it needs the client window in front, and it refuses
# when Windows will not put it there.
import sys
import time

from dayz_mcp import winui

MOVE = winui.MOUSEEVENTF_ABSOLUTE | winui.MOUSEEVENTF_MOVE


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
        sys.exit("usage: click.py <pid> <x> <y> [count] [gap_ms]")
    pid, x, y = (int(a) for a in argv[1:4])
    count = int(argv[4]) if len(argv) > 4 else 1
    gap_ms = int(argv[5]) if len(argv) > 5 else 120
    if count < 1:
        sys.exit("count at least 1")

    refusal = winui._require_focus(pid)
    if refusal:
        sys.exit("refused: %s" % refusal.error)
    hwnd = winui.find_window(pid)
    width, height = winui.client_size(hwnd)
    if not (0 <= x < width and 0 <= y < height):
        sys.exit("(%d, %d) is outside the client area, which is %dx%d" % (x, y, width, height))

    # The game reads the cursor a frame later than the button: the move is an
    # event of its own, with a pause after it.
    winui._mouse_event(MOVE, *absolute(hwnd, x, y))
    time.sleep(0.2)
    for i in range(count):
        winui._mouse_event(winui.MOUSEEVENTF_LEFTDOWN)
        time.sleep(0.06)
        winui._mouse_event(winui.MOUSEEVENTF_LEFTUP)
        if i + 1 < count:
            time.sleep(gap_ms / 1000.0)
    print("clicked %d time(s) at (%d, %d)" % (count, x, y))


if __name__ == "__main__":
    main(sys.argv)
