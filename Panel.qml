import QtQuick
import QtQuick.Controls
import Quickshell
import Quickshell.Io
import Quickshell.Hyprland
import qs.Ui
import qs.Commons

Panel {
  id: root
  moduleName: "azterisk.minimize"
  ipcTarget: "azterisk.minimize"
  manageIpc: true

  property int minimizedCount: 0
  property bool hasMinimized: false
  property var groups: []
  property var allWindows: []

  readonly property color foreground: bar ? bar.foreground : Color.foreground
  readonly property color urgent: bar ? bar.urgent : Color.urgent
  readonly property string fontFamily: bar ? bar.fontFamily : Style.font.family

  readonly property string scriptPath: Quickshell.env("HOME") + "/.config/omarchy/plugins/azterisk.minimize/scripts/omarchy-minimize"
  readonly property bool alwaysShow: setting("alwaysShow", false) === true

  function refresh() {
    if (!statusProc.running) {
      statusProc.running = true
    }
  }

  function runScript(args) {
    Quickshell.execDetached(["bash", "-c", root.scriptPath + " " + args])
    refreshTimer.restart()
  }

  function restoreAll() {
    runScript("restore-all")
  }

  function restoreLast() {
    runScript("restore-last")
  }

  function restoreApp(appClass) {
    runScript("restore-app " + Util.shellQuote(appClass))
  }

  function restoreWindow(addr) {
    runScript("restore " + Util.shellQuote(addr))
  }

  function minimizeFocused() {
    runScript("minimize")
  }

  function closeWindow(addr) {
    runScript("close " + Util.shellQuote(addr))
  }

  // --- Backend Process ---
  Process {
    id: statusProc
    command: ["bash", "-c", root.scriptPath + " status --json"]
    stdout: StdioCollector {
      waitForEnd: true
      onStreamFinished: {
        try {
          var data = JSON.parse(text.trim())
          root.minimizedCount = data.count || 0
          root.hasMinimized = data.has_minimized || false
          root.groups = data.groups || []
          root.allWindows = data.windows || []
        } catch(e) {}
      }
    }
  }

  Timer {
    id: refreshTimer
    interval: 200
    repeat: false
    onTriggered: root.refresh()
  }

  Timer {
    interval: 2000
    running: true
    repeat: true
    onTriggered: root.refresh()
  }

  Component.onCompleted: root.refresh()
  onOpenedChanged: if (opened) root.refresh()

  // React to Hyprland toplevel count changes
  Connections {
    target: Hyprland.toplevels
    function onValuesChanged() {
      root.refresh()
    }
  }

  // Bar button geometry
  visible: root.alwaysShow || root.hasMinimized
  implicitWidth: visible ? button.implicitWidth : 0
  implicitHeight: visible ? button.implicitHeight : 0

  BarIconButton {
    id: button
    anchors.fill: parent
    bar: root.bar
    text: "󰖰"
    active: root.hasMinimized
    tooltipText: root.hasMinimized ? (root.minimizedCount + " minimized window(s) (Super+Alt+M to restore all)") : "No minimized windows (Super+M to minimize)"
    onPressed: function(b) {
      if (b === Qt.RightButton) {
        root.restoreAll()
      } else if (b === Qt.MiddleButton) {
        root.restoreLast()
      } else {
        root.toggle()
      }
    }
  }

  KeyboardPanel {
    id: panel
    anchorItem: button
    owner: root
    bar: root.bar
    open: root.opened
    focusTarget: keyCatcher
    contentWidth: panel.fittedContentWidth(Style.space(420))
    contentHeight: panel.fittedContentHeight(panelColumn.implicitHeight, Style.space(640))

    PanelKeyCatcher {
      id: keyCatcher
      anchors.fill: parent
      onCloseRequested: root.close()

      ScrollView {
        id: scrollArea
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: panelColumn.implicitHeight > height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
        Binding {
          target: scrollArea.contentItem
          property: "interactive"
          value: panelColumn.implicitHeight > scrollArea.height
        }

        Column {
          id: panelColumn
          width: scrollArea.availableWidth
          spacing: Style.space(14)

          // ---------- Header / Hero Section ----------
          Item {
            width: parent.width
            implicitHeight: Math.max(heroIcon.implicitHeight, heroLabels.implicitHeight, restoreAllBtn.implicitHeight)

            Text {
              id: heroIcon
              text: "󰖰"
              color: root.hasMinimized ? root.urgent : root.foreground
              font.family: root.fontFamily
              font.pixelSize: Style.font.display
              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter
            }

            Column {
              id: heroLabels
              anchors.left: heroIcon.right
              anchors.leftMargin: Style.space(12)
              anchors.right: restoreAllBtn.left
              anchors.rightMargin: Style.space(10)
              anchors.verticalCenter: parent.verticalCenter
              spacing: Style.space(2)

              Text {
                text: "Minimized Windows"
                color: root.foreground
                font.family: root.fontFamily
                font.pixelSize: Style.font.title
                font.bold: true
              }

              Text {
                text: root.hasMinimized
                  ? (root.minimizedCount + " window" + (root.minimizedCount === 1 ? "" : "s") + " across " + root.groups.length + " group" + (root.groups.length === 1 ? "" : "s"))
                  : "No windows currently minimized"
                color: Util.alpha(root.foreground, 0.6)
                font.family: root.fontFamily
                font.pixelSize: Style.font.caption
              }
            }

            Button {
              id: restoreAllBtn
              anchors.right: parent.right
              anchors.verticalCenter: parent.verticalCenter
              text: "Restore All"
              fontSize: Style.font.caption
              foreground: root.foreground
              fontFamily: root.fontFamily
              active: root.hasMinimized
              bordered: true
              visible: root.hasMinimized
              onClicked: {
                root.restoreAll()
                root.close()
              }
            }
          }

          PanelSeparator {
            foreground: root.foreground
          }

          // ---------- Minimized App Groups ----------
          Column {
            width: parent.width
            spacing: Style.space(12)
            visible: root.hasMinimized

            Repeater {
              model: root.groups

              Item {
                id: groupItem
                required property var modelData
                width: panelColumn.width
                implicitHeight: groupCard.implicitHeight

                Rectangle {
                  id: groupCard
                  width: parent.width
                  implicitHeight: groupInnerColumn.implicitHeight + Style.space(16)
                  color: Style.hoverFillFor(root.foreground, Color.accent, Color.urgent)
                  radius: Style.cornerRadius
                  border.color: Util.alpha(root.foreground, 0.2)
                  border.width: 1

                  Column {
                    id: groupInnerColumn
                    anchors.fill: parent
                    anchors.margins: Style.space(10)
                    spacing: Style.space(8)

                    // App Group Header
                    Item {
                      width: parent.width
                      implicitHeight: Math.max(groupTitleRow.implicitHeight, restoreGroupBtn.implicitHeight)

                      Row {
                        id: groupTitleRow
                        anchors.left: parent.left
                        anchors.right: restoreGroupBtn.left
                        anchors.rightMargin: Style.space(8)
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Style.space(8)

                        Text {
                          text: "󰣆"
                          color: Color.accent
                          font.family: root.fontFamily
                          font.pixelSize: Style.font.body
                          anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                          text: groupItem.modelData.class.toUpperCase()
                          color: root.foreground
                          font.family: root.fontFamily
                          font.pixelSize: Style.font.body
                          font.bold: true
                          anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                          implicitWidth: groupCountText.implicitWidth + Style.space(10)
                          implicitHeight: Style.space(18)
                          color: Util.alpha(Color.accent, 0.2)
                          radius: Math.max(2, Math.round(Style.cornerRadius / 2))
                          anchors.verticalCenter: parent.verticalCenter

                          Text {
                            id: groupCountText
                            anchors.centerIn: parent
                            text: groupItem.modelData.count + ""
                            color: root.foreground
                            font.family: root.fontFamily
                            font.pixelSize: Style.font.bodySmall
                            font.bold: true
                          }
                        }
                      }

                      Button {
                        id: restoreGroupBtn
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Restore Group"
                        fontSize: Style.font.caption
                        foreground: root.foreground
                        fontFamily: root.fontFamily
                        bordered: true
                        horizontalPadding: Style.space(8)
                        verticalPadding: Style.space(3)
                        onClicked: root.restoreApp(groupItem.modelData.class)
                      }
                    }

                    // Windows in Group
                    Repeater {
                      model: groupItem.modelData.windows

                      Rectangle {
                        id: winRow
                        required property var modelData
                        width: parent.width
                        implicitHeight: Style.space(32)
                        color: "transparent"
                        radius: Math.max(2, Math.round(Style.cornerRadius / 2))

                        Row {
                          anchors.left: parent.left
                          anchors.right: winActionBtns.left
                          anchors.rightMargin: Style.space(6)
                          anchors.verticalCenter: parent.verticalCenter
                          spacing: Style.space(8)

                          Rectangle {
                            id: wsBadge
                            implicitWidth: wsText.implicitWidth + Style.space(8)
                            implicitHeight: Style.space(18)
                            color: Util.alpha(root.foreground, 0.15)
                            radius: Math.max(2, Math.round(Style.cornerRadius / 2))
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                              id: wsText
                              anchors.centerIn: parent
                              text: "WS " + (winRow.modelData.workspace || "1")
                              color: Util.alpha(root.foreground, 0.7)
                              font.family: root.fontFamily
                              font.pixelSize: Style.font.bodySmall
                            }
                          }

                          Text {
                            text: winRow.modelData.title || "Untitled"
                            color: root.foreground
                            font.family: root.fontFamily
                            font.pixelSize: Style.font.caption
                            elide: Text.ElideRight
                            width: winRow.width - wsBadge.implicitWidth - winActionBtns.width - Style.space(24)
                            anchors.verticalCenter: parent.verticalCenter
                          }
                        }

                        Row {
                          id: winActionBtns
                          anchors.right: parent.right
                          anchors.verticalCenter: parent.verticalCenter
                          spacing: Style.space(4)

                          Button {
                            id: restoreWinBtn
                            text: "󰁌"
                            fontSize: Style.font.caption
                            foreground: root.foreground
                            fontFamily: root.fontFamily
                            bordered: true
                            horizontalPadding: Style.space(6)
                            verticalPadding: Style.space(2)
                            onClicked: root.restoreWindow(winRow.modelData.address)
                          }

                          Button {
                            id: closeWinBtn
                            text: "󰅖"
                            fontSize: Style.font.caption
                            foreground: Util.alpha(root.foreground, 0.7)
                            fontFamily: root.fontFamily
                            bordered: true
                            horizontalPadding: Style.space(6)
                            verticalPadding: Style.space(2)
                            onClicked: root.closeWindow(winRow.modelData.address)
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          // ---------- Empty State & Tips ----------
          Column {
            width: parent.width
            spacing: Style.space(8)
            visible: !root.hasMinimized

            Item {
              width: parent.width
              implicitHeight: Style.space(60)

              Column {
                anchors.centerIn: parent
                spacing: Style.space(4)

                Text {
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: "󰖲 No windows minimized"
                  color: Util.alpha(root.foreground, 0.6)
                  font.family: root.fontFamily
                  font.pixelSize: Style.font.body
                }

                Text {
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: "Press Super+M to minimize any active window"
                  color: Util.alpha(root.foreground, 0.4)
                  font.family: root.fontFamily
                  font.pixelSize: Style.font.caption
                }
              }
            }
          }

          // ---------- Shortcuts Footer ----------
          PanelSeparator {
            foreground: root.foreground
          }

          Row {
            width: parent.width
            spacing: Style.space(12)

            Text {
              text: "󰌌 Super+M : Minimize"
              color: Util.alpha(root.foreground, 0.5)
              font.family: root.fontFamily
              font.pixelSize: Style.font.bodySmall
            }

            Text {
              text: "•"
              color: Util.alpha(root.foreground, 0.3)
              font.pixelSize: Style.font.bodySmall
            }

            Text {
              text: "Super+Alt+M : Restore All"
              color: Util.alpha(root.foreground, 0.5)
              font.family: root.fontFamily
              font.pixelSize: Style.font.bodySmall
            }
          }
        }
      }
    }
  }
}
