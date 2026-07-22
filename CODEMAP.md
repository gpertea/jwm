# JWM Codemap

All code lives in `src/` (52 C modules, ~26k lines, plain C + Xlib).
One module = one `.c`/`.h` pair. No test suite; verification is
compile + live run.

## Lifecycle pattern (the key convention)

Every module exports up to four entry points, called in strict order
from `main.c`:

    Initialize*()  - before config parsing (allocate defaults)
    Startup*()     - after config parsing (connect to X, create windows)
    Shutdown*()    - on exit/restart (release X resources)
    Destroy*()     - after shutdown (free memory)

`main()` runs `do { Initialize; ParseConfig; Startup; EventLoop;
Shutdown; Destroy } while(shouldRestart);` - a "restart" re-runs this
loop in the SAME process (the binary on disk is not reloaded). On final
exit, `exitCommand` (set by an `<Exit>` action) is `execl`'d in place -
this is the mechanism for hot-swapping a new binary during live testing.

## Module groups

### Core / infrastructure
- `main.c` - entry point, X connection, global state (`display`,
  `rootWindow`, `rootWidth/Height`), startup/shutdown ordering.
- `jwm.h` - master header; feature macros (`USE_XINERAMA`, `USE_SHAPE`,
  `USE_XRENDER`, ...) from configure.
- `jxlib.h` - `JX*` macros wrapping every Xlib call (debug counting).
  Always use `JXFoo(...)`, never raw `XFoo(...)`.
- `debug.h/.c` - `Allocate`/`Release` memory wrappers (use instead of
  malloc/free), debug tracing.
- `settings.c` - global `settings` struct (all runtime options).
- `error.c` (Warning/FatalError), `misc.c` (CopyString, string utils),
  `timing.c` (timevals), `grab.c` (server grabs), `help.c` (usage).

### Event handling
- `event.c` - main event loop (`EventLoop`, `WaitForEvent`,
  `ProcessEvent`), dispatch to modules, and the lazy-update system:
  `RequireTaskUpdate`, `RequirePagerUpdate`, `RequireRestack` set flags
  serviced when the queue drains. New X extension events are wired here.
- `binding.c` - key/mouse bindings; `action.c` - executing bound
  actions; `command.c` - spawning external programs; `cursor.c` -
  cursors and `GetMousePosition`.

### Configuration
- `parse.c` - jwmrc XML -> settings/menus/trays (largest module).
- `lex.c` - hand-rolled XML tokenizer.
- `default.c` - built-in default config.
- `group.c` - per-window `<Group>` option matching; `match.c` - glob
  matching.

### Client (window) management
- `client.c` - `ClientNode` lifecycle: map, focus, maximize,
  fullscreen, minimize, shade, kill.
- `clientlist.c` - stacking order, layers, alt-tab walk order.
- `border.c` - frame decoration drawing and hit-testing;
  `GetBorderSize` is the highest fan-in function in the tree.
- `hint.c` - ICCCM/EWMH property read/write (`_NET_*` atoms).
- `place.c` - initial placement, struts, per-screen bounds.
- `move.c` / `resize.c` - interactive move/resize (includes per-screen
  edge snapping); `outline.c` - rubber-band outline; `status.c` -
  move/resize coordinate popup.
- `desktop.c` - virtual desktops; `winmenu.c` - per-window menu;
  `confirm.c` - confirm dialogs; `swallow.c` - swallowed clients.

### Screens (multi-monitor) - current state
- `screen.c/.h` - "screen" = physical monitor. Queried ONCE at startup
  via Xinerama (`XineramaQueryScreens`); no RandR support, no reaction
  to monitor hotplug/layout changes (a restart is required today).
  `ScreenType` is `{index, x, y, width, height}` only.
  API: `GetCurrentScreen(x,y)`, `GetMouseScreen()`, `GetScreen(i)`,
  `GetScreenCount()`.
- Callers of the screen API (the surface RandR work will touch):
  `place.c` (placement/struts), `client.c` (fullscreen/maximize
  bounds), `move.c` (snapping), `tray.c` (tray geometry), `menu.c`,
  `popup.c`, `confirm.c`, `status.c` (keep-onscreen), `taskbar.c`,
  `action.c`, `event.c`.
- `rootWidth`/`rootHeight` (main.c) hold the full X screen size; many
  modules use them directly as the fallback bounding box.

### Tray and applets
- `tray.c` - tray container windows, layout, autohide.
- `taskbar.c`, `traybutton.c`, `clock.c`, `pager.c`, `spacer.c` -
  tray components; `dock.c` - system notification area (XEmbed).
- `popup.c` - tooltips.

### Menus
- `menu.c` - menu display/navigation state machine (`MenuLoop`).
- `root.c` - root menus, Restart/Exit actions (sets `exitCommand`).

### Drawing / assets
- `font.c` (Pango or core fonts), `color.c`, `gradient.c`,
  `button.c` (themed button drawing), `background.c`.
- `icon.c`, `image.c` (PNG/JPEG/SVG/XPM loaders), `render.c`
  (XRender compositing).

## Build plumbing
- `configure.ac` + `Makefile.in` (autoconf, no automake). Feature
  libraries are optional; each defines a `USE_*` macro.
- `po/` - gettext translations; user-visible strings wrapped in `_()`.
- `example.jwmrc` - default config installed as `/etc/jwm/system.jwmrc`.
- `jwm.1.in` - man page (the primary user documentation).
