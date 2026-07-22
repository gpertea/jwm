# Project goal: jwm-extended (multi-monitor JWM)

This fork exists for ONE main reason: give JWM real per-monitor
taskbar/tray support so tint2 (and its duct tape) can be removed from
the owner's desktop. Guiding constraint: JWM's lightness is a feature.
No new hard dependencies, no compositor, minimal footprint impact -
new behavior must be opt-in via jwmrc and cost nothing when unused.

## The main feature: independent per-screen trays/taskbars

Wanted (this is what tint2 currently provides and JWM cannot):

1. A separate bottom bar on EACH monitor.
2. Each bar's TaskList shows ONLY the windows on THAT monitor
   (decide by window center or dominant overlap, like tint2).
3. Granular per-bar contents: menu button, pager, dock, clock etc.
   enabled/disabled per screen independently (mostly falls out of
   JWM's existing per-Tray children, once trays can bind to screens).
4. Screen-relative tray placement in jwmrc, e.g.
   <Tray screen="2" valign="bottom" ...> instead of hand-computed
   root-coordinate x/y pixel math. Screen reference by index; by
   RandR output name (DP-2) is nicer if cheap.
5. Decent taskbar icon handling (tint2 does this better today; some
   legacy apps need fallbacks/overrides).

## Multi-monitor model: static is fine

- No hotplug/dynamic RandR event support needed. The layout is set
  once by an xrandr script at session start, before jwm execs.
- Xinerama geometry is sufficient for detection: modern Xorg
  synthesizes Xinerama rectangles from the RandR CRTC config, so
  XineramaQueryScreens already returns correct post-rotation
  rectangles (the vertical monitor reports as 1200x1920). JWM
  re-queries on Restart, which is the acceptable "re-freeze" action
  after any layout change.
- The only things Xinerama lacks are output NAMES and the primary
  flag. If per-name config (screen="DP-2") is wanted, add a tiny
  optional RandR name query at startup - still no event handling.

## Roadmap: minor features (after the main one)

- Drag-and-drop reordering of taskbar window buttons (as in Windows
  taskbar / xfce4-panel).
- Proper right-click menu on taskbar buttons (close, move-to-desktop,
  etc.) that works while the window is minimized.
- Rename window caption/title label (including the taskbar label).
- Better Alt+Tab display.
- Luxury, not a must: thumbnail preview of (minimized) windows in the
  taskbar right-click menu / popup.

## Owner's environment (the live test target)

- DISPLAY :1 = VNC session (single virtual output VNC-0, ~1914x1142).
  This is where development happens remotely. NEVER assume $DISPLAY
  is the real desktop; check it. Test WM changes on :1 only inside
  Xephyr, never by replacing the VNC session's WM.
- DISPLAY :0 = the real session: 6960x1920 total, 4 active monitors
  (xrandr --listmonitors):
      DP-6  1920x1200 +0+487      (primary)
      DP-4  1920x1200 +1920+487
      DP-2  1200x1920 +3840+0     (rotated left - vertical)
      DP-0  1920x1200 +5040+487
  Layout set by ~/.screenlayout/4m_valign.sh, called from ~/.xinitrc
  before "exec jwm".
- Current duct tape that this project should make unnecessary:
  - tint2 started from ~/.xinitrc providing per-monitor taskbars.
  - ~/.screenlayout/wm-tint-mv.sh repositions the tint2 panels with
    wmctrl (sorted by X coordinate; the 3rd one is the vertical
    screen and needs a different Y), run as a jwm StartupCommand and
    from a "Realign taskbars" root-menu entry.
  - ~/.jwmrc Group hack pinning Tint2 windows (sticky, y:1534) and a
    single jwm Tray at the bottom of the primary screen with a
    1532px Spacer to leave room for tint2.
- Live binary hot-swap and build/test workflow: see AGENTS.md.
