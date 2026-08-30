#!/usr/bin/env bash
# Complete All-In-One Installer for azterisk.minimize

set -euo pipefail

PLUGIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$HOME/.local/bin"
CLI_TARGET="$BIN_DIR/omarchy-minimize"
HYPR_BINDINGS="$HOME/.config/hypr/bindings.lua"
HYPR_AUTOSTART="$HOME/.config/hypr/autostart.lua"
SHELL_JSON="$HOME/.config/omarchy/shell.json"
HOOK_DIR="$PLUGIN_DIR/hyprland-plugin"
HOOK_SO="$HOOK_DIR/minimize-hook.so"

echo "📦 Installing Omarchy Window Minimize plugin (azterisk.minimize)..."

# 1. Build Hyprland C++ CSD Minimize Hook if needed
if [[ -d "$HOOK_DIR" ]]; then
  echo "  ⚙ Compiling Hyprland C++ CSD interceptor hook..."
  make -C "$HOOK_DIR" >/dev/null 2>&1 || make -C "$HOOK_DIR"
  echo "  ✓ Compiled $HOOK_SO"
  
  # Load into running Hyprland session
  if command -v hyprctl >/dev/null 2>&1; then
    hyprctl plugin load "$HOOK_SO" >/dev/null 2>&1 || true
    echo "  ✓ Loaded minimize-hook.so into Hyprland"
  fi
fi

# 2. Ensure scripts are executable & symlink CLI
chmod +x "$PLUGIN_DIR/scripts/omarchy-minimize"
mkdir -p "$BIN_DIR"
ln -sf "$PLUGIN_DIR/scripts/omarchy-minimize" "$CLI_TARGET"
echo "  ✓ Symlinked omarchy-minimize to $CLI_TARGET"

# 3. Register autostart hook in autostart.lua
if [[ -f "$HYPR_AUTOSTART" ]]; then
  if ! grep -q "minimize-hook.so" "$HYPR_AUTOSTART"; then
    cat << 'AUTOSTART' >> "$HYPR_AUTOSTART"

-- Load Window Minimize CSD Button Interceptor Hook
local hook_so = (os.getenv("HOME") or "/home/azterisk") .. "/.config/omarchy/plugins/azterisk.minimize/hyprland-plugin/minimize-hook.so"
o.exec_on_start("hyprctl plugin load " .. hook_so)
AUTOSTART
    echo "  ✓ Registered plugin hook in $HYPR_AUTOSTART"
  fi
fi

# 4. Register keybindings in bindings.lua
if [[ -f "$HYPR_BINDINGS" ]]; then
  if ! grep -q "omarchy-minimize" "$HYPR_BINDINGS"; then
    cat << 'BINDINGS' >> "$HYPR_BINDINGS"

-- Window Minimize Plugin (azterisk.minimize)
local minimize_cmd = (os.getenv("HOME") or "/home/azterisk") .. "/.local/bin/omarchy-minimize"
o.bind("SUPER + M", "Minimize window", minimize_cmd .. " minimize")
o.bind("SUPER + ALT + M", "Restore all minimized windows", minimize_cmd .. " restore-all")
o.bind("SUPER + CTRL + M", "Restore last minimized window", minimize_cmd .. " restore-last")
o.bind("SUPER + mouse:274", "Minimize window (middle click)", minimize_cmd .. " mouse-action", { mouse = true })
BINDINGS
    echo "  ✓ Added keybindings (SUPER+M, SUPER+ALT+M, SUPER+CTRL+M, SUPER+MiddleClick) to $HYPR_BINDINGS"
  fi
fi

# 5. Add to shell.json if not present
if [[ -f "$SHELL_JSON" ]]; then
  python3 - << 'PYEOF'
import json
from pathlib import Path

shell_path = Path.home() / ".config" / "omarchy" / "shell.json"
try:
    with open(shell_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    right_section = data.get("bar", {}).get("layout", {}).get("right", [])
    has_widget = any(item.get("id") == "azterisk.minimize" for item in right_section)
    if not has_widget:
        power_idx = next((i for i, item in enumerate(right_section) if item.get("id") == "omarchy.power"), len(right_section))
        right_section.insert(power_idx, {"id": "azterisk.minimize"})

    plugins = data.get("plugins", [])
    has_plugin = any(item.get("id") == "azterisk.minimize" for item in plugins)
    if not has_plugin:
        plugins.append({"id": "azterisk.minimize"})

    with open(shell_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print("  ✓ Registered azterisk.minimize in shell.json")
except Exception as e:
    print(f"  ⚠ Note: {e}")
PYEOF
fi

# 6. Rescan plugins & reload
if command -v hyprctl >/dev/null 2>&1; then
  hyprctl reload >/dev/null 2>&1 || true
fi

if command -v omarchy-shell >/dev/null 2>&1; then
  omarchy-shell shell rescanPlugins >/dev/null 2>&1 || true
fi

echo "✨ All-in-one installation complete! Keybindings, CSD titlebar hook, and bar widget are live."
