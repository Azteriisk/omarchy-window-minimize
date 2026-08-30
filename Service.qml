import QtQuick
import Quickshell
import Quickshell.Io
import Quickshell.Hyprland

Item {
  id: root

  readonly property string scriptPath: Quickshell.env("HOME") + "/.config/omarchy/plugins/azterisk.minimize/scripts/omarchy-minimize"

  function cleanState() {
    if (!cleanProc.running) {
      cleanProc.running = true
    }
  }

  Process {
    id: cleanProc
    command: ["bash", "-c", root.scriptPath + " clean"]
  }

  // Periodic cleanup timer (every 10 seconds)
  Timer {
    interval: 10000
    running: true
    repeat: true
    onTriggered: root.cleanState()
  }

  // Clean state whenever toplevel windows in Hyprland change
  Connections {
    target: Hyprland.toplevels
    function onValuesChanged() {
      root.cleanState()
    }
  }

  Component.onCompleted: root.cleanState()
}
