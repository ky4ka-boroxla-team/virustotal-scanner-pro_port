#pragma once
// Real X11 system tray icon (freedesktop.org "System Tray Protocol" / XEmbed).
// No GTK/Qt/DBus dependency - uses the same Xlib connection as main_linux.cpp.
//
// Supported by: XFCE, MATE, LXDE/LXQt, KDE Plasma, i3+trayer/stalonetray, polybar, etc.
// NOT supported by vanilla GNOME Shell (it has no legacy tray and no StatusNotifierItem
// host by default) - on GNOME the icon silently stays undocked until an extension such
// as "AppIndicator and KStatusNotifierItem Support" is installed. This is a desktop
// environment limitation, not a bug in this code.

#include <X11/Xlib.h>
#include <functional>
#include <string>

namespace Tray {

// Must be called once, after the X11 Display/root window exist, and before entering
// the main event loop. tooltip/showLabel/exitLabel are UTF-8.
void Init(Display* display, int screen, Window mainWindow,
          const std::string& tooltip,
          const std::string& showLabel,
          const std::string& exitLabel,
          const std::function<void()>& onShowRequested,
          const std::function<void()>& onExitRequested);

// Feed every XEvent from the main loop here. Returns true if the event belonged to the
// tray subsystem (icon window, context menu, or a tray-manager announcement) and should
// not be processed further by the caller.
bool HandleEvent(const XEvent& event);

// Call once before XCloseDisplay.
void Shutdown();

} // namespace Tray
