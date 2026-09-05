#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/protocols/XDGShell.hpp>
#include <hyprland/src/xwayland/XSurface.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

inline HANDLE PHANDLE = nullptr;

static std::string os_getenv_or(const char* name, const std::string& fallback) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : fallback;
}

static void execDetached(const std::string& cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        // Intermediate process
        pid_t grandChild = fork();
        if (grandChild == 0) {
            // Detached worker process decoupled completely from compositor
            setsid();
            int devnull = open("/dev/null", O_RDWR);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            _exit(127);
        }
        // Intermediate child exits immediately so grandchild is reparented to init
        _exit(0);
    }
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

static bool isNativeTrayApp(PHLWINDOW pWindow) {
    if (!pWindow)
        return false;
    std::string cls = pWindow->fetchClass();
    std::transform(cls.begin(), cls.end(), cls.begin(), [](unsigned char c) { return std::tolower(c); });

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

static void logMsg(const std::string& msg) {
    std::ofstream ofs("/tmp/minimize-hook.log", std::ios::app);
    if (ofs.is_open()) {
        auto now = std::chrono::system_clock::now();
        ofs << std::format("[{:%FT%T}] {}\n", now, msg);
    }
}

static void triggerMinimize(PHLWINDOW pWindow) {
    if (!pWindow)
        return;

    bool isNative = isNativeTrayApp(pWindow);
    logMsg(std::format("triggerMinimize on 0x{:x} ({}), isNativeTrayApp={}", (uintptr_t)pWindow.get(), pWindow->fetchClass(), isNative));
    if (isNative)
        return;

    bool isMax = Fullscreen::controller()->isFullscreen(pWindow, Fullscreen::FSMODE_MAXIMIZED);
    if (isMax) {
        logMsg(std::format("triggerMinimize: unmaximizing 0x{:x} before moving off-screen", (uintptr_t)pWindow.get()));
        Fullscreen::controller()->setFullscreenMode(pWindow, Fullscreen::FSMODE_NONE, Fullscreen::FSMODE_NONE, true);
        if (pWindow->m_xwaylandSurface) {
            auto xsurf = pWindow->m_xwaylandSurface.lock();
            if (xsurf)
                xsurf->m_maximized = false;
        }
    }

    std::string addr = std::format("0x{:x}", (uintptr_t)pWindow.get());
    std::string cmd = os_getenv_or("HOME", "/home/azterisk") + "/.local/bin/omarchy-minimize minimize " + addr + (isMax ? " --was-maximized" : "");
    execDetached(cmd);
}

static bool s_inMaximizeHandling = false;

static void triggerMaximize(PHLWINDOW pWindow, bool wantMaximize) {
    if (!pWindow || s_inMaximizeHandling)
        return;

    s_inMaximizeHandling = true;

    bool isMax = Fullscreen::controller()->isFullscreen(pWindow, Fullscreen::FSMODE_MAXIMIZED);
    logMsg(std::format("triggerMaximize on 0x{:x} ({}): wantMaximize={}, isMaxCurrently={}",
        (uintptr_t)pWindow.get(), pWindow->fetchClass(), wantMaximize, isMax));

    if (wantMaximize && !isMax) {
        // Maximize to workspace monocle area (respecting top bar and gaps)
        logMsg(std::format("triggerMaximize: maximizing 0x{:x}", (uintptr_t)pWindow.get()));
        Fullscreen::controller()->setFullscreenMode(pWindow, Fullscreen::FSMODE_MAXIMIZED, Fullscreen::FSMODE_MAXIMIZED, true);
        if (pWindow->m_xwaylandSurface) {
            auto xsurf = pWindow->m_xwaylandSurface.lock();
            if (xsurf)
                xsurf->m_maximized = true;
        }
    } else if (!wantMaximize && isMax) {
        // Restore / Unmaximize
        logMsg(std::format("triggerMaximize: unmaximizing 0x{:x}", (uintptr_t)pWindow.get()));
        Fullscreen::controller()->setFullscreenMode(pWindow, Fullscreen::FSMODE_NONE, Fullscreen::FSMODE_NONE, true);
        if (pWindow->m_xwaylandSurface) {
            auto xsurf = pWindow->m_xwaylandSurface.lock();
            if (xsurf)
                xsurf->m_maximized = false;
        }
    } else {
        logMsg(std::format("triggerMaximize: no-op on 0x{:x} (already matches wantMaximize={})", (uintptr_t)pWindow.get(), wantMaximize));
    }

    s_inMaximizeHandling = false;
}

static void triggerClose(PHLWINDOW pWindow) {
    if (!pWindow)
        return;

    logMsg(std::format("triggerClose on 0x{:x} ({})", (uintptr_t)pWindow.get(), pWindow->fetchClass()));
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
    logMsg(std::format("attachWindowListener on 0x{:x} ({})", (uintptr_t)pWindow.get(), pWindow->fetchClass()));

    if (pWindow->m_xdgSurface && pWindow->m_xdgSurface->m_toplevel) {
        auto toplevel = pWindow->m_xdgSurface->m_toplevel.lock();
        if (toplevel) {
            toplevel->m_events.stateChanged.listenStatic([pw, toplevel]() {
                auto win = pw.lock();
                if (!win)
                    return;
                bool hasMax = toplevel->m_state.requestsMaximize.has_value();
                bool reqMax = toplevel->m_state.requestsMaximize.value_or(false);
                bool reqMin = toplevel->m_state.requestsMinimize.value_or(false);

                if (hasMax) {
                    toplevel->m_state.requestsMaximize.reset();
                    toplevel->m_state.requestsMinimize.reset();
                    logMsg(std::format("XDG stateChanged maximize request on 0x{:x} ({}): reqMax={}",
                        (uintptr_t)win.get(), win->fetchClass(), reqMax));
                    triggerMaximize(win, reqMax);
                } else if (reqMin) {
                    toplevel->m_state.requestsMinimize.reset();
                    logMsg(std::format("XDG stateChanged minimize request on 0x{:x} ({})",
                        (uintptr_t)win.get(), win->fetchClass()));
                    triggerMinimize(win);
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
                bool hasMax = xsurf->m_state.requestsMaximize.has_value();
                bool reqMax = xsurf->m_state.requestsMaximize.value_or(false);
                bool reqMin = xsurf->m_state.requestsMinimize.value_or(false);

                if (hasMax) {
                    xsurf->m_state.requestsMaximize.reset();
                    xsurf->m_state.requestsMinimize.reset();
                    logMsg(std::format("XWayland stateChanged maximize request on 0x{:x} ({}): reqMax={}",
                        (uintptr_t)win.get(), win->fetchClass(), reqMax));
                    triggerMaximize(win, reqMax);
                } else if (reqMin) {
                    xsurf->m_state.requestsMinimize.reset();
                    logMsg(std::format("XWayland stateChanged minimize request on 0x{:x} ({})",
                        (uintptr_t)win.get(), win->fetchClass()));
                    triggerMinimize(win);
                }
            });

            // Automatically restore minimized XWayland window if client requests activation (e.g. system tray click)
            xsurf->m_events.activate.listenStatic([pw]() {
                auto win = pw.lock();
                if (!win)
                    return;
                std::string addr = std::format("0x{:x}", (uintptr_t)win.get());
                std::string cmd = os_getenv_or("HOME", "/home/azterisk") + "/.local/bin/omarchy-minimize restore " + addr;
                execDetached(cmd);
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
        std::string cmd = os_getenv_or("HOME", "/home/azterisk") + "/.local/bin/omarchy-minimize clean";
        execDetached(cmd);
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
