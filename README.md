# Omarchy Window Control & Minimize Plugin (azterisk.minimize)

An intelligent window minimization, maximize control, off-screen application grouping, CSD titlebar interceptor, and multi-workspace restoration plugin for Omarchy Linux and Hyprland.

---

## Features

- **Off-Screen Application Clustering**: When minimized, windows are moved to an uninhabited staging coordinate space (`50000+`, `50000+`) outside physical displays, organized into horizontal columns by application class and vertically cascaded.
- **Native CSD Titlebar Minimize Button Interceptor**: Intercepts `xdg_toplevel.set_minimized` and XWayland Iconic signals from native Wayland and XWayland applications (including Electron, Antigravity IDE, VS Code, Chrome, Spotify, and Discord). Clicking the application minimize button (`_`) docks the window into the off-screen cluster.
- **Native CSD Titlebar Maximize & Restore Interceptor**: Intercepts maximize requests from Wayland and XWayland applications (including Steam, Chromium, and Electron). Clicking the titlebar maximize button (`□`) toggles the window into Hyprland's `FSMODE_MAXIMIZED` (workarea monocle), cleanly expanding the window to fill the workspace while preserving the top bar and gaps, and restoring it when clicked again (`❐`).
- **Full Steam & Windows-Style UI Compatibility**: Handles Windows-style client-side decorations cleanly. Modern Steam client windows (`steamwebhelper`) and Steam games have full Maximize (`□`) and Close (`✕`) support, while their native minimize-to-tray behavior is preserved.
- **Instant Close (X) Reconciliation**: Real-time cleanup when windows are closed or destroyed externally or via titlebar X buttons, plus dedicated Close actions directly in the CLI and UI drawer.
- **Zero-Dependency Compositor Safety**: Decoupled asynchronous process execution (`execDetached`) via POSIX double-fork, ensuring zero ABI friction or memory interference with Hyprland's internal dispatcher map.
- **Smart Mouse Actions**: Middle-clicking while holding Super minimizes the window under the cursor. Middle-clicking while holding Super on empty desktop space, a window gap, or the top bar restores and unhides all minimized windows.
- **Multi-Workspace Restoration**: Restores windows back to their original workspace, position, dimensions, floating versus tiled layout, and fullscreen state.
- **Zero Latency Hyprland Batch IPC**: Uses atomic hyprctl batching and native C++ plugin hooks for sub-millisecond minimize, maximize, and restore actions.
- **Top Bar Widget and Interactive Drawer**: Displays a minimized window count indicator in the Omarchy status bar with individual restore and close buttons, group restores, and a global Restore All button.

---

## Shortcuts Reference

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| **Titlebar Minimize Button** (`_`) | Minimize Window | Intercepted via C++ hook; minimizes window into off-screen cluster |
| **Titlebar Maximize Button** (`□`) | Maximize / Restore | Intercepted via C++ hook; toggles workarea maximize (`FSMODE_MAXIMIZED`) |
| **Titlebar Close Button** (`✕`) | Close Window | Native client close with instant state cache purge |
| **Super + M** | Minimize Window | Keyboard shortcut to move focused window off-screen |
| **Super + Middle Click** *(on Window)* | Minimize Window | Middle-click any open window while holding Super to minimize it |
| **Super + Middle Click** *(on Desktop / Gap / Top Bar)* | Restore All Windows | Middle-click empty desktop wallpaper, window gaps, or the top bar while holding Super to restore and unhide all minimized windows |
| **Super + Alt + M** | Restore All Windows | Keyboard shortcut to restore all minimized windows back to their workspaces |
| **Super + Ctrl + M** | Restore Last Window | Keyboard shortcut to restore the most recently minimized window (LIFO stack) |

---

## Handling Tray Applications (Steam, Discord, Telegram, etc.)

Certain applications have built-in minimize-to-tray functionality. When you click their minimize button, they internally hide their window and dock into the top bar system tray.

### How Steam & Tray Applications Work in v1.1.0:
- **Steam Titlebar Minimize (`_`)**: When clicking Minimize (`_`) in Steam, the window cleanly unmaximizes if needed and moves off-screen into the Omarchy minimize drawer (`50000, 50000`). If minimized while maximized, restoring it returns it directly to its maximized monocle layout.
- **Steam Titlebar Maximize (`□`) & Restore (`❐`)**: Fully functional! Clicking Maximize (`□`) expands the window into Hyprland's workarea monocle mode (`FSMODE_MAXIMIZED`) while keeping Omarchy gaps and the top status bar visible, and toggles the titlebar icon between Maximize and Restore.
- **Steam Titlebar Close (`✕`)**: Fully functional! Clicking Close (`✕`) closes the window cleanly and automatically purges any cached state.
- **Tray Activation**: Clicking Steam's tray icon or running `steam` automatically unhides/restores the window from the minimized drawer.

### Optional Tray Ignore List
If you have other applications (like Discord or Telegram) that you prefer to manage their own minimize-to-tray behavior without the Omarchy drawer, you can add their window class to `~/.config/omarchy/minimize-ignored-apps.txt`.

If you use an application that manages its own minimize-to-tray behavior, you can add its window class to the ignore list using either method below.

#### Method 1: Configuration File (No Recompilation Required)

1. Find the window class of the running application:
   ```bash
   hyprctl activewindow -j | grep -i "class"
   ```

2. Add the class name to `~/.config/omarchy/minimize-ignored-apps.txt`:
   ```bash
   mkdir -p ~/.config/omarchy
   cat << 'APPS' >> ~/.config/omarchy/minimize-ignored-apps.txt
   # Applications that handle their own minimize-to-tray behavior
   discord
   vesktop
   telegramdesktop
   APPS
   ```

The C++ plugin hook reads this file automatically.

#### Method 2: In C++ Source Code

You can also add the application class directly into `hyprland-plugin/main.cpp` inside `isNativeTrayApp`:

```cpp
static bool isNativeTrayApp(PHLWINDOW pWindow) {
    if (!pWindow)
        return false;
    std::string cls = pWindow->fetchClass();
    std::transform(cls.begin(), cls.end(), cls.begin(), [](unsigned char c) { return std::tolower(c); });
    
    // Steam (steam, steamwebhelper, steam_app_*) and games manage tray minimization natively.
    if (cls.find("steam") != std::string::npos || cls == "discord" || cls == "telegramdesktop")
        return true;

    return false;
}
```

Then recompile and reload the plugin:
```bash
make -C ~/.config/omarchy/plugins/azterisk.minimize/hyprland-plugin
hyprctl plugin unload ~/.config/omarchy/plugins/azterisk.minimize/hyprland-plugin/minimize-hook.so
hyprctl plugin load ~/.config/omarchy/plugins/azterisk.minimize/hyprland-plugin/minimize-hook.so
```

---

## Installation & Distribution

The plugin can be installed and distributed using three standard methods:

### Method 1: Omarchy Plugin Manager (Recommended)

```bash
omarchy plugin add https://github.com/Azteriisk/omarchy-window-minimize.git --enable --yes
```

### Method 2: All-In-One Shell Installer

Clone the repository and run the automated installer:

```bash
git clone https://github.com/Azteriisk/omarchy-window-minimize.git ~/.config/omarchy/plugins/azterisk.minimize
~/.config/omarchy/plugins/azterisk.minimize/install.sh
```

The installer automatically:
1. Compiles the C++ CSD interceptor hook against your installed Hyprland version.
2. Symlinks `omarchy-minimize` into `~/.local/bin/`.
3. Registers autostart hooks in `~/.config/hypr/autostart.lua`.
4. Adds keybindings to `~/.config/hypr/bindings.lua`.
5. Registers the widget in `~/.config/omarchy/shell.json`.
6. Reloads Quickshell and Hyprland.

### Method 3: Arch Linux / AUR Package (PKGBUILD)

For system-wide package management:

```bash
git clone https://github.com/Azteriisk/omarchy-window-minimize.git
cd omarchy-window-minimize
makepkg -si
```

---

## CLI Reference (`omarchy-minimize`)

The plugin includes a standalone CLI binary symlinked to `~/.local/bin/omarchy-minimize`:

```bash
# Minimize currently focused window
omarchy-minimize minimize

# Minimize a specific window by address
omarchy-minimize minimize 0x557ac141fd10

# Smart mouse action (minimize on window, restore all on desktop/gap/topbar)
omarchy-minimize mouse-action

# Restore all minimized windows
omarchy-minimize restore-all

# Restore the most recently minimized window (LIFO)
omarchy-minimize restore-last

# Restore all windows for a specific app (e.g. Zen Browser or Ghostty)
omarchy-minimize restore-app zen-browser
omarchy-minimize restore-app ghostty

# Restore a single window by address
omarchy-minimize restore 0x557ac141fd10

# View all minimized windows
omarchy-minimize list
omarchy-minimize list --json

# Maximize or toggle maximize for active window
omarchy-minimize maximize

# Maximize or toggle maximize for a specific window
omarchy-minimize maximize 0x557ac141fd10

# Close active window
omarchy-minimize close

# Close a specific window (safely purging from minimized cache if minimized)
omarchy-minimize close 0x557ac141fd10

# Reconcile state against currently living windows
omarchy-minimize clean
```

---

## Repository Structure

```
azterisk.minimize/
├── PKGBUILD                 # Arch Linux / AUR build script
├── manifest.json            # Plugin manifest (schemaVersion: 1)
├── Panel.qml                # Quickshell UI panel and status bar widget drawer
├── BarWidget.qml            # Bar widget entrypoint
├── Service.qml              # Background daemon for window state reconciliation
├── hyprland-plugin/         # C++ CSD Titlebar Interceptor Hook
│   ├── main.cpp             # Intercepts xdg_toplevel and XWayland Iconic/Maximize signals
│   ├── Makefile             # C++26 shared library build configuration
│   └── minimize-hook.so     # Compiled Hyprland dynamic library
├── scripts/
│   └── omarchy-minimize     # Core Python minimize, maximize, and grouping engine
├── install.sh               # All-in-one automated installer
├── uninstall.sh             # Complete uninstaller
└── README.md                # Documentation and usage guide
```

---

## Uninstallation

To cleanly remove keybindings, hooks, symlinks, and the bar widget:

```bash
~/.config/omarchy/plugins/azterisk.minimize/uninstall.sh
```

---

## Author
Created by Azteriisk for Omarchy.
