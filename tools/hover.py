# Rests the real cursor on a point of the game client's window, without a
# press. A stand tool: the trade screen's item card comes up under a resting
# cursor, and nothing in the MCP moves the mouse without clicking.
#
# It stands on the MCP server's own window helpers (dayz_mcp.winui), so it has
# to be run with that server's Python:
#
#   <mcp>/.venv/Scripts/python.exe -I tools/hover.py <pid> <x> <y>
#
# <pid> is the client's process id (client_start's summary names it). The
# coordinates are pixels of the CLIENT AREA, the ones ui_tree reports.
#
# It needs the client window in front and refuses when the window under the
# point is not the client's: on this machine the taskbar covers the left strip
# of a window that opens in the corner (move the window clear first).
import ctypes
import sys
import time
from ctypes import wintypes

from dayz_mcp import winui

MOVE = winui.MOUSEEVENTF_ABSOLUTE | winui.MOUSEEVENTF_MOVE


def main(argv):
    if len(argv) < 4:
        sys.exit("usage: hover.py <pid> <x> <y>")
    pid, x, y = (int(a) for a in argv[1:4])
    refusal = winui._require_focus(pid)
    if refusal:
        sys.exit("refused: %s" % refusal.error)
    hwnd = winui.find_window(pid)
    width, height = winui.client_size(hwnd)
    if not (0 <= x < width and 0 <= y < height):
        sys.exit("(%d, %d) is outside the client area, which is %dx%d" % (x, y, width, height))

    u = winui.user32
    sx, sy = winui.client_to_screen(hwnd, x, y)
    u.WindowFromPoint.argtypes = [wintypes.POINT]
    u.WindowFromPoint.restype = wintypes.HWND
    u.GetAncestor.restype = wintypes.HWND
    if u.GetAncestor(u.WindowFromPoint(wintypes.POINT(sx, sy)), 2) != hwnd:
        sys.exit("refused: another window lies over the client at (%d, %d)" % (x, y))

    ax, ay = winui.virtual_to_absolute(
        sx, sy,
        u.GetSystemMetrics(winui.SM_XVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_YVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_CXVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_CYVIRTUALSCREEN),
    )
    # Two moves a pixel apart: the game takes a hover from a cursor that moves.
    winui._mouse_event(MOVE, *winui.virtual_to_absolute(
        sx - 1, sy,
        u.GetSystemMetrics(winui.SM_XVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_YVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_CXVIRTUALSCREEN),
        u.GetSystemMetrics(winui.SM_CYVIRTUALSCREEN),
    ))
    time.sleep(0.05)
    winui._mouse_event(MOVE, ax, ay)
    print("the cursor rests at (%d, %d)" % (x, y))


if __name__ == "__main__":
    main(sys.argv)
