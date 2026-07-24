# Tips & tricks

Unusual setups and workarounds that do not belong in the main
documentation. Nothing here is required to use jwm.

## Swapping the window manager binary without ending the X session

`jwm -restart` re-runs the main loop in the same process; it never
loads a new binary. To replace the running jwm with a different build
in place (all applications survive; only the WM process is replaced),
use an Exit menu action with a command - on exit, jwm execl()s the
command as `sh -c <command>`, so the command becomes the tail of the
X session's exec chain and MUST end by exec-ing a window manager (or
the X session ends):

    <Exit label="Dev build" confirm="false">exec /path/to/src/jwm</Exit>
    <Exit label="Installed jwm" confirm="false">exec /usr/local/bin/jwm</Exit>
    <Exit label="Stock jwm (fallback)" confirm="false">exec /usr/bin/jwm</Exit>

Notes:
- The freshly exec'd jwm is a new process, so `<StartupCommand>`s run
  again (they do NOT run on `<Restart/>`). Expect one-time duplicates
  of anything they spawn.
- For crash-safe testing of an experimental build, exec a wrapper
  script instead, with a known-good fallback after it:

      #!/bin/sh
      /path/to/test/jwm -f "$HOME/.jwmrc-test" >>"$HOME/.jwm-test.log" 2>&1
      exec /usr/bin/jwm    # reached on crash OR normal exit

## Two flameshot instances on one machine (one per X display)

Scenario: a physical session on `:0` and a VNC session on `:1`, and
you want a flameshot tray daemon + capture hotkey in each, capturing
its own display. Naively starting flameshot on `:1` exits silently
and `flameshot gui` on `:1` captures `:0` instead.

Cause: flameshot is single-instance per user, enforced two ways, and
both must be defeated:

1. a QLocalSocket single-instance guard with a hashed socket name
   under `$TMPDIR` (visible with `strace -e trace=connect flameshot`:
   a connect to `/tmp/<base64-hash>=`);
2. the `org.flameshot.Flameshot` name on the D-Bus session bus, which
   both displays usually share (`flameshot gui` delegates the capture
   to whichever daemon owns the name - the wrong display's).

Workaround: run the second instance through a wrapper that gives it a
private TMPDIR and a private D-Bus session bus, and route BOTH the
daemon and the capture triggers through the same wrapper:

    #!/bin/sh
    # flameshot-vnc: flameshot isolated for the VNC display
    RUNDIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
    BUS="$RUNDIR/flameshot-vnc-bus"
    export DBUS_SESSION_BUS_ADDRESS="unix:path=$BUS"
    export TMPDIR="$RUNDIR/flameshot-vnc-tmp"
    mkdir -p "$TMPDIR"
    if ! dbus-send --session --dest=org.freedesktop.DBus --print-reply \
          / org.freedesktop.DBus.ListNames >/dev/null 2>&1; then
       dbus-daemon --session --address="unix:path=$BUS" --fork
    fi
    exec /usr/bin/flameshot "$@"

jwmrc wiring for the VNC session (the `<Dock/>` is required - that is
the system tray the icon lives in):

    <StartupCommand>/path/to/flameshot-vnc</StartupCommand>
    ...
    <Tray ...> ... <Dock/> ... </Tray>
    ...
    <Key mask="CA" key="P">exec:/path/to/flameshot-vnc gui</Key>

Notifications: if a D-Bus-activatable notification daemon is
installed (e.g. xfce4-notifyd), the private bus auto-activates its
own instance on first use, inheriting the wrapper's DISPLAY - so
notifications from the isolated flameshot correctly appear on the
VNC display. Both instances share `~/.config/flameshot`. The same
pattern should apply to other Qt apps using the SingleApplication
library plus a D-Bus service.
