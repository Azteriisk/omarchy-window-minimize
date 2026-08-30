#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/protocols/XDGShell.hpp>
#include <hyprland/src/xwayland/XSurface.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>

#include <format>
#include <cstdlib>

inline HANDLE PHANDLE = nullptr;

static std::string os_getenv_or(const char* name, const std::string& fallback) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : fallback;
}

static void triggerMinimize(PHLWINDOW pWindow) {
    if (!pWindow)
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
    if (!pWindow)
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
        .version = "1.0.0",
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
}
