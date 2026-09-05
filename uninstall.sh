#!/usr/bin/env bash
# Uninstaller script for azterisk.minimize plugin

set -euo pipefail

PLUGIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$HOME/.local/bin"
CLI_TARGET="$BIN_DIR/omarchy-minimize"
HYPR_BINDINGS="$HOME/.config/hypr/bindings.lua"
HYPR_AUTOSTART="$HOME/.config/hypr/autostart.lua"
SHELL_JSON="$HOME/.config/omarchy/shell.json"
HOOK_SO="$PLUGIN_DIR/hyprland-plugin/minimize-hook.so"

echo "🗑️ Uninstalling Omarchy Window Minimize plugin (azterisk.minimize)..."

# 1. Unload Hyprland plugin hook
if command -v hyprctl >/dev/null 2>&1; then
  hyprctl plugin unload "$HOOK_SO" >/dev/null 2>&1 || true
  echo "  ✓ Unloaded minimize-hook from Hyprland"
fi

# 2. Remove symlink
if [[ -L "$CLI_TARGET" || -f "$CLI_TARGET" ]]; then
  rm -f "$CLI_TARGET"
  echo "  ✓ Removed $CLI_TARGET"
fi

# 3. Clean autostart.lua
if [[ -f "$HYPR_AUTOSTART" ]]; then
  sed -i '/minimize-hook/d' "$HYPR_AUTOSTART"
  sed -i '/Load Window Minimize CSD Button Interceptor Hook/d' "$HYPR_AUTOSTART"
  echo "  ✓ Removed plugin hook from $HYPR_AUTOSTART"
fi

# 4. Clean bindings.lua
if [[ -f "$HYPR_BINDINGS" ]]; then
  sed -i '/omarchy-minimize/d' "$HYPR_BINDINGS"
  sed -i '/Window Minimize Plugin (azterisk.minimize)/d' "$HYPR_BINDINGS"
  echo "  ✓ Removed keybindings from $HYPR_BINDINGS"
fi

# 5. Clean shell.json
if [[ -f "$SHELL_JSON" ]]; then
  python3 - << 'PYEOF'
import json
from pathlib import Path

shell_path = Path.home() / ".config" / "omarchy" / "shell.json"
try:
    with open(shell_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    for section in ["left", "center", "right"]:
        items = data.get("bar", {}).get("layout", {}).get(section, [])
        data["bar"]["layout"][section] = [it for it in items if it.get("id") != "azterisk.minimize"]

    if "plugins" in data:
        data["plugins"] = [p for p in data["plugins"] if p.get("id") != "azterisk.minimize"]

    with open(shell_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print("  ✓ Removed azterisk.minimize from shell.json")
except Exception as e:
    print(f"  ⚠ Failed to update shell.json: {e}")
PYEOF
fi

if command -v hyprctl >/dev/null 2>&1; then
  hyprctl reload >/dev/null 2>&1 || true
fi

echo "✨ azterisk.minimize uninstalled successfully."
