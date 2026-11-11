# A real mouse drag inside the game client's window: press at one point of the
# client area, travel to another in small steps, release. A stand tool: the
# trade screen's drag and drop cannot be asked from script, only done.
#
# It stands on the MCP server's own window helpers (dayz_mcp.winui), so it has
# to be run with that server's Python:
#
#   <mcp>/.venv/Scripts/python.exe -I tools/drag.py <pid> <x1> <y1> <x2> <y2> [steps] [hold_ms] [button]
#
# <pid> is the client's process id (client_start's summary names it). The
# coordinates are pixels of the CLIENT AREA, the ones ui_tree reports, so they
# hold wherever the window stands. `steps` is how many moves the travel is cut
# into (default 12), `hold_ms` the pause before the release (default 150),
# `button` is left (default) or right.
#
# Like every real input it needs the client window in front, and it refuses
# when Windows will not put it there: a drag into somebody else's window is
# the accident this guards against.
import sys
import time

from dayz_mcp import winui

MOVE = winui.MOUSEEVENTF_ABSOLUTE | winui.MOUSEEVENTF_MOVE
DOWN = {"left": winui.MOUSEEVENTF_LEFTDOWN, "right": 0x0008}
UP = {"left": winui.MOUSEEVENTF_LEFTUP, "right": 0x0010}


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
    if len(argv) < 6:
        sys.exit("usage: drag.py <pid> <x1> <y1> <x2> <y2> [steps] [hold_ms] [button]")
    pid, x1, y1, x2, y2 = (int(a) for a in argv[1:6])
    steps = int(argv[6]) if len(argv) > 6 else 12
    hold_ms = int(argv[7]) if len(argv) > 7 else 150
    button = argv[8] if len(argv) > 8 else "left"
    if button not in DOWN or steps < 1:
        sys.exit("button is left or right, steps at least 1")

    refusal = winui._require_focus(pid)
    if refusal:
        sys.exit("refused: %s" % refusal.error)
    hwnd = winui.find_window(pid)
    width, height = winui.client_size(hwnd)
    for x, y in ((x1, y1), (x2, y2)):
        if not (0 <= x < width and 0 <= y < height):
            sys.exit("(%d, %d) is outside the client area, which is %dx%d" % (x, y, width, height))

    # Every move is an event of its own with a pause after it: the game reads
    # the cursor a frame later than the button, and events glued together are
    # how a press lands where the cursor used to be.
    winui._mouse_event(MOVE, *absolute(hwnd, x1, y1))
    time.sleep(0.2)
    winui._mouse_event(DOWN[button])
    time.sleep(0.12)
    for i in range(1, steps + 1):
        x = x1 + (x2 - x1) * i // steps
        y = y1 + (y2 - y1) * i // steps
        winui._mouse_event(MOVE, *absolute(hwnd, x, y))
        time.sleep(0.04)
    time.sleep(hold_ms / 1000.0)
    winui._mouse_event(UP[button])
    print("dragged %s from (%d, %d) to (%d, %d) in %d steps" % (button, x1, y1, x2, y2, steps))


if __name__ == "__main__":
    main(sys.argv)
