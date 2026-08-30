#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/protocols/XDGShell.hpp>
#include <hyprland/src/xwayland/XSurface.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <cstdlib>

inline HANDLE PHANDLE = nullptr;

static std::string os_getenv_or(const char* name, const std::string& fallback) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : fallback;
}

static bool isNativeTrayApp(PHLWINDOW pWindow) {
    if (!pWindow)
        return false;
    std::string cls = pWindow->fetchClass();
    std::transform(cls.begin(), cls.end(), cls.begin(), [](unsigned char c) { return std::tolower(c); });
    
    // Steam and Steam games handle their own window unmapping and tray minimization natively.
    if (cls == "steam" || cls.rfind("steam_app_", 0) == 0)
        return true;

    // Optional user-defined ignore list: ~/.config/omarchy/minimize-ignored-apps.txt
    std::string configPath = os_getenv_or("HOME", "/home/azterisk") + "/.config/omarchy/minimize-ignored-apps.txt";
    std::ifstream infile(configPath);
    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty() || line[0] == '#')
                continue;
            std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });
            if (cls == line)
                return true;
        }
    }

    return false;
}

static void triggerMinimize(PHLWINDOW pWindow) {
    if (!pWindow || isNativeTrayApp(pWindow))
        return;

    std::string addr = std::format("0x{:x}", (uintptr_t)pWindow.get());
    std::string cmd = os_getenv_or("HOME", "/home/azterisk") + "/.local/bin/omarchy-minimize minimize " + addr + " &";
    
    if (g_pKeybindManager && g_pKeybindManager->m_dispatchers.contains("exec")) {
        g_pKeybindManager->m_dispatchers["exec"](cmd);
    } else {
        std::system(cmd.c_str());
    }
}

static void attachWindowListener(PHLWINDOW pWindow) {
    if (!pWindow || isNativeTrayApp(pWindow))
        return;

    if (pWindow->m_xdgSurface && pWindow->m_xdgSurface->m_toplevel) {
        auto toplevel = pWindow->m_xdgSurface->m_toplevel.lock();
        if (toplevel) {
            toplevel->m_events.stateChanged.listenStatic([pWindow, toplevel]() {
                if (toplevel->m_state.requestsMinimize.value_or(false)) {
                    triggerMinimize(pWindow);
                }
            });
        }
    }

    if (pWindow->m_xwaylandSurface) {
        auto xsurf = pWindow->m_xwaylandSurface;
        xsurf->m_events.stateChanged.listenStatic([pWindow, xsurf]() {
            if (xsurf->m_state.requestsMinimize.value_or(false)) {
                triggerMinimize(pWindow);
            }
        });
    }
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    if (Desktop::windowState()) {
        for (const auto& w : Desktop::windowState()->windows()) {
            attachWindowListener(w);
        }
    }

    Event::bus()->m_events.window.open.listenStatic([](PHLWINDOW pWindow) {
        attachWindowListener(pWindow);
    });

    return {
        .name = "azterisk.minimize-hook",
        .description = "Intercepts application CSD minimize button clicks and routes to omarchy-minimize",
        .author = "Azteriisk",
        .version = "1.0.2",
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
}
