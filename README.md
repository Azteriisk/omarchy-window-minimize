# Omarchy Window Minimize Plugin (azterisk.minimize)

An intelligent window minimization, off-screen application grouping, CSD titlebar interceptor, and multi-workspace restoration plugin for Omarchy Linux and Hyprland.

## Features

- Off-Screen Application Clustering: When minimized, windows are moved to an uninhabited staging coordinate space (50000+, 50000+) outside physical displays, organized into horizontal columns by application class and vertically cascaded.
- Native CSD Titlebar Minimize Button Interceptor: Includes a compiled Hyprland C++ plugin hook (minimize-hook.so) that intercepts xdg_toplevel.set_minimized signals from native Wayland and XWayland applications (including Electron, Antigravity IDE, VS Code, Chrome, Spotify, and Discord). Clicking the application minimize button minimizes the window.
- Smart Mouse Actions: Middle-clicking while holding Super minimizes the window under the cursor. Middle-clicking while holding Super on empty desktop space, a window gap, or the top bar restores and unhides all minimized windows.
- Multi-Workspace Restoration: Restores windows back to their original workspace, position, dimensions, floating versus tiled layout, and fullscreen state.
- Zero Latency Hyprland Batch IPC: Uses atomic hyprctl batching and native hl.dsp Lua dispatches for sub-millisecond minimize and restore actions.
- Native Hyprland Tagging: Automatically tags minimized windows with +minimized and +min_grp_<app_class> for Hyprland rules integration.
- Top Bar Widget and Interactive Drawer: Displays a minimized window count indicator in the Omarchy status bar. Includes an interactive popup drawer listing minimized windows grouped by application with workspace tags, individual restore buttons, and a global Restore All button. Middle-clicking the bar icon restores the last minimized window; right-clicking restores all windows.
- Auto-Reconciliation Daemon: Background service in omarchy-shell automatically purges state if a minimized window is closed or killed externally.

## Shortcuts Reference

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| Titlebar Minimize Button | Minimize Window | Intercepted via Hyprland C++ hook; minimizes the window |
| Super + M | Minimize Window | Keyboard shortcut to move focused window off-screen |
| Super + Middle Click (on Window) | Minimize Window | Middle-click any open window while holding Super to minimize it |
| Super + Middle Click (on Desktop / Gap / Top Bar) | Restore All Windows | Middle-click empty desktop wallpaper, window gaps, or the top bar while holding Super to restore and unhide all minimized windows |
| Super + Alt + M | Restore All Windows | Keyboard shortcut to restore all minimized windows back to their workspaces |
| Super + Ctrl + M | Restore Last Window | Keyboard shortcut to restore the most recently minimized window (LIFO stack) |

## Installation

Run this command in your terminal to install and enable the plugin:

```bash
git clone https://github.com/Azteriisk/omarchy-window-minimize.git ~/.config/omarchy/plugins/azterisk.minimize && ~/.config/omarchy/plugins/azterisk.minimize/install.sh
```

Or install via the Omarchy plugin manager:

```bash
omarchy plugin add https://github.com/Azteriisk/omarchy-window-minimize.git --enable --yes
```

## CLI Reference (omarchy-minimize)

The plugin includes a standalone CLI binary symlinked to ~/.local/bin/omarchy-minimize:

```bash
# Minimize currently focused window
omarchy-minimize minimize

# Minimize a specific window by address
omarchy-minimize minimize 0x557ac141fd10

# Smart mouse action (minimize on window, restore all on desktop/gap/topbar)
omarchy-minimize mouse-action

# Restore all minimized windows
omarchy-minimize restore-all

# Restore the most recently minimized window
omarchy-minimize restore-last

# Restore all windows for a specific app (e.g. Zen Browser or Ghostty)
omarchy-minimize restore-app zen-browser
omarchy-minimize restore-app ghostty

# Restore a single window by address
omarchy-minimize restore 0x557ac141fd10

# View all minimized windows
omarchy-minimize list
omarchy-minimize list --json

# Get status summary for bar widgets
omarchy-minimize status

# Reconcile state against currently living windows
omarchy-minimize clean
```

## Repository Structure

```
azterisk.minimize/
├── manifest.json            # Plugin manifest (schemaVersion: 1)
├── Panel.qml                # Quickshell UI panel and status bar widget
├── BarWidget.qml            # Bar widget entrypoint
├── Service.qml              # Background daemon for window state reconciliation
├── hyprland-plugin/         # C++ CSD Titlebar Interceptor Hook
│   ├── main.cpp             # Intercepts xdg_toplevel.set_minimized and XWayland IconicState
│   ├── Makefile             # C++26 shared library build configuration
│   └── minimize-hook.so     # Compiled Hyprland dynamic library
├── scripts/
│   └── omarchy-minimize     # Core Python minimize and grouping engine
├── install.sh               # All-in-one automated installer
├── uninstall.sh             # Complete uninstaller
└── README.md                # Documentation and usage guide
```

## Uninstallation

To cleanly remove keybindings, hooks, symlinks, and the bar widget:

```bash
~/.config/omarchy/plugins/azterisk.minimize/uninstall.sh
```

## Author
Created by Azteriisk for Omarchy.
