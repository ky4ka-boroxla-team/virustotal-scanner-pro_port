#include "tray_linux.h"
#include <X11/Xatom.h>
#include <cstdio>
#include <cstring>

namespace Tray {
namespace {

Display* g_display = nullptr;
int g_screen = 0;
Window g_root = 0;
Colormap g_colormap = 0;
GC g_gc = 0;
XFontStruct* g_font = nullptr;

Window g_iconWin = 0;
Window g_menuWin = 0;
int g_menuHover = -1; // -1 = none, 0 = "show", 1 = "exit"

std::string g_tooltip;
std::string g_showLabel;
std::string g_exitLabel;
std::function<void()> g_onShow;
std::function<void()> g_onExit;

Atom g_atomNetSystemTray = 0;
Atom g_atomManager = 0;
Atom g_atomOpcode = 0;
Atom g_atomXEmbedInfo = 0;

constexpr int kIconSize = 24;
constexpr int kMenuWidth = 170;
constexpr int kMenuItemH = 26;

unsigned long AllocColor(int r, int g, int b) {
    XColor c;
    c.red = (unsigned short)(r * 257);
    c.green = (unsigned short)(g * 257);
    c.blue = (unsigned short)(b * 257);
    c.flags = DoRed | DoGreen | DoBlue;
    if (!XAllocColor(g_display, g_colormap, &c)) return BlackPixel(g_display, g_screen);
    return c.pixel;
}

Window FindTrayManager() {
    return XGetSelectionOwner(g_display, g_atomNetSystemTray);
}

void DrawIcon() {
    if (!g_iconWin) return;
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(g_display, g_iconWin, &attrs)) return;
    int w = attrs.width > 0 ? attrs.width : kIconSize;
    int h = attrs.height > 0 ? attrs.height : kIconSize;

    static unsigned long bg = AllocColor(30, 34, 42);
    static unsigned long shield = AllocColor(56, 132, 227);
    static unsigned long check = AllocColor(120, 220, 140);

    XSetForeground(g_display, g_gc, bg);
    XFillRectangle(g_display, g_iconWin, g_gc, 0, 0, w, h);

    // Simple shield glyph, scaled to whatever size the panel actually gave us.
    XPoint pts[6] = {
        { (short)(w * 0.50), (short)(h * 0.08) },
        { (short)(w * 0.85), (short)(h * 0.22) },
        { (short)(w * 0.85), (short)(h * 0.55) },
        { (short)(w * 0.50), (short)(h * 0.92) },
        { (short)(w * 0.15), (short)(h * 0.55) },
        { (short)(w * 0.15), (short)(h * 0.22) },
    };
    XSetForeground(g_display, g_gc, shield);
    XFillPolygon(g_display, g_iconWin, g_gc, pts, 6, Convex, CoordModeOrigin);

    XSetForeground(g_display, g_gc, check);
    XSetLineAttributes(g_display, g_gc, w <= 20 ? 2 : 3, LineSolid, CapRound, JoinRound);
    XDrawLine(g_display, g_iconWin, g_gc, (int)(w * 0.30), (int)(h * 0.52),
              (int)(w * 0.45), (int)(h * 0.68));
    XDrawLine(g_display, g_iconWin, g_gc, (int)(w * 0.45), (int)(h * 0.68),
              (int)(w * 0.72), (int)(h * 0.35));
}

void CreateIconWindow() {
    if (g_iconWin) {
        XDestroyWindow(g_display, g_iconWin);
        g_iconWin = 0;
    }

    XSetWindowAttributes swa = {};
    swa.colormap = g_colormap;
    swa.background_pixel = BlackPixel(g_display, g_screen);
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask;

    g_iconWin = XCreateWindow(g_display, g_root, 0, 0, kIconSize, kIconSize, 0,
                               DefaultDepth(g_display, g_screen), InputOutput,
                               DefaultVisual(g_display, g_screen),
                               CWColormap | CWBackPixel | CWBorderPixel | CWEventMask, &swa);

    // Required by the XEmbed spec so the tray host manages us correctly.
    long info[2] = { 0, 1 }; // version 0, XEMBED_MAPPED
    XChangeProperty(g_display, g_iconWin, g_atomXEmbedInfo, g_atomXEmbedInfo, 32,
                     PropModeReplace, (unsigned char*)info, 2);

    XStoreName(g_display, g_iconWin, g_tooltip.c_str());
}

void DockIcon() {
    Window manager = FindTrayManager();
    if (manager == None) return; // no tray host running (yet) - handled gracefully

    CreateIconWindow();

    XClientMessageEvent ev = {};
    ev.type = ClientMessage;
    ev.window = manager;
    ev.message_type = g_atomOpcode;
    ev.format = 32;
    ev.data.l[0] = CurrentTime;
    ev.data.l[1] = 0; // SYSTEM_TRAY_REQUEST_DOCK
    ev.data.l[2] = (long)g_iconWin;
    ev.data.l[3] = 0;
    ev.data.l[4] = 0;

    XSendEvent(g_display, manager, False, NoEventMask, (XEvent*)&ev);
    XSync(g_display, False);
}

void DestroyMenu() {
    if (g_menuWin) {
        XUngrabPointer(g_display, CurrentTime);
        XDestroyWindow(g_display, g_menuWin);
        g_menuWin = 0;
    }
    g_menuHover = -1;
}

void DrawMenuItem(int index, const std::string& label) {
    int y = index * kMenuItemH;
    unsigned long itemBg = (index == g_menuHover) ? AllocColor(60, 90, 140) : AllocColor(36, 40, 48);
    XSetForeground(g_display, g_gc, itemBg);
    XFillRectangle(g_display, g_menuWin, g_gc, 0, y, kMenuWidth, kMenuItemH);
    XSetForeground(g_display, g_gc, AllocColor(230, 230, 235));
    if (g_font) XSetFont(g_display, g_gc, g_font->fid);
    XDrawString(g_display, g_menuWin, g_gc, 14, y + kMenuItemH / 2 + 5,
                label.c_str(), (int)label.size());
}

void DrawMenu() {
    if (!g_menuWin) return;
    DrawMenuItem(0, g_showLabel);
    DrawMenuItem(1, g_exitLabel);
}

int MenuItemAt(int y) {
    int idx = y / kMenuItemH;
    return (idx == 0 || idx == 1) ? idx : -1;
}

void ShowContextMenu() {
    DestroyMenu();

    Window rootRet, childRet;
    int rx, ry, wx, wy;
    unsigned int mask;
    XQueryPointer(g_display, g_root, &rootRet, &childRet, &rx, &ry, &wx, &wy, &mask);

    int menuH = kMenuItemH * 2;
    int screenW = DisplayWidth(g_display, g_screen);
    int screenH = DisplayHeight(g_display, g_screen);
    int x = rx;
    int y = ry;
    if (x + kMenuWidth > screenW) x = screenW - kMenuWidth;
    if (y + menuH > screenH) y = ry - menuH;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    XSetWindowAttributes swa = {};
    swa.override_redirect = True;
    swa.background_pixel = AllocColor(36, 40, 48);
    swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask;
    g_menuWin = XCreateWindow(g_display, g_root, x, y, kMenuWidth, menuH, 1,
                               DefaultDepth(g_display, g_screen), InputOutput,
                               DefaultVisual(g_display, g_screen),
                               CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);
    XMapRaised(g_display, g_menuWin);
    XGrabPointer(g_display, g_menuWin, True,
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    DrawMenu();
}

} // namespace

void Init(Display* display, int screen, Window mainWindow,
          const std::string& tooltip,
          const std::string& showLabel,
          const std::string& exitLabel,
          const std::function<void()>& onShowRequested,
          const std::function<void()>& onExitRequested) {
    (void)mainWindow;
    g_display = display;
    g_screen = screen;
    g_root = RootWindow(display, screen);
    g_colormap = DefaultColormap(display, screen);
    g_tooltip = tooltip;
    g_showLabel = showLabel;
    g_exitLabel = exitLabel;
    g_onShow = onShowRequested;
    g_onExit = onExitRequested;

    char atomName[32];
    snprintf(atomName, sizeof(atomName), "_NET_SYSTEM_TRAY_S%d", screen);
    g_atomNetSystemTray = XInternAtom(display, atomName, False);
    g_atomManager = XInternAtom(display, "MANAGER", False);
    g_atomOpcode = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", False);
    g_atomXEmbedInfo = XInternAtom(display, "_XEMBED_INFO", False);

    g_gc = XCreateGC(display, g_root, 0, nullptr);
    g_font = XLoadQueryFont(display, "-*-dejavu sans-medium-r-*-*-13-*-*-*-*-*-*-*");
    if (!g_font) g_font = XLoadQueryFont(display, "-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*");
    if (!g_font) g_font = XLoadQueryFont(display, "fixed");

    // Listen for the "MANAGER" broadcast so we (re)dock if the tray host starts or
    // restarts after us - this is what makes the icon real rather than a one-shot stub.
    XWindowAttributes rootAttrs;
    XGetWindowAttributes(display, g_root, &rootAttrs);
    XSelectInput(display, g_root, rootAttrs.your_event_mask | StructureNotifyMask);

    DockIcon(); // in case a tray host is already running
}

bool HandleEvent(const XEvent& event) {
    if (event.type == ClientMessage && event.xclient.window == g_root &&
        event.xclient.message_type == g_atomManager) {
        if ((Atom)event.xclient.data.l[1] == (long)g_atomNetSystemTray) {
            DockIcon();
        }
        return true;
    }

    if (g_iconWin && event.xany.window == g_iconWin) {
        switch (event.type) {
        case Expose:
        case ConfigureNotify:
            DrawIcon();
            return true;
        case ButtonPress:
            if (event.xbutton.button == Button3) ShowContextMenu();
            return true;
        case ButtonRelease:
            if (event.xbutton.button == Button1 && g_onShow) g_onShow();
            return true;
        default:
            return true;
        }
    }

    if (g_menuWin && event.xany.window == g_menuWin) {
        switch (event.type) {
        case Expose:
            DrawMenu();
            return true;
        case MotionNotify: {
            int idx = MenuItemAt(event.xmotion.y);
            if (idx != g_menuHover) { g_menuHover = idx; DrawMenu(); }
            return true;
        }
        case ButtonRelease: {
            int idx = MenuItemAt(event.xbutton.y);
            DestroyMenu();
            if (idx == 0 && g_onShow) g_onShow();
            else if (idx == 1 && g_onExit) g_onExit();
            return true;
        }
        default:
            return true;
        }
    }

    if (g_menuWin && event.type == ButtonPress) {
        // A click landed outside both the icon and the menu (the active grab routed it
        // to us) - treat it as "dismiss", same as clicking outside any dropdown menu.
        DestroyMenu();
        return true;
    }

    return false;
}

void Shutdown() {
    DestroyMenu();
    if (g_iconWin) { XDestroyWindow(g_display, g_iconWin); g_iconWin = 0; }
    if (g_gc) { XFreeGC(g_display, g_gc); g_gc = 0; }
    if (g_font) { XFreeFont(g_display, g_font); g_font = nullptr; }
}

} // namespace Tray
