# Design: built-in notification daemon (jwmx)

Status: DESIGNED, not yet implemented. Decisions below were settled
2026-07-24; this document is the reference for the implementation.

## Vision

jwmx should be standalone: it already replaced tint2 (per-monitor
taskbars); the next elimination target is the external notification
daemon (xfce4-notifyd). "Everything without bloat" holds because the
notification problem splits into two halves with a natural seam:

- The DISPLAY/DAEMON half (what xfce4-notifyd does): receive
  org.freedesktop.Notifications D-Bus calls, draw popups. This IS
  window-manager territory - jwm already owns screen geometry,
  fonts/colors/styles, popup rendering (popup.c, PopupStyle), and
  per-screen placement. jwmx renders its own notification banners,
  styled like the rest of the WM, on the right monitor. BUILT IN.
- The PROTOCOL/NETWORK half (Pushover, email, any remote service):
  TLS, JSON, WebSockets/polling. STAYS EXTERNAL, permanently - a
  network hiccup or TLS CVE must never stall or kill the WM. Once
  jwmx is the notification daemon, a bridge is trivial: a Pushover
  receiver is a small script that gets a message and calls
  notify-send (or libnotify) - jwmx displays it. Bridges need no
  tray icon and no UI.

Terminology recap (settled): notifications are NOT related to the
XEmbed system tray. `<Dock>` (dock.c) implements _NET_SYSTEM_TRAY_S<n>
and embeds tray icons only (the spec's balloon-message opcodes are
received and deliberately ignored, dock.c; nothing modern uses them).
Desktop notifications are org.freedesktop.Notifications on the D-Bus
session bus - a completely separate channel that jwm currently does
not speak at all.

## Dependency decision

libdbus-1: small, plain C, no glib requirement, present on
effectively every Linux desktop, integrates into jwm's existing
select() event loop via its watch/timeout API. Compile-gated
`#ifdef USE_DBUS` per the house pattern (configure.ac: check lib,
AC_DEFINE, summary line) - zero cost when compiled out, and the
build must keep working with the feature absent.

(Footnote: jwm builds with SVG support already link glib/gio
transitively via librsvg, so libdbus is not even the heaviest guest
in the room - but it keeps us independent of glib.)

## v1 scope (DECIDED)

Implement the org.freedesktop.Notifications interface (spec is
small: 4 methods, 2 signals):

- Notify: summary + body text, urgency hint (low/normal/critical
  mapped to per-urgency colors/timeouts; critical stays until
  dismissed), expire_timeout honored (with configured default).
- CloseNotification, GetCapabilities (report a minimal, honest set:
  "body"), GetServerInformation ("jwm", ...).
- NotificationClosed signal (reason: expired/dismissed).
- Body markup: strip tags in v1 (do not claim "body-markup").
- Click on banner dismisses it.

Deferred (explicitly out of v1): action buttons (ActionInvoked),
raw image-data icons, body markup rendering, notification history,
do-not-disturb toggle (natural later as a TrayButton action), an
exec hook (<Notifications exec="...">) piping notifications to an
external command for outbound forwarding (e.g. TO Pushover).

## Session/bus ownership (DECIDED)

One daemon, no multi-session gymnastics. The daemon role is opt-in
via jwmrc (a `<Notifications ...>` element enables it); the owner
enables it in ~/.jwmrc (:0) only, not in ~/.jwmrc-vnc. Once jwm owns
the org.freedesktop.Notifications name, xfce4-notifyd's D-Bus
activation simply never fires - no need to uninstall it. Notifications
raised by :1 apps on the shared bus appearing on :0 is a non-issue
(no real use case; if one ever appears, the private-bus pattern
documented in TIPS.md for flameshot covers it).

## Implementation sketch

- New module src/notify.c (+ notify.h) following the
  Initialize/Startup/Shutdown/Destroy lifecycle; register in main.c
  in all four places. All code under `#ifdef USE_DBUS`.
- Startup: connect to the session bus, request the well-known name
  (fail soft with a Warning if unavailable), export the interface.
  Shutdown: release name, close connection.
- Event loop: dbus watch fds added to the select() in event.c's
  WaitForEvent (same pattern as the X connection fd); dispatch
  non-blocking. Defensive parsing - a malformed Notify must never
  crash the WM.
- Rendering: reuse/extend popup.c drawing and PopupStyle; add a
  NotificationStyle-ish config surface only if PopupStyle proves too
  narrow (colors per urgency). Placement: configurable corner of a
  chosen screen; default top-right of the primary screen; stacking
  for multiple concurrent banners.
- jwmrc: `<Notifications enabled implicit by presence; attributes for
  corner/screen/timeout defaults>`.
- Upstreamability: assume this stays a fork feature (a notification
  daemon inside a WM is a big philosophical ask upstream); keep the
  module cleanly isolated and gated so the fork stays mergeable.
  The multi-monitor taskbar work remains the upstream PR.

## External bridges (kept out of the WM by design)

- Pushover inbound: small standalone script/daemon (working name
  flnotify, now reduced to a headless protocol bridge): receive via
  Pushover's API, emit via notify-send/libnotify. jwmx displays.
- Outbound (local events -> Pushover): later, via the deferred exec
  hook, so forwarding rules live in scripts, not in jwm.
