# Project goal: jwm-extended (multi-monitor JWM)

STATUS: the main feature is IMPLEMENTED and ADOPTED (2026-07-24) -
jwmx v2.4.7 is the daily-driver WM on both of the owner's sessions
and tint2 is retired. See README.md (feature summary), jwm.1.in
(TaskList screen=/rows= reference), TIPS.md (binary hot-swap,
flameshot-per-display), AGENTS.md (build/test). Remaining work:
the built-in notification daemon (NEXT major feature - design
settled in DESIGN-notifications.md), the minor-features roadmap
below, and optional RandR output names (screen="DP-2"/"primary"
instead of indices).

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
   The edge choice is PER SCREEN (top or bottom, full screen width),
   and placement must be computed from that screen's actual geometry
   (position, size, rotation-adjusted dimensions) - never assume a
   uniform row of monitors. Motivating case: in the owner's home
   layout a 4th monitor sits ON TOP of the 1st, and the natural spot
   for that screen's bar is its TOP edge, so tall windows spanning
   the two stacked monitors keep visual continuity. The same
   screen-relative jwmrc semantics should produce sensible bars in
   both the work (row + vertical) and home (stacked) layouts.
5. Per-screen bar height and multi-row task buttons: each bar can be
   made taller independently to hold 2-3 rows of task buttons.
   Motivating case: the vertical monitor (1200 wide) fills its single
   row quickly, so its bar should allow 2-3 rows while the other
   screens keep 1 - combined with the per-screen filtering in (2),
   it only holds buttons for windows mostly on that screen anyway.
6. Decent taskbar icon handling (tint2 does this better today; some
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

## Next major feature: built-in notification daemon

Make jwmx fully standalone: after tint2 (done), eliminate
xfce4-notifyd. jwmx implements the org.freedesktop.Notifications
display/daemon half natively (libdbus-1, #ifdef USE_DBUS, popup.c
reuse, per-screen placement); network protocol bridges (Pushover
etc.) stay external and dumb, feeding the standard interface.
Full design and settled decisions: DESIGN-notifications.md.
v1 scope: summary/body/urgency/timeout; no actions, no history,
single daemon opt-in from ~/.jwmrc only (no :0/:1 gymnastics).

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
- Xinerama index order on :0 is NOT the xrandr order (match
  rectangles via `xdpyinfo -ext XINERAMA`, never assume):
  0=DP-6, 1=DP-0, 2=DP-2 (vertical), 3=DP-4.
- Deployment (since 2026-07-24): /usr/local/bin/jwm is the installed
  jwmx (release build); stock v2.4.4 kept at /usr/bin/jwm as
  fallback. ~/.jwmrc (4 per-screen bars) and ~/.jwmrc-vnc both have
  Dev(src/jwm)/Installed/Stock root-menu switch entries; tint2 and
  wm-tint-mv.sh are gone from ~/.xinitrc. Pre-adoption backups:
  ~/.jwmrc.og, ~/.jwmrc-vnc.og, ~/.xinitrc.og.
- Live binary hot-swap and build/test workflow: see AGENTS.md and
  TIPS.md.
