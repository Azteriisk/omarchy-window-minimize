# 󰖰 Omarchy Window Minimize Plugin (`azterisk.minimize`)

An intelligent window minimization, off-screen application grouping, CSD titlebar interceptor, and instant workspace restoration plugin for **Omarchy Linux** and **Hyprland**.

---

## ✨ Features

- 🗕 **Off-Screen Application Clustering**: When minimized, windows are moved smoothly to an uninhabited staging coordinate space (`(50000+, 50000+)`) far outside your physical monitors, organized into horizontal columns by application `class` and vertically cascaded.
- 🎯 **Native CSD Titlebar Minimize Button Interceptor**: Includes a compiled Hyprland C++ plugin hook (`minimize-hook.so`) that intercepts `xdg_toplevel.set_minimized` signals from native Wayland & XWayland apps (like Electron, Antigravity IDE, VS Code, Chrome, Spotify, Discord). Clicking the app's `[-]` button now actually minimizes the window!
- 🔄 **Flawless Multi-Workspace Restoration**: Restores windows back to their exact original workspace, position, dimensions, floating vs tiled layout, and fullscreen state.
- 🖱️ **Super + Middle Click Fast Minimize**: Click any window while holding <kbd>Super</kbd> with the middle mouse button to minimize instantly.
- ⚡ **Zero Latency Hyprland Batch IPC**: Uses atomic `hyprctl --batch` and native `hl.dsp` Lua dispatches for sub-millisecond minimize and restore actions.
- 🏷️ **Native Hyprland Tagging**: Automatically tags minimized windows with `+minimized` and `+min_grp_<app_class>` for Hyprland rules integration.
- 📊 **Dynamic Top-Bar Badge & Interactive Drawer**:
  - Displays a clean `󰖰 <count>` indicator in your Omarchy status bar when windows are minimized.
  - Interactive pop-out panel listing minimized windows grouped by application with workspace tags, individual restore buttons, and a global "Restore All" button.
  - Middle-click the bar icon to pop/restore the last minimized window; right-click to restore all windows instantly.
- 🧹 **Auto-Reconciliation Daemon**: Long-running background service in `omarchy-shell` automatically purges state if a minimized window is closed or killed externally.

---

## ⌨️ Shortcuts Reference

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| **CSD `[-]` Button** | **Click Titlebar Minimize** | Intercepted via Hyprland C++ hook; minimizes the window |
| <kbd>Super</kbd> + <kbd>M</kbd> | **Minimize Window** | Moves focused window off-screen to its application group cluster |
| <kbd>Super</kbd> + <kbd>Middle Click</kbd> | **Minimize Window (Mouse)** | Click any window with Super + Middle Mouse button to minimize |
| <kbd>Super</kbd> + <kbd>Alt</kbd> + <kbd>M</kbd> | **Restore All Windows** | Brings all minimized windows back to their original workspaces & positions |
| <kbd>Super</kbd> + <kbd>Ctrl</kbd> + <kbd>M</kbd> | **Restore Last Window** | Restores the most recently minimized window (LIFO stack) |

---

## 🚀 Quick All-In-One Installation

To install and enable everything (CLI, Quickshell bar widget, keybindings, and the C++ CSD interceptor hook):

```bash
~/.config/omarchy/plugins/azterisk.minimize/install.sh
```

---

## ⚙️ CLI Reference (`omarchy-minimize`)

The plugin includes a standalone CLI binary symlinked to `~/.local/bin/omarchy-minimize`:

```bash
# Minimize currently focused window
omarchy-minimize minimize

# Minimize a specific window by address
omarchy-minimize minimize 0x557ac141fd10

# Restore all minimized windows
omarchy-minimize restore-all

# Restore the most recently minimized window
omarchy-minimize restore-last

# Restore all windows for a specific app (e.g. Zen Browser or Ghostty)
omarchy-minimize restore-app zen-browser
omarchy-minimize restore-app ghostty

# Restore a single window by address
omarchy-minimize restore 0x557ac141fd10

# View all minimized windows (formatted list or JSON)
omarchy-minimize list
omarchy-minimize list --json

# Get status summary for bar widgets
omarchy-minimize status

# Reconcile state against currently living windows
omarchy-minimize clean
```

---

## 📁 Repository Structure

```
azterisk.minimize/
├── manifest.json            # Plugin manifest (schemaVersion: 1)
├── Panel.qml                # Quickshell UI panel & status bar widget
├── BarWidget.qml            # Bar widget entrypoint
├── Service.qml              # Background daemon for window state reconciliation
├── hyprland-plugin/         # C++ CSD Titlebar Interceptor Hook
│   ├── main.cpp             # Intercepts xdg_toplevel.set_minimized & XWayland IconicState
│   ├── Makefile             # C++26 compilation flags
│   └── minimize-hook.so     # Compiled Hyprland dynamic shared library
├── scripts/
│   └── omarchy-minimize     # Core Python minimize & grouping engine
├── install.sh               # All-in-one automated installer
├── uninstall.sh             # Complete uninstaller
└── README.md                # Documentation & guide
```

---

## 🗑️ Uninstallation

To cleanly remove keybindings, hooks, symlinks, and the bar widget:

```bash
~/.config/omarchy/plugins/azterisk.minimize/uninstall.sh
```

---

## 👤 Author
Created by **Azteriisk** for Omarchy.
