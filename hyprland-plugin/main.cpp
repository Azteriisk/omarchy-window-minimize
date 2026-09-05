#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/protocols/XDGShell.hpp>
#include <hyprland/src/xwayland/XSurface.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>

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

static bool s_inMaximizeHandling = false;

static void triggerMaximize(PHLWINDOW pWindow, bool wantMaximize) {
    if (!pWindow || s_inMaximizeHandling)
        return;

    s_inMaximizeHandling = true;

    bool isMax = Fullscreen::controller()->isFullscreen(pWindow, Fullscreen::FSMODE_MAXIMIZED);

    // If currently maximized and client requests unmaximize (false), or requests toggle while already maximized:
    if (isMax && (!wantMaximize || isMax)) {
        // Restore / Unmaximize
        Fullscreen::controller()->setFullscreenMode(pWindow, Fullscreen::FSMODE_NONE, Fullscreen::FSMODE_NONE, true);
        if (pWindow->m_xwaylandSurface) {
            auto xsurf = pWindow->m_xwaylandSurface.lock();
            if (xsurf)
                xsurf->m_maximized = false;
        }
    } else if (!isMax && wantMaximize) {
        // Maximize to workspace monocle area (respecting top bar and gaps)
        Fullscreen::controller()->setFullscreenMode(pWindow, Fullscreen::FSMODE_MAXIMIZED, Fullscreen::FSMODE_MAXIMIZED, true);
        if (pWindow->m_xwaylandSurface) {
            auto xsurf = pWindow->m_xwaylandSurface.lock();
            if (xsurf)
                xsurf->m_maximized = true;
        }
    }

    s_inMaximizeHandling = false;
}

static void triggerClose(PHLWINDOW pWindow) {
    if (!pWindow)
        return;

    pWindow->sendClose();
}

static PHLWINDOW getWindowByAddress(const std::string& addrStr) {
    if (!Desktop::windowState())
        return nullptr;
    uintptr_t addr = 0;
    try {
        addr = std::stoull(addrStr, nullptr, 16);
    } catch (...) {
        return nullptr;
    }
    for (const auto& w : Desktop::windowState()->windows()) {
        if ((uintptr_t)w.get() == addr)
            return w;
    }
    return nullptr;
}

static void attachWindowListener(PHLWINDOW pWindow) {
    if (!pWindow)
        return;

    WP<Desktop::View::CWindow> pw = pWindow;

    if (pWindow->m_xdgSurface && pWindow->m_xdgSurface->m_toplevel) {
        auto toplevel = pWindow->m_xdgSurface->m_toplevel.lock();
        if (toplevel) {
            toplevel->m_events.stateChanged.listenStatic([pw, toplevel]() {
                auto win = pw.lock();
                if (!win)
                    return;
                if (toplevel->m_state.requestsMinimize.value_or(false)) {
                    if (!isNativeTrayApp(win)) {
                        triggerMinimize(win);
                    }
                }
                if (toplevel->m_state.requestsMaximize.has_value()) {
                    triggerMaximize(win, toplevel->m_state.requestsMaximize.value());
                }
            });
        }
    }

    if (pWindow->m_xwaylandSurface) {
        auto xsurf = pWindow->m_xwaylandSurface.lock();
        if (xsurf) {
            xsurf->m_events.stateChanged.listenStatic([pw, xsurf]() {
                auto win = pw.lock();
                if (!win)
                    return;
                if (xsurf->m_state.requestsMinimize.value_or(false)) {
                    if (!isNativeTrayApp(win)) {
                        triggerMinimize(win);
                    }
                }
                if (xsurf->m_state.requestsMaximize.has_value()) {
                    triggerMaximize(win, xsurf->m_state.requestsMaximize.value());
                }
            });
        }
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

    // Synchronize XWayland client m_maximized state when window fullscreen/maximized state changes
    Event::bus()->m_events.window.fullscreen.listenStatic([](PHLWINDOW pWindow) {
        if (!pWindow || !pWindow->m_xwaylandSurface)
            return;
        auto xsurf = pWindow->m_xwaylandSurface.lock();
        if (!xsurf)
            return;
        bool isMax = Fullscreen::controller()->isFullscreen(pWindow, Fullscreen::FSMODE_MAXIMIZED);
        xsurf->m_maximized = isMax;
    });

    // Instant state cleanup when any window is closed/destroyed
    Event::bus()->m_events.window.destroy.listenStatic([](PHLWINDOWREF) {
        std::string cmd = os_getenv_or("HOME", "/home/azterisk") + "/.local/bin/omarchy-minimize clean &";
        if (g_pKeybindManager && g_pKeybindManager->m_dispatchers.contains("exec")) {
            g_pKeybindManager->m_dispatchers["exec"](cmd);
        } else {
            std::system(cmd.c_str());
        }
    });

    // Custom dispatchers for keybindings and scripts
    HyprlandAPI::addDispatcherV2(handle, "omarchy:maximize", [](std::string args) -> SDispatchResult {
        PHLWINDOW target = args.empty() ? (Desktop::focusState() ? Desktop::focusState()->window() : nullptr) : getWindowByAddress(args);
        if (!target)
            return {.success = false, .error = "Window not found"};
        bool isMax = Fullscreen::controller()->isFullscreen(target, Fullscreen::FSMODE_MAXIMIZED);
        triggerMaximize(target, !isMax);
        return {};
    });

    HyprlandAPI::addDispatcherV2(handle, "omarchy:close", [](std::string args) -> SDispatchResult {
        PHLWINDOW target = args.empty() ? (Desktop::focusState() ? Desktop::focusState()->window() : nullptr) : getWindowByAddress(args);
        if (!target)
            return {.success = false, .error = "Window not found"};
        triggerClose(target);
        return {};
    });

    return {
        .name = "azterisk.minimize-hook",
        .description = "Intercepts application CSD minimize/maximize button clicks and manages window states",
        .author = "Azteriisk",
        .version = "1.1.0",
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
}
